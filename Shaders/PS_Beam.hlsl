//---------------------------------------------------------------------------
//! @file   PS_Beam.hlsl
//! @brief  Beam weapon pixel shader — core/inner/outer glow layers + pulse
//---------------------------------------------------------------------------

cbuffer BeamConstants : register(b0)
{
    matrix ViewProjection;
    float3 BeamStart;
    float  BeamWidth;
    float3 BeamEnd;
    float  BeamLife;
    float4 BeamColor;
    float3 CameraPosition;
    float  Time;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Distance from center
    // uv.y ranges [0,1]. subtract 0.5 to center, abs symmetry, *2 to normalize to [0,1].
    float distFromCenter = abs(input.uv.y - 0.5) * 2.0;
    
    // Glow layers
    // 1 - smoothstep inverts: 1 at center, 0 at edge
    float core      = 1.0 - smoothstep(0.0, 0.1, distFromCenter);
    float innerGlow = 1.0 - smoothstep(0.0, 0.4, distFromCenter);
    float outerGlow = 1.0 - smoothstep(0.0, 1.0, distFromCenter);
    
    // Color composition
    float3 coreColor = float3(1.0, 1.0, 1.0);
    float3 glowColor = BeamColor.rgb;
    float3 color = coreColor * core * 2.0        // overexposed center
                 + glowColor * innerGlow * 1.5   // saturated body
                 + glowColor * outerGlow * 0.5;  // atmospheric edge
    
    
    // Pulse animation
    float pulse = sin(input.uv.x * 30.0 - Time * 20.0) * 0.15 + 1.0;
    color *= pulse;  // brightness ripples [0.85, 1.15]
    
    // Fades
    float tipFade   = smoothstep(0.98, 0.85, input.uv.x);  // soft tip (reversed)
    float startFade = smoothstep(0.0, 0.05, input.uv.x);   // soft origin
    float lifeFade = BeamLife;
    float alpha = outerGlow * tipFade * startFade * lifeFade;
    
    // Pre-multiplied output
    return float4(color * alpha, alpha);
}