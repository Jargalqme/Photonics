//---------------------------------------------------------------------------
//! @file   WeaponFrame.h
//! @brief  1フレーム分の武器入力スナップショット
//---------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>

//! 1フレーム分の武器入力 (Player が組み立て、PlayerWeapon / WeaponMotion が共有で読む)
struct PlayerWeaponFrame
{
    float deltaTime = 0.0f;
    DirectX::SimpleMath::Vector3 hitScanOrigin = DirectX::SimpleMath::Vector3::Zero;        //!< 目線レイの始点
    DirectX::SimpleMath::Vector3 hitScanDirection = DirectX::SimpleMath::Vector3::UnitZ;    //!< 目線レイの方向 (正規化済み)
    DirectX::SimpleMath::Vector2 viewDeltaDegrees = DirectX::SimpleMath::Vector2::Zero;     //!< 実際に適用された視点差分 (度) -> スウェイ入力
    float moveSpeed = 0.0f;        //!< 水平移動速度 -> ボブ入力
    bool fireHeld = false;
    bool adsHeld = false;
    bool reloadPressed = false;    //!< リロード入力 (押した瞬間のみ true)
    bool isGrounded = true;        //!< 接地中 (空中はボブ停止)
    DirectX::SimpleMath::Matrix cameraWorld = DirectX::SimpleMath::Matrix::Identity;        //!< 目線カメラのワールド行列 (ビュー行列の逆)
};
