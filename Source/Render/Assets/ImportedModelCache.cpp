#include "pch.h"
#include "Render/Assets/ImportedModelCache.h"

#include "Render/Assets/AssimpModelImporter.h"
#include "Render/Assets/PBRMaterialLoader.h"
#include "Render/Assets/PrimitiveMesh.h"

namespace
{
    void TraceLine(const std::string& text)
    {
        OutputDebugStringA(text.c_str());
        OutputDebugStringA("\n");
    }

    std::string MakeAmbientCGCacheKey(
        const std::string& path,
        const std::filesystem::path& materialDirectory)
    {
        return path + "|ambientCG=" + materialDirectory.lexically_normal().generic_string();
    }

    std::string MakePBRBoxAmbientCGCacheKey(
        const std::filesystem::path& materialDirectory,
        DirectX::SimpleMath::Vector2 uvTiling)
    {
        return std::string("Primitive:PBRBox")
            + "|ambientCG=" + materialDirectory.lexically_normal().generic_string()
            + "|uv="
            + std::to_string(uvTiling.x)
            + ","
            + std::to_string(uvTiling.y);
    }
}

void ImportedModelCache::initialize(ID3D11Device* device)
{
    m_device = device;
}

void ImportedModelCache::finalize()
{
    m_models.clear();
    m_device = nullptr;
}

const ImportedModel* ImportedModelCache::get(const std::string& path)
{
    auto it = m_models.find(path);
    if (it != m_models.end())
    {
        return it->second.get();
    }

    if (!m_device)
    {
        return nullptr;
    }

    ImportedModelData data;
    std::string error;
    if (!AssimpModelImporter::LoadImportedModelData(path, data, &error))
    {
        TraceLine("[ImportedModelCache] Failed to load " + path + ": " + error);
        return nullptr;
    }

    auto model = std::make_unique<ImportedModel>();
    if (!model->initialize(m_device, std::move(data)))
    {
        TraceLine("[ImportedModelCache] Failed to create GPU buffers for " + path);
        return nullptr;
    }

    const ImportedModel* result = model.get();
    m_models.emplace(path, std::move(model));

    TraceLine("[ImportedModelCache] Loaded " + path
        + " vertices=" + std::to_string(result->vertexCount())
        + " indices=" + std::to_string(result->indexCount())
        + " submeshes=" + std::to_string(result->submeshes().size())
        + " textures=" + std::to_string(result->textures().size())
        + " VM nodes=" + std::to_string(result->namedNodes().size()));

    for (const ImportedModelNode& node : result->namedNodes())
    {
        TraceLine("[ImportedModelCache]   " + node.name
            + " pos=("
            + std::to_string(node.position.x) + ", "
            + std::to_string(node.position.y) + ", "
            + std::to_string(node.position.z) + ")"
            + " meshes=" + std::to_string(node.meshCount));
    }

    return result;
}

const ImportedModel* ImportedModelCache::getWithAmbientCGMaterial(
    const std::string& path,
    const std::filesystem::path& materialDirectory)
{
    const std::string cacheKey = MakeAmbientCGCacheKey(path, materialDirectory);
    auto it = m_models.find(cacheKey);
    if (it != m_models.end())
    {
        return it->second.get();
    }

    if (!m_device)
    {
        return nullptr;
    }

    ImportedModelData data;
    std::string error;
    if (!AssimpModelImporter::LoadImportedModelData(path, data, &error))
    {
        TraceLine("[ImportedModelCache] Failed to load " + path + ": " + error);
        return nullptr;
    }

    for (uint32_t materialIndex = 0; materialIndex < data.materials.size(); ++materialIndex)
    {
        std::string materialError;
        if (!PBRMaterialLoader::ApplyAmbientCGTextureSet(
            data,
            materialIndex,
            materialDirectory,
            &materialError))
        {
            TraceLine("[ImportedModelCache] Failed to apply ambientCG material "
                + materialDirectory.generic_string()
                + " to "
                + path
                + ": "
                + materialError);
            return nullptr;
        }
    }

    auto model = std::make_unique<ImportedModel>();
    if (!model->initialize(m_device, std::move(data)))
    {
        TraceLine("[ImportedModelCache] Failed to create GPU buffers for "
            + path
            + " with ambientCG material "
            + materialDirectory.generic_string());
        return nullptr;
    }

    const ImportedModel* result = model.get();
    m_models.emplace(cacheKey, std::move(model));

    TraceLine("[ImportedModelCache] Loaded " + path
        + " with ambientCG material " + materialDirectory.generic_string()
        + " vertices=" + std::to_string(result->vertexCount())
        + " indices=" + std::to_string(result->indexCount())
        + " submeshes=" + std::to_string(result->submeshes().size())
        + " textures=" + std::to_string(result->textures().size()));

    return result;
}

const ImportedModel* ImportedModelCache::getPBRBoxWithAmbientCGMaterial(
    const std::filesystem::path& materialDirectory,
    DirectX::SimpleMath::Vector2 uvTiling)
{
    const std::string cacheKey = MakePBRBoxAmbientCGCacheKey(materialDirectory, uvTiling);
    auto it = m_models.find(cacheKey);
    if (it != m_models.end())
    {
        return it->second.get();
    }

    if (!m_device)
    {
        return nullptr;
    }

    ImportedModelData data = PrimitiveMesh::CreatePBRBox(1.0f, 1.0f, 1.0f, uvTiling);
    data.sourcePath = cacheKey;

    std::string materialError;
    if (!PBRMaterialLoader::ApplyAmbientCGTextureSet(
        data,
        0,
        materialDirectory,
        &materialError))
    {
        TraceLine("[ImportedModelCache] Failed to apply ambientCG material "
            + materialDirectory.generic_string()
            + " to generated PBR box: "
            + materialError);
        return nullptr;
    }

    auto model = std::make_unique<ImportedModel>();
    if (!model->initialize(m_device, std::move(data)))
    {
        TraceLine("[ImportedModelCache] Failed to create GPU buffers for generated PBR box with ambientCG material "
            + materialDirectory.generic_string());
        return nullptr;
    }

    const ImportedModel* result = model.get();
    m_models.emplace(cacheKey, std::move(model));

    TraceLine("[ImportedModelCache] Loaded generated PBR box with ambientCG material "
        + materialDirectory.generic_string()
        + " vertices=" + std::to_string(result->vertexCount())
        + " indices=" + std::to_string(result->indexCount())
        + " textures=" + std::to_string(result->textures().size()));

    return result;
}
