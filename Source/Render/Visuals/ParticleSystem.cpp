#include "pch.h"
#include "Render/Visuals/ParticleSystem.h"
#include "Render/Pipeline/RenderUtil.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

ParticleSystem::ParticleSystem(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
{
}

void ParticleSystem::initialize()
{
    auto device = m_deviceResources->GetD3DDevice();

    // CPUパーティクル配列 — 全て非生存状態（lifetime = 0）
    m_particles.resize(MAX_PARTICLES, Particle{});

    // 動的構造化バッファ — 毎フレームCPU書き込み、GPU SRV読み取り
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = sizeof(Particle) * MAX_PARTICLES;
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufDesc.StructureByteStride = sizeof(Particle);
    DX::ThrowIfFailed(device->CreateBuffer(&bufDesc, nullptr, m_particleBuffer.ReleaseAndGetAddressOf()));

    // SRV（頂点シェーダーから読み取り用）
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements = MAX_PARTICLES;
    DX::ThrowIfFailed(device->CreateShaderResourceView(m_particleBuffer.Get(), &srvDesc, m_particleSRV.ReleaseAndGetAddressOf()));

    // 定数バッファ
    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<ParticleCB>(device);

    // シェーダー読み込み（VS + PS のみ、コンピュートなし）
    m_vertexShader = RenderUtil::loadVS(device, L"VS_Particle.cso");
    m_pixelShader = RenderUtil::loadPS(device, L"PS_Particle.cso");

    // ブレンドステート（加算合成）
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

    // 深度ステンシル（読み取りのみ — パーティクルは深度書き込みしない）
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    DX::ThrowIfFailed(device->CreateDepthStencilState(&dsDesc, m_depthStencilState.ReleaseAndGetAddressOf()));

    // ラスタライザ（カリングなし — ビルボードはカメラを向く）
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;
    DX::ThrowIfFailed(device->CreateRasterizerState(&rsDesc, m_rasterizerState.ReleaseAndGetAddressOf()));
}

void ParticleSystem::update(float deltaTime)
{
    m_totalTime += deltaTime;

    // --- 発生: リクエスト処理 ---
    for (auto& req : m_emitRequests)
    {
        uint32_t spawned = 0;
        for (uint32_t i = 0; i < MAX_PARTICLES && spawned < req.count; i++)
        {
            Particle& p = m_particles[i];
            if (p.lifetime > 0.0f) { continue; }

            // ランダム方向（上方向バイアス付き）
            float rx = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
            float ry = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
            float rz = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;

            Vector3 dir;
            dir.x = rx * req.spread;
            dir.y = fabsf(ry) + UPWARD_BIAS;
            dir.z = rz * req.spread;
            dir.Normalize();

            float speedVariance = 0.5f + 0.5f * (static_cast<float>(rand()) / RAND_MAX);
            float lifeVariance  = 0.5f + 0.5f * (static_cast<float>(rand()) / RAND_MAX);

            p.position = req.position;
            p.velocity = dir * req.speed * speedVariance;
            p.lifetime = req.lifetime * lifeVariance;
            p.maxLifetime = p.lifetime;
            p.color = req.color;
            p.size = BASE_SIZE + SIZE_VARIANCE * (static_cast<float>(rand()) / RAND_MAX);

            spawned++;
        }
    }
    m_emitRequests.clear();

    // --- 更新: 生存パーティクルを移動 ---
    for (uint32_t i = 0; i < MAX_PARTICLES; i++)
    {
        Particle& p = m_particles[i];
        if (p.lifetime <= 0.0f) { continue; }

        p.lifetime -= deltaTime;
        p.velocity.y -= GRAVITY * deltaTime;
        p.position += p.velocity * deltaTime;

        if (p.lifetime <= 0.0f)
        {
            p.lifetime = 0.0f;
        }
    }

    // --- GPUへアップロード ---
    auto context = m_deviceResources->GetD3DDeviceContext();
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = context->Map(m_particleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        memcpy(mapped.pData, m_particles.data(), sizeof(Particle) * MAX_PARTICLES);
        context->Unmap(m_particleBuffer.Get(), 0);
    }
}

void ParticleSystem::render(
    const Matrix& view,
    const Matrix& projection)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    ParticleCB cb = {};
    cb.inverseView = view.Invert().Transpose();
    cb.viewProjection = (view * projection).Transpose();

    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    context->VSSetShaderResources(0, 1, m_particleSRV.GetAddressOf());

    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    context->RSSetState(m_rasterizerState.Get());

    context->Draw(6 * MAX_PARTICLES, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->VSSetShaderResources(0, 1, &nullSRV);

    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}

void ParticleSystem::emit(
    const Vector3& position,
    const Vector4& color,
    uint32_t count,
    float speed,
    float lifetime,
    float spread)
{
    m_emitRequests.push_back({ position, color, count, speed, lifetime, spread });
}

void ParticleSystem::finalize()
{
    m_particles.clear();
    m_emitRequests.clear();
    m_particleBuffer.Reset();
    m_particleSRV.Reset();
    m_constantBuffer.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_blendState.Reset();
    m_depthStencilState.Reset();
    m_rasterizerState.Reset();
}
