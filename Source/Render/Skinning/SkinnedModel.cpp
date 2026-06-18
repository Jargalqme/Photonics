#include "pch.h"
#include "SkinnedModel.h"

#include "Render/Pipeline/RenderUtil.h"
#include "WICTextureLoader.h"

namespace
{
    void TraceLine(const std::string& text)
    {
        OutputDebugStringA(text.c_str());
        OutputDebugStringA("\n");
    }

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCompressedSRV(
        ID3D11Device* device,
        const SkinnedTextureData& tex)
    {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        if (tex.bytes.empty()) { return srv; }

        const auto loadFlags = tex.srgb
            ? DirectX::WIC_LOADER_FORCE_SRGB
            : DirectX::WIC_LOADER_IGNORE_SRGB;

        const HRESULT hr = DirectX::CreateWICTextureFromMemoryEx(
            device,
            tex.bytes.data(),
            tex.bytes.size(),
            0,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE,
            0,
            0,
            loadFlags,
            nullptr,
            srv.ReleaseAndGetAddressOf());

        if (FAILED(hr))
        {
            TraceLine("[SkinnedModel] Failed to decode texture: " + tex.name);
            return {};
        }
        return srv;
    }

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateRawSRV(
        ID3D11Device* device,
        const SkinnedTextureData& tex)
    {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

        if (tex.width == 0 || tex.height == 0 ||
            tex.bytes.size() !=
                static_cast<size_t>(tex.width) * static_cast<size_t>(tex.height) * 4)
        {
            return srv;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width      = tex.width;
        desc.Height     = tex.height;
        desc.MipLevels  = 1;
        desc.ArraySize  = 1;
        desc.Format     = tex.srgb
            ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
            : DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage      = D3D11_USAGE_DEFAULT;
        desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = tex.bytes.data();
        init.SysMemPitch = tex.width * 4;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
        if (FAILED(device->CreateTexture2D(&desc, &init, texture2d.ReleaseAndGetAddressOf())))
        {
            TraceLine("[SkinnedModel] Failed to create raw texture: " + tex.name);
            return srv;
        }
        if (FAILED(device->CreateShaderResourceView(texture2d.Get(), nullptr,
            srv.ReleaseAndGetAddressOf())))
        {
            TraceLine("[SkinnedModel] Failed to create raw texture SRV: " + tex.name);
            return {};
        }
        return srv;
    }
}

bool SkinnedModel::initialize(ID3D11Device* device, SkinnedModelData data)
{
    finalize();

    if (!device || data.vertices.empty() || data.indices.empty())
    {
        return false;
    }

    const size_t vertexBytes = data.vertices.size() * sizeof(SkinnedVertex);
    const size_t indexBytes  = data.indices.size() * sizeof(uint32_t);
    if (vertexBytes > UINT32_MAX || indexBytes > UINT32_MAX)
    {
        return false;
    }

    m_vertexBuffer = RenderUtil::createStaticVertexBuffer(
        device,
        data.vertices.data(),
        static_cast<UINT>(data.vertices.size()));
    m_indexBuffer = RenderUtil::createStaticIndexBuffer(
        device,
        data.indices.data(),
        static_cast<UINT>(data.indices.size()));

    m_textureSRVs.resize(data.textures.size());
    for (size_t i = 0; i < data.textures.size(); ++i)
    {
        const SkinnedTextureData& tex = data.textures[i];
        m_textureSRVs[i] = tex.compressed
            ? CreateCompressedSRV(device, tex)
            : CreateRawSRV(device, tex);
    }

    m_data = std::move(data);
    return true;
}

void SkinnedModel::finalize()
{
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_textureSRVs.clear();
    m_data = SkinnedModelData{};
}

ID3D11ShaderResourceView* SkinnedModel::textureSRV(int32_t textureIndex) const
{
    if (textureIndex < 0 ||
        static_cast<size_t>(textureIndex) >= m_textureSRVs.size())
    {
        return nullptr;
    }
    return m_textureSRVs[textureIndex].Get();
}
