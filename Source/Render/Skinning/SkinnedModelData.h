#pragma once

#include "SkinnedVertex.h"

#include <SimpleMath.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

inline constexpr int32_t  INVALID_BONE_INDEX      = -1;
inline constexpr int32_t  SKINNED_TEXTURE_NONE    = -1;
inline constexpr uint32_t MAX_SKINNING_INFLUENCES = 4;
inline constexpr uint32_t MAX_BONES_PER_MODEL     = 128;

// ---------------------------------------------------------------------------
// Mesh layout & materials
// ---------------------------------------------------------------------------

struct SkinnedSubmesh
{
    uint32_t baseVertex    = 0;
    uint32_t startIndex    = 0;
    uint32_t indexCount    = 0;
    uint32_t materialIndex = 0;
};

struct SkinnedMaterial
{
    DirectX::SimpleMath::Color baseColor =
        DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
    int32_t baseColorTextureIndex = SKINNED_TEXTURE_NONE;
};

struct SkinnedTextureData
{
    std::string name;
    std::vector<uint8_t> bytes;
    uint32_t width  = 0;
    uint32_t height = 0;
    bool compressed = true;
    bool srgb       = false;
};

// ---------------------------------------------------------------------------
// Skeleton
// ---------------------------------------------------------------------------

struct Bone
{
    std::string name;
    int32_t parentIndex = INVALID_BONE_INDEX;

    // Inverse bind pose. Transforms a vertex from the loader's shared model
    // space into this bone's local space at bind time. Loaded from
    // aiBone::mOffsetMatrix, adjusted by the owning mesh node transform, and
    // transposed on import (Assimp is column-major, DX HLSL is row-major).
    DirectX::SimpleMath::Matrix offsetMatrix =
        DirectX::SimpleMath::Matrix::Identity;

    // Rest pose of this bone in its parent's space, sampled from the aiNode
    // tree. Used as the fallback whenever an animation clip has no channel
    // for the bone.
    DirectX::SimpleMath::Matrix localBindTransform =
        DirectX::SimpleMath::Matrix::Identity;
};

// ---------------------------------------------------------------------------
// Animation keyframes. Times stay in Assimp tick units; conversion to seconds
// happens at sample time using AnimationClip::ticksPerSecond.
// ---------------------------------------------------------------------------

struct PositionKey
{
    float time = 0.0f;
    DirectX::SimpleMath::Vector3 value = DirectX::SimpleMath::Vector3::Zero;
};

struct RotationKey
{
    float time = 0.0f;
    DirectX::SimpleMath::Quaternion value =
        DirectX::SimpleMath::Quaternion::Identity;
};

struct ScaleKey
{
    float time = 0.0f;
    DirectX::SimpleMath::Vector3 value = DirectX::SimpleMath::Vector3::One;
};

// One bone's animated TRS tracks for the duration of a clip. A clip may
// animate only a subset of bones; unanimated bones fall back to
// Bone::localBindTransform at sample time.
struct AnimationChannel
{
    int32_t boneIndex = INVALID_BONE_INDEX;
    std::vector<PositionKey> positions;
    std::vector<RotationKey> rotations;
    std::vector<ScaleKey>    scales;
};

struct AnimationClip
{
    std::string name;
    float duration       = 0.0f;   // ticks
    float ticksPerSecond = 25.0f;
    std::vector<AnimationChannel> channels;
};

// ---------------------------------------------------------------------------
// Top-level CPU-side container produced by SkinnedModelImporter.
// ---------------------------------------------------------------------------

struct SkinnedModelData
{
    std::filesystem::path sourcePath;

    std::vector<SkinnedVertex>  vertices;
    std::vector<uint32_t>       indices;
    std::vector<SkinnedSubmesh> submeshes;

    std::vector<SkinnedMaterial>    materials;
    std::vector<SkinnedTextureData> textures;

    std::vector<Bone> bones;

    // Inverse of aiScene::mRootNode->mTransformation. Applied when composing
    // each final bone matrix.
    DirectX::SimpleMath::Matrix globalInverseTransform =
        DirectX::SimpleMath::Matrix::Identity;

    std::vector<AnimationClip> clips;
};
