//---------------------------------------------------------------------------
//! @file   VS_ImportedModel.hlsl
//! @brief  Vertex shader for imported Assimp model buffers.
//---------------------------------------------------------------------------

cbuffer ImportedModelCB : register(b0)
{
    float4x4 World;
    float4x4 WorldViewProjection;
    float4x4 WorldInverseTranspose;
    float4 TintColor;
    float4 LightDirectionAndAmbient;
    float4 LightColor;
    float4 CameraPosition;
    float4 MaterialParams;
    float4 MaterialFlags;
    float4 MaterialFlags2;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float2 texCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 tangent  : TANGENT;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.position = mul(float4(input.position, 1.0), WorldViewProjection);
    output.worldPos = mul(float4(input.position, 1.0), World).xyz;
    output.normal   = normalize(mul(float4(input.normal, 0.0), WorldInverseTranspose).xyz);
    output.tangent  = normalize(mul(float4(input.tangent, 0.0), World).xyz);
    output.texCoord = input.texCoord;

    return output;
}
