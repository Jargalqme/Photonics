//---------------------------------------------------------------------------
//! @file   VS_Billboard.hlsl
//! @brief  Camera-facing quad billboard (SV_VertexID, 4 corners)
//---------------------------------------------------------------------------

cbuffer BillboardCB : register(b0)
{
    float4x4 ViewProjection;
    float3 WorldPosition;
    float BillboardSize;
    float3 CameraRight;
    float Pad0;
    float3 CameraUp;
    float Pad1;
}

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    // 4 corners: bottom-left, top-left, bottom-right, top-right
    float2 offsets[4] =
    {
        float2(-1, -1),
        float2(-1, +1),
        float2(+1, -1),
        float2(+1, +1)
    };
    
    float2 offset = offsets[vertexID];
    
    // Expand quad in world space using camera axes (billboard)
    float3 worldPos = WorldPosition
    + CameraRight * offset.x * BillboardSize
    + CameraUp * offset.y * BillboardSize;
    
    output.position = mul(float4(worldPos, 1.0), ViewProjection);
    
    // Map offsets to UV: (-1, -1) > (0, 1), (+1, +1) > (1, 0)
    output.texCoord = float2(offset.x * 0.5 + 0.5, 0.5 - offset.y * 0.5);
    
    return output;
}