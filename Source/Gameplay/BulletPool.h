#pragma once
#include "Gameplay/Bullet.h"

class BulletPool
{
public:
    Bullet* acquire(
        const Vector3& position,
        const Vector3& direction,
        float speed,
        float lifetime,
        float damage,
        const Vector4& color = Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    void update(float deltaTime);

    Bullet* getBullets() { return m_bullets; }
    int getMaxBullets() const { return MAX_BULLETS; }

private:
    static constexpr int MAX_BULLETS = 800;
    Bullet m_bullets[MAX_BULLETS];
};
