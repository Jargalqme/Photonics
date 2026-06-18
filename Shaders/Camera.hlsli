//---------------------------------------------------------------------------
//! @file   Camera.hlsli
//! @brief  全シェーダー共有のカメラ定数（b10）
//---------------------------------------------------------------------------
#ifndef CAMERA_HLSLI
#define CAMERA_HLSLI

//---------------------------------------------------------------------------
// カメラ情報構造体
//---------------------------------------------------------------------------
cbuffer CameraInfo : register(b10)
{
    float4x4 cameraView;               // ビュー行列
    float4x4 cameraProj;               // 投影行列
    float4x4 cameraInvView;            // ビュー行列の逆行列
    float4x4 cameraInvProj;            // 投影行列の逆行列
    float3   cameraPosition;           // カメラワールド座標
    float    _pad;
};

#endif // CAMERA_HLSLI