//---------------------------------------------------------------------------
//! @file   PS_Orb.hlsl
//! @brief  Colored circle for enemy/boss bullets
//---------------------------------------------------------------------------

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 localPos : TEXCOORD0;
    float4 color    : COLOR0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float r = length(input.localPos);

    // Soft circular edge - smoothstep gives cheap AA
    float alpha = 1.0 - smoothstep(0.9, 1.0, r);
    if (alpha < 0.001) discard;

    return float4(input.color.rgb, alpha);
}
