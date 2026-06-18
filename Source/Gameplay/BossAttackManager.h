#pragma once
#include <SimpleMath.h>
#include <vector>

class BulletPool;

class BossAttackManager
{
private:
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;

public:
    void initialize(BulletPool* pool);
    void update(float deltaTime);

    void setPosition(const Vector3& pos) { m_position = pos; }
    void setPlayerTarget(const Vector3* target) { m_playerTarget = target; }
    void setPhase(int phase);

private:
    // --- 行動チェイン型 ---
    enum class Pattern { Rain, Burst, Torus };
    enum class Stage   { Telegraph, Attack, Recovery };

    struct Maneuver
    {
        Pattern pattern;
        float   telegraphDuration;
        float   recoveryDuration;
    };

    // --- 弾パラメータ ---
    static constexpr float BULLET_SPEED    = 15.0f;
    static constexpr float BULLET_LIFETIME = 5.0f;
    static constexpr float BULLET_DAMAGE   = 15.0f;

    // フォーリングレイン
    static constexpr float RAIN_SPAWN_HEIGHT    = 12.0f;
    static constexpr int   RAIN_BULLET_COUNT    = 45;
    static constexpr float RAIN_ANGLE_JITTER    = 0.3f;    // 角度のばらつき
    static constexpr int   RAIN_RING_COUNT      = 30;

    // 展開フェーズ
    static constexpr float RAIN_EXPAND_FAST     = 10.0f;
    static constexpr float RAIN_EXPAND_MEDIUM   = 7.0f;
    static constexpr float RAIN_EXPAND_SLOW     = 4.0f;
    static constexpr float RAIN_EXPAND_DURATION = 2.5f;

    // 落下フェーズ
    static constexpr float RAIN_FALL_SPEED      = 6.0f;
    static constexpr float RAIN_FALL_SPREAD     = 0.05f;    // 落下中の横広がり
    static constexpr float RAIN_TOTAL_LIFETIME  = 12.0f;    // 展開 + 落下の合計

    static constexpr int   BURST_BULLET_COUNT = 15;
    static constexpr float BURST_INTERVAL     = 0.06f;
    static constexpr float BURST_BULLET_SPEED = 35.0f;

    static constexpr int   TORUS_BULLET_COUNT = 30;
    static constexpr float TORUS_SPEED        = 10.0f;
    static constexpr float TORUS_SPAWN_HEIGHT = 0.5f;

    // 攻撃パターン別色
    static constexpr Vector4 RAIN_COLOR  = Vector4(1.70f, 0.15f, 2.80f, 1.0f); // violet
    static constexpr Vector4 BURST_COLOR = Vector4(2.75f, 0.06f, 0.08f, 1.0f); // red
    static constexpr Vector4 TORUS_COLOR = Vector4(0.55f, 2.60f, 0.20f, 1.0f); // acid green

    // バースト連射状態
    bool m_burstActive   = false;
    int m_burstRemaining = 0;
    float m_burstTimer   = 0.0f;

    // チェイン状態
    std::vector<Maneuver> m_chain;
    int   m_chainIndex = 0;
    Stage m_stage      = Stage::Telegraph;
    float m_stageTimer = 0.0f;

    // --- 攻撃メソッド ---
    void firePattern(Pattern p);
    float attackDurationFor(Pattern p) const;
    std::vector<Maneuver> buildChainForPhase(int phase) const;

    void fireRain();
    void fireRainRing(int count, float expandSpeed, float angleOffset);
    void fireAimedBurst();
    void fireBurstBullet();
    void fireTorus();

    // --- メンバ ---
    BulletPool* m_bulletPool      = nullptr;
    const Vector3* m_playerTarget = nullptr;
    Vector3 m_position;
};
