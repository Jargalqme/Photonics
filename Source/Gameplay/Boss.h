//---------------------------------------------------------------------------
//! @file   Boss.h
//! @brief  ボス (3フェーズ移動 + 攻撃管理)
//---------------------------------------------------------------------------
#pragma once
#include <DirectXCollision.h>
#include "Gameplay/BossAttackManager.h"
#include "Gameplay/ICombatTarget.h"
#include "Common/StateMachine.h"
#include "Common/Transform.h"
#include "Render/Visuals/Billboard.h"

class BulletPool;
class ImportedModel;
class ParticleSystem;
class PlayerCamera;
class RenderCommandQueue;
struct SceneContext;

//! ボスのフェーズ (HP割合の閾値で遷移)
enum class BossPhase
{
    phase1, phase2, phase3
};

//===========================================================================
//! ボス
//! HP閾値でフェーズFSMを遷移させ、移動パターンと攻撃セットを切り替える
//===========================================================================
class Boss : public ICombatTarget
{
public:
    Boss(SceneContext& context);

    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    //! メッシュ・ビルボードを構築します (攻撃側の初期化は activate() で)
    void initialize();

    //! フラッシュ減衰 -> フェーズ判定 -> FSM -> 攻撃更新
    void update(float deltaTime);

    //! コア球 + 2リング + 頭部モデル + 目の発光球 (+ _DEBUG 判定球) を積む
    void submitRender(RenderCommandQueue& queue) const;

    void finalize();

    //!@}
    //----------------------------------------------------------
    //! @name   アクティベーション
    //----------------------------------------------------------
    //!@{

    //! 戦闘を開始します (setBulletPool / setPlayerTarget の後に呼ぶこと)
    void activate();

    void deactivate() { m_activated = false; m_active = false; }
    bool isActivated() const { return m_activated; }

    //!@}
    //----------------------------------------------------------
    //! @name   体力
    //----------------------------------------------------------
    //!@{

    bool isDead() const { return m_health <= 0.0f; }
    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    float getHealthPercent() const { return m_health / m_maxHealth; }

    //!@}
    //----------------------------------------------------------
    //! @name   ICombatTarget 実装
    //----------------------------------------------------------
    //!@{

    //! ボディ + スカル弱点 (2倍ダメージ) の2球を積む
    void collectHitColliders(std::vector<CombatHitCollider>& out) override;

    //! 被弾フラッシュ + 体力減算。死亡で BossDiedEvent 発行
    void onHit(const CombatHit& hit) override;

    //!@}
    //----------------------------------------------------------
    //! @name   外部システム接続 (非所有・シーンが配線)
    //----------------------------------------------------------
    //!@{

    void setPlayerTarget(const Vector3* target) { m_playerTarget = target; }
    void setBulletPool(BulletPool* pool) { m_bulletPool = pool; }
    void setParticles(ParticleSystem* particles) { m_particles = particles; }
    void setCamera(PlayerCamera* camera) { m_camera = camera; }

    //!@}
    //----------------------------------------------------------
    //! @name   取得・設定
    //----------------------------------------------------------
    //!@{

    Vector3 getPosition() const { return m_transform.position; }
    void setPosition(const Vector3& pos) { m_transform.position = pos; }

    //!@}

private:

    // 体力
    static constexpr float BOSS_MAX_HEALTH = 500.0f;

    // ヒットフラッシュ
    static constexpr float HIT_FLASH_DURATION = 0.1f;

    // フェーズ閾値 (HP割合)
    static constexpr float PHASE2_HP_THRESHOLD = 0.50f;
    static constexpr float PHASE3_HP_THRESHOLD = 0.25f;

    // リング描画パラメータ
    static constexpr float RING_SPIN_SPEED    = 1.5f;    //!< リング自転速度 (rad/s)
    static constexpr float RING_TILT          = 0.3f;
    static constexpr float SKULL_OFFSET_Y     = 4.0f;    //!< スカルの浮遊高さ (ボディ基準)
    static constexpr float SKULL_SIZE         = 2.5f;

    // 頭部モデル描画パラメータ
    static constexpr const char* HEAD_MODEL_PATH   = "Assets/Characters/Boss/boss_head.glb";
    static constexpr const char* HEAD_MATERIAL_DIR = "Assets/Textures/PBR/Stone";
    static constexpr float HEAD_TARGET_HEIGHT = 3.0f;              //!< 正規化後の頭の高さ (弱点判定球の直径に合わせる)
    static constexpr float HEAD_YAW_OFFSET    = DirectX::XM_PI;    //!< モデル前方補正 (glTF +Z が取込後 -Z を向くため)
    static constexpr float EYE_GLOW_RADIUS    = 0.15f;             //!< 目の発光球の半径
    static constexpr float EYE_EMISSIVE       = 4.0f;              //!< 目のブルーム発光倍率

