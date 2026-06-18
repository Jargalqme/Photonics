#include "pch.h"
#include "Render/Visuals/BulletRenderer.h"

#include "Gameplay/BulletPool.h"
#include "Render/Pipeline/RenderUtil.h"

#include <cstring>

using namespace DirectX;
using namespace DirectX::SimpleMath;

BulletRenderer::BulletRenderer(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
{
}

void BulletRenderer::initialize()
{
    auto device = m_deviceResources->GetD3DDevice();

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(BulletRenderData) * MAX_BULLETS_TO_RENDER;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(BulletRenderData);
    DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_bulletBuffer.ReleaseAndGetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements = MAX_BULLETS_TO_RENDER;
    DX::ThrowIfFailed(device->CreateShaderResourceView(m_bulletBuffer.Get(), &srvDesc, m_bulletSRV.ReleaseAndGetAddressOf()));

    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<BulletRenderCB>(device);

    m_vertexShader = RenderUtil::loadVS(device, L"VS_Orb.cso");
    m_pixelShader = RenderUtil::loadPS(device, L"PS_Orb.cso");

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    DX::ThrowIfFailed(device->CreateBlendState(&blendDesc, m_blendState.ReleaseAndGetAddressOf()));

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    DX::ThrowIfFailed(device->CreateDepthStencilState(&depthDesc, m_depthStencilState.ReleaseAndGetAddressOf()));

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;
    DX::ThrowIfFailed(device->CreateRasterizerState(&rasterizerDesc, m_rasterizerState.ReleaseAndGetAddressOf()));
}

void BulletRenderer::update(float deltaTime)
{
    m_time += deltaTime;
}

void BulletRenderer::render(
    BulletPool* bulletPool,
    const Matrix& view,
    const Matrix& projection,
    const Vector3& cameraPosition)
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    Bullet* bullets = bulletPool->getBullets();
    const int maxBullets = bulletPool->getMaxBullets();

    uint32_t bulletCount = 0;
    for (int i = 0; i < maxBullets && bulletCount < MAX_BULLETS_TO_RENDER; ++i)
    {
        if (!bullets[i].isActive())
        {
            continue;
        }

        BulletRenderData& data = m_bulletData[bulletCount];
        data.position = bullets[i].getPosition();
        data.speed = bullets[i].getMaxSpeed();
        data.direction = bullets[i].getDirection();
        data.age = bullets[i].getAge();
        data.color = bullets[i].getColor();
        ++bulletCount;
    }

    if (bulletCount == 0)
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    DX::ThrowIfFailed(context->Map(m_bulletBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    std::memcpy(mapped.pData, m_bulletData, sizeof(BulletRenderData) * bulletCount);
    context->Unmap(m_bulletBuffer.Get(), 0);

    BulletRenderCB constants = {};
    constants.viewProjection = (view * projection).Transpose();
    constants.cameraPosition = cameraPosition;
    constants.time = m_time;
    constants.boltLength = 0.0f;
    constants.boltWidth = BULLET_ORB_RADIUS;
    constants.boltCount = bulletCount;
    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, constants);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->VSSetShaderResources(0, 1, m_bulletSRV.GetAddressOf());

    const float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    context->RSSetState(m_rasterizerState.Get());

    context->Draw(VERTICES_PER_ORB * bulletCount, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->VSSetShaderResources(0, 1, &nullSRV);
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}

void BulletRenderer::finalize()
{
    m_bulletBuffer.Reset();
    m_bulletSRV.Reset();
    m_constantBuffer.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_blendState.Reset();
    m_depthStencilState.Reset();
    m_rasterizerState.Reset();
}
