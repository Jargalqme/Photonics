#include "pch.h"
#include "SkinnedModelImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <charconv>
#include <fstream>
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
        aiProcess_LimitBoneWeights |
        aiProcess_ConvertToLeftHanded;

    using TextureIndexMap = std::unordered_map<std::string, int32_t>;
    using BoneIndexMap    = std::unordered_map<std::string, int32_t>;

    void TraceLine(const std::string& text)
    {
        OutputDebugStringA(text.c_str());
        OutputDebugStringA("\n");
    }

    void ConfigureImporter(Assimp::Importer& importer)
    {
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    }

    bool Exists(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    std::string ToString(const aiString& value)
    {
        return value.length > 0 ? std::string(value.C_Str()) : std::string();
    }

    DirectX::SimpleMath::Vector3 ToVector3(const aiVector3D& value)
    {
        return DirectX::SimpleMath::Vector3(value.x, value.y, value.z);
    }

    DirectX::SimpleMath::Vector2 ToVector2(const aiVector3D& value)
    {
        return DirectX::SimpleMath::Vector2(value.x, value.y);
    }

    DirectX::SimpleMath::Quaternion ToQuaternion(const aiQuaternion& value)
    {
        return DirectX::SimpleMath::Quaternion(value.x, value.y, value.z, value.w);
    }

    DirectX::SimpleMath::Color ToColor(const aiColor4D& value)
    {
        return DirectX::SimpleMath::Color(value.r, value.g, value.b, value.a);
    }

    DirectX::SimpleMath::Matrix ToMatrix(const aiMatrix4x4& m)
    {
        return DirectX::SimpleMath::Matrix(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4);
    }

    aiVector3D NormalizeOrFallback(const aiVector3D& v, const aiVector3D& fallback)
    {
        const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (lenSq <= 1e-6f) { return fallback; }
        const float invLen = 1.0f / std::sqrt(lenSq);
        return aiVector3D(v.x * invLen, v.y * invLen, v.z * invLen);
    }

    bool IsEmbeddedTextureReference(const std::string& ref)
    {
        return ref.size() > 1 && ref[0] == '*';
    }

    int32_t ParseEmbeddedTextureIndex(const std::string& ref)
    {
        if (!IsEmbeddedTextureReference(ref)) { return SKINNED_TEXTURE_NONE; }
        int32_t idx = SKINNED_TEXTURE_NONE;
        const char* begin = ref.data() + 1;
        const char* end   = ref.data() + ref.size();
        const auto result = std::from_chars(begin, end, idx);
        if (result.ec != std::errc() || result.ptr != end || idx < 0)
        {
            return SKINNED_TEXTURE_NONE;
        }
        return idx;
    }

    bool ReadBinaryFile(const std::filesystem::path& path, std::vector<uint8_t>& out)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) { return false; }

        const std::ifstream::pos_type size = file.tellg();
        if (size <= 0) { return false; }

        out.resize(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(out.data()),
                  static_cast<std::streamsize>(size));
        return file.good();
    }

    void AppendEmbeddedTextures(const aiScene* scene, SkinnedModelData& outData)
    {
        outData.textures.reserve(scene->mNumTextures);
        for (unsigned int i = 0; i < scene->mNumTextures; ++i)
        {
            SkinnedTextureData tex;
            const std::string fallbackName = "*" + std::to_string(i);

            const aiTexture* src = scene->mTextures[i];
            if (!src)
            {
                tex.name = fallbackName;
                outData.textures.push_back(std::move(tex));
                continue;
            }

            tex.name       = ToString(src->mFilename);
            if (tex.name.empty()) { tex.name = fallbackName; }
            tex.width      = src->mWidth;
            tex.height     = src->mHeight;
            tex.compressed = src->mHeight == 0;

            if (tex.compressed)
            {
                if (src->pcData && src->mWidth > 0)
                {
                    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(src->pcData);
                    tex.bytes.assign(bytes, bytes + src->mWidth);
                }
            }
            else if (src->pcData)
            {
                const size_t texelCount =
                    static_cast<size_t>(src->mWidth) * static_cast<size_t>(src->mHeight);
                tex.bytes.reserve(texelCount * 4);
                for (size_t t = 0; t < texelCount; ++t)
                {
                    const aiTexel& px = src->pcData[t];
                    tex.bytes.push_back(px.r);
                    tex.bytes.push_back(px.g);
                    tex.bytes.push_back(px.b);
                    tex.bytes.push_back(px.a);
                }
            }

            outData.textures.push_back(std::move(tex));
        }
    }

    bool EqualIgnoreCase(std::string_view a, std::string_view b)
    {
        if (a.size() != b.size()) { return false; }
        for (size_t i = 0; i < a.size(); ++i)
        {
            const char ca = (a[i] >= 'A' && a[i] <= 'Z') ? char(a[i] + 32) : a[i];
            const char cb = (b[i] >= 'A' && b[i] <= 'Z') ? char(b[i] + 32) : b[i];
            if (ca != cb) { return false; }
        }
        return true;
    }

    int32_t FindEmbeddedTextureByBaseName(
        const std::string& ref,
        const SkinnedModelData& outData)
    {
        const std::string baseName =
            std::filesystem::path(ref).filename().generic_string();
        if (baseName.empty()) { return SKINNED_TEXTURE_NONE; }

        for (size_t i = 0; i < outData.textures.size(); ++i)
        {
            const std::string& candidate = outData.textures[i].name;
            const std::string candidateBase =
                std::filesystem::path(candidate).filename().generic_string();

            if (EqualIgnoreCase(candidate, baseName) ||
                EqualIgnoreCase(candidateBase, baseName))
            {
                return static_cast<int32_t>(i);
            }
        }
        return SKINNED_TEXTURE_NONE;
    }

    int32_t AppendExternalTexture(
        const std::string& ref,
        const std::filesystem::path& modelDir,
        SkinnedModelData& outData,
        bool srgb,
        TextureIndexMap& indexByPath)
    {
        if (ref.empty()) { return SKINNED_TEXTURE_NONE; }

        std::filesystem::path path(ref);
        if (path.is_relative()) { path = modelDir / path; }
        const std::string resolved = path.lexically_normal().generic_string();

        const auto found = indexByPath.find(resolved);
        if (found != indexByPath.end())
        {
            const int32_t idx = found->second;
            outData.textures[idx].srgb = outData.textures[idx].srgb || srgb;
            return idx;
        }

        const int32_t embedded = FindEmbeddedTextureByBaseName(ref, outData);
        if (embedded != SKINNED_TEXTURE_NONE)
        {
            outData.textures[embedded].srgb =
                outData.textures[embedded].srgb || srgb;
            indexByPath.emplace(resolved, embedded);
            return embedded;
        }

        SkinnedTextureData tex;
        tex.name       = std::filesystem::path(ref).filename().generic_string();
        tex.compressed = true;
        tex.srgb       = srgb;

        if (!ReadBinaryFile(path, tex.bytes))
        {
            TraceLine("[SkinnedModelImporter] Failed to read texture: "
                + path.generic_string()
                + " (ref: " + ref + ")");
            return SKINNED_TEXTURE_NONE;
        }

        const int32_t idx = static_cast<int32_t>(outData.textures.size());
        outData.textures.push_back(std::move(tex));
        indexByPath.emplace(resolved, idx);
        return idx;
    }

    int32_t ResolveTextureIndex(
        const std::string& ref,
        const std::filesystem::path& modelDir,
        SkinnedModelData& outData,
        bool srgb,
        TextureIndexMap& indexByPath)
    {
        if (ref.empty()) { return SKINNED_TEXTURE_NONE; }
        if (IsEmbeddedTextureReference(ref))
        {
            const int32_t idx = ParseEmbeddedTextureIndex(ref);
            if (idx >= 0 && static_cast<size_t>(idx) < outData.textures.size())
            {
                outData.textures[idx].srgb = outData.textures[idx].srgb || srgb;
                return idx;
            }
            return SKINNED_TEXTURE_NONE;
        }
        return AppendExternalTexture(ref, modelDir, outData, srgb, indexByPath);
    }

    // Mixamo's FBX exporter is inconsistent about which texture slot it puts
    // the diffuse in — newer files use BASE_COLOR, older use DIFFUSE, some
    // put it under AMBIENT or even UNKNOWN. Try all common slots in order.
    int32_t TryResolveBaseColor(
        const aiMaterial* src,
        const std::filesystem::path& modelDir,
        SkinnedModelData& outData,
        TextureIndexMap& indexByPath)
    {
        constexpr aiTextureType candidates[] = {
            aiTextureType_BASE_COLOR,
            aiTextureType_DIFFUSE,
            aiTextureType_AMBIENT,
            aiTextureType_EMISSIVE,
            aiTextureType_UNKNOWN,
        };

        for (aiTextureType type : candidates)
        {
            const unsigned int count = src->GetTextureCount(type);
            for (unsigned int t = 0; t < count; ++t)
            {
                aiString texPath;
                if (src->GetTexture(type, t, &texPath) != AI_SUCCESS) { continue; }
                const std::string ref = ToString(texPath);
                if (ref.empty()) { continue; }

                const int32_t idx = ResolveTextureIndex(
                    ref, modelDir, outData, true, indexByPath);
                if (idx != SKINNED_TEXTURE_NONE)
                {
                    return idx;
                }
            }
        }
        return SKINNED_TEXTURE_NONE;
    }

    void AppendMaterials(
        const aiScene* scene,
        const std::filesystem::path& modelDir,
        SkinnedModelData& outData,
        TextureIndexMap& indexByPath)
    {
        outData.materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            const aiMaterial* src = scene->mMaterials[i];
            SkinnedMaterial mat;
            if (src)
            {
                aiColor4D base;
                if (aiGetMaterialColor(src, AI_MATKEY_BASE_COLOR, &base) == AI_SUCCESS)
                {
                    mat.baseColor = ToColor(base);
                }

                mat.baseColorTextureIndex = TryResolveBaseColor(
                    src, modelDir, outData, indexByPath);
            }
            outData.materials.push_back(std::move(mat));
        }
    }

    int32_t GetOrCreateBone(
        const std::string& name,
        SkinnedModelData& outData,
        BoneIndexMap& indexByName)
    {
        const auto found = indexByName.find(name);
        if (found != indexByName.end()) { return found->second; }

        const int32_t idx = static_cast<int32_t>(outData.bones.size());
        Bone bone;
        bone.name = name;
        outData.bones.push_back(std::move(bone));
        indexByName.emplace(name, idx);
        return idx;
    }

    void CollectBonesFromMesh(
        const aiMesh* mesh,
        const aiMatrix4x4& nodeTransform,
        SkinnedModelData& outData,
        BoneIndexMap& indexByName)
    {
        if (!mesh) { return; }

        const DirectX::SimpleMath::Matrix meshToModel = ToMatrix(nodeTransform);
        const DirectX::SimpleMath::Matrix modelToMesh = meshToModel.Invert();
        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            const aiBone* aiBoneSrc = mesh->mBones[b];
            if (!aiBoneSrc) { continue; }
            const std::string name = ToString(aiBoneSrc->mName);
            const int32_t idx = GetOrCreateBone(name, outData, indexByName);

            outData.bones[idx].offsetMatrix =
                modelToMesh * ToMatrix(aiBoneSrc->mOffsetMatrix);
        }
    }

    void CollectBonesFromMeshNodes(
        const aiScene* scene,
        const aiNode* node,
        const aiMatrix4x4& parentTransform,
        SkinnedModelData& outData,
        BoneIndexMap& indexByName)
    {
        if (!scene || !node) { return; }

        const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            const unsigned int meshIndex = node->mMeshes[i];
            if (meshIndex < scene->mNumMeshes)
            {
                CollectBonesFromMesh(
                    scene->mMeshes[meshIndex],
                    nodeTransform,
                    outData,
                    indexByName);
            }
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            CollectBonesFromMeshNodes(
                scene,
                node->mChildren[i],
                nodeTransform,
                outData,
                indexByName);
        }
    }

    void FillBoneHierarchy(
        const aiNode* node,
        int32_t parentBoneIndex,
        const aiMatrix4x4& accumulatedNonBone,
        SkinnedModelData& outData,
        BoneIndexMap& indexByName)
    {
        if (!node) { return; }

        const aiMatrix4x4 nodeWithAccum = accumulatedNonBone * node->mTransformation;
        const std::string name = ToString(node->mName);
        const auto found = indexByName.find(name);

        if (found != indexByName.end())
        {
            const int32_t myIndex = found->second;
            outData.bones[myIndex].parentIndex        = parentBoneIndex;
            outData.bones[myIndex].localBindTransform = ToMatrix(nodeWithAccum);

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
            {
                FillBoneHierarchy(node->mChildren[i], myIndex, aiMatrix4x4(),
                                  outData, indexByName);
            }
        }
        else
        {
            // Non-bone: accumulate its transform and propagate the same parent
            // bone index to children.
            for (unsigned int i = 0; i < node->mNumChildren; ++i)
            {
                FillBoneHierarchy(node->mChildren[i], parentBoneIndex,
                                  nodeWithAccum, outData, indexByName);
            }
        }
    }

    struct VertexInfluence
    {
        uint32_t boneIndex = 0;
        float    weight    = 0.0f;
    };

    void AppendMesh(
        const aiMesh* mesh,
        SkinnedModelData& outData,
        const BoneIndexMap& indexByName,
        const aiMatrix4x4& nodeTransform,
        int32_t fallbackBoneIndex)
    {
        if (!mesh || !mesh->HasPositions() || !mesh->HasFaces()) { return; }

        SkinnedSubmesh submesh;
        submesh.baseVertex    = static_cast<uint32_t>(outData.vertices.size());
        submesh.startIndex    = static_cast<uint32_t>(outData.indices.size());
        submesh.materialIndex = mesh->mMaterialIndex;
        const uint32_t fallbackBone =
            (fallbackBoneIndex >= 0) ? static_cast<uint32_t>(fallbackBoneIndex) : 0u;

        aiMatrix3x3 normalTransform(nodeTransform);
        normalTransform.Inverse();
        normalTransform.Transpose();

        // For each vertex, collect up to 4 (bone, weight) influences from the
        // mesh's bone arrays.
        std::vector<std::vector<VertexInfluence>> influences(mesh->mNumVertices);
        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            const aiBone* aiBoneSrc = mesh->mBones[b];
            if (!aiBoneSrc) { continue; }
            const auto found = indexByName.find(ToString(aiBoneSrc->mName));
            if (found == indexByName.end()) { continue; }
            const uint32_t boneIdx = static_cast<uint32_t>(found->second);

            for (unsigned int w = 0; w < aiBoneSrc->mNumWeights; ++w)
            {
                const aiVertexWeight& vw = aiBoneSrc->mWeights[w];
                if (vw.mVertexId >= mesh->mNumVertices || vw.mWeight <= 0.0f) { continue; }
                influences[vw.mVertexId].push_back({ boneIdx, vw.mWeight });
            }
        }

        outData.vertices.reserve(outData.vertices.size() + mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            aiVector3D pos = nodeTransform * mesh->mVertices[i];

            aiVector3D nrm(0.0f, 1.0f, 0.0f);
            if (mesh->HasNormals())
            {
                nrm = NormalizeOrFallback(normalTransform * mesh->mNormals[i], nrm);
            }

            aiVector3D tan(1.0f, 0.0f, 0.0f);
            if (mesh->HasTangentsAndBitangents())
            {
                tan = NormalizeOrFallback(normalTransform * mesh->mTangents[i], tan);
            }

            aiVector3D uv(0.0f, 0.0f, 0.0f);
            if (mesh->HasTextureCoords(0)) { uv = mesh->mTextureCoords[0][i]; }

            SkinnedVertex v;
            v.position = ToVector3(pos);
            v.normal   = ToVector3(nrm);
            v.tangent  = ToVector3(tan);
            v.texcoord = ToVector2(uv);

            auto& list = influences[i];
            std::sort(list.begin(), list.end(),
                [](const VertexInfluence& a, const VertexInfluence& b)
                { return a.weight > b.weight; });
            if (list.size() > MAX_SKINNING_INFLUENCES)
            {
                list.resize(MAX_SKINNING_INFLUENCES);
            }

            float weightSum = 0.0f;
            for (const auto& inf : list) { weightSum += inf.weight; }
            const float invSum = (weightSum > 1e-6f) ? (1.0f / weightSum) : 0.0f;

            if (list.empty() || invSum <= 0.0f)
            {
                v.boneIndices[0] = fallbackBone;
                v.boneWeights[0] = 1.0f;
            }
            else
            {
                for (size_t s = 0; s < list.size(); ++s)
                {
                    v.boneIndices[s] = list[s].boneIndex;
                    v.boneWeights[s] = list[s].weight * invSum;
                }
            }

            outData.vertices.push_back(v);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) { continue; }
            outData.indices.push_back(face.mIndices[0] + submesh.baseVertex);
            outData.indices.push_back(face.mIndices[1] + submesh.baseVertex);
            outData.indices.push_back(face.mIndices[2] + submesh.baseVertex);
        }

        submesh.indexCount =
            static_cast<uint32_t>(outData.indices.size()) - submesh.startIndex;
        if (submesh.indexCount > 0)
        {
            outData.submeshes.push_back(std::move(submesh));
        }
    }

    void TraverseMeshNodes(
        const aiScene* scene,
        const aiNode* node,
        const aiMatrix4x4& parentTransform,
        int32_t parentBoneIndex,
        SkinnedModelData& outData,
        const BoneIndexMap& indexByName)
    {
        if (!scene || !node) { return; }

        const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
        int32_t currentBoneIndex = parentBoneIndex;

        const auto bone = indexByName.find(ToString(node->mName));
        if (bone != indexByName.end())
        {
            currentBoneIndex = bone->second;
        }

        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            const unsigned int meshIndex = node->mMeshes[i];
            if (meshIndex < scene->mNumMeshes)
            {
                AppendMesh(
                    scene->mMeshes[meshIndex],
                    outData,
                    indexByName,
                    nodeTransform,
                    currentBoneIndex);
            }
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            TraverseMeshNodes(
                scene,
                node->mChildren[i],
                nodeTransform,
                currentBoneIndex,
                outData,
                indexByName);
        }
    }

    // -----------------------------------------------------------------------
    // Animation clip extraction
    // -----------------------------------------------------------------------

    void AppendAnimations(
        const aiScene* scene,
        SkinnedModelData& outData,
        const BoneIndexMap& indexByName)
    {
        outData.clips.reserve(outData.clips.size() + scene->mNumAnimations);

        for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
        {
            const aiAnimation* src = scene->mAnimations[a];
            if (!src) { continue; }

            AnimationClip clip;
            clip.name           = ToString(src->mName);
            clip.duration       = static_cast<float>(src->mDuration);
            clip.ticksPerSecond = (src->mTicksPerSecond > 0.0)
                ? static_cast<float>(src->mTicksPerSecond)
                : 25.0f;

            clip.channels.reserve(src->mNumChannels);
            for (unsigned int c = 0; c < src->mNumChannels; ++c)
            {
                const aiNodeAnim* node = src->mChannels[c];
                if (!node) { continue; }
                const auto found = indexByName.find(ToString(node->mNodeName));
                if (found == indexByName.end()) { continue; }

                AnimationChannel channel;
                channel.boneIndex = found->second;

                channel.positions.reserve(node->mNumPositionKeys);
                for (unsigned int k = 0; k < node->mNumPositionKeys; ++k)
                {
                    const aiVectorKey& key = node->mPositionKeys[k];
                    channel.positions.push_back({
                        static_cast<float>(key.mTime),
                        ToVector3(key.mValue)
                    });
                }

                channel.rotations.reserve(node->mNumRotationKeys);
                for (unsigned int k = 0; k < node->mNumRotationKeys; ++k)
                {
                    const aiQuatKey& key = node->mRotationKeys[k];
                    channel.rotations.push_back({
                        static_cast<float>(key.mTime),
                        ToQuaternion(key.mValue)
                    });
                }

                channel.scales.reserve(node->mNumScalingKeys);
                for (unsigned int k = 0; k < node->mNumScalingKeys; ++k)
                {
                    const aiVectorKey& key = node->mScalingKeys[k];
                    channel.scales.push_back({
                        static_cast<float>(key.mTime),
                        ToVector3(key.mValue)
                    });
                }

                clip.channels.push_back(std::move(channel));
            }

            if (clip.channels.empty())
            {
                continue;
            }

            outData.clips.push_back(std::move(clip));
        }
    }

} // namespace

