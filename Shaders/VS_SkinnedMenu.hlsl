//---------------------------------------------------------------------------
//! @file   VS_SkinnedMenu.hlsl
//! @brief  Vertex shader for the menu skinned-character path. Performs a
//!         standard 4-bone linear-blend skin then transforms by WVP.
//---------------------------------------------------------------------------

// Must match MAX_BONES_PER_MODEL in SkinnedModelData.h.
#define SKINNED_MAX_BONES 128

cbuffer SkinnedTransformCB : register(b0)
{
    float4x4 WorldViewProjection;
    float4x4 WorldInverseTranspose;
    float4 TintColor;
    float4 LightDirectionAndAmbient;
    float4 MaterialFlags;
};

cbuffer SkinnedPaletteCB : register(b1)
{
    // Final bone matrices = globalInverseTransform * boneAnimatedWorld
    //                       * Bone.offsetMatrix.
    // Composed CPU-side in AnimationPlayer and uploaded transposed (to match
    // HLSL's default column-major matrix convention, same as WVP above).
    float4x4 BoneMatrices[SKINNED_MAX_BONES];
};

struct VS_INPUT
{
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float3 tangent     : TANGENT;
    float2 texCoord    : TEXCOORD0;
    uint4  boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // Linear-blend skin. Build the weighted skinning matrix from the four
    // influencing bones. Vertices with fewer than four influences leave the
    // trailing boneWeights at 0, so unused slots contribute nothing.
    float4x4 skin =
        BoneMatrices[input.boneIndices.x] * input.boneWeights.x +
        BoneMatrices[input.boneIndices.y] * input.boneWeights.y +
        BoneMatrices[input.boneIndices.z] * input.boneWeights.z +
        BoneMatrices[input.boneIndices.w] * input.boneWeights.w;

    // Transform position and normal into model space first via the skin
    // matrix, then into clip space via WVP.
    float4 skinnedPos = mul(float4(input.position, 1.0), skin);
    output.position   = mul(skinnedPos, WorldViewProjection);

    // Normal transform: rotate by the skin matrix (uniform-scale assumption,
    // which is fine for Mixamo's rigid-bone skinning) then by the standard
    // world-inverse-transpose. The skin's row .w shouldn't influence the
    // normal, so we drop translation by passing w=0.
    float3 skinnedNormal = mul(float4(input.normal, 0.0), skin).xyz;
    output.normal = normalize(mul(float4(skinnedNormal, 0.0), WorldInverseTranspose).xyz);

    output.texCoord = input.texCoord;
    return output;
}
