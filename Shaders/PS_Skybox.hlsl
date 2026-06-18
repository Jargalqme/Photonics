//---------------------------------------------------------------------------
//! @file   PS_Skybox.hlsl
//! @brief  Cubemap texture sample for skybox
//---------------------------------------------------------------------------

TextureCube skyboxTexture  : register(t0);
SamplerState skyboxSampler : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float3 dir = normalize(input.direction);
    float4 color = skyboxTexture.Sample(skyboxSampler, dir);
    return float4(color.rgb * 0.5, color.a);
}