bool SkinnedModelImporter::LoadSkinnedModelData(
    const std::filesystem::path& path,
    SkinnedModelData& outData,
    std::string* outError)
{
    outData = SkinnedModelData{};
    outData.sourcePath = ResolvePath(path);

    if (!Exists(outData.sourcePath))
    {
        if (outError) { *outError = "File does not exist."; }
        return false;
    }

    Assimp::Importer importer;
    ConfigureImporter(importer);
    const aiScene* scene = importer.ReadFile(outData.sourcePath.string(), kImportFlags);
    if (!scene)
    {
        if (outError) { *outError = importer.GetErrorString(); }
        return false;
    }
    if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode)
    {
        if (outError) { *outError = "Scene is incomplete."; }
        return false;
    }
    if (!scene->HasMeshes())
    {
        if (outError) { *outError = "Scene contains no meshes."; }
        return false;
    }

    const DirectX::SimpleMath::Matrix globalRoot = ToMatrix(scene->mRootNode->mTransformation);
    outData.globalInverseTransform = globalRoot.Invert();

    TextureIndexMap textureIndexByPath;
    AppendEmbeddedTextures(scene, outData);
    AppendMaterials(scene, outData.sourcePath.parent_path(), outData, textureIndexByPath);

    BoneIndexMap boneIndexByName;
    CollectBonesFromMeshNodes(scene, scene->mRootNode, aiMatrix4x4(),
                              outData, boneIndexByName);
    FillBoneHierarchy(scene->mRootNode, INVALID_BONE_INDEX, aiMatrix4x4(),
                      outData, boneIndexByName);

    TraverseMeshNodes(scene, scene->mRootNode, aiMatrix4x4(),
                      INVALID_BONE_INDEX, outData, boneIndexByName);

    AppendAnimations(scene, outData, boneIndexByName);

    if (outData.vertices.empty() || outData.indices.empty())
    {
        if (outError) { *outError = "No renderable geometry was imported."; }
        return false;
    }
    if (outData.bones.size() > MAX_BONES_PER_MODEL)
    {
        if (outError)
        {
            *outError = "Model has " + std::to_string(outData.bones.size())
                + " bones; the shader palette is capped at "
                + std::to_string(MAX_BONES_PER_MODEL) + ".";
        }
        return false;
    }

    return true;
}

