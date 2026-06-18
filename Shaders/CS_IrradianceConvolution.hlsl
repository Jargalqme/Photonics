#include "Includes/IBLCommon.hlsli"

RWTexture2DArray<float4> o_Irradiance : register(u0);
TextureCube u_Radiance : register(t0);
SamplerState u_Sampler : register(s0);

static const uint NUM_SAMPLES = 2048;

[numthreads(32, 32, 1)] // 1024 threads = D3D11 cap; one group per 32x32 face
void main(uint3 id : SV_DispatchThreadID)
{
    uint width, height, elements;
    o_Irradiance.GetDimensions(width, height, elements); // HLSL analog of imageSize()
    
    float3 N = GetCubeMapTexCoord(id, float2(width, height));
    
    float3 S, T;
    ComputeBasisVectors(N, S, T);
    
    float3 irradiance = 0.0;
    for (uint i = 0; i < NUM_SAMPLES; ++i)
    {
        float2 u = SampleHammersley(i, NUM_SAMPLES);
        float3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N, S, T);
        float cosTheta = max(0.0, dot(Li, N));
        
        float3 radiance = u_Radiance.SampleLevel(u_Sampler, Li, 0).rgb;
        
        radiance = pow(max(radiance, 0.0), 2.2);
        
        irradiance += 2.0 * radiance * cosTheta;
    }
    irradiance /= (float) NUM_SAMPLES;
    
    o_Irradiance[id] = float4(irradiance, 1.0);
}