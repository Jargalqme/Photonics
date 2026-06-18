#ifndef IBL_COMMON_HLSLI
#define IBL_COMMON_HLSLI

#include "../Common.hlsli"

static const float PI = 3.14159265359;
static const float TWO_PI = 6.28318530718;

// Van der Corput radical inverse - the bit-reversal half of Hammersley.
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return (float) bits * 2.3283064365386963e-10; // / 0x100000000
}

// i-th of 'samples' low-discrepancy 2D points.
float2 SampleHammersley(uint i, uint samples)
{
    return float2((float) i / (float) samples, RadicalInverse_VdC(i));
}

// Orthonormal tangent frame (S, T) around N. Branchless degenerate guard.
void ComputeBasisVectors(float3 N, out float3 S, out float3 T)
{
    T = cross(N, float3(0.0, 1.0, 0.0));
    T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(EPSILON, dot(T, T)));
    T = normalize(T);
    S = normalize(cross(N, T));
}

// Tangent/shading space -> world space.
float3 TangentToWorld(float3 v, float3 N, float3 S, float3 T)
{
    return S * v.x + T * v.y + N * v.z;
}

// Uniform point on the hemisphere (z = cosTheta, up).
float3 SampleHemisphere(float u1, float u2)
{
    float u1p = sqrt(max(0.0, 1.0 - u1 * u1));
    return float3(cos(TWO_PI * u2) * u1p, sin(TWO_PI * u2) * u1p, u1);
}

// Map a compute thread (x, y, face) to the world direction of that cube texel.
float3 GetCubeMapTexCoord(uint3 globalID, float2 imageSize)
{
    float2 st = float2(globalID.xy) / imageSize;
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - 1.0;
    
    float3 ret;
    if (globalID.z == 0)
        ret = float3(1.0, uv.y, -uv.x);
    else if (globalID.z == 1)
        ret = float3(-1.0, uv.y, uv.x);
    else if (globalID.z == 2)
        ret = float3(uv.x, 1.0, -uv.y);
    else if (globalID.z == 3)
        ret = float3(uv.x, -1.0, uv.y);
    else if (globalID.z == 4)
        ret = float3(uv.x, uv.y, 1.0);
    else 
        ret = float3(-uv.x, uv.y, -1.0);
    return normalize(ret);
}

#endif