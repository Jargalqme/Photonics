//---------------------------------------------------------------------------
//! @file   VS_Beam.hlsl
//! @brief  Beam weapon vertex shader — camera-facing quad between start/end
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

struct VS_OUTPUT
{
    float4 position : SV_POSITION; // System_Value Semantics
    float2 uv       : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

// 6 vertices = 2 triangles forming a quad
static const float2 quadUVs[6] =
{
    float2(0, 0), float2(0, 1), float2(1, 0), // triangle 1
    float2(1, 0), float2(0, 1), float2(1, 1)  // triangle 2
};

VS_OUTPUT main(uint id : SV_VertexID)
{
    VS_OUTPUT output;
    float2 uv = quadUVs[id];

    float3 beamPos  = lerp(BeamStart, BeamEnd, uv.x);
    float3 beamDir  = normalize(BeamEnd - BeamStart);
    float3 toCamera = normalize(CameraPosition - beamPos);
    float3 right    = normalize(cross(beamDir, toCamera));

    float offset = (uv.y - 0.5) * BeamWidth;
    float3 worldPos = beamPos + right * offset;

    output.position = mul(float4(worldPos, 1.0), ViewProjection);
    output.uv = uv;
    output.worldPos = worldPos;
    return output;
}
