#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "../BRDF.hlsli"

struct DirectionalLightData
{
    // Direction from the shaded surface toward the light.
    float3 directionToLight;
    float intensity;
    float3 color;
    float pad0;
};

float3 EvaluateDirectionalPBR(
    float3 normal,
    float3 viewDirection,
    float3 albedo,
    float metallic,
    float roughness,
    DirectionalLightData light)
{
    float3 N = normalize(normal);
    float3 V = normalize(viewDirection);
    float3 L = normalize(light.directionToLight);
    
    float NoL = saturate(dot(N, L));
    
    float3 F0 = lerp((float3) 0.04, albedo, metallic);
    float3 specular = CookTorranceSpecular(N, V, L, roughness, F0);
    
    float3 H = normalize(V + L);
    float3 F = F_Schlick(max(dot(H, V), 0.0), F0);
    float3 kD = (1.0 - F) * (1.0 - metallic);
    float3 diffuse = kD * albedo / PI;
    
    return (diffuse + specular) * light.color * light.intensity * NoL;
}

#endif