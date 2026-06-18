#pragma once

#include "Gameplay/ICombatTarget.h"

#include <vector>

class BulletPool;
struct WeaponShot;

class CombatSystem
{
public:
    CombatSystem() = default;

    void update(
        float deltaTime,
        std::vector<ICombatTarget*>& shotTargets,
        std::vector<ICombatTarget*>& bulletTargets,
        BulletPool& bullets,
        std::vector<WeaponShot>& weaponShots);

private:
    void collectColliders(std::vector<ICombatTarget*>& targets, std::vector<CombatHitCollider>& out);
    void resolveWeaponShots(std::vector<WeaponShot>& shots);
    void resolveBullets(BulletPool& bullets);

    std::vector<CombatHitCollider> m_shotColliders;
    std::vector<CombatHitCollider> m_bulletColliders;
};
