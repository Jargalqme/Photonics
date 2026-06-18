//---------------------------------------------------------------------------
//! @file   PS_WaveWorld.hlsl
//! @brief  Arena floor wave effect — animated sine distortion
//---------------------------------------------------------------------------

cbuffer WaveWorldConstants : register(b0)
{
    matrix WorldViewProjection;
    float Time;
    float Speed;
    float Brightness;
    float Alpha;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Scale world XZ to reasonable range for sine math
    float2 uv = input.uv * 0.012;

    for (float i = 1.0; i < 8.0; i++)
    {
        uv.x += 0.6 / i * cos(i * 2.5 * uv.y + Time * Speed);
        uv.y += 0.6 / i * cos(i * 1.5 * uv.x + Time * Speed);
    }

    // Keep the original menu wave color language: restrained magenta/cyan,
    // not full rainbow hue cycling.
    float t = Time * Speed;
    float val = (0.1 * Brightness) / max(abs(sin(t - uv.y - uv.x)), 0.05);
    float3 color1 = float3(0.7, 0.0, 0.5);
    float3 color2 = float3(0.0, 0.6, 0.9);
    float kick = pow(saturate(sin(t * 9.4)), 8.0);
    float blend = sin(uv.y * 2.0 + t * 0.3) * 0.5 + 0.5;
    blend = lerp(blend, 1.0, kick * 0.6);
    float3 col = val * lerp(color1, color2, blend);

    return float4(col, Alpha);
}
