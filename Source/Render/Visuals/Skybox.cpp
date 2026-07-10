#include "pch.h"
#include "Render/Visuals/Skybox.h"
#include "Render/Pipeline/RenderUtil.h"
#include "WICTextureLoader.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

Skybox::Skybox(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
{
}

// === 初期化・終了 ===

void Skybox::initialize()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    // 6面テクスチャ読み込み
    // DX11キューブ面順序: +X(右), -X(左), +Y(上), -Y(下), +Z(前), -Z(後)
    const wchar_t* faceFiles[6] = {
        L"Assets/Textures/skybox_cyan_right.png",
        L"Assets/Textures/skybox_cyan_left.png",
        L"Assets/Textures/skybox_cyan_top.png",
        L"Assets/Textures/skybox_cyan_bottom.png",
        L"Assets/Textures/skybox_cyan_front.png",
        L"Assets/Textures/skybox_cyan_back.png",
    };

    ComPtr<ID3D11Texture2D> faceTextures[6];

    for (uint32_t i = 0; i < 6; i++)
    {
        ComPtr<ID3D11Resource> resource;
        DX::ThrowIfFailed(CreateWICTextureFromFile(
            device, faceFiles[i], resource.GetAddressOf(), nullptr));
        DX::ThrowIfFailed(resource.As(&faceTextures[i]));
    }

    // 最初の面からフォーマットとサイズを取得
    D3D11_TEXTURE2D_DESC faceDesc;
    faceTextures[0]->GetDesc(&faceDesc);

    // キューブテクスチャ作成
    D3D11_TEXTURE2D_DESC cubeDesc = {};
    cubeDesc.Width = faceDesc.Width;
    cubeDesc.Height = faceDesc.Height;
    cubeDesc.MipLevels = 1;
    cubeDesc.ArraySize = 6;
    cubeDesc.Format = faceDesc.Format;
    cubeDesc.SampleDesc.Count = 1;
    cubeDesc.Usage = D3D11_USAGE_DEFAULT;
    cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    ComPtr<ID3D11Texture2D> cubeTexture;
    DX::ThrowIfFailed(device->CreateTexture2D(&cubeDesc, nullptr, cubeTexture.GetAddressOf()));

    // 各面をキューブテクスチャにコピー
    for (uint32_t i = 0; i < 6; i++)
    {
        uint32_t destSubresource = D3D11CalcSubresource(0, i, 1);
        context->CopySubresourceRegion(
            cubeTexture.Get(), destSubresource, 0, 0, 0,
            faceTextures[i].Get(), 0, nullptr);
    }

    // SRV作成（TextureCube）
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = faceDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = 1;
    DX::ThrowIfFailed(device->CreateShaderResourceView(
        cubeTexture.Get(), &srvDesc, m_cubeSRV.GetAddressOf()));

    // サンプラー（線形フィルタリング、クランプ）
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    DX::ThrowIfFailed(device->CreateSamplerState(&sampDesc, m_sampler.GetAddressOf()));

    // シェーダー読み込み
    m_vertexShader = RenderUtil::loadVS(device, L"VS_Skybox.cso");
    m_pixelShader = RenderUtil::loadPS(device, L"PS_Skybox.cso");

    // 深度ステンシル：テストなし、書き込みなし
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    DX::ThrowIfFailed(device->CreateDepthStencilState(
        &dsDesc, m_depthStencilState.ReleaseAndGetAddressOf()));

    // ラスタライザー：カリングなし
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = FALSE;
    DX::ThrowIfFailed(device->CreateRasterizerState(
        &rsDesc, m_rasterizerState.ReleaseAndGetAddressOf()));
}

void Skybox::finalize()
{
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_cubeSRV.Reset();
    m_sampler.Reset();
    m_depthStencilState.Reset();
    m_rasterizerState.Reset();
}

// === 描画 ===

void Skybox::render()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // パイプライン設定
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // キューブマップテクスチャとサンプラーをバインド
    context->PSSetShaderResources(0, 1, m_cubeSRV.GetAddressOf());
    context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

    context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    context->RSSetState(m_rasterizerState.Get());

    // フルスクリーン三角形描画
    context->Draw(3, 0);

    // ステート復元
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}
