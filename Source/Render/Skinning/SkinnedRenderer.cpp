#include "pch.h"
#include "SkinnedRenderer.h"

#include "SkinnedModel.h"
#include "Render/Pipeline/RenderUtil.h"

namespace
{
    // Must match the HLSL definitions in VS_SkinnedMenu.hlsl /
    // PS_SkinnedMenu.hlsl exactly.
    struct SkinnedTransformCB
    {
        DirectX::SimpleMath::Matrix worldViewProjection;
        DirectX::SimpleMath::Matrix worldInverseTranspose;
        DirectX::SimpleMath::Vector4 tintColor;
        DirectX::SimpleMath::Vector4 lightDirectionAndAmbient;
        DirectX::SimpleMath::Vector4 materialFlags;
    };

    struct SkinnedPaletteCB
    {
        DirectX::SimpleMath::Matrix bones[MAX_BONES_PER_MODEL];
    };
}

bool SkinnedRenderer::initialize(ID3D11Device* device)
{
    if (!device) { return false; }
    finalize();

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    m_vs = RenderUtil::loadVS(device, L"VS_SkinnedMenu.cso", &vsBlob);
    m_ps = RenderUtil::loadPS(device, L"PS_SkinnedMenu.cso");

    // Mirrors SkinnedVertex layout (76 bytes per vertex). Offsets are:
    //   POSITION       0  (3 * float = 12)
    //   NORMAL        12  (3 * float = 12)
    //   TANGENT       24  (3 * float = 12)
    //   TEXCOORD0     36  (2 * float =  8)
    //   BLENDINDICES  44  (4 * uint  = 16)
    //   BLENDWEIGHT   60  (4 * float = 16)
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,     0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,        0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,   0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    DX::ThrowIfFailed(device->CreateInputLayout(
        layout, _countof(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        m_inputLayout.ReleaseAndGetAddressOf()));

    m_transformCB = RenderUtil::createDynamicConstantBuffer<SkinnedTransformCB>(device);
    m_paletteCB   = RenderUtil::createDynamicConstantBuffer<SkinnedPaletteCB>(device);

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    DX::ThrowIfFailed(device->CreateSamplerState(
        &sampDesc, m_sampler.ReleaseAndGetAddressOf()));

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    // Some accessories in imported character files use opposite triangle
    // winding, so the menu character is rendered double-sided.
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;
    DX::ThrowIfFailed(device->CreateRasterizerState(
        &rasterDesc, m_rasterizer.ReleaseAndGetAddressOf()));

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable    = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
    DX::ThrowIfFailed(device->CreateDepthStencilState(
        &depthDesc, m_depthState.ReleaseAndGetAddressOf()));

    return true;
}

void SkinnedRenderer::finalize()
{
    m_vs.Reset();
    m_ps.Reset();
    m_inputLayout.Reset();
    m_transformCB.Reset();
    m_paletteCB.Reset();
    m_sampler.Reset();
    m_rasterizer.Reset();
    m_depthState.Reset();
}

void SkinnedRenderer::draw(
    ID3D11DeviceContext* context,
    const SkinnedModel& model,
    const DirectX::SimpleMath::Matrix* palette,
    uint32_t paletteCount,
    const DirectX::SimpleMath::Matrix& world,
    const DirectX::SimpleMath::Matrix& view,
    const DirectX::SimpleMath::Matrix& projection,
    const DirectX::SimpleMath::Vector4& tintColor,
    const DirectX::SimpleMath::Vector4& lightDirectionAndAmbient)
{
    if (!context || !m_vs || !m_ps || !m_inputLayout ||
        !m_transformCB || !m_paletteCB || !m_sampler)
    {
        return;
    }
    if (!model.vertexBuffer() || !model.indexBuffer())
    {
        return;
    }

    // ------ Transform cbuffer (b0) ------
    DirectX::SimpleMath::Matrix normalWorld = world;
    normalWorld._41 = 0.0f;
    normalWorld._42 = 0.0f;
    normalWorld._43 = 0.0f;
    const DirectX::SimpleMath::Matrix normalMatrix = normalWorld.Invert().Transpose();

    SkinnedTransformCB tcb = {};
    tcb.worldViewProjection      = (world * view * projection).Transpose();
    tcb.worldInverseTranspose    = normalMatrix.Transpose();
    tcb.tintColor                = tintColor;
    tcb.lightDirectionAndAmbient = lightDirectionAndAmbient;

    RenderUtil::updateDynamicConstantBuffer(context, m_transformCB, tcb);

    // ------ Palette cbuffer (b1) ------
    SkinnedPaletteCB pcb = {};
    const uint32_t count = (paletteCount < MAX_BONES_PER_MODEL)
        ? paletteCount : MAX_BONES_PER_MODEL;
    const DirectX::SimpleMath::Matrix identity =
        DirectX::SimpleMath::Matrix::Identity;
    for (uint32_t i = 0; i < count; ++i)
    {
        // HLSL expects column-major matrices by default.
        pcb.bones[i] = palette[i].Transpose();
    }
    for (uint32_t i = count; i < MAX_BONES_PER_MODEL; ++i)
    {
        pcb.bones[i] = identity;
    }

    RenderUtil::updateDynamicConstantBuffer(context, m_paletteCB, pcb);

    // ------ Pipeline ------
    UINT stride = model.vertexStride();
    UINT offset = 0;
    ID3D11Buffer* vb = model.vertexBuffer();

    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(model.indexBuffer(), DXGI_FORMAT_R32_UINT, 0);

    ID3D11Buffer* transformCB = m_transformCB.Get();
    ID3D11Buffer* paletteCB   = m_paletteCB.Get();
    context->VSSetShader(m_vs.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &transformCB);
    context->VSSetConstantBuffers(1, 1, &paletteCB);

    context->PSSetShader(m_ps.Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, &transformCB);
    ID3D11SamplerState* sampler = m_sampler.Get();
    context->PSSetSamplers(0, 1, &sampler);

    context->RSSetState(m_rasterizer.Get());
    context->OMSetDepthStencilState(m_depthState.Get(), 0);

    // ------ Submesh loop. Bind per-submesh material flags + base color SRV. ------
    const auto& submeshes  = model.submeshes();
    const auto& materials  = model.materials();
    for (const auto& sub : submeshes)
    {
        if (sub.indexCount == 0) { continue; }

        // Patch material flags + tint into the transform cbuffer.
        ID3D11ShaderResourceView* srv = nullptr;
        DirectX::SimpleMath::Vector4 mtlFlags(0.0f, 0.0f, 0.0f, 0.0f);
        DirectX::SimpleMath::Vector4 mtlTint = tintColor;

        if (sub.materialIndex < materials.size())
        {
            const SkinnedMaterial& mat = materials[sub.materialIndex];
            mtlTint.x *= mat.baseColor.x;
            mtlTint.y *= mat.baseColor.y;
            mtlTint.z *= mat.baseColor.z;
            mtlTint.w *= mat.baseColor.w;
            if (mat.baseColorTextureIndex != SKINNED_TEXTURE_NONE)
            {
                srv = model.textureSRV(mat.baseColorTextureIndex);
                mtlFlags.x = srv ? 1.0f : 0.0f;
            }
        }

        SkinnedTransformCB tcbSub = tcb;
        tcbSub.tintColor    = mtlTint;
        tcbSub.materialFlags = mtlFlags;

        RenderUtil::updateDynamicConstantBuffer(context, m_transformCB, tcbSub);

        context->PSSetShaderResources(0, 1, &srv);

        context->DrawIndexed(sub.indexCount, sub.startIndex, 0);
    }

    // Leave the state where we found it for the rest of the frame.
    context->RSSetState(nullptr);
}
