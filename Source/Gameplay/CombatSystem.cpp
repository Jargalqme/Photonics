#include "pch.h"
#include "CombatSystem.h"

#include "Gameplay/Bullet.h"
#include "Gameplay/BulletPool.h"
#include "Gameplay/CollisionSystem.h"
#include "Gameplay/Weapon/WeaponShot.h"
#include "Gameplay/EventBus.h"
#include "Gameplay/EventTypes.h"

void CombatSystem::update(
    float deltaTime,
    std::vector<ICombatTarget*>& shotTargets,
    std::vector<ICombatTarget*>& bulletTargets,
    BulletPool& bullets,
    std::vector<WeaponShot>& weaponShots)
{
    bullets.update(deltaTime);
    collectColliders(shotTargets, m_shotColliders);
    collectColliders(bulletTargets, m_bulletColliders);
    resolveWeaponShots(weaponShots);
    resolveBullets(bullets);
}

void CombatSystem::collectColliders(
    std::vector<ICombatTarget*>& targets,
    std::vector<CombatHitCollider>& out)
{
    out.clear();
    for (ICombatTarget* target : targets)
    {
        if (target)
        {
            target->collectHitColliders(out);
        }
    }
}

void CombatSystem::resolveWeaponShots(std::vector<WeaponShot>& shots)
{
    for (const auto& shot : shots)
    {
        float closest = shot.maxRange;
        const CombatHitCollider* hitCollider = nullptr;

        for (const auto& collider : m_shotColliders)
        {
            float dist = 0.0f;
            if (CollisionSystem::checkRaySphere(shot.hitScanOrigin, shot.hitScanDirection, collider.bounds, dist)
                && dist < closest)
            {
                closest = dist;
                hitCollider = &collider;
            }
        }

        const Vector3 hitPoint = hitCollider
            ? (shot.hitScanOrigin + shot.hitScanDirection * closest)
            : (shot.hitScanOrigin + shot.hitScanDirection * shot.maxRange);

        if (hitCollider)
        {
            CombatHit hit;
            hit.part = hitCollider->part;
            hit.point = hitPoint;
            hit.direction = shot.hitScanDirection;
            hit.rawDamage = shot.damage;
            hit.finalDamage = shot.damage * hitCollider->damageMultiplier;
            hitCollider->target->onHit(hit);
        }

        EventBus::publish(ShotResolvedEvent{ shot.tracerStart, hitPoint, shot.tracerColor });
    }

    shots.clear();
}

void CombatSystem::resolveBullets(BulletPool& bullets)
{
    Bullet* arr = bullets.getBullets();
    const int n = bullets.getMaxBullets();

    for (int i = 0; i < n; i++)
    {
        if (!arr[i].isActive())
        {
            continue;
        }

        for (const auto& collider : m_bulletColliders)
        {
            if (CollisionSystem::checkSphereSphere(arr[i].getBoundingSphere(), collider.bounds))
            {
                CombatHit hit;
                hit.part = collider.part;
                hit.point = arr[i].getPosition();
                hit.direction = arr[i].getDirection();
                hit.rawDamage = arr[i].getDamage();
                hit.finalDamage = arr[i].getDamage() * collider.damageMultiplier;
                collider.target->onHit(hit);
                arr[i].deactivate();
                break;
            }
        }
    }
}
