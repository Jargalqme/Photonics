#include "pch.h"
#include "Render/Assets/AssimpModelImporter.h"

#include "Render/Assets/ImportedModel.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <fstream>
#include <charconv>
#include <unordered_map>

namespace
{
    constexpr unsigned int kImportFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_ValidateDataStructure |
        aiProcess_ConvertToLeftHanded;

    using TextureIndexMap = std::unordered_map<std::string, int32_t>;

    bool Exists(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    std::string ToString(const aiString& value)
    {
        return value.length > 0 ? std::string(value.C_Str()) : std::string();
    }

    void TraceLine(const std::string& text)
    {
        OutputDebugStringA(text.c_str());
        OutputDebugStringA("\n");
    }

    DirectX::SimpleMath::Vector3 ToVector3(const aiVector3D& value)
    {
        return DirectX::SimpleMath::Vector3(value.x, value.y, value.z);
    }

    DirectX::SimpleMath::Vector2 ToVector2(const aiVector3D& value)
    {
        return DirectX::SimpleMath::Vector2(value.x, value.y);
    }

    DirectX::SimpleMath::Matrix ToMatrix(const aiMatrix4x4& value)
    {
        return DirectX::SimpleMath::Matrix(
            value.a1, value.b1, value.c1, value.d1,
            value.a2, value.b2, value.c2, value.d2,
            value.a3, value.b3, value.c3, value.d3,
            value.a4, value.b4, value.c4, value.d4);
    }

    DirectX::SimpleMath::Color ToColor(const aiColor4D& value)
    {
        return DirectX::SimpleMath::Color(value.r, value.g, value.b, value.a);
    }

    bool IsViewmodelNodeName(const std::string& name)
    {
        return name.rfind("VM_", 0) == 0;
    }

    aiVector3D TransformOrigin(const aiMatrix4x4& transform)
    {
        return transform * aiVector3D(0.0f, 0.0f, 0.0f);
    }

    std::string FormatHint(const aiTexture& texture)
    {
        std::string result;
        for (char c : texture.achFormatHint)
        {
            if (c == '\0')
            {
                break;
            }
            result.push_back(c);
        }
        return result;
    }

    bool IsEmbeddedTextureReference(const std::string& textureReference)
    {
        return textureReference.size() > 1 && textureReference[0] == '*';
    }

    int32_t ParseEmbeddedTextureIndex(const std::string& textureReference)
    {
        if (!IsEmbeddedTextureReference(textureReference))
        {
            return IMPORTED_TEXTURE_NONE;
        }

        int32_t textureIndex = IMPORTED_TEXTURE_NONE;
        const char* begin = textureReference.data() + 1;
        const char* end = textureReference.data() + textureReference.size();
        const auto result = std::from_chars(begin, end, textureIndex);
        if (result.ec != std::errc() || result.ptr != end || textureIndex < 0)
        {
            return IMPORTED_TEXTURE_NONE;
        }

        return textureIndex;
    }

    bool ReadBinaryFile(const std::filesystem::path& path, std::vector<uint8_t>& outBytes)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return false;
        }

        const std::ifstream::pos_type fileSize = file.tellg();
        if (fileSize <= 0)
        {
            return false;
        }

        outBytes.resize(static_cast<size_t>(fileSize));
        file.seekg(0, std::ios::beg);
        file.read(
            reinterpret_cast<char*>(outBytes.data()),
            static_cast<std::streamsize>(fileSize));
        return file.good();
    }

    void AppendEmbeddedTextureData(const aiScene* scene, ImportedModelData& outData)
    {
        outData.textures.reserve(scene->mNumTextures);

        for (unsigned int i = 0; i < scene->mNumTextures; ++i)
        {
            ImportedTextureData importedTexture;
            importedTexture.source = "*" + std::to_string(i);

            const aiTexture* texture = scene->mTextures[i];
            if (!texture)
            {
                importedTexture.name = importedTexture.source;
                outData.textures.push_back(std::move(importedTexture));
                continue;
            }

            importedTexture.name = ToString(texture->mFilename);
            if (importedTexture.name.empty())
            {
                importedTexture.name = importedTexture.source;
            }
            importedTexture.formatHint = FormatHint(*texture);
            importedTexture.width = texture->mWidth;
            importedTexture.height = texture->mHeight;
            importedTexture.compressed = texture->mHeight == 0;

            if (importedTexture.compressed)
            {
                if (texture->pcData && texture->mWidth > 0)
                {
                    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(texture->pcData);
                    importedTexture.bytes.assign(bytes, bytes + texture->mWidth);
                }
            }
            else if (texture->pcData)
            {
                importedTexture.bytes.reserve(
                    static_cast<size_t>(texture->mWidth) *
                    static_cast<size_t>(texture->mHeight) * 4);

                const size_t texelCount =
                    static_cast<size_t>(texture->mWidth) *
                    static_cast<size_t>(texture->mHeight);
                for (size_t texelIndex = 0; texelIndex < texelCount; ++texelIndex)
                {
                    const aiTexel& texel = texture->pcData[texelIndex];
                    importedTexture.bytes.push_back(texel.r);
                    importedTexture.bytes.push_back(texel.g);
                    importedTexture.bytes.push_back(texel.b);
                    importedTexture.bytes.push_back(texel.a);
                }
            }

            outData.textures.push_back(std::move(importedTexture));
        }
    }

    int32_t AppendExternalTextureData(
        const std::string& textureReference,
        const std::filesystem::path& modelDirectory,
        ImportedModelData& outData,
        bool srgb,
        TextureIndexMap& textureIndexByPath)
    {
        if (textureReference.empty())
        {
            return IMPORTED_TEXTURE_NONE;
        }

        std::filesystem::path texturePath(textureReference);
        if (texturePath.is_relative())
        {
            texturePath = modelDirectory / texturePath;
        }

        const std::string resolvedTexturePath = texturePath.lexically_normal().generic_string();
        const auto found = textureIndexByPath.find(resolvedTexturePath);
        if (found != textureIndexByPath.end())
        {
            const int32_t textureIndex = found->second;
            outData.textures[textureIndex].srgb = outData.textures[textureIndex].srgb || srgb;
            return textureIndex;
        }

        ImportedTextureData importedTexture;
        importedTexture.name = std::filesystem::path(textureReference).filename().generic_string();
        importedTexture.source = resolvedTexturePath;
        importedTexture.formatHint = texturePath.extension().string();
        importedTexture.compressed = true;
        importedTexture.srgb = srgb;

        if (!ReadBinaryFile(texturePath, importedTexture.bytes))
        {
            TraceLine("[AssimpModelImporter] Failed to read texture: " + resolvedTexturePath);
            return IMPORTED_TEXTURE_NONE;
        }

        const int32_t textureIndex = static_cast<int32_t>(outData.textures.size());
        outData.textures.push_back(std::move(importedTexture));
        textureIndexByPath.emplace(resolvedTexturePath, textureIndex);
        return textureIndex;
    }

    int32_t ResolveTextureIndex(
        const std::string& textureReference,
        const std::filesystem::path& modelDirectory,
        ImportedModelData& outData,
        bool srgb,
        TextureIndexMap& textureIndexByPath)
    {
        if (textureReference.empty())
        {
            return IMPORTED_TEXTURE_NONE;
        }

        if (IsEmbeddedTextureReference(textureReference))
        {
            const int32_t textureIndex = ParseEmbeddedTextureIndex(textureReference);
            if (textureIndex >= 0 &&
                static_cast<size_t>(textureIndex) < outData.textures.size())
            {
                outData.textures[textureIndex].srgb = outData.textures[textureIndex].srgb || srgb;
                return textureIndex;
            }

            return IMPORTED_TEXTURE_NONE;
        }

        return AppendExternalTextureData(textureReference, modelDirectory, outData, srgb, textureIndexByPath);
    }

    aiVector3D NormalizeOrFallback(const aiVector3D& value, const aiVector3D& fallback)
    {
        aiVector3D result = value;
        const float lengthSquared =
            result.x * result.x +
            result.y * result.y +
            result.z * result.z;

        if (lengthSquared <= 0.000001f)
        {
            return fallback;
        }

        const float invLength = 1.0f / std::sqrt(lengthSquared);
        result.x *= invLength;
        result.y *= invLength;
        result.z *= invLength;
        return result;
    }

    void AppendMaterialData(
        const aiScene* scene,
        const std::filesystem::path& modelDirectory,
        ImportedModelData& outData,
        TextureIndexMap& textureIndexByPath)
    {
        outData.materials.reserve(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            const aiMaterial* material = scene->mMaterials[i];

            ImportedMaterial importedMaterial;
            if (material)
            {
                aiString materialName;
                if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
                {
                    importedMaterial.name = ToString(materialName);
                }

                aiColor4D baseColor;
                if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS)
                {
                    importedMaterial.baseColor = ToColor(baseColor);
                }

                aiString baseColorTexturePath;
                if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &baseColorTexturePath) == AI_SUCCESS)
                {
                    importedMaterial.baseColorTexture = ToString(baseColorTexturePath);
                    importedMaterial.baseColorTextureIndex = ResolveTextureIndex(
                        importedMaterial.baseColorTexture,
                        modelDirectory,
                        outData,
                        true,
                        textureIndexByPath);
                }

                aiString diffuseTexturePath;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexturePath) == AI_SUCCESS)
                {
                    importedMaterial.diffuseTexture = ToString(diffuseTexturePath);
                    if (importedMaterial.baseColorTextureIndex == IMPORTED_TEXTURE_NONE)
                    {
                        importedMaterial.baseColorTextureIndex = ResolveTextureIndex(
                            importedMaterial.diffuseTexture,
                            modelDirectory,
                            outData,
                            true,
                            textureIndexByPath);
                    }
                }

                ai_real metallic = importedMaterial.metallicFactor;
                if (aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &metallic) == AI_SUCCESS)
                {
                    importedMaterial.metallicFactor =
                        std::clamp(static_cast<float>(metallic), 0.0f, 1.0f);
                }

                ai_real roughness = importedMaterial.roughnessFactor;
                if (aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughness) == AI_SUCCESS)
                {
                    importedMaterial.roughnessFactor =
                        std::clamp(static_cast<float>(roughness), 0.0f, 1.0f);
                }

                aiColor4D emissiveColor;
                if (aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor) == AI_SUCCESS)
                {
                    importedMaterial.emissiveColor = ToColor(emissiveColor);
                }

                // KHR_materials_emissive_strength; intentionally not clamped above 1 (HDR)
                ai_real emissiveIntensity = importedMaterial.emissiveIntensity;
                if (aiGetMaterialFloat(material, AI_MATKEY_EMISSIVE_INTENSITY, &emissiveIntensity) == AI_SUCCESS)
                {
                    importedMaterial.emissiveIntensity =
                        std::max(static_cast<float>(emissiveIntensity), 0.0f);
                }

                aiString normalPath;
                if (material->GetTexture(aiTextureType_NORMALS, 0, &normalPath) == AI_SUCCESS)
                {
                    importedMaterial.normalTextureIndex = ResolveTextureIndex(
                        ToString(normalPath), modelDirectory, outData,
                        /*srgb*/ false, textureIndexByPath);
                }

                aiString metalRoughPath;
                if (material->GetTexture(aiTextureType_METALNESS, 0, &metalRoughPath) == AI_SUCCESS)
                {
                    importedMaterial.metallicRoughnessTextureIndex = ResolveTextureIndex(
                        ToString(metalRoughPath), modelDirectory, outData,
                        /*srgb*/ false, textureIndexByPath);
                }

                aiString roughnessPath;
                if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessPath) == AI_SUCCESS)
                {
                    const int32_t roughnessTextureIndex = ResolveTextureIndex(
                        ToString(roughnessPath),
                        modelDirectory,
                        outData,
                        false,
                        textureIndexByPath);

                    importedMaterial.roughnessTextureIndex = roughnessTextureIndex;
                }

                aiString emissivePath;
                if (material->GetTexture(aiTextureType_EMISSIVE, 0, &emissivePath) == AI_SUCCESS)
                {
                    importedMaterial.emissiveTextureIndex = ResolveTextureIndex(
                        ToString(emissivePath), modelDirectory, outData,
                        /*srgb*/ true, textureIndexByPath);
                }

                // glTF occlusionTexture arrives as LIGHTMAP ("aka Ambient Occlusion");
                // AMBIENT_OCCLUSION covers importers that use the newer PBR slot
                aiString occlusionPath;
                if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &occlusionPath) == AI_SUCCESS ||
                    material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &occlusionPath) == AI_SUCCESS)
                {
                    importedMaterial.ambientOcclusionTextureIndex = ResolveTextureIndex(
                        ToString(occlusionPath), modelDirectory, outData,
                        /*srgb*/ false, textureIndexByPath);
                }
            }

            outData.materials.push_back(std::move(importedMaterial));
        }
    }

    void AppendMeshData(
        const aiScene* scene,
        const aiMesh* mesh,
        const aiMatrix4x4& nodeTransform,
        ImportedModelData& outData)
    {
        if (!mesh || !mesh->HasPositions() || !mesh->HasFaces())
        {
            return;
        }

        ImportedSubmesh submesh;
        submesh.name = ToString(mesh->mName);
        submesh.baseVertex = static_cast<uint32_t>(outData.vertices.size());
        submesh.startIndex = static_cast<uint32_t>(outData.indices.size());
        submesh.materialIndex = mesh->mMaterialIndex;

        aiMatrix3x3 normalTransform(nodeTransform);
        normalTransform.Inverse();
        normalTransform.Transpose();

        outData.vertices.reserve(outData.vertices.size() + mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            const aiVector3D position = nodeTransform * mesh->mVertices[i];

            aiVector3D normal(0.0f, 1.0f, 0.0f);
            if (mesh->HasNormals())
            {
                normal = NormalizeOrFallback(normalTransform * mesh->mNormals[i], normal);
            }

            aiVector3D tangent(1.0f, 0.0f, 0.0f);
            if (mesh->HasTangentsAndBitangents())
            {
                tangent = NormalizeOrFallback(normalTransform * mesh->mTangents[i], tangent);
            }

            aiVector3D texcoord(0.0f, 0.0f, 0.0f);
            if (mesh->HasTextureCoords(0))
            {
                texcoord = mesh->mTextureCoords[0][i];
            }

            ImportedModelVertex vertex;
            vertex.position = ToVector3(position);
            vertex.normal = ToVector3(normal);
            vertex.tangent = ToVector3(tangent);
            vertex.texcoord = ToVector2(texcoord);
            outData.vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& face = mesh->mFaces[i];
            if (face.mNumIndices != 3)
            {
                continue;
            }

            outData.indices.push_back(face.mIndices[0] + submesh.baseVertex);
            outData.indices.push_back(face.mIndices[1] + submesh.baseVertex);
            outData.indices.push_back(face.mIndices[2] + submesh.baseVertex);
        }

        submesh.indexCount = static_cast<uint32_t>(outData.indices.size()) - submesh.startIndex;
        if (submesh.indexCount > 0)
        {
            outData.submeshes.push_back(std::move(submesh));
        }

        (void)scene;
    }

    void TraverseModelNode(
        const aiScene* scene,
        const aiNode* node,
        const aiMatrix4x4& parentTransform,
        ImportedModelData& outData)
    {
        if (!node)
        {
            return;
        }

        const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
        const std::string nodeName = ToString(node->mName);
        if (IsViewmodelNodeName(nodeName))
        {
            ImportedModelNode namedNode;
            namedNode.name = nodeName;
            namedNode.modelTransform = ToMatrix(nodeTransform);
            namedNode.modelPosition = ToVector3(TransformOrigin(nodeTransform));
            namedNode.meshCount = node->mNumMeshes;
            outData.namedNodes.push_back(std::move(namedNode));
        }

        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            const unsigned int meshIndex = node->mMeshes[i];
            if (meshIndex < scene->mNumMeshes)
            {
                AppendMeshData(scene, scene->mMeshes[meshIndex], nodeTransform, outData);
            }
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            TraverseModelNode(scene, node->mChildren[i], nodeTransform, outData);
        }
    }

}

