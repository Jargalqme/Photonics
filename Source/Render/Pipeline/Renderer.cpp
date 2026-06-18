#include "pch.h"
#include "Render/Pipeline/Renderer.h"
#include "Render/Pipeline/RenderCommandQueue.h"
#include "Render/Visuals/Billboard.h"
#include "Render/Assets/ImportedModel.h"
#include "Render/Pipeline/RenderUtil.h"

#include <cstddef>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace
{
    struct ImportedModelCB
    {
        Matrix world;
        Matrix worldViewProjection;
        Matrix worldInverseTranspose;
        Vector4 color;
        Vector4 lightDirectionAndAmbient;
        Vector4 lightColor;
        Vector4 cameraPosition;
        Vector4 materialParams;
        Vector4 materialFlags;
        Vector4 materialFlags2;
    };

    static_assert((sizeof(ImportedModelCB) % 16) == 0);

    Vector4 CombineImportedModelColor(
        const ImportedModelCommand& command,
        const ImportedModel& model,
        const ImportedSubmesh& submesh)
    {
        Vector4 color(command.color.x, command.color.y, command.color.z, command.color.w);

        const auto& materials = model.materials();
        if (submesh.materialIndex < materials.size())
        {
            const Color& materialColor = materials[submesh.materialIndex].baseColor;
            color.x *= materialColor.x;
            color.y *= materialColor.y;
            color.z *= materialColor.z;
            color.w *= materialColor.w;
        }

        return color;
    }

    int32_t GetImportedModelBaseColorTextureIndex(
        const ImportedModel& model,
        const ImportedSubmesh& submesh)
    {
        const auto& materials = model.materials();
        if (submesh.materialIndex >= materials.size())
        {
            return IMPORTED_TEXTURE_NONE;
        }

        return materials[submesh.materialIndex].baseColorTextureIndex;
    }

    Color ApplyEmissiveIntensity(Color color, float emissiveIntensity)
    {
        if (emissiveIntensity <= 0.0f)
        {
            return color;
        }

        const float multiplier = 1.0f + emissiveIntensity;
        color.x *= multiplier;
        color.y *= multiplier;
        color.z *= multiplier;
        return color;
    }

    void SetImportedModelBlendState(
        ID3D11DeviceContext* context,
        const ImportedModelCommand& command,
        ID3D11BlendState* alphaBlendState,
        ID3D11BlendState* additiveBlendState)
    {
        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        switch (command.blendMode)
        {
        case BlendMode::AlphaBlend:
            context->OMSetBlendState(alphaBlendState, blendFactor, 0xFFFFFFFF);
            break;
        case BlendMode::Additive:
            context->OMSetBlendState(additiveBlendState, blendFactor, 0xFFFFFFFF);
            break;
        case BlendMode::Opaque:
        default:
            context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
            break;
        }
    }
}

