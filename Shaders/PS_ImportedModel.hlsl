//---------------------------------------------------------------------------
//! @file   PS_ImportedModel.hlsl
//! @brief  Physically based (Cook-Torrance) shading for imported models.
//---------------------------------------------------------------------------

#include "Includes/Lighting.hlsli"

Texture2D    BaseColorTexture  : register(t0);
Texture2D    NormalTexture     : register(t1);
Texture2D    MetalRoughTexture : register(t2);
TextureCube  IrradianceCube    : register(t3);
Texture2D    MetalnessTexture  : register(t4);
Texture2D    AOTexture         : register(t5);
SamplerState BaseColorSampler  : register(s0);

cbuffer ImportedModelCB : register(b0)
{
    float4x4 World;
    float4x4 WorldViewProjection;
    float4x4 WorldInverseTranspose;
    float4   TintColor;
    float4   LightDirectionAndAmbient;
    float4   LightColor;
    float4   CameraPosition;
    float4   MaterialParams;
    float4   MaterialFlags;
    float4   MaterialFlags2;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 tangent  : TANGENT;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // --- world-space normal (with normal map) ---
    float3 N = normalize(input.normal);
    if (MaterialParams.z > 0.5)   // hasNormalMap
    {
        float3 T  = normalize(input.tangent - N * dot(N, input.tangent)); // Gram-Schmidt
        float3 B  = cross(N, T);
        float3 tn = NormalTexture.Sample(BaseColorSampler, input.texCoord).xyz * 2.0 - 1.0;
        N = normalize(mul(tn, float3x3(T, B, N))); // tangent -> world
    }

    // --- material inputs ---
    float4 base = TintColor;
    if (MaterialFlags.x > 0.5)
        base *= BaseColorTexture.Sample(BaseColorSampler, input.texCoord); // sRGB -> linear
    float3 albedo = base.rgb;

    float metallic  = MaterialParams.x;
    float roughness = MaterialParams.y;
    bool hasStandaloneMetalnessMap = MaterialFlags.z > 0.5;
    bool hasStandaloneRoughnessMap = MaterialFlags.w > 0.5;
    
    if (MaterialParams.w > 0.5)
    {
        float4 mr = MetalRoughTexture.Sample(BaseColorSampler, input.texCoord);
        
        if (hasStandaloneRoughnessMap)
        {
            roughness *= mr.r;
        }
        else
        {
            roughness *= mr.g;
            
            if (!hasStandaloneMetalnessMap)
            {
                metallic *= mr.b;
            }
        }
    }
    
    if (hasStandaloneMetalnessMap)
    {
        metallic *= MetalnessTexture.Sample(BaseColorSampler, input.texCoord).r;
    }

    roughness = clamp(roughness, 0.04, 1.0);

    DirectionalLightData keyLight;
    keyLight.directionToLight = LightDirectionAndAmbient.xyz;
    keyLight.intensity = 1.0;
    keyLight.color = LightColor.rgb;
    keyLight.pad0 = 0.0;
    
    float3 V = normalize(CameraPosition.xyz - input.worldPos);
    float3 Lo = EvaluateDirectionalPBR(N, V, albedo, metallic, roughness, keyLight);

    // --- placeholder ambient until IBL (Stage 2) ---
    float3 irradiance = IrradianceCube.Sample(BaseColorSampler, N).rgb;

    float ao = 1.0;
    if (MaterialFlags2.x > 0.5)
    {
        ao = AOTexture.Sample(BaseColorSampler, input.texCoord).r;
    }

    float ambientIntensity = LightDirectionAndAmbient.w;
    float3 ambient = irradiance * albedo * (1.0 - metallic) * ao * ambientIntensity;
    
    float  emissive = MaterialFlags.y;
    float3 color    = Lo + ambient + albedo * emissive;

    return float4(color, base.a); // linear HDR; ACES tonemapper finishes it
}
