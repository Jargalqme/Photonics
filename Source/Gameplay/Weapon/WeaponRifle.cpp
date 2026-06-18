#include "pch.h"
#include "Gameplay/Weapon/WeaponRifle.h"
#include "Gameplay/Weapon/WeaponShot.h"

void WeaponRifle::initialize()
{
    m_clipSize = 30;
    m_fireInterval = 0.15f;
    m_damage = 2.5f;
    m_maxRange = 150.0f;
    m_reloadDuration = 1.5f;
    Weapon::initialize();
}

bool WeaponRifle::shoot(
    const Vector3& hitScanOrigin,
    const Vector3& hitScanDirection,
    const Vector3& tracerStart,
    std::vector<WeaponShot>& outShots)
{
    outShots.push_back(WeaponShot{
        hitScanOrigin,
        hitScanDirection,
        tracerStart,
        m_damage,
        m_maxRange,
        Vector4(2.0f, 4.25f, 5.0f, 1.0f),
    });

    return true;
}
