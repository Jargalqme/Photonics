//---------------------------------------------------------------------------
//! @file   PS_Billboard.hlsl
//! @brief  Simple texture sample for billboard quads
//---------------------------------------------------------------------------

Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct PS_INPUT
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 color = tex.Sample(sam, input.texCoord);
    return float4(color.rgb * 3.0, color.a);
}