int32_t SkinnedModelImporter::AppendClipsFromFile(
    const std::filesystem::path& path,
    SkinnedModelData& existingData,
    std::string* outError)
{
    const std::filesystem::path resolved = ResolvePath(path);
    if (!Exists(resolved))
    {
        if (outError) { *outError = "File does not exist."; }
        return -1;
    }

    Assimp::Importer importer;
    ConfigureImporter(importer);
    const aiScene* scene = importer.ReadFile(resolved.string(), kImportFlags);
    if (!scene)
    {
        if (outError) { *outError = importer.GetErrorString(); }
        return -1;
    }
    if (scene->mNumAnimations == 0)
    {
        if (outError) { *outError = "File contains no animations."; }
        return 0;
    }

    BoneIndexMap indexByName;
    for (size_t i = 0; i < existingData.bones.size(); ++i)
    {
        indexByName.emplace(existingData.bones[i].name, static_cast<int32_t>(i));
    }

    const size_t before = existingData.clips.size();
    AppendAnimations(scene, existingData, indexByName);
    return static_cast<int32_t>(existingData.clips.size() - before);
}

std::filesystem::path SkinnedModelImporter::ResolvePath(const std::filesystem::path& path)
{
    if (path.is_absolute() || Exists(path)) { return path; }
    const std::filesystem::path exePath = std::filesystem::path(GetExecutableDir()) / path;
    if (Exists(exePath)) { return exePath; }
    return path;
}