    // 当たり判定コライダ (collectHitColliders で使用)
    static constexpr float BODY_RADIUS        = 2.4f;    //!< ボディ判定球
    static constexpr float SKULL_RADIUS       = 1.5f;    //!< 弱点判定球
    static constexpr float WEAK_POINT_MULT    = 2.0f;    //!< 弱点ダメージ倍率

    // フェーズ1 - ゆっくり軌道
    static constexpr float P1_ORBIT_RADIUS = 25.0f;
    static constexpr float P1_ORBIT_SPEED  = 0.3f;    // rad/s
    static constexpr float P1_HEIGHT       = 10.0f;

    // フェーズ2 - 速い軌道 + 上下バウンド
    static constexpr float P2_ORBIT_RADIUS = 35.0f;
    static constexpr float P2_ORBIT_SPEED  = 0.7f;
    static constexpr float P2_BASE_HEIGHT  = 10.0f;
    static constexpr float P2_BOB_AMPLITUDE = 4.0f;
    static constexpr float P2_BOB_SPEED    = 2.0f;

    // フェーズ3 - ダッシュ再配置
    static constexpr float P3_DASH_INTERVAL   = 1.8f;   // 次ダッシュまでの秒数
    static constexpr float P3_DASH_LERP       = 5.0f;   // 補間速度 (大きいほど素早い)
    static constexpr float P3_RADIUS_MIN      = 15.0f;
    static constexpr float P3_RADIUS_MAX      = 45.0f;
    static constexpr float P3_HEIGHT_MIN      = 6.0f;
    static constexpr float P3_HEIGHT_MAX      = 14.0f;

    // --- メンバ変数 ---
    SceneContext* m_context;

    void buildBoss();
    Billboard m_skull;                                     //!< スカルビルボード (頭部モデル未ロード時のフォールバック)
    const ImportedModel* m_headModel = nullptr;            //!< 頭部モデル (ImportedModelCache から借用・非所有)
    Vector3 m_headCenter = Vector3::Zero;                  //!< 頭部バウンディングボックス中心 (原点が首元のため必要)
    float m_headLongestSide = 1.0f;                        //!< 頭部正規化用の最長辺
    Vector3 m_eyeLocalL = Vector3::Zero;                   //!< Eye_L 空ノード位置 (モデル空間)
    Vector3 m_eyeLocalR = Vector3::Zero;                   //!< Eye_R 空ノード位置 (モデル空間)
    DirectX::GeometricPrimitive* m_coreMesh = nullptr;     // MeshCache から借用 (非所有)
    DirectX::GeometricPrimitive* m_outerringMesh = nullptr;
    DirectX::GeometricPrimitive* m_innerRingMesh = nullptr;
    float m_ringOrbitAngle = 0.0f;                         //!< リング自転角 (rad)

#ifdef _DEBUG
    DirectX::GeometricPrimitive* m_debugSphere = nullptr;  //!< 判定球の可視化用
#endif

    BossAttackManager m_attacks;    //!< 攻撃パターン管理
    Color m_color;

    // 体力
    float m_health = BOSS_MAX_HEALTH;
    float m_maxHealth = BOSS_MAX_HEALTH;
    float m_hitFlashTimer = 0.0f;    //!< 被弾フラッシュの残り秒数

    // アクティベーション
    bool m_activated = false;

    // フェーズ
    StateMachine<BossPhase> m_phaseFSM;    //!< フェーズ遷移FSM (update が HP閾値で遷移させる)

    void updatePhase1(float dt);
    void updatePhase2(float dt);
    void updatePhase3(float dt);

    // 移動状態
    float m_moveAngle = 0.0f;   //!< P1/P2 の軌道角度
    float m_bobPhase = 0.0f;    //!< P2 の上下位相
    float m_dashTimer = 0.0f;   //!< P3 の次ダッシュまでのタイマー
    Vector3 m_dashTarget = Vector3::Zero;    //!< P3 のダッシュ目標位置

    // 外部システム (非所有)
    ParticleSystem* m_particles = nullptr;
    PlayerCamera* m_camera = nullptr;
    const Vector3* m_playerTarget = nullptr;    //!< プレイヤー足元位置 (BossScene が配線)
    BulletPool* m_bulletPool = nullptr;

    // 旧 GameObject 由来
    Transform m_transform;
    bool m_active = true;
};
