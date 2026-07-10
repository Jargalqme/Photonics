#ifndef BRDF_HLSLI
#define BRDF_HLSLI
//---------------------------------------------------------------------------
//! @file   BRDF.hlsli
//! @brief  Cook-Torrance microfacet BRDF (GGX + Smith + Fresnel-Schlick)
//!         Shared by direct PBR shading and (later) IBL passes.
//---------------------------------------------------------------------------

static const float PI = 3.14159265359;

float D_GGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float G_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 CookTorranceSpecular(float3 N, float3 V, float3 L, float roughness, float3 F0)
{
    float3 H = normalize(V + L);
    
    float D = D_GGX(N, H, roughness);
    float G = G_Smith(N, V, L, roughness);
    float3 F = F_Schlick(max(dot(H, V), 0.0), F0);
    
    float3 numerator = D * G * F;
    float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L));
    
    return numerator / max(denominator, 0.001);
}

#endif