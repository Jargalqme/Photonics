#pragma once
#include <SimpleMath.h>
#include <DirectXCollision.h>
#include "Gameplay/ICombatTarget.h"

using namespace DirectX::SimpleMath;

class Bullet
{
public:
    /// @brief プールから取り出した弾を初期化する（Nystrom Object Pool パターン）
    void initialize(
        const Vector3& position,
        const Vector3& direction,
        float speed,
        float lifetime,
        float damage,
        const Vector4& color = Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    void update(float deltaTime);

    void render(
        DirectX::GeometricPrimitive* mesh,
        const Matrix& view,
        const Matrix& projection);

    // --- プール管理 ---
    bool isActive() const { return m_active; }
    void deactivate() { m_active = false; }

    // --- ゲッター ---
    float getDamage() const { return m_damage; }
    Vector3 getPosition() const { return m_position; }
    Vector3 getDirection() const { return m_direction; }
    float getMaxSpeed() const { return m_maxSpeed; }
    float getAge() const { return m_age; }
    const Vector4& getColor() const { return m_color; }
    const DirectX::BoundingSphere& getBoundingSphere() const { return m_boundingSphere; }

    void setPhaseSwitch(float delay, const Vector3& newDirection, float newSpeed);

private:
    static constexpr float COLLISION_RADIUS  = 0.3f;
    static constexpr float SPEED_RAMP_TIME   = 0.3f;   // イージング完了までの時間
    static constexpr float MIN_SPEED_RATIO   = 0.4f;   // 最低速度の割合

    bool m_active = false;

    Vector3 m_position  = Vector3::Zero;
    Vector3 m_direction = Vector3::Zero;
    float m_lifetime = 0.0f;
    float m_damage   = 0.0f;

    float m_maxSpeed      = 0.0f;
    float m_minSpeed      = 0.0f;
    float m_age           = 0.0f;
    float m_totalLifetime = 0.0f;

    // フェーズ切り替え
    float m_phaseTimer = 0.0f;
    Vector3 m_phaseDirection;
    float m_phaseSpeed = 0.0f;
    bool m_hasPhaseSwitch = false;

    Vector4 m_color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    DirectX::BoundingSphere m_boundingSphere = { XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f };
};
