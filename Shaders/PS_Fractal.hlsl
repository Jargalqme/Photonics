//---------------------------------------------------------------------------
//! @file   PS_Fractal.hlsl
//! @brief  Kishimisu fractal (Shadertoy port) + cosine palette + vignette
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
    float  VignetteStrength;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

// Inigo Quilez's cosine palette
float3 palette(float t)
{
    float3 a = float3(0.5, 0.5, 0.5);
    float3 b = float3(0.5, 0.5, 0.5);
    float3 c = float3(1.0, 1.0, 1.0);
    float3 d = float3(0.0, 0.33, 0.67);

    return a + b * cos(6.28318 * (c * t + d));
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 fragCoord = input.uv * Resolution;
    float2 uv = (fragCoord * 2.0 - Resolution) / Resolution.y;
    float2 uv0 = uv;

    float3 finalColor = float3(0.0, 0.0, 0.0);
    float zoom = PatternScale;

    // Fractal iteration loop
    for (float i = 0.0; i < 4.0; i++)
    {
        uv = frac(uv * zoom) - 0.5;
        float d = length(uv) * exp(-length(uv0));
        float3 col = palette(length(uv0) + i * 0.4 + Time * Speed);
        d = sin(d * 8.0 + Time * Speed) / 8.0;
        d = abs(d);
        d = pow(0.01 / d, 1.2);

        finalColor += col * d;
    }

    // Apply color tint AFTER loop
    finalColor *= ColorTint;
    
    // Circular vignette - darken center for menu readability
    float dist = length(uv0);
    float vignette = smoothstep(0.0, 1.5, dist);
    finalColor *= lerp(1.0, vignette, VignetteStrength);

    return float4(finalColor, 1.0);
}