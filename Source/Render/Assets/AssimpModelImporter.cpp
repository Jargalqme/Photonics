#include "pch.h"
#include "Render/Assets/AssimpModelImporter.h"

#include "Render/Assets/ImportedModel.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <fstream>
#include <charconv>
#include <sstream>
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

    std::string ToDisplayPath(const std::filesystem::path& path)
    {
        return path.generic_string();
    }

    std::string ToString(const aiString& value)
    {
        return value.length > 0 ? std::string(value.C_Str()) : std::string();
    }

    uint32_t CountNodes(const aiNode* node)
    {
        if (!node)
        {
            return 0;
        }

        uint32_t count = 1;
        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            count += CountNodes(node->mChildren[i]);
        }
        return count;
    }

    std::string BoolText(bool value)
    {
        return value ? "yes" : "no";
    }

    void TraceLine(const std::string& text)
    {
        OutputDebugStringA(text.c_str());
        OutputDebugStringA("\n");
    }

    std::string FormatColor(const std::array<float, 4>& color)
    {
        std::ostringstream out;
        out << "("
            << color[0] << ", "
            << color[1] << ", "
            << color[2] << ", "
            << color[3] << ")";
        return out.str();
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

    std::array<float, 3> ToArray3(const aiVector3D& value)
    {
        return { value.x, value.y, value.z };
    }

    std::string FormatVector3(const std::array<float, 3>& value)
    {
        std::ostringstream out;
        out << "("
            << value[0] << ", "
            << value[1] << ", "
            << value[2] << ")";
        return out.str();
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
            namedNode.localTransform = ToMatrix(nodeTransform);
            namedNode.position = ToVector3(TransformOrigin(nodeTransform));
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

    void CollectNamedNodeReports(
        const aiNode* node,
        const aiMatrix4x4& parentTransform,
        std::vector<AssimpNamedNodeReport>& outNodes)
    {
        if (!node)
        {
            return;
        }

        const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
        const std::string nodeName = ToString(node->mName);
        if (IsViewmodelNodeName(nodeName))
        {
            AssimpNamedNodeReport nodeReport;
            nodeReport.name = nodeName;
            nodeReport.position = ToArray3(TransformOrigin(nodeTransform));
            nodeReport.meshCount = node->mNumMeshes;
            outNodes.push_back(std::move(nodeReport));
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            CollectNamedNodeReports(node->mChildren[i], nodeTransform, outNodes);
        }
    }

    std::vector<std::string> BuildReportLines(const AssimpModelReport& report)
    {
        std::vector<std::string> lines;

        auto add = [&lines](std::string line)
        {
            lines.push_back(std::move(line));
        };

        add("[AssimpModelImporter] Inspecting model");
        add("  requested: " + ToDisplayPath(report.requestedPath));
        add("  resolved:  " + ToDisplayPath(report.resolvedPath));

        if (!report.loaded)
        {
            add("  load failed: " + report.error);
            return lines;
        }

        add("  loaded: yes");
        add("  nodes: " + std::to_string(report.nodeCount));
        add("  meshes: " + std::to_string(report.meshCount));
        add("  materials: " + std::to_string(report.materialCount));
        add("  textures: " + std::to_string(report.textureCount));
        add("  VM nodes: " + std::to_string(report.namedNodes.size()));

        if (!report.error.empty())
        {
            add("  warning: " + report.error);
        }

        for (size_t i = 0; i < report.namedNodes.size(); ++i)
        {
            const AssimpNamedNodeReport& node = report.namedNodes[i];
            add("  VM node[" + std::to_string(i) + "] " + node.name);
            add("    position: " + FormatVector3(node.position));
            add("    meshes: " + std::to_string(node.meshCount));
        }

        for (size_t i = 0; i < report.meshes.size(); ++i)
        {
            const AssimpMeshReport& mesh = report.meshes[i];
            add("  mesh[" + std::to_string(i) + "] " + mesh.name);
            add("    vertices: " + std::to_string(mesh.vertexCount));
            add("    faces: " + std::to_string(mesh.faceCount));
            add("    material: " + std::to_string(mesh.materialIndex));
            add("    normals: " + BoolText(mesh.hasNormals));
            add("    texcoords: " + BoolText(mesh.hasTexCoords));
            add("    tangents: " + BoolText(mesh.hasTangents));
        }

        for (size_t i = 0; i < report.materials.size(); ++i)
        {
            const AssimpMaterialReport& material = report.materials[i];
            add("  material[" + std::to_string(i) + "] " + material.name);
            add("    base color: " + FormatColor(material.baseColor));

            if (!material.baseColorTexture.empty())
            {
                add("    base color texture: " + material.baseColorTexture);
            }

            if (!material.diffuseTexture.empty())
            {
                add("    diffuse texture: " + material.diffuseTexture);
            }
        }

        return lines;
    }
}

AssimpModelReport AssimpModelImporter::Inspect(const std::filesystem::path& path)
{
    AssimpModelReport report;
    report.requestedPath = path;
    report.resolvedPath = ResolvePath(path);

    if (!Exists(report.resolvedPath))
    {
        report.error = "File does not exist.";
        return report;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(report.resolvedPath.string(), kImportFlags);
    if (!scene)
    {
        report.error = importer.GetErrorString();
        return report;
    }

    report.loaded = true;
    report.nodeCount = CountNodes(scene->mRootNode);
    report.meshCount = scene->mNumMeshes;
    report.materialCount = scene->mNumMaterials;
    report.textureCount = scene->mNumTextures;
    CollectNamedNodeReports(scene->mRootNode, aiMatrix4x4(), report.namedNodes);

    if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0)
    {
        report.error = "Scene loaded but Assimp marked it incomplete.";
    }
    else if (!scene->HasMeshes())
    {
        report.error = "Scene loaded but contains no meshes.";
    }

    report.meshes.reserve(scene->mNumMeshes);
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        if (!mesh)
        {
            continue;
        }

        AssimpMeshReport meshReport;
        meshReport.name = ToString(mesh->mName);
        meshReport.vertexCount = mesh->mNumVertices;
        meshReport.faceCount = mesh->mNumFaces;
        meshReport.materialIndex = mesh->mMaterialIndex;
        meshReport.hasNormals = mesh->HasNormals();
        meshReport.hasTexCoords = mesh->HasTextureCoords(0);
        meshReport.hasTangents = mesh->HasTangentsAndBitangents();
        report.meshes.push_back(std::move(meshReport));
    }

    report.materials.reserve(scene->mNumMaterials);
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        const aiMaterial* material = scene->mMaterials[i];

        AssimpMaterialReport materialReport;
        if (!material)
        {
            report.materials.push_back(std::move(materialReport));
            continue;
        }

        aiString materialName;
        if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
        {
            materialReport.name = ToString(materialName);
        }

        aiColor4D baseColor;
        if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS)
        {
            materialReport.baseColor = {
                baseColor.r,
                baseColor.g,
                baseColor.b,
                baseColor.a
            };
        }

        aiString texturePath;
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS)
        {
            materialReport.baseColorTexture = ToString(texturePath);
        }

        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
        {
            materialReport.diffuseTexture = ToString(texturePath);
        }

        report.materials.push_back(std::move(materialReport));
    }

    return report;
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

void AssimpModelImporter::DumpReport(const AssimpModelReport& report)
{
    for (const std::string& line : BuildReportLines(report))
    {
        TraceLine(line);
    }
}

bool AssimpModelImporter::WriteReportFile(
    const AssimpModelReport& report,
    const std::filesystem::path& path)
{
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        return false;
    }

    for (const std::string& line : BuildReportLines(report))
    {
        file << line << '\n';
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
