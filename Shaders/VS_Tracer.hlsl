//---------------------------------------------------------------------------
//! @file   VS_Tracer.hlsl
//! @brief  Bullet tracer ribbon vertex shader.
//---------------------------------------------------------------------------

cbuffer TracerConstants : register(b0)
{
    matrix ViewProjection;
    float3 TracerStart;
    float  TracerWidth;
    float3 TracerEnd;
    float  TracerLife;
    float4 TracerColor;
    float3 CameraPosition;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

static const float2 quadUVs[6] =
{
    float2(0, 0), float2(0, 1), float2(1, 0),
    float2(1, 0), float2(0, 1), float2(1, 1)
};

float3 makeRibbonRight(float3 dir, float3 toCamera)
{
    float3 right = cross(dir, toCamera);
    float lenSq = dot(right, right);

    if (lenSq < 0.00001)
    {
        right = cross(dir, float3(0.0, 1.0, 0.0));
        lenSq = dot(right, right);
    }

    if (lenSq < 0.00001)
    {
        right = cross(dir, float3(1.0, 0.0, 0.0));
        lenSq = dot(right, right);
    }

    return right * rsqrt(max(lenSq, 0.00001));
}

VS_OUTPUT main(uint id : SV_VertexID)
{
    VS_OUTPUT output;
    float2 uv = quadUVs[id];

    float3 tracerPos = lerp(TracerStart, TracerEnd, uv.x);
    float3 tracerDir = normalize(TracerEnd - TracerStart);
    float3 toCamera = normalize(CameraPosition - tracerPos);
    float3 right = makeRibbonRight(tracerDir, toCamera);

    float offset = (uv.y - 0.5) * TracerWidth;
    float3 worldPos = tracerPos + right * offset;

    output.position = mul(float4(worldPos, 1.0), ViewProjection);
    output.uv = uv;
    output.worldPos = worldPos;
    return output;
}
