//---------------------------------------------------------------------------
//! @file   PS_Cubemap.hlsl
//! @brief  Cubemap background sample
//---------------------------------------------------------------------------

TextureCube cubemapTexture  : register(t0);
SamplerState cubemapSampler : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 dir = normalize(input.direction);
    float4 color = cubemapTexture.Sample(cubemapSampler, dir);
    return float4(color.rgb * 0.5, color.a);
}
