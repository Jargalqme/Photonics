#include "pch.h"
#include "Render/Pipeline/FinalCompositePass.h"
#include "Render/Pipeline/RenderUtil.h"

FinalCompositePass::FinalCompositePass(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
{
}

void FinalCompositePass::createDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Shared fullscreen-triangle VS (SV_VertexID-driven, no vertex buffer).
    m_vertexShader = RenderUtil::loadVS(device, L"VS_FullscreenTriangle.cso");

    // Final output PS: exposure, tonemap, gamma, then write to backbuffer.
    m_pixelShader = RenderUtil::loadPS(device, L"PS_FinalComposite.cso");

    // Linear clamp sampler. Backbuffer write never wraps.
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    DX::ThrowIfFailed(device->CreateSamplerState(&sampDesc,
        m_sampler.ReleaseAndGetAddressOf()));

    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<FinalCompositeCB>(device);
}

void FinalCompositePass::finalize()
{
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_sampler.Reset();
    m_constantBuffer.Reset();
}

void FinalCompositePass::process(
    ID3D11ShaderResourceView* inputSRV,
    ID3D11RenderTargetView* outputRTV,
    float exposure)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    context->OMSetRenderTargets(1, &outputRTV, nullptr);
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);

    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    FinalCompositeCB cb = {};
    cb.exposure = exposure;
    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

    context->PSSetShaderResources(0, 1, &inputSRV);
    context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // Fullscreen triangle: 3 vertices, SV_VertexID-driven, no IA buffer/layout.
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);
    context->Draw(3, 0);

    // Unbind SRV so the next frame can rebind it as RT without D3D warnings.
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);
}
