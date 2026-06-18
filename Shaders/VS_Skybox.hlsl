//---------------------------------------------------------------------------
//! @file   VS_Skybox.hlsl
//! @brief  Fullscreen triangle at depth=1 with world direction from inverse VP
//---------------------------------------------------------------------------
#include "Camera.hlsli"

struct VS_OUTPUT
{
    float4 position  : SV_POSITION;
    float3 direction : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;

    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);

    float4 view = mul(output.position, cameraInvProj);
    
    float4x4 invViewNoTranslation = cameraInvView;
    invViewNoTranslation._41 = 0.0f;
    invViewNoTranslation._42 = 0.0f;
    invViewNoTranslation._43 = 0.0f;
    
    float4 world = mul(view, invViewNoTranslation);
    output.direction = world.xyz;

    return output;
}
