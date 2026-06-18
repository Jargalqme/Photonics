//---------------------------------------------------------------------------
//! @file   PS_Wave.hlsl
//! @brief  Iterative sine wave distortion menu background effect
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

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 fragCoord = input.uv * Resolution;
    float2 uv = (2.0 * fragCoord - Resolution) / min(Resolution.x, Resolution.y);

    for (float i = 1.0; i < 8.0; i++)
    {
        uv.x += 0.6 / i * cos(i * 2.5 * uv.y + Time);
        uv.y += 0.6 / i * cos(i * 1.5 * uv.x + Time);
    }

    float val = 0.1 / abs(sin(Time - uv.y - uv.x));
    float3 color1 = float3(0.7, 0.0, 0.5); // dark magenta
    float3 color2 = float3(0.0, 0.6, 0.9); // cyan
    float kick = pow(saturate(sin(Time * 9.4)), 8.0);
    float blend = sin(uv.y * 2.0 + Time * 0.3) * 0.5 + 0.5;
    blend = lerp(blend, 1.0, kick * 0.6); // each kick pulls blend toward color2
    float3 col = val * lerp(color1, color2, blend);
    
    return float4(col, 1.0);
}