bool AssimpModelImporter::LoadImportedModelData(
    const std::filesystem::path& path,
    ImportedModelData& outData,
    std::string* outError)
{
    outData = ImportedModelData{};
    outData.sourcePath = ResolvePath(path);

    if (!Exists(outData.sourcePath))
    {
        if (outError)
        {
            *outError = "File does not exist.";
        }
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(outData.sourcePath.string(), kImportFlags);
    if (!scene)
    {
        if (outError)
        {
            *outError = importer.GetErrorString();
        }
        return false;
    }

    if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode)
    {
        if (outError)
        {
            *outError = "Scene is incomplete.";
        }
        return false;
    }

    if (!scene->HasMeshes())
    {
        if (outError)
        {
            *outError = "Scene contains no meshes.";
        }
        return false;
    }

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        if (!mesh || !mesh->HasPositions())
        {
            continue;
        }

        vertexCount += mesh->mNumVertices;
        indexCount += mesh->mNumFaces * 3;
    }

    outData.vertices.reserve(vertexCount);
    outData.indices.reserve(indexCount);
    outData.submeshes.reserve(scene->mNumMeshes);

    TextureIndexMap textureIndexByPath;
    AppendEmbeddedTextureData(scene, outData);
    AppendMaterialData(scene, outData.sourcePath.parent_path(), outData, textureIndexByPath);
    TraverseModelNode(scene, scene->mRootNode, aiMatrix4x4(), outData);

    if (outData.vertices.empty() || outData.indices.empty())
    {
        if (outError)
        {
            *outError = "No renderable geometry was imported.";
        }
        return false;
    }

    return true;
}

std::filesystem::path AssimpModelImporter::ResolvePath(const std::filesystem::path& path)
{
    if (path.is_absolute() || Exists(path))
    {
        return path;
    }

    const std::filesystem::path executablePath = std::filesystem::path(GetExecutableDir()) / path;
    if (Exists(executablePath))
    {
        return executablePath;
    }

    return path;
}
