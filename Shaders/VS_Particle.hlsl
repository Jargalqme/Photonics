//---------------------------------------------------------------------------
//! @file   VS_Particle.hlsl
//! @brief  GPU particle billboard rendering (StructuredBuffer + SV_VertexID)
//---------------------------------------------------------------------------

struct Particle
{
    float3 position;
    float lifetime;
    float3 velocity;
    float maxLifetime;
    float4 color;
    float size;
    float pad1;
    float pad2;
    float pad3;
};

StructuredBuffer<Particle> Particles : register(t0);

cbuffer ParticleCB : register(b0)
{
    float4x4 InverseView;
    float4x4 ViewProjection;
    float DeltaTime;
    float TotalTime;
    uint MaxParticles;
    float Padding;
    float4 EmitPosition;
    float4 EmitColor;
    float EmitSpeed;
    float EmitLifetime;
    uint EmitCount;
    float EmitSpread;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float life : TEXCOORD1;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    uint particleIndex = vertexID / 6;
    uint cornerIndex = vertexID % 6;

    Particle p = Particles[particleIndex];

    // Dead particle -> degenerate triangle (zero area, auto-culled)
    if (p.lifetime <= 0.0)
    {
        output.position = float4(0, 0, 0, 1);
        return output;
    }

    // Quad corner offsets (two triangles: 0-1-2, 2-1-3)
    // Corners: 0=TL, 1=TR, 2=BL, 3=BR
    static const float2 corners[4] = {
        float2(-1,  1),  // TL
        float2( 1,  1),  // TR
        float2(-1, -1),  // BL
        float2( 1, -1),  // BR
    };
    static const uint indices[6] = { 0, 1, 2, 2, 1, 3 };

    float2 corner = corners[indices[cornerIndex]];

    // Billboard: extract camera right/up from inverse view matrix
    float3 camRight = InverseView[0].xyz;
    float3 camUp    = InverseView[1].xyz;

    // Expand quad in world space
    float halfSize = p.size;
    float3 worldPos = p.position
        + camRight * corner.x * halfSize
        + camUp    * corner.y * halfSize;

    output.position = mul(float4(worldPos, 1.0), ViewProjection);

    // UV for soft circle (0 to 1)
    output.uv = corner * 0.5 + 0.5;

    // Pass color and lifetime ratio
    output.color = p.color;
    output.life = p.lifetime / max(p.maxLifetime, 0.001);

    return output;
}
