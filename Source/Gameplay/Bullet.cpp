#include "pch.h"
#include "Gameplay/Bullet.h"
#include "Common/Easing.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void Bullet::initialize(
    const Vector3& position,
    const Vector3& direction,
    float speed,
    float lifetime,
    float damage,
    const Vector4& color)
{
    m_position = position;
    m_direction = direction;
    m_maxSpeed = speed;
    m_minSpeed = speed * MIN_SPEED_RATIO;
    m_age = 0.0f;
    m_totalLifetime = lifetime;
    m_lifetime = lifetime;
    m_damage = damage;
    m_color = color;
    m_active = true;

    m_hasPhaseSwitch = false;
    m_phaseTimer = 0.0f;

    m_boundingSphere.Center.x = position.x;
    m_boundingSphere.Center.y = position.y;
    m_boundingSphere.Center.z = position.z;
    m_boundingSphere.Radius = COLLISION_RADIUS;
}

void Bullet::update(float deltaTime)
{
    if (!m_active)
    {
        return;
    }

    m_age += deltaTime;
    m_lifetime -= deltaTime;
    if (m_lifetime <= 0.0f)
    {
        m_active = false;
        return;
    }

    // 地面以下の弾を消す
    if (m_position.y < 0.0f)
    {
        m_active = false;
        return;
    }

    // イージングによる速度カーブ（発射直後に加速）
    float t = std::min(m_age / SPEED_RAMP_TIME, 1.0f);
    float easedT = Easing::easeOutExpo(t);
    float currentSpeed = m_minSpeed + (m_maxSpeed - m_minSpeed) * easedT;

    if (m_hasPhaseSwitch && m_phaseTimer > 0.0f)
    {
        m_phaseTimer -= deltaTime;
        if (m_phaseTimer <= 0.0f)
        {
            m_direction = m_phaseDirection;
            m_maxSpeed = m_phaseSpeed;
            m_minSpeed = m_phaseSpeed;
            m_hasPhaseSwitch = false;
        }
    }

    m_position += m_direction * currentSpeed * deltaTime;

    m_boundingSphere.Center.x = m_position.x;
    m_boundingSphere.Center.y = m_position.y;
    m_boundingSphere.Center.z = m_position.z;
}

void Bullet::setPhaseSwitch(float delay, const Vector3& newDirection, float newSpeed)
{
    m_hasPhaseSwitch = true;
    m_phaseTimer = delay;
    m_phaseDirection = newDirection;
    m_phaseSpeed = newSpeed;
}

void Bullet::render(
    DirectX::GeometricPrimitive* mesh,
    const Matrix& view,
    const Matrix& projection)
{
    if (!m_active)
    {
        return;
    }

    Matrix world = Matrix::CreateScale(COLLISION_RADIUS) * Matrix::CreateTranslation(m_position);
    mesh->Draw(world, view, projection, Colors::White);
}
