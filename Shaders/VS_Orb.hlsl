//---------------------------------------------------------------------------
//! @file   VS_Orb.hlsl
//! @brief  Camera-facing quad billboard for colored bullet circles
//---------------------------------------------------------------------------

#define VERTS_PER_ORB 6

struct BoltData
{
    float3 position;
    float speed;
    float3 direction;
    float age;
    float4 color;
};

StructuredBuffer<BoltData> Bolts : register(t0);

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

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 localPos : TEXCOORD0;
    float4 color    : COLOR0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;

    uint orbIndex = vertexID / VERTS_PER_ORB;
    uint localVert = vertexID % VERTS_PER_ORB;

    if (orbIndex >= BoltCount)
    {
        output.position = float4(0, 0, 0, 1);
        return output;
    }

    BoltData bolt = Bolts[orbIndex];

    // Quad corners (two triangles forming a [-1,1] square)
    float2 corners[6] =
    {
        float2(-1, -1), float2(1, -1), float2(-1, 1),
          float2(1, -1), float2(1, 1), float2(-1, 1)
    };
    float2 localPos = corners[localVert];

    // Camera-facing frame
    float3 forward = normalize(bolt.position - CameraPosition);
    float3 right = cross(forward, float3(0, 1, 0));
    if (length(right) < 0.001)
        right = cross(forward, float3(1, 0, 0));
    right = normalize(right);
    float3 up = normalize(cross(right, forward));

    float3 worldPos = bolt.position
                      + right * localPos.x * BoltWidth
                      + up * localPos.y * BoltWidth;

    output.position = mul(float4(worldPos, 1.0), ViewProjection);
    output.localPos = localPos;
    output.color = bolt.color;

    return output;
}
