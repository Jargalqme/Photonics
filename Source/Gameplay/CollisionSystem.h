//---------------------------------------------------------------------------
//! @file   CollisionSystem.h
//! @brief  衝突判定ヘルパ (球 vs 球 / レイ vs 球)
//---------------------------------------------------------------------------
#pragma once
#include <DirectXCollision.h>

//===========================================================================
//! 衝突判定ヘルパ (DirectXCollision の薄いラッパ)
//===========================================================================
namespace CollisionSystem
{
    //! 球同士の交差判定
    bool checkSphereSphere(
        const DirectX::BoundingSphere& a,
        const DirectX::BoundingSphere& b);

    //! レイと球の交差判定 (rayDirection は正規化済み前提)
    //! 交差時 outDistance = 始点から交点までの距離
    bool checkRaySphere(
        const DirectX::SimpleMath::Vector3& rayStart,
        const DirectX::SimpleMath::Vector3& rayDirection,
        const DirectX::BoundingSphere& sphere,
        float& outDistance);
}
