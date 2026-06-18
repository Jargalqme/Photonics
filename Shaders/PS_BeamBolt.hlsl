//---------------------------------------------------------------------------
//! @file   PS_BeamBolt.hlsl
//! @brief  Fresnel rim glow + emissive surface for cylindrical bolt geometry
//---------------------------------------------------------------------------

cbuffer BoltCB : register(b0)
{
    float4x4 ViewProjection;
    float3 CameraPosition;
    float Time;
    float BoltLength;
    float BoltWidth;
    uint BoltCount;
    float Padding;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
    float  age      : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float3 normal   : TEXCOORD3;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // View direction for fresnel
    float3 viewDir = normalize(CameraPosition - input.worldPos);
    float ndotv = saturate(dot(input.normal, viewDir));

    // Fresnel rim — edges glow brighter at grazing angles
    float rim = pow(1.0 - ndotv, 2.0);

    // Color composition
    float3 glowColor = input.color.rgb;
    float3 color = glowColor * 1.5                     // base emissive everywhere
                 + glowColor * rim * 2.5               // bright rim glow
                 + float3(1, 1, 1) * ndotv * ndotv;    // white center highlight

    // Pulse animation (energy ripples along bolt)
    float pulse = sin(input.uv.x * 30.0 - Time * 20.0) * 0.1 + 1.0;
    color *= pulse;

    // Fades
    float tipFade   = smoothstep(1.0, 0.8, input.uv.x);    // soft tip
    float tailFade  = smoothstep(0.0, 0.15, input.uv.x);    // soft tail
    float spawnFade = smoothstep(0.0, 0.05, input.age);      // fade in on spawn

    float alpha = (0.6 + rim * 0.4) * tipFade * tailFade * spawnFade;

    if (alpha < 0.001)
        discard;

    // Pre-multiplied alpha for additive blending
    return float4(color * alpha, alpha);
}
