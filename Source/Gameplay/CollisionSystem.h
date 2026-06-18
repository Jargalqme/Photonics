#pragma once
#include <DirectXCollision.h>

namespace CollisionSystem
{
    bool checkSphereSphere(
        const DirectX::BoundingSphere& a,
        const DirectX::BoundingSphere& b);

    bool checkRaySphere(
        const DirectX::SimpleMath::Vector3& rayStart,
        const DirectX::SimpleMath::Vector3& rayDirection,
        const DirectX::BoundingSphere& sphere,
        float& outDistance);
}
