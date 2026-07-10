//---------------------------------------------------------------------------
//! @file   CollisionSystem.cpp
//! @brief  衝突判定ヘルパ (球 vs 球 / レイ vs 球)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Gameplay/CollisionSystem.h"

namespace CollisionSystem
{
    //---------------------------------------------------------------------------
    //! 球同士の交差判定
    //---------------------------------------------------------------------------
    bool checkSphereSphere(
        const DirectX::BoundingSphere& a,
        const DirectX::BoundingSphere& b)
    {
        return a.Intersects(b);
    }

    //---------------------------------------------------------------------------
    //! レイと球の交差判定
    //! Intersects は方向の正規化を前提とする (Debug ではアサートされる)
    //---------------------------------------------------------------------------
    bool checkRaySphere(
        const DirectX::SimpleMath::Vector3& rayStart,
        const DirectX::SimpleMath::Vector3& rayDirection,
        const DirectX::BoundingSphere& sphere,
        float& outDistance)
    {
        DirectX::XMFLOAT3 startFloat(rayStart.x, rayStart.y, rayStart.z);
        DirectX::XMFLOAT3 dirFloat(rayDirection.x, rayDirection.y, rayDirection.z);

        DirectX::XMVECTOR origin = DirectX::XMLoadFloat3(&startFloat);
        DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&dirFloat);

        return sphere.Intersects(origin, direction, outDistance);
    }
}
