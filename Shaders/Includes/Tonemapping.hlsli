#ifndef TONEMAPPING_HLSLI
#define TONEMAPPING_HLSLI

//---------------------------------------------------------------------------
// Tonemap_ACES_Hill
// Hue-correct ACES fit. Bright saturated colors desaturate toward white
// instead of locking at maximum chroma.
//
// Source:  https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// Author:  Stephen Hill (@self_shadow)
// License: MIT
//---------------------------------------------------------------------------

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 Tonemap_ACES_Hill(float3 color)
{
    color = mul(ACESInputMat, color);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul(ACESOutputMat, color);

    // Clamp to [0, 1]
    color = saturate(color);

    return color;
}



//---------------------------------------------------------------------------
// Tonemap_ACES_Narkowicz
// Cheap one-rational ACES curve fit. No hue correction — bright saturated
// colors clip to their primary instead of desaturating toward white.
//
// Source:  https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// Author:  Krzysztof Narkowicz
// License: MIT
//---------------------------------------------------------------------------

float3 Tonemap_ACES_Narkowicz(float3 color)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

#endif
