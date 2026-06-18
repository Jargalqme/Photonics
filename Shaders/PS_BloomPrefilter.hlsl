//---------------------------------------------------------------------------
//! @file   PS_BloomPrefilter.hlsl
//! @brief  Bloom threshold + 13-tap downsample (scene to mip[0])
//---------------------------------------------------------------------------

#include "Common.hlsli"
#include "Sampling.hlsli"

cbuffer BloomParams : register(b0)
{
    float2 texelSize;
    float  sampleScale;
    float  padding1;
    float4 threshold; // x: value, y: value-knee, z: knee*2, w: 0.25/knee
};

Texture2D    sourceTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = DownsampleBox13Tap(sourceTexture, linearSampler, input.uv, texelSize);
    color = Prefilter(color, threshold.x, threshold.yzw);
    return color;
}
