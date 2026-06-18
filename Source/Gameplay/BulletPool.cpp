#include "pch.h"
#include "Gameplay/BulletPool.h"

Bullet* BulletPool::acquire(
    const Vector3& position,
    const Vector3& direction,
    float speed,
    float lifetime,
    float damage,
    const Vector4& color)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!m_bullets[i].isActive())
        {
            m_bullets[i].initialize(position, direction, speed, lifetime, damage, color);
            return &m_bullets[i];
        }
    }
    return nullptr;
}

void BulletPool::update(float deltaTime)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        m_bullets[i].update(deltaTime);
    }
}
