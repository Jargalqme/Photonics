//---------------------------------------------------------------------------
//! @file   VS_PristineGrid.hlsl
//! @brief  Grid floor vertex transform — passes world XZ to pixel shader
//---------------------------------------------------------------------------

cbuffer GridConstants : register(b0)
{
    matrix worldViewProjection;
    float4 gridParams;
    float4 lineColor;
    float4 baseColor;
};

struct VS_INPUT
{
    float3 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 worldPos : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0), worldViewProjection);
    output.worldPos = input.position.xz;
    return output;
}