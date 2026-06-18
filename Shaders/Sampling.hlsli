//---------------------------------------------------------------------------
//! @file   Sampling.hlsli
//! @brief  Bloom helpers: downsample, upsample, threshold, prefilter.
//
//          Kernel reference (DownsampleBox13Tap, UpsampleTent9):
//          "Next Generation Post Processing in Call of Duty Advanced
//          Warfare", Jorge Jimenez, SIGGRAPH 2014.
//          https://advances.realtimerendering.com/s2014/
//---------------------------------------------------------------------------

#ifndef SAMPLING_HLSLI
#define SAMPLING_HLSLI

#include "Common.hlsli"

// 13-tap box downsample.
//     F . G . H
//     . B . E .
//     I . A . J
//     . C . D .
//     K . L . M
//
// Boxes:
//   inner        (0.5)   : B C D E
//   top-left     (0.125) : F G I A
//   top-right    (0.125) : G H A J
//   bottom-left  (0.125) : I A K L
//   bottom-right (0.125) : A J L M
float4 DownsampleBox13Tap(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize)
{
    // Center
    float4 A = tex.Sample(samp, uv);

    texelSize *= 0.5; // Sample from center of texels

    // Inner box (2x2 at half-texel diagonals)
    float4 B = tex.Sample(samp, uv + texelSize * float2(-1.0, -1.0));
    float4 C = tex.Sample(samp, uv + texelSize * float2(-1.0,  1.0));
    float4 D = tex.Sample(samp, uv + texelSize * float2( 1.0,  1.0));
    float4 E = tex.Sample(samp, uv + texelSize * float2( 1.0, -1.0));

    // Outer 3x3 (full-texel offsets; skip center, which is A)
    float4 F = tex.Sample(samp, uv + texelSize * float2(-2.0, -2.0));
    float4 G = tex.Sample(samp, uv + texelSize * float2( 0.0, -2.0));
    float4 H = tex.Sample(samp, uv + texelSize * float2( 2.0, -2.0));
    float4 I = tex.Sample(samp, uv + texelSize * float2(-2.0,  0.0));
    float4 J = tex.Sample(samp, uv + texelSize * float2( 2.0,  0.0));
    float4 K = tex.Sample(samp, uv + texelSize * float2(-2.0,  2.0));
    float4 L = tex.Sample(samp, uv + texelSize * float2( 0.0,  2.0));
    float4 M = tex.Sample(samp, uv + texelSize * float2( 2.0,  2.0));

    float4 result = float4(0, 0, 0, 0);

    // Inner box
    result += (B + C + D + E) * 0.5;

    // Four outer 2x2 boxes; each shares the center A
    result += (F + G + I + A) * 0.125;
    result += (G + H + A + J) * 0.125;
    result += (I + A + K + L) * 0.125;
    result += (A + J + L + M) * 0.125;

    // Each box summed 4 samples without division; normalize by 4
    result *= 0.25;

    return result;
}

float karisWeight(float3 c)
{
    float luma = dot(c, float3(0.2126, 0.7152, 0.0722)); // Rec.709
    return 1.0 / (1.0 + luma);
}

float4 DownsampleBox13TapKaris(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize)
{
    float3 A = tex.Sample(samp, uv).rgb;
    texelSize *= 0.5;

    float3 B = tex.Sample(samp, uv + texelSize * float2(-1.0, -1.0)).rgb;
    float3 C = tex.Sample(samp, uv + texelSize * float2(-1.0, 1.0)).rgb;
    float3 D = tex.Sample(samp, uv + texelSize * float2(1.0, 1.0)).rgb;
    float3 E = tex.Sample(samp, uv + texelSize * float2(1.0, -1.0)).rgb;
    float3 F = tex.Sample(samp, uv + texelSize * float2(-2.0, -2.0)).rgb;
    float3 G = tex.Sample(samp, uv + texelSize * float2(0.0, -2.0)).rgb;
    float3 H = tex.Sample(samp, uv + texelSize * float2(2.0, -2.0)).rgb;
    float3 I = tex.Sample(samp, uv + texelSize * float2(-2.0, 0.0)).rgb;
    float3 J = tex.Sample(samp, uv + texelSize * float2(2.0, 0.0)).rgb;
    float3 K = tex.Sample(samp, uv + texelSize * float2(-2.0, 2.0)).rgb;
    float3 L = tex.Sample(samp, uv + texelSize * float2(0.0, 2.0)).rgb;
    float3 M = tex.Sample(samp, uv + texelSize * float2(2.0, 2.0)).rgb;

      // Same 5 boxes as DownsampleBox13Tap, but Karis-weighted.
    float3 box0 = (B + C + D + E) * 0.25; // inner  (spatial weight 0.5)
    float3 box1 = (F + G + I + A) * 0.25; // TL     (0.125)
    float3 box2 = (G + H + A + J) * 0.25; // TR
    float3 box3 = (I + A + K + L) * 0.25; // BL
    float3 box4 = (A + J + L + M) * 0.25; // BR

    float w0 = karisWeight(box0) * 0.5;
    float w1 = karisWeight(box1) * 0.125;
    float w2 = karisWeight(box2) * 0.125;
    float w3 = karisWeight(box3) * 0.125;
    float w4 = karisWeight(box4) * 0.125;

    float3 result = (box0 * w0 + box1 * w1 + box2 * w2 + box3 * w3 + box4 * w4)
                    / (w0 + w1 + w2 + w3 + w4);
    return float4(result, 1.0);
}

// 9-tap tent upsample.
// Samples a 3x3 grid around uv with the kernel
//     1 2 1
//     2 4 2
//     1 2 1
// (sum 16)
float4 UpsampleTent9(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize, float radius)
{
    float4 offset = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0) * radius;

    // Center
    float4 result = tex.Sample(samp, uv) * 4.0;

    result += tex.Sample(samp, uv - offset.xy);
    result += tex.Sample(samp, uv - offset.wy) * 2.0;
    result += tex.Sample(samp, uv - offset.zy);

    result += tex.Sample(samp, uv + offset.zw) * 2.0;
    result += tex.Sample(samp, uv + offset.xw) * 2.0;

    result += tex.Sample(samp, uv + offset.zy);
    result += tex.Sample(samp, uv + offset.wy) * 2.0;
    result += tex.Sample(samp, uv + offset.xy);

    return result * (1.0 / 16.0);
}

float4 QuadraticThreshold(float4 color, float threshold, float3 curve)
{
    // Maximum pixel brightness
    float brightness = max(max(color.r, color.g), color.b);
    // Quadratic curve
    float rq = clamp(brightness - curve.x, 0.0, curve.y);
    rq = (rq * rq) * curve.z;
    color *= max(rq, brightness - threshold) / max(brightness, EPSILON);
    return color;
}

float4 Prefilter(float4 color, float threshold, float3 curve)
{
    float clampValue = 20.0;
    color = clamp(color, 0.0, clampValue);
    color = QuadraticThreshold(color, threshold, curve);
    return color;
}

#endif