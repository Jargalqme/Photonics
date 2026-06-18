//---------------------------------------------------------------------------
//! @file   PS_FinalComposite.hlsl
//! @brief  Final output transform from linear HDR scene color into the
//          BGRA UNORM backbuffer.
//          Applies exposure, ACES tonemap (Hill fit), and gamma encoding.
//---------------------------------------------------------------------------

#include "Tonemapping.hlsli"

cbuffer FinalCompositeParams : register(b0)
{
    float exposure;
    float3 padding;
};

Texture2D sceneTexture : register(t0);
SamplerState sceneSampler : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 color = sceneTexture.Sample(sceneSampler, input.uv).rgb;
    color *= exposure;
    color = Tonemap_ACES_Hill(color);
    color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return float4(color, 1.0);
}
