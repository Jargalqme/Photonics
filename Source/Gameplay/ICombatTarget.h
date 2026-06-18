#pragma once

#include <DirectXCollision.h>
#include <SimpleMath.h>
#include <vector>

class ICombatTarget;

enum class HitPart
{
    Body,
    Head,
    Core,
    Armor,
    WeakPoint
};

struct CombatHitCollider
{
    ICombatTarget*           target           = nullptr;
    HitPart                  part             = HitPart::Body;
    DirectX::BoundingSphere  bounds;
    float                    damageMultiplier = 1.0f;
};

struct CombatHit
{
    HitPart                       part        = HitPart::Body;
    DirectX::SimpleMath::Vector3  point       = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector3  direction   = DirectX::SimpleMath::Vector3::Zero;
    float                         rawDamage   = 0.0f;
    float                         finalDamage = 0.0f;
};

class ICombatTarget
{
public:
    virtual ~ICombatTarget() = default;
    virtual void collectHitColliders(std::vector<CombatHitCollider>& out) = 0;
    virtual void onHit(const CombatHit& hit) = 0;
};
