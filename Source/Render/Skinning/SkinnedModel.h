#pragma once

#include "SkinnedModelData.h"

#include <wrl/client.h>
#include <cstdint>
#include <vector>

struct ID3D11Buffer;
struct ID3D11Device;
struct ID3D11ShaderResourceView;

// GPU-side companion to SkinnedModelData. Owns the vertex/index buffers and
// per-texture SRVs. Mirrors the role ImportedModel.cpp plays for the static
// path but lives in the Skinning folder and references SkinnedVertex layout.
class SkinnedModel
{
public:
    bool initialize(ID3D11Device* device, SkinnedModelData data);
    void finalize();

    const SkinnedModelData& data() const { return m_data; }
    const std::vector<SkinnedSubmesh>& submeshes() const { return m_data.submeshes; }
    const std::vector<SkinnedMaterial>& materials() const { return m_data.materials; }
    const std::vector<AnimationClip>& clips() const { return m_data.clips; }

    ID3D11Buffer* vertexBuffer() const { return m_vertexBuffer.Get(); }
    ID3D11Buffer* indexBuffer()  const { return m_indexBuffer.Get(); }
    ID3D11ShaderResourceView* textureSRV(int32_t textureIndex) const;

    uint32_t vertexStride() const { return sizeof(SkinnedVertex); }

private:
    SkinnedModelData m_data;
    com_ptr<ID3D11Buffer> m_vertexBuffer;
    com_ptr<ID3D11Buffer> m_indexBuffer;
    std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textureSRVs;
};
