//---------------------------------------------------------------------------
//! @file   PS_Particle.hlsl
//! @brief  Soft glowing particle circles with lifetime fade
//---------------------------------------------------------------------------

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float life : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Distance from center of quad (UV 0.5, 0.5 = center)
    float2 center = input.uv - 0.5;
    float dist = length(center) * 2.0; // 0 at center, 1 at edge

    // Soft circle with glow falloff
    float circle = 1.0 - smoothstep(0.0, 1.0, dist);
    circle *= circle; // squared for glow effect

    // Discard fully transparent pixels
    if (circle < 0.001)
        discard;

    // Lifetime fade: smooth fade-out in last 30% of life
    float lifeFade = smoothstep(0.0, 0.3, input.life);

    // Final alpha
    float alpha = circle * lifeFade * input.color.a;

    // Pre-multiplied alpha output for additive blending
    return float4(input.color.rgb * alpha, alpha);
}
