#pragma once

#include <SimpleMath.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

inline constexpr int32_t IMPORTED_TEXTURE_NONE = -1;

struct ImportedModelVertex
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector3 normal   = DirectX::SimpleMath::Vector3::UnitY;
    DirectX::SimpleMath::Vector3 tangent  = DirectX::SimpleMath::Vector3::UnitX;
    DirectX::SimpleMath::Vector2 texcoord = DirectX::SimpleMath::Vector2::Zero;
};

struct ImportedSubmesh
{
    std::string name;
    uint32_t baseVertex = 0;
    uint32_t startIndex = 0;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
};

struct ImportedMaterial
{
    DirectX::SimpleMath::Color baseColor = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
    std::string name;
    std::string baseColorTexture;
    std::string diffuseTexture;
    float metallicFactor  = 0.0f;
    float roughnessFactor = 1.0f;
    int32_t baseColorTextureIndex         = IMPORTED_TEXTURE_NONE;
    int32_t normalTextureIndex            = IMPORTED_TEXTURE_NONE;
    int32_t metallicRoughnessTextureIndex = IMPORTED_TEXTURE_NONE;
    int32_t roughnessTextureIndex         = IMPORTED_TEXTURE_NONE;
    int32_t metalnessTextureIndex         = IMPORTED_TEXTURE_NONE;
    int32_t ambientOcclusionTextureIndex  = IMPORTED_TEXTURE_NONE;
    int32_t heightTextureIndex            = IMPORTED_TEXTURE_NONE;
};

struct ImportedTextureData
{
    std::string name;
    std::string source;
    std::string formatHint;
    std::vector<uint8_t> bytes;
    uint32_t width = 0;
    uint32_t height = 0;
    bool compressed = true;
    bool srgb = false;
    bool forceRGBA32 = false;
};

struct ImportedModelNode
{
    std::string name;
    DirectX::SimpleMath::Matrix localTransform = DirectX::SimpleMath::Matrix::Identity;
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
    uint32_t meshCount = 0;
};

struct ImportedModelData
{
    std::filesystem::path sourcePath;
    std::vector<ImportedModelVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ImportedSubmesh> submeshes;
    std::vector<ImportedMaterial> materials;
    std::vector<ImportedTextureData> textures;
    std::vector<ImportedModelNode> namedNodes;
};

class ImportedModel
{
public:
    bool initialize(ID3D11Device* device, ImportedModelData data);
    void finalize();

    const ImportedModelData& data() const { return m_data; }
    const std::vector<ImportedSubmesh>& submeshes() const { return m_data.submeshes; }
    const std::vector<ImportedMaterial>& materials() const { return m_data.materials; }
    const std::vector<ImportedTextureData>& textures() const { return m_data.textures; }
    const std::vector<ImportedModelNode>& namedNodes() const { return m_data.namedNodes; }
    const ImportedModelNode* findNamedNode(std::string_view name) const;

    ID3D11Buffer* vertexBuffer() const { return m_vertexBuffer.Get(); }
    ID3D11Buffer* indexBuffer() const { return m_indexBuffer.Get(); }
    ID3D11ShaderResourceView* textureSRV(int32_t textureIndex) const;
    uint32_t vertexStride() const { return sizeof(ImportedModelVertex); }
    uint32_t indexCount() const { return static_cast<uint32_t>(m_data.indices.size()); }
    uint32_t vertexCount() const { return static_cast<uint32_t>(m_data.vertices.size()); }

private:
    ImportedModelData m_data;
    com_ptr<ID3D11Buffer> m_vertexBuffer;
    com_ptr<ID3D11Buffer> m_indexBuffer;
    std::vector<com_ptr<ID3D11ShaderResourceView>> m_textureSRVs;
};
