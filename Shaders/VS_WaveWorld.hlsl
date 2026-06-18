//---------------------------------------------------------------------------
//! @file   VS_WaveWorld.hlsl
//! @brief  Arena floor wave effect — vertex transform
//---------------------------------------------------------------------------

cbuffer WaveWorldConstants : register(b0)
{
    matrix WorldViewProjection;
    float Time;
    float Speed;
    float Brightness;
    float Alpha;
};

struct VS_INPUT
{
    float3 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0), WorldViewProjection);
    output.uv = input.position.xy;
    return output;
}