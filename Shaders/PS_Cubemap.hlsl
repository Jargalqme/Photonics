//---------------------------------------------------------------------------
//! @file   PS_Cubemap.hlsl
//! @brief  Cubemap background sample
//---------------------------------------------------------------------------

TextureCube  cubemapTexture : register(t0);
SamplerState cubemapSampler : register(s0);

struct PS_INPUT
{
    float4 position  : SV_Position;
    float3 direction : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Prefiltered cube: mips encode roughness, not image detail.
    // The background must read mip 0 explicity (mirror-sharp radiance).
    float3 color = cubemapTexture.SampleLevel(cubemapSampler, input.direction, 0).rgb;
    return float4(color, 1.0);
}
