//---------------------------------------------------------------------------
//! @file   PS_BloomDownsample.hlsl
//! @brief  13-tap box downsample (mip[i-1] to mip[i])
//---------------------------------------------------------------------------

#include "Common.hlsli"
#include "Sampling.hlsli"

cbuffer BloomParams : register(b0)
{
    float2 texelSize;
    float  sampleScale;
    float  padding1;
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
    return DownsampleBox13Tap(sourceTexture, linearSampler, input.uv, texelSize);
}