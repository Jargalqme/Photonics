#include "pch.h"
#include "Render/Pipeline/Bloom.h"
#include "Render/Pipeline/RenderUtil.h"

using namespace DirectX;

Bloom::Bloom(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
{
}

// === デバイス依存リソース生成 ===

void Bloom::createDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // フルスクリーン三角形 頂点シェーダー
    m_fullscreenVS = RenderUtil::loadVS(device, L"VS_FullscreenTriangle.cso");

    // ピクセルシェーダー読み込み
    m_prefilterPS  = RenderUtil::loadPS(device, L"PS_BloomPrefilter.cso");
    m_downsamplePS = RenderUtil::loadPS(device, L"PS_BloomDownsample.cso");
    m_upsamplePS   = RenderUtil::loadPS(device, L"PS_BloomUpsample.cso");
    m_compositePS  = RenderUtil::loadPS(device, L"PS_BloomComposite.cso");

    // 定数バッファ
    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<BloomParamsCB>(device);

    // サンプラー（リニアクランプ）
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    DX::ThrowIfFailed(device->CreateSamplerState(&sampDesc,
        m_sampler.ReleaseAndGetAddressOf()));

    // 加算ブレンドステート（アップサンプルパス用）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    DX::ThrowIfFailed(device->CreateBlendState(&blendDesc, m_additiveBlend.ReleaseAndGetAddressOf()));
}

// === ウィンドウサイズ依存リソース生成 ===

void Bloom::createWindowSizeDependentResources(int width, int height)
{
    auto device = m_deviceResources->GetD3DDevice();
    m_targetWidth  = static_cast<UINT>(width);
    m_targetHeight = static_cast<UINT>(height);

    // ブルームミップチェーン（各レベルは前の半分の解像度）
    UINT mipW = width / 2;
    UINT mipH = height / 2;

    for (int i = 0; i < MIP_COUNT; i++)
    {
        m_mipWidths[i] = mipW;
        m_mipHeights[i] = mipH;

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = mipW;
        texDesc.Height = mipH;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        DX::ThrowIfFailed(device->CreateTexture2D(&texDesc, nullptr,
            m_mipTextures[i].ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(device->CreateRenderTargetView(m_mipTextures[i].Get(), nullptr,
            m_mipRTVs[i].ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(device->CreateShaderResourceView(m_mipTextures[i].Get(), nullptr,
            m_mipSRVs[i].ReleaseAndGetAddressOf()));

        mipW = std::max(1u, mipW / 2);
        mipH = std::max(1u, mipH / 2);
    }

    // 合成出力RT（フル解像度、HDR）
    D3D11_TEXTURE2D_DESC compDesc = {};
    compDesc.Width = width;
    compDesc.Height = height;
    compDesc.MipLevels = 1;
    compDesc.ArraySize = 1;
    compDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    compDesc.SampleDesc.Count = 1;
    compDesc.Usage = D3D11_USAGE_DEFAULT;
    compDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    DX::ThrowIfFailed(device->CreateTexture2D(&compDesc, nullptr,
        m_compositeTexture.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateRenderTargetView(m_compositeTexture.Get(), nullptr,
        m_compositeRTV.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateShaderResourceView(m_compositeTexture.Get(), nullptr,
        m_compositeSRV.ReleaseAndGetAddressOf()));
}

// === 終了処理 ===

void Bloom::finalize()
{
    m_fullscreenVS.Reset();
    m_prefilterPS.Reset();
    m_downsamplePS.Reset();
    m_upsamplePS.Reset();
    m_compositePS.Reset();
    m_constantBuffer.Reset();
    m_sampler.Reset();
    m_additiveBlend.Reset();

    for (int i = 0; i < MIP_COUNT; i++)
    {
        m_mipTextures[i].Reset();
        m_mipRTVs[i].Reset();
        m_mipSRVs[i].Reset();
    }

    m_compositeTexture.Reset();
    m_compositeRTV.Reset();
    m_compositeSRV.Reset();
}

// === フルスクリーンパス（共通ヘルパー） ===

void Bloom::renderFullscreenPass(ID3D11PixelShader* ps,
    ID3D11RenderTargetView* outputRTV,
    ID3D11ShaderResourceView* input0,
    ID3D11ShaderResourceView* input1,
    UINT width, UINT height)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    context->OMSetRenderTargets(1, &outputRTV, nullptr);

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MaxDepth = 1.0f;
    context->RSSetViewports(1, &vp);

    // シェーダー設定
    context->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);

    // テクスチャ・サンプラー設定
    context->PSSetShaderResources(0, 1, &input0);
    if (input1)
    {
        context->PSSetShaderResources(1, 1, &input1);
    }
    context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // フルスクリーン三角形描画
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);
    context->Draw(3, 0);

    // SRV解除（DX11警告防止）
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRVs);
}

// === ブルーム描画 ===

void Bloom::render(ID3D11ShaderResourceView* sceneSRV)
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    UINT sceneWidth  = m_targetWidth;
    UINT sceneHeight = m_targetHeight;

    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);

    // 閾値カーブ事前計算（フレームごとに1回）
    float knee = m_threshold * m_knee;
    float safeKnee = std::max(knee, 0.0001f);

    BloomParamsCB cb = {};
    cb.threshold = XMFLOAT4(m_threshold,
        m_threshold - knee,
        safeKnee * 2.0f,
        0.25f / safeKnee);
    cb.sampleScale = m_upsampleScale;
    cb.bloomIntensity = m_intensity;

    // --- パス1: プリフィルタ（シーン → mip[0]） ---
    cb.texelSize = XMFLOAT2(1.0f / sceneWidth, 1.0f / sceneHeight);
    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

    renderFullscreenPass(m_prefilterPS.Get(), m_mipRTVs[0].Get(),
        sceneSRV, nullptr,
        m_mipWidths[0], m_mipHeights[0]);

    // --- パス2-5: ダウンサンプル（mip[i-1] → mip[i]） ---
    for (int i = 1; i < MIP_COUNT; i++)
    {
        cb.texelSize = XMFLOAT2(1.0f / m_mipWidths[i - 1], 1.0f / m_mipHeights[i - 1]);
        RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

        renderFullscreenPass(m_downsamplePS.Get(), m_mipRTVs[i].Get(),
            m_mipSRVs[i - 1].Get(), nullptr,
            m_mipWidths[i], m_mipHeights[i]);
    }

    // --- パス6-9: アップサンプル（加算ブレンド） ---
    context->OMSetBlendState(m_additiveBlend.Get(), nullptr, 0xFFFFFFFF);

    for (int i = MIP_COUNT - 2; i >= 0; i--)
    {
        cb.texelSize = XMFLOAT2(1.0f / m_mipWidths[i + 1], 1.0f / m_mipHeights[i + 1]);
        RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

        renderFullscreenPass(m_upsamplePS.Get(), m_mipRTVs[i].Get(),
            m_mipSRVs[i + 1].Get(), nullptr,
            m_mipWidths[i], m_mipHeights[i]);
    }

    // デフォルトブレンドステート復元
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    // --- パス10: 合成（シーン + ブルーム → 合成RT） ---
    cb.texelSize = XMFLOAT2(1.0f / m_mipWidths[0], 1.0f / m_mipHeights[0]);
    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

    renderFullscreenPass(m_compositePS.Get(), m_compositeRTV.Get(),
        sceneSRV, m_mipSRVs[0].Get(),
        sceneWidth, sceneHeight);
}
