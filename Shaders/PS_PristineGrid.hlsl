//---------------------------------------------------------------------------
//! @file   PS_PristineGrid.hlsl
//! @brief  Anti-aliased procedural grid lines (Ben Golus pristine grid)
//---------------------------------------------------------------------------

#include "Camera.hlsli"

cbuffer GridConstants : register(b0)
{
    matrix worldViewProjection;
    float4 gridParams;
    float4 lineColor;
    float4 baseColor;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 worldPos : TEXCOORD0;
};

float pristineGrid(float2 uv, float2 lineWidth)
{
    float2 ddxUV = ddx(uv);
    float2 ddyUV = ddy(uv);
    float2 uvDeriv = float2(length(float2(ddxUV.x, ddyUV.x)), length(float2(ddxUV.y, ddyUV.y)));

    bool2 invertLine = bool2(lineWidth.x > 0.5, lineWidth.y > 0.5);
    float2 targetWidth = float2(
          invertLine.x ? 1.0 - lineWidth.x : lineWidth.x,
          invertLine.y ? 1.0 - lineWidth.y : lineWidth.y
      );

    float2 drawWidth = clamp(targetWidth, uvDeriv, float2(0.5, 0.5));
    float2 lineAA = uvDeriv * 1.5;
    float2 gridUV = abs(frac(uv) * 2.0 - 1.0);

    gridUV.x = invertLine.x ? gridUV.x : 1.0 - gridUV.x;
    gridUV.y = invertLine.y ? gridUV.y : 1.0 - gridUV.y;

    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
    grid2 = lerp(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));

    grid2.x = invertLine.x ? 1.0 - grid2.x : grid2.x;
    grid2.y = invertLine.y ? 1.0 - grid2.y : grid2.y;

    return lerp(grid2.x, 1.0, grid2.y);
}

//float4 main(PS_INPUT input) : SV_TARGET
//{
//    float2 uv = input.worldPos * gridParams.z;
//    float2 lineWidth = float2(gridParams.x, gridParams.y);
//    float grid = pristineGrid(uv, lineWidth);
//    float lineEmissiveIntensity = gridParams.w;
//    float3 lineHDR = lineColor.rgb * (1.0 + lineEmissiveIntensity);
//    //return float4(lineHDR, grid);
    
//    float3 color = lerp(baseColor.rgb, lineHDR, grid);
//    return float4(color, 1.0);
//}

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.worldPos * gridParams.z;
    float2 lineWidth = float2(gridParams.x, gridParams.y);

    float grid = pristineGrid(uv, lineWidth);

    float cellsPerPixel = max(fwidth(uv).x, fwidth(uv).y);
    float fade = 1.0 - smoothstep(0.35, 1.0, cellsPerPixel);
    grid *= fade;

    float lineEmissiveIntensity = gridParams.w * fade;
    float3 lineHDR = lineColor.rgb * (1.0 + lineEmissiveIntensity);
    float3 color = lerp(baseColor.rgb, lineHDR, grid);

    // --- Distance fog into a horizon color --------------------------------
    const float3 fogColor = float3(0.0, 0.0, 0.0); // linear synthwave horizon (tune)
    const float fogDensity = 0.01; // larger = fog closer (tune)
    float dist = length(input.worldPos - cameraPosition.xz); // worldPos is world XZ
    float fog = 1.0 - exp(-dist * fogDensity); // 0 near Å® 1 far
    color = lerp(color, fogColor, fog);
    // ---------------------------------------------------------------------

    return float4(color, 1.0);
}