#include "pch.h"
#include "Gameplay/CollisionSystem.h"

namespace CollisionSystem
{
    bool checkSphereSphere(
        const DirectX::BoundingSphere& a,
        const DirectX::BoundingSphere& b)
    {
        return a.Intersects(b);
    }

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
