//---------------------------------------------------------------------------
//! @file   PS_Warping.hlsl
//! @brief  Chromatic RGB warping menu background effect
//---------------------------------------------------------------------------

cbuffer MenuConstants : register(b0)
{
    float  Time;
    float2 Resolution;
    float  Speed;
    float  PatternScale;
    float  WarpIntensity;
    float  Brightness;
    float  ChromaticOffset;
    float3 ColorTint;
    float  Padding;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float2 mod(float2 x, float y)
{
    return x - y * floor(x / y);
}

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 fragCoord = input.uv * Resolution;

    float3 c = float3(0.0, 0.0, 0.0);
    float l;
    float z = Time * Speed;

    for (int i = 0; i < 3; i++)
    {
        float2 p = fragCoord / Resolution;
        float2 uv = p;

        p -= 0.5;
        p.x *= Resolution.x / Resolution.y;

        z += ChromaticOffset;
        l = length(p);

        float warp = (sin(z) + 1.0) * abs(sin(l * PatternScale - z - z));
        uv += p / l * warp * WarpIntensity;

        float b = Brightness / length(mod(uv, 1.0) - 0.5);

        if (i == 0)
            c.r = b;
        if (i == 1)
            c.g = b;
        if (i == 2)
            c.b = b;
    }

    float3 finalColor = c / l;
    finalColor *= ColorTint;

    return float4(finalColor, 1.0);
}