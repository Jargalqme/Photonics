#pragma once
#include <DirectXCollision.h>
#include "Gameplay/BossAttackManager.h"
#include "Gameplay/ICombatTarget.h"
#include "Common/StateMachine.h"
#include "Common/Transform.h"
#include "Render/Visuals/Billboard.h"

class BulletPool;
class ParticleSystem;
class PlayerCamera;
class RenderCommandQueue;
struct SceneContext;

enum class BossPhase
{
    phase1, phase2, phase3
};

class Boss : public ICombatTarget
{
public:
    Boss(SceneContext& context);

    // --- ライフサイクル ---
    void initialize();
    void update(float deltaTime);
    void submitRender(RenderCommandQueue& queue) const;
    void finalize();

    // --- アクティベーション ---
    void activate();
    void deactivate() { m_activated = false; m_active = false; }
    bool isActivated() const { return m_activated; }

    // --- 体力 ---
    bool isDead() const { return m_health <= 0.0f; }
    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    float getHealthPercent() const { return m_health / m_maxHealth; }

    // --- ICombatTarget ---
    void collectHitColliders(std::vector<CombatHitCollider>& out) override;
    void onHit(const CombatHit& hit) override;

    // --- 外部システム接続 ---
    void setPlayerTarget(const Vector3* target) { m_playerTarget = target; }
    void setBulletPool(BulletPool* pool) { m_bulletPool = pool; }
    void setParticles(ParticleSystem* particles) { m_particles = particles; }
    void setCamera(PlayerCamera* camera) { m_camera = camera; }

    // --- ゲッター / セッター ---
    Vector3 getPosition() const { return m_transform.position; }
    void setPosition(const Vector3& pos) { m_transform.position = pos; }

private:

    // 体力
    static constexpr float BOSS_MAX_HEALTH = 500.0f;

    // ヒットフラッシュ
    static constexpr float HIT_FLASH_DURATION = 0.1f;

    // フェーズ閾値（HP割合）
    static constexpr float PHASE2_HP_THRESHOLD = 0.50f;
    static constexpr float PHASE3_HP_THRESHOLD = 0.25f;

    // 撃破演出
    // リング描画パラメータ
    static constexpr float RING_SPIN_SPEED    = 1.5f;
    static constexpr float RING_TILT          = 0.3f;
    static constexpr float SKULL_OFFSET_Y     = 4.0f;
    static constexpr float SKULL_SIZE         = 2.5f;

    // 当たり判定コライダ（collectHitColliders で使用）
    static constexpr float BODY_RADIUS        = 2.4f;
    static constexpr float SKULL_RADIUS       = 1.5f;
    static constexpr float WEAK_POINT_MULT    = 2.0f;

    // フェーズ1 — ゆっくり軌道
    static constexpr float P1_ORBIT_RADIUS = 25.0f;
    static constexpr float P1_ORBIT_SPEED  = 0.3f;    // rad/s
    static constexpr float P1_HEIGHT       = 10.0f;

    // フェーズ2 — 速い軌道 + 上下バウンド
    static constexpr float P2_ORBIT_RADIUS = 35.0f;
    static constexpr float P2_ORBIT_SPEED  = 0.7f;
    static constexpr float P2_BASE_HEIGHT  = 10.0f;
    static constexpr float P2_BOB_AMPLITUDE = 4.0f;
    static constexpr float P2_BOB_SPEED    = 2.0f;

    // フェーズ3 — ダッシュ再配置
    static constexpr float P3_DASH_INTERVAL   = 1.8f;   // 次ダッシュまでの秒数
    static constexpr float P3_DASH_LERP       = 5.0f;   // 補間速度（大きいほど素早い）
    static constexpr float P3_RADIUS_MIN      = 15.0f;
    static constexpr float P3_RADIUS_MAX      = 45.0f;
    static constexpr float P3_HEIGHT_MIN      = 6.0f;
    static constexpr float P3_HEIGHT_MAX      = 14.0f;

    // --- メンバ変数 ---
    SceneContext* m_context;

    void buildBoss();
    Billboard m_skull;
    DirectX::GeometricPrimitive* m_coreMesh = nullptr;  // MeshCache から借用（非所有）
    DirectX::GeometricPrimitive* m_outerringMesh = nullptr;
    DirectX::GeometricPrimitive* m_innerRingMesh = nullptr;
    float m_ringOrbitAngle = 0.0f;

#ifdef _DEBUG
    DirectX::GeometricPrimitive* m_debugSphere = nullptr;
#endif

    BossAttackManager m_attacks;
    Color m_color;

    // 体力
    float m_health = BOSS_MAX_HEALTH;
    float m_maxHealth = BOSS_MAX_HEALTH;
    float m_hitFlashTimer = 0.0f;

    // アクティベーション
    bool m_activated = false;

    //　フェーズ
    StateMachine<BossPhase> m_phaseFSM;

    void updatePhase1(float dt);
    void updatePhase2(float dt);
    void updatePhase3(float dt);

    // 移動状態
    float m_moveAngle = 0.0f;   // P1/P2 の軌道角度
    float m_bobPhase = 0.0f;    // P2 の上下位相
    float m_dashTimer = 0.0f;   // P3 の次ダッシュまでのタイマー
    Vector3 m_dashTarget = Vector3::Zero;

    // 外部システム（非所有）
    ParticleSystem* m_particles = nullptr;
    PlayerCamera* m_camera = nullptr;
    const Vector3* m_playerTarget = nullptr;
    BulletPool* m_bulletPool = nullptr;

    // 旧 GameObject 由来
    Transform m_transform;
    bool m_active = true;
};
