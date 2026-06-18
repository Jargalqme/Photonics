//---------------------------------------------------------------------------
//! @file   VS_MenuBackground.hlsl
//! @brief  Passthrough vertex shader for menu background effects
//---------------------------------------------------------------------------

cbuffer MenuConstants : register(b0)
{
    float  Time;
    float2 Resolution;
    float  Speed;
    float  PatternScale;
    float  WarpIntensity;
    float  Brightness;
    float  ChromaticOffset;
    float3 ColorTint;
    float  Padding;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0);
    output.uv = input.uv;
    return output;
}