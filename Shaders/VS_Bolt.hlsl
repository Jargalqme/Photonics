//---------------------------------------------------------------------------
//! @file   VS_Bolt.hlsl
//! @brief  Cylindrical mesh generation for Returnal-style projectiles
//---------------------------------------------------------------------------

#define SEGMENTS 6
#define RINGS 4
#define TRIS_PER_BOLT (SEGMENTS * (RINGS - 1) * 2)
#define VERTS_PER_BOLT (TRIS_PER_BOLT * 3)  // 108

static const float PI = 3.14159265359;
static const float TWO_PI = 6.28318530718;

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
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float age : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float3 normal : TEXCOORD3;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    uint boltIndex = vertexID / VERTS_PER_BOLT;
    uint localVert = vertexID % VERTS_PER_BOLT;

    // Past the active bolt count -> degenerate triangle
    if (boltIndex >= BoltCount)
    {
        output.position = float4(0, 0, 0, 1);
        return output;
    }

    BoltData bolt = Bolts[boltIndex];

    // Decode vertex ID to (ring, segment) pair
    // Each strip between two rings has SEGMENTS quads, each quad = 2 triangles
    uint triIndex = localVert / 3;
    uint corner = localVert % 3;

    uint strip = triIndex / (SEGMENTS * 2);      // which ring pair (0 to RINGS-2)
    uint triInStrip = triIndex % (SEGMENTS * 2);
    uint quad = triInStrip / 2;                   // which segment (0 to SEGMENTS-1)
    uint triInQuad = triInStrip % 2;              // which triangle in quad

    // Map triangle corner to (ring, segment) indices
    // Quad between ring r/r+1 and segment s/s+1:
    //   Triangle 0: (r,s), (r,s+1), (r+1,s)
    //   Triangle 1: (r+1,s), (r,s+1), (r+1,s+1)
    uint ring, seg;
    if (triInQuad == 0)
    {
        if (corner == 0)      { ring = strip;     seg = quad; }
        else if (corner == 1) { ring = strip;     seg = (quad + 1) % SEGMENTS; }
        else                  { ring = strip + 1; seg = quad; }
    }
    else
    {
        if (corner == 0)      { ring = strip + 1; seg = quad; }
        else if (corner == 1) { ring = strip;     seg = (quad + 1) % SEGMENTS; }
        else                  { ring = strip + 1; seg = (quad + 1) % SEGMENTS; }
    }

    // Position along bolt (0=tail, 1=tip) and angle around tube
    float t = (float)ring / (float)(RINGS - 1);
    float angle = (float)seg * (TWO_PI / (float)SEGMENTS);

    // Radius profile — tapers at both ends, fat in the middle
    float profile = smoothstep(0.0, 0.3, t) * smoothstep(1.0, 0.7, t);
    float radius = BoltWidth * profile;

    // Build coordinate frame from bolt direction (no camera dependency)
    float3 dir = normalize(bolt.direction);
    float3 right = cross(dir, float3(0, 1, 0));
    if (length(right) < 0.001)
        right = cross(dir, float3(1, 0, 0));
    right = normalize(right);
    float3 up = normalize(cross(right, dir));

    // Position on tube surface
    // Tip at bolt.position, tail extends backwards along -dir
    float3 center = bolt.position - dir * BoltLength * (1.0 - t);
    float3 tubeOffset = right * cos(angle) * radius + up * sin(angle) * radius;
    float3 worldPos = center + tubeOffset;

    // Surface normal (outward from tube axis)
    float3 normal = normalize(right * cos(angle) + up * sin(angle));

    output.position = mul(float4(worldPos, 1.0), ViewProjection);
    output.uv = float2(t, angle / TWO_PI);
    output.color = bolt.color;
    output.age = bolt.age;
    output.worldPos = worldPos;
    output.normal = normal;

    return output;
}
