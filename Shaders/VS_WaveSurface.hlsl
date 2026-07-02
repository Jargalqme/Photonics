cbuffer WaveSurfaceConstants : register(b0)
{
    matrix WorldViewProjection;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float  height   : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1.0), WorldViewProjection);
    output.normal   = input.normal;       // world は回転なし → 変換不要
    output.height   = input.position.y;   // 高さ（色付け用）
    return output;
}
