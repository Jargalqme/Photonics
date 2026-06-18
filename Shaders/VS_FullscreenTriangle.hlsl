//---------------------------------------------------------------------------
//! @file   VS_FullscreenTriangle.hlsl
//! @brief  Fullscreen triangle from SV_VertexID (no vertex buffer)
//---------------------------------------------------------------------------

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;

    // Generate fullscreen triangle from vertex ID
    // ID 0 -> (-1, -1)  ID 1 -> (3, -1)  ID 2 -> (-1, 3)
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.uv = uv;
    output.position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);

    return output;
}