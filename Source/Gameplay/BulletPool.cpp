//---------------------------------------------------------------------------
//! @file   BulletPool.cpp
//! @brief  敵弾の固定長プール (ボス戦専用)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Gameplay/BulletPool.h"

//---------------------------------------------------------------------------
//! 非アクティブな弾を確保して初期化します (満杯なら nullptr)
//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
//! 全弾を更新
//---------------------------------------------------------------------------
void BulletPool::update(float deltaTime)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        m_bullets[i].update(deltaTime);
    }
}

//---------------------------------------------------------------------------
//! 全弾を非アクティブ化します
//---------------------------------------------------------------------------
void BulletPool::deactivateAll()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        m_bullets[i].deactivate();
    }
}

//---------------------------------------------------------------------------
//! アクティブな弾を球メッシュとしてキューへ積む
//---------------------------------------------------------------------------
void BulletPool::submitRender(RenderCommandQueue& queue, DirectX::GeometricPrimitive* mesh) const
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        m_bullets[i].submitRender(queue, mesh);
    }
}

//---------------------------------------------------------------------------
//! アクティブな弾数を数えます
//---------------------------------------------------------------------------
int BulletPool::countActive() const
{
    int count = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (m_bullets[i].isActive())
        {
            count++;
        }
    }
    return count;
}
