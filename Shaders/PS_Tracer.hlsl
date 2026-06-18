//---------------------------------------------------------------------------
//! @file   PS_Tracer.hlsl
//! @brief  Bullet tracer pixel shader.
//---------------------------------------------------------------------------

cbuffer TracerConstants : register(b0)
{
    matrix ViewProjection;
    float3 TracerStart;
    float  TracerWidth;
    float3 TracerEnd;
    float  TracerLife;
    float4 TracerColor;
    float3 CameraPosition;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float distFromCenter = abs(input.uv.y - 0.5) * 2.0;
    float edge = 1.0 - smoothstep(0.82, 1.0, distFromCenter);
    float tailFade = smoothstep(0.0, 0.18, input.uv.x);
    float headFade = 1.0 - smoothstep(0.96, 1.0, input.uv.x);
    float alpha = edge * tailFade * headFade * saturate(TracerLife) * TracerColor.a;

    return float4(TracerColor.rgb, alpha);
}
