#pragma once

#include <SimpleMath.h>
#include <vector>

struct WeaponShot;

class Weapon
{
public:
    virtual ~Weapon() = default;

    virtual void initialize();
    virtual void finalize() {}

    bool update(
        float deltaTime,
        const DirectX::SimpleMath::Vector3& hitScanOrigin,
        const DirectX::SimpleMath::Vector3& hitScanDirection,
        const DirectX::SimpleMath::Vector3& tracerStart,
        std::vector<WeaponShot>& outShots);

    void startFire();
    void stopFire();
    bool canFire() const;
    bool isFiring() const { return m_firing; }

    void reload();
    void cancelReload();
    bool isReloading() const { return m_reloading; }

    int getAmmoCount() const { return m_ammo; }
    int getClipSize() const { return m_clipSize; }
    bool outOfAmmo() const { return m_ammo <= 0; }

protected:
    virtual bool shoot(
        const DirectX::SimpleMath::Vector3& hitScanOrigin,
        const DirectX::SimpleMath::Vector3& hitScanDirection,
        const DirectX::SimpleMath::Vector3& tracerStart,
        std::vector<WeaponShot>& outShots) = 0;

    int m_ammo = 0;
    int m_clipSize = 30;
    float m_fireInterval = 0.1f;
    float m_nextShotTime = 0.0f;
    float m_reloadDuration = 2.0f;
    float m_reloadTimer = 0.0f;
    float m_damage = 10.0f;
    float m_maxRange = 200.0f;
    bool m_firing = false;
    bool m_reloading = false;
};
