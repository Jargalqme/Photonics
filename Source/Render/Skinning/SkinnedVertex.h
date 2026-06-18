#pragma once

#include <SimpleMath.h>
#include <cstdint>

// Per-vertex layout for the skinned-mesh path. The HLSL VS_INPUT in
// VS_SkinnedMenu.hlsl must mirror this layout exactly; the D3D11 input layout
// described in SkinnedRenderer must match both.
//
// boneIndices[i] / boneWeights[i] form a (bone, weight) pair. The four pairs
// are sorted by weight at import time and re-normalized to sum to 1.0. Vertices
// with fewer than four influencing bones leave the trailing weights at 0.
struct SkinnedVertex
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector3 normal   = DirectX::SimpleMath::Vector3::UnitY;
    DirectX::SimpleMath::Vector3 tangent  = DirectX::SimpleMath::Vector3::UnitX;
    DirectX::SimpleMath::Vector2 texcoord = DirectX::SimpleMath::Vector2::Zero;

    uint32_t boneIndices[4] = { 0, 0, 0, 0 };
    float    boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

static_assert(sizeof(SkinnedVertex) == 76,
    "SkinnedVertex layout drifted; update the HLSL VS_INPUT and the D3D11 "
    "input layout in SkinnedRenderer to match.");
