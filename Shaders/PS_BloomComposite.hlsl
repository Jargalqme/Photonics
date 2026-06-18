//---------------------------------------------------------------------------
//! @file   PS_BloomComposite.hlsl
//! @brief  HDR bloom composite: scene + tent-upsampled bloom.
//---------------------------------------------------------------------------

#include "Common.hlsli"
#include "Sampling.hlsli"

cbuffer BloomParams : register(b0)
{
    float2 texelSize;
    float  sampleScale;
    float  padding1;
    float4 threshold;
    float  bloomIntensity;
    float3 padding2;
};

Texture2D    sceneTexture : register(t0);
Texture2D    bloomTexture : register(t1);
SamplerState linearSampler : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 scene = sceneTexture.Sample(linearSampler, input.uv).rgb;
    float3 bloom = UpsampleTent9(bloomTexture, linearSampler, input.uv, texelSize, sampleScale).rgb;

    return float4(scene + bloom * bloomIntensity, 1.0);
}
