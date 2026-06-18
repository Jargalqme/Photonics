//---------------------------------------------------------------------------
//! @file   PS_BloomUpsample.hlsl
//! @brief  9-tap tent upsample (mip[i+1] to mip[i], additive blend)
//---------------------------------------------------------------------------

#include "Common.hlsli"
#include "Sampling.hlsli"

cbuffer BloomParams : register(b0)
{
    float2 texelSize;
    float sampleScale;
    float padding1;
};

Texture2D sourceTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    return UpsampleTent9(sourceTexture, linearSampler, input.uv, texelSize, sampleScale);
}