#include "pch.h"
#include "Render/Visuals/MenuBackground.h"
#include "Render/Pipeline/RenderUtil.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

// === 生成 ===

MenuBackground::MenuBackground(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
    , m_time(0.0f)
    , m_vertexCount(0)
{
}

// === 初期化 ===

void MenuBackground::initialize()
{
    createFullscreenQuad();
    createDeviceDependentResources();
}

// === 更新 ===

void MenuBackground::update(float deltaTime)
{
    m_time += deltaTime;
}

// === フルスクリーン四角形生成 ===

void MenuBackground::createFullscreenQuad()
{
    m_vertices.clear();

    // 三角形1: 左上、左下、右上
    m_vertices.push_back({ XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) });
    m_vertices.push_back({ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) });
    m_vertices.push_back({ XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) });

    // 三角形2: 右上、左下、右下
    m_vertices.push_back({ XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) });
    m_vertices.push_back({ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) });
    m_vertices.push_back({ XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) });

    m_vertexCount = m_vertices.size();
}

// === デバイス依存リソース生成 ===

void MenuBackground::createDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // 頂点バッファ
    m_vertexBuffer = RenderUtil::createStaticVertexBuffer(
        device,
        m_vertices.data(),
        static_cast<UINT>(m_vertices.size()));

    // 定数バッファ
    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<MenuBackgroundCB>(device);

    // シェーダー
    createShaders();

    // ブレンドステート（アルファブレンド）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    DX::ThrowIfFailed(device->CreateBlendState(&blendDesc, m_blendState.ReleaseAndGetAddressOf()));

    // 深度ステンシル（2Dオーバーレイのため深度テスト無効）
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.StencilEnable = FALSE;

    DX::ThrowIfFailed(device->CreateDepthStencilState(&depthStencilDesc, m_depthStencilState.ReleaseAndGetAddressOf()));

    // ラスタライザ（フルスクリーンのためカリングなし）
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;

    DX::ThrowIfFailed(device->CreateRasterizerState(&rsDesc, m_rasterizerState.ReleaseAndGetAddressOf()));
}

// === シェーダー読み込み ===

void MenuBackground::createShaders()
{
    auto device = m_deviceResources->GetD3DDevice();

    // 頂点シェーダー
    ComPtr<ID3DBlob> vertexShaderBlob;
    m_vertexShader = RenderUtil::loadVS(device, L"VS_MenuBackground.cso", &vertexShaderBlob);

    // ピクセルシェーダー1: ワーピング
    m_pixelShaderWarping = RenderUtil::loadPS(device, L"PS_Warping.cso");

    // ピクセルシェーダー2: フラクタル（Kishimisu風）
    m_pixelShaderFractal = RenderUtil::loadPS(device, L"PS_Fractal.cso");

    // ピクセルシェーダー3: 波形
    m_pixelShaderWave = RenderUtil::loadPS(device, L"PS_Wave.cso");

    // 入力レイアウト
    static const D3D11_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    DX::ThrowIfFailed(device->CreateInputLayout(
        inputElements,
        ARRAYSIZE(inputElements),
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        m_inputLayout.ReleaseAndGetAddressOf()
    ));
}

// === 描画 ===

void MenuBackground::render(int width, int height)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // 定数バッファ更新
    MenuBackgroundCB cb = {};
    cb.Time = m_time;
    cb.Resolution = XMFLOAT2(static_cast<float>(width), static_cast<float>(height));
    cb.Speed = m_speed;
    cb.PatternScale = m_patternScale;
    cb.WarpIntensity = m_warpIntensity;
    cb.Brightness = m_brightness;
    cb.ChromaticOffset = m_chromaticOffset;
    cb.ColorTint = m_colorTint;
    cb.VignetteStrength = m_vignetteStrength;

    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

    // パイプライン設定
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(MenuVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    // シェーダータイプに応じてピクセルシェーダーを切り替え
    if (m_shaderType == ShaderType::Warping)
    {
        context->PSSetShader(m_pixelShaderWarping.Get(), nullptr, 0);
    }
    else if (m_shaderType == ShaderType::Fractal)
    {
        context->PSSetShader(m_pixelShaderFractal.Get(), nullptr, 0);
    }
    else
    {
        context->PSSetShader(m_pixelShaderWave.Get(), nullptr, 0);
    }

    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // レンダーステート設定（深度なし、アルファブレンド）
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);
    context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    context->RSSetState(m_rasterizerState.Get());

    // フルスクリーン四角形描画
    context->Draw(static_cast<UINT>(m_vertexCount), 0);

    // ステート復元
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}

void MenuBackground::setShaderType(ShaderType type)
{
    m_shaderType = type;
}

// === デバイスロスト処理 ===

void MenuBackground::onDeviceLost()
{
    m_vertexBuffer.Reset();
    m_constantBuffer.Reset();
    m_vertexShader.Reset();

    m_pixelShaderWarping.Reset();
    m_pixelShaderFractal.Reset();
    m_pixelShaderWave.Reset();

    m_inputLayout.Reset();
    m_blendState.Reset();
    m_depthStencilState.Reset();
    m_rasterizerState.Reset();
}
