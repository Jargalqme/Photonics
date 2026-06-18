#pragma once
#include "Gameplay/Weapon/Weapon.h"

class WeaponRifle : public Weapon
{
public:
    void initialize() override;

protected:
    bool shoot(
        const DirectX::SimpleMath::Vector3& hitScanOrigin,
        const DirectX::SimpleMath::Vector3& hitScanDirection,
        const DirectX::SimpleMath::Vector3& tracerStart,
        std::vector<WeaponShot>& outShots) override;
};