Renderer::Renderer(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
    , m_sceneRenderer(std::make_unique<SceneRenderer>(deviceResources))
    , m_ui(std::make_unique<UIRenderer>())
{
}

void Renderer::CreateDeviceDependentResources()
{
    m_sceneRenderer->createDeviceDependentResources();
    CreateImportedModelResources();
    m_ui->createDeviceDependentResources(
        m_deviceResources->GetD3DDevice(),
        m_deviceResources->GetD3DDeviceContext());
}

void Renderer::CreateWindowSizeDependentResources()
{
    // シーン HDR テクスチャ + ブルームはレンダー解像度ベース
    int renderW = GetRenderWidth();
    int renderH = GetRenderHeight();
    m_sceneRenderer->createWindowSizeDependentResources(renderW, renderH);

    auto device = m_deviceResources->GetD3DDevice();
    UINT width = static_cast<UINT>(renderW);
    UINT height = static_cast<UINT>(renderH);

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    DX::ThrowIfFailed(device->CreateTexture2D(&texDesc, nullptr,
        m_sceneTexture.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateRenderTargetView(m_sceneTexture.Get(), nullptr,
        m_sceneRTV.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateShaderResourceView(m_sceneTexture.Get(), nullptr,
        m_sceneSRV.ReleaseAndGetAddressOf()));

    // シーン専用 DSV（レンダー解像度サイズ）— バックバッファ DSV とは別個に持つ。
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    DX::ThrowIfFailed(device->CreateTexture2D(&depthDesc, nullptr,
        m_sceneDepthTexture.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateDepthStencilView(m_sceneDepthTexture.Get(), nullptr,
        m_sceneDSV.ReleaseAndGetAddressOf()));

    // UI スケーリングはバックバッファ（ウィンドウ）サイズ
    auto windowSize = m_deviceResources->GetOutputSize();
    m_ui->setScreenSize(
        static_cast<float>(windowSize.right - windowSize.left),
        static_cast<float>(windowSize.bottom - windowSize.top));
}

void Renderer::SetRenderResolution(int width, int height)
{
    if (width == m_renderWidth && height == m_renderHeight)
    {
        return;
    }
    m_renderWidth = width;
    m_renderHeight = height;
    CreateWindowSizeDependentResources();
}

int Renderer::GetRenderWidth() const
{
    if (m_renderWidth > 0)
    {
        return m_renderWidth;
    }
    auto size = m_deviceResources->GetOutputSize();
    return int(size.right - size.left);
}

int Renderer::GetRenderHeight() const
{
    if (m_renderHeight > 0)
    {
        return m_renderHeight;
    }
    auto size = m_deviceResources->GetOutputSize();
    return int(size.bottom - size.top);
}

void Renderer::ApplyPostProcess()
{
    m_sceneRenderer->renderPostProcess(
        m_sceneSRV.Get(),
        m_deviceResources->GetRenderTargetView());
}

void Renderer::OnDeviceLost()
{
    m_sceneRenderer->onDeviceLost();
    m_sceneTexture.Reset();
    m_sceneRTV.Reset();
    m_sceneSRV.Reset();
    m_sceneDepthTexture.Reset();
    m_sceneDSV.Reset();
    m_importedModelVS.Reset();
    m_importedModelPS.Reset();
    m_importedModelInputLayout.Reset();
    m_importedModelConstantBuffer.Reset();
    m_importedModelSolidRasterizer.Reset();
    m_importedModelWireframeRasterizer.Reset();
    m_importedModelSampler.Reset();
    m_importedModelAlphaBlendState.Reset();
    m_importedModelAdditiveBlendState.Reset();

    m_ui->onDeviceLost();
}

void Renderer::BeginScene()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Scene color
    static const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Get RTV as local variable (important)
    ID3D11RenderTargetView* rtv = m_sceneRTV.Get();
    ID3D11DepthStencilView* dsv = m_sceneDSV.Get();

    // Set render target — シーン RTV + シーン DSV（両方ともレンダー解像度）
    context->OMSetRenderTargets(1, &rtv, dsv);

    // Clear
    context->ClearRenderTargetView(rtv, clearColor);
    context->ClearDepthStencilView(dsv,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // ビューポートはレンダー解像度に合わせる（デフォルトはバックバッファサイズ）。
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width  = float(GetRenderWidth());
    viewport.Height = float(GetRenderHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);
}

void Renderer::EndScene()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Unbind scene texture so we can read from it
    ID3D11RenderTargetView* nullRTV = nullptr;
    context->OMSetRenderTargets(1, &nullRTV, nullptr);

    // Hand the HDR scene to SceneRenderer for post-process + backbuffer copy.
    ApplyPostProcess();
}

void Renderer::CreateImportedModelResources()
{
    auto* device = m_deviceResources->GetD3DDevice();

    ComPtr<ID3DBlob> vsBlob;
    m_importedModelVS = RenderUtil::loadVS(device, L"VS_ImportedModel.cso", &vsBlob);
    m_importedModelPS = RenderUtil::loadPS(device, L"PS_ImportedModel.cso");

    const D3D11_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(ImportedModelVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(ImportedModelVertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(ImportedModelVertex, tangent),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(ImportedModelVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    DX::ThrowIfFailed(device->CreateInputLayout(
        inputElements,
        static_cast<UINT>(std::size(inputElements)),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        m_importedModelInputLayout.ReleaseAndGetAddressOf()));

    m_importedModelConstantBuffer = RenderUtil::createDynamicConstantBuffer<ImportedModelCB>(device);

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.DepthClipEnable = TRUE;
    DX::ThrowIfFailed(device->CreateRasterizerState(
        &rasterizerDesc,
        m_importedModelSolidRasterizer.ReleaseAndGetAddressOf()));

    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    DX::ThrowIfFailed(device->CreateRasterizerState(
        &rasterizerDesc,
        m_importedModelWireframeRasterizer.ReleaseAndGetAddressOf()));

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    DX::ThrowIfFailed(device->CreateSamplerState(
        &samplerDesc,
        m_importedModelSampler.ReleaseAndGetAddressOf()));

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    DX::ThrowIfFailed(device->CreateBlendState(
        &blendDesc,
        m_importedModelAlphaBlendState.ReleaseAndGetAddressOf()));

    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    DX::ThrowIfFailed(device->CreateBlendState(
        &blendDesc,
        m_importedModelAdditiveBlendState.ReleaseAndGetAddressOf()));
}

void Renderer::DrawImportedModelCommand(
    const ImportedModelCommand& command,
    const Matrix& view,
    const Matrix& projection,
    const Vector3& cameraPosition,
    const SceneLighting& lighting)
{
    if (!command.model ||
        !command.model->vertexBuffer() ||
        !command.model->indexBuffer() ||
        !m_importedModelVS ||
        !m_importedModelPS ||
        !m_importedModelInputLayout ||
        !m_importedModelConstantBuffer ||
        !m_importedModelSampler)
    {
        return;
    }

    auto* context = m_deviceResources->GetD3DDeviceContext();
    const ImportedModel& model = *command.model;

    UINT stride = model.vertexStride();
    UINT offset = 0;
    ID3D11Buffer* vertexBuffer = model.vertexBuffer();
    ID3D11Buffer* constantBuffer = m_importedModelConstantBuffer.Get();

    context->IASetInputLayout(m_importedModelInputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(model.indexBuffer(), DXGI_FORMAT_R32_UINT, 0);

    context->VSSetShader(m_importedModelVS.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(m_importedModelPS.Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, &constantBuffer);
    ID3D11SamplerState* sampler = m_importedModelSampler.Get();
    context->PSSetSamplers(0, 1, &sampler);

    ID3D11ShaderResourceView* irradianceSRV = m_irradianceSRV;
    context->PSSetShaderResources(3, 1, &irradianceSRV);

    ID3D11RasterizerState* rasterizer =
        command.wireframe ? m_importedModelWireframeRasterizer.Get() : m_importedModelSolidRasterizer.Get();
    context->RSSetState(rasterizer);

    Matrix normalWorld = command.world;
    normalWorld._41 = 0.0f;
    normalWorld._42 = 0.0f;
    normalWorld._43 = 0.0f;

    const Matrix normalMatrix = normalWorld.Invert().Transpose();

    ImportedModelCB cb = {};
    cb.world = command.world.Transpose();
    cb.worldViewProjection = (command.world * view * projection).Transpose();
    cb.worldInverseTranspose = normalMatrix.Transpose();


    const auto& keyLight = lighting.keyLight;
    const auto& ambient = lighting.ambient;
    const auto& lightColor = keyLight.color;
    Vector3 lightDir = keyLight.directionToLight;
    if (lightDir.LengthSquared() > 0.0001f)
    {
        lightDir.Normalize();
    }
    else
    {
        lightDir = Vector3::Up;
    }

    cb.lightDirectionAndAmbient = Vector4(
        lightDir.x,
        lightDir.y,
        lightDir.z,
        ambient.intensity);

    cb.lightColor = Vector4(
        lightColor.x * keyLight.intensity,
        lightColor.y * keyLight.intensity,
        lightColor.z * keyLight.intensity,
        1.0f);

    cb.cameraPosition = Vector4(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f);

    const auto& submeshes = model.submeshes();
    if (submeshes.empty())
    {
        cb.color = Vector4(command.color.x, command.color.y, command.color.z, command.color.w);
        cb.materialFlags = Vector4(0.0f, command.emissiveIntensity, 0.0f, 0.0f);
        cb.materialFlags2 = Vector4::Zero;
        cb.materialParams = Vector4(0.0f, 1.0f, 0.0f, 0.0f);   // metallic 0, rough 1, no maps

        ID3D11ShaderResourceView* nullSRVs[3] = { nullptr, nullptr, nullptr };
        context->PSSetShaderResources(0, 3, nullSRVs);

        RenderUtil::updateDynamicConstantBuffer(context, m_importedModelConstantBuffer, cb);

        context->DrawIndexed(model.indexCount(), 0, 0);
        context->RSSetState(nullptr);
        return;
    }

    for (const auto& submesh : submeshes)
    {
        if (submesh.indexCount == 0)
        {
            continue;
        }

        cb.color = CombineImportedModelColor(command, model, submesh);

        const int32_t baseColorIndex = GetImportedModelBaseColorTextureIndex(model, submesh);
        ID3D11ShaderResourceView* baseColorSRV = model.textureSRV(baseColorIndex);

        float metallic = 1.0f, roughness = 1.0f;

        int32_t normalIndex = IMPORTED_TEXTURE_NONE;
        int32_t slot2TextureIndex = IMPORTED_TEXTURE_NONE;
        int32_t metalnessIndex = IMPORTED_TEXTURE_NONE;
        int32_t aoIndex = IMPORTED_TEXTURE_NONE;
        bool slot2IsStandaloneRoughness = false;

        const auto& materials = model.materials();
        if (submesh.materialIndex < materials.size())
        {
            const ImportedMaterial& mat = materials[submesh.materialIndex];
            metallic = mat.metallicFactor;
            roughness = mat.roughnessFactor;
            normalIndex = mat.normalTextureIndex;
            metalnessIndex = mat.metalnessTextureIndex;
            aoIndex = mat.ambientOcclusionTextureIndex;

            if (mat.metallicRoughnessTextureIndex != IMPORTED_TEXTURE_NONE)
            {
                slot2TextureIndex = mat.metallicRoughnessTextureIndex;
                slot2IsStandaloneRoughness = false;
            }
            else if (mat.roughnessTextureIndex != IMPORTED_TEXTURE_NONE)
            {
                slot2TextureIndex = mat.roughnessTextureIndex;
                slot2IsStandaloneRoughness = true;
            }
        }

        ID3D11ShaderResourceView* normalSRV = model.textureSRV(normalIndex);
        ID3D11ShaderResourceView* slot2SRV = model.textureSRV(slot2TextureIndex);
        ID3D11ShaderResourceView* metalnessSRV = model.textureSRV(metalnessIndex);
        ID3D11ShaderResourceView* aoSRV = model.textureSRV(aoIndex);

        cb.materialFlags = Vector4(
            baseColorSRV ? 1.0f : 0.0f,
            command.emissiveIntensity,
            metalnessSRV ? 1.0f : 0.0f,
            (slot2SRV && slot2IsStandaloneRoughness) ? 1.0f : 0.0f);

        cb.materialFlags2 = Vector4(
            aoSRV ? 1.0f : 0.0f,
            0.0f,
            0.0f,
            0.0f);

        cb.materialParams = Vector4(
            metallic,
            roughness,
            normalSRV ? 1.0f : 0.0f,
            slot2SRV ? 1.0f : 0.0f);

        ID3D11ShaderResourceView* srvs[3] = { baseColorSRV, normalSRV, slot2SRV };
        context->PSSetShaderResources(0, 3, srvs);
        context->PSSetShaderResources(4, 1, &metalnessSRV);
        context->PSSetShaderResources(5, 1, &aoSRV);

        RenderUtil::updateDynamicConstantBuffer(context, m_importedModelConstantBuffer, cb);

        context->DrawIndexed(submesh.indexCount, submesh.startIndex, 0);
    }

    ID3D11ShaderResourceView* nullSRVs[6] = {};
    context->PSSetShaderResources(0, 6, nullSRVs);
    context->RSSetState(nullptr);
}

void Renderer::ExecuteRenderCommands(
    const RenderCommandQueue& queue,
    const Matrix& view,
    const Matrix& projection,
    const Vector3& cameraPosition,
    const SceneLighting& lighting)
{
    auto drawMesh = [&](const MeshCommand& command)
    {
        if (!command.mesh)
        {
            return;
        }

        command.mesh->Draw(
            command.world,
            view,
            projection,
            ApplyEmissiveIntensity(command.color, command.emissiveIntensity),
            nullptr,
            command.wireframe);
    };

    for (const auto& command : queue.opaqueMeshes())
    {
        drawMesh(command);
    }

    auto* context = m_deviceResources->GetD3DDeviceContext();
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (const auto& command : queue.opaqueImportedModels())
    {
        context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
        DrawImportedModelCommand(command, view, projection, cameraPosition, lighting);
    }

    for (const auto& command : queue.transparentMeshes())
    {
        drawMesh(command);
    }

    for (const auto& command : queue.transparentImportedModels())
    {
        SetImportedModelBlendState(
            context,
            command,
            m_importedModelAlphaBlendState.Get(),
            m_importedModelAdditiveBlendState.Get());
        DrawImportedModelCommand(command, view, projection, cameraPosition, lighting);
    }

    context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);

    for (const auto& command : queue.transparentBillboards())
    {
        if (!command.billboard)
        {
            continue;
        }

        command.billboard->render(view, projection, command.position, command.size);
    }
}

void Renderer::BeginViewmodelPass()
{
    // 深度をクリアしてビューモデル（武器）を最前面に描画できるようにする。
    // シーン DSV を対象にする（レンダー解像度サイズ）。
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->ClearDepthStencilView(
        m_sceneDSV.Get(),
        D3D11_CLEAR_DEPTH, 1.0f, 0);
}

