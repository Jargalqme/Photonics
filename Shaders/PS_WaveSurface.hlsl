struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float  height   : TEXCOORD0;
};

static const float3 LIGHT_DIR = normalize(float3(0.5, -1.0, 0.3));

float4 main(PS_INPUT input) : SV_TARGET
{
    float  diff  = saturate(dot(normalize(input.normal), -LIGHT_DIR));
    float3 deep  = float3(0.0, 0.15, 0.1);
    float3 crest = float3(0.3, 0.9, 0.6);
    float3 color = lerp(deep, crest, saturate(input.height * 0.05 + 0.5));
    return float4(color * (0.3 + 0.7 * diff), 1.0);   // ambient + diffuse
}
