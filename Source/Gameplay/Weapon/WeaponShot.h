#pragma once

#include <SimpleMath.h>

// Data emitted by a weapon when it fires a hitscan shot.
struct WeaponShot
{
    DirectX::SimpleMath::Vector3 hitScanOrigin;
    DirectX::SimpleMath::Vector3 hitScanDirection;
    DirectX::SimpleMath::Vector3 tracerStart;
    float damage;
    float maxRange;
    DirectX::SimpleMath::Vector4 tracerColor;
};
