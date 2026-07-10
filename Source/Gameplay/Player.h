//---------------------------------------------------------------------------
//! @file   Player.h
//! @brief  プレイヤー (FPS視点・移動・体力)
//---------------------------------------------------------------------------
#pragma once

#include <DirectXCollision.h>
#include <memory>
#include <vector>

#include "Gameplay/ICombatTarget.h"

struct SceneContext;
struct WeaponShot;
class  InputManager;
class  PlayerWeapon;

//===========================================================================
//! プレイヤー
//! 入力を視点・移動・武器更新へ反映する起点。ICombatTarget として被弾も受ける
//===========================================================================
class Player : public ICombatTarget
{
public:
    explicit Player(SceneContext& context);
    ~Player();

    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    void initialize();
    void finalize();

    //! 位置・体力・視点を開始状態に戻します
    void reset();

    //! 押しっぱなし入力を解除します (デバッグモード移行などマウス解放時)
    void clearInputState();

    //!@}
    //----------------------------------------------------------
    //! @name   更新
    //----------------------------------------------------------
    //!@{

    //! 入力 -> 視点 -> 移動 -> 武器の順で1フレーム分を反映
    void update(InputManager& input, float deltaTime, std::vector<WeaponShot>& outShots);

    //!@}
    //----------------------------------------------------------
    //! @name   武器
    //----------------------------------------------------------
    //!@{

    PlayerWeapon& weapon();
    const PlayerWeapon& weapon() const;

    //!@}
    //----------------------------------------------------------
    //! @name   視点・位置
    //----------------------------------------------------------
    //!@{

    float   lookYaw()     const noexcept { return m_lookYaw;   }
    float   lookPitch()   const noexcept { return m_lookPitch; }

    //! 視線方向を取得します (正のピッチ = 下向き)
    Vector3 lookForward() const;

    //! 視線の右方向を取得します (常に水平)
    Vector3 lookRight()   const;

    Vector3  rootPosition() const noexcept { return m_rootPosition; }
    Vector3  eyeOffset()    const noexcept { return m_eyeOffset; }
    Vector3  eyePosition()  const noexcept { return m_rootPosition + m_eyeOffset; }

    //! ボスの照準先として渡す生ポインタ (BossScene::setPlayerTarget 用)
    Vector3* rootPositionPtr()    noexcept { return &m_rootPosition; }

    void setRootPosition(const Vector3& position) noexcept { m_rootPosition = position; }
    void setEyeOffset   (const Vector3& offset)   noexcept { m_eyeOffset = offset; }

    //!@}
    //----------------------------------------------------------
    //! @name   状態・体力
    //----------------------------------------------------------
    //!@{

    bool isGrounded()   const noexcept { return m_isGrounded; }
    bool isAiming()     const noexcept { return m_isAiming; }
    bool isDead()       const noexcept { return m_health <= 0.0f; }
    bool isInvincible() const noexcept { return m_invincibleTimer > 0.0f; }
    bool isReloading()  const;

    float health()    const noexcept { return m_health; }
    float maxHealth() const noexcept { return m_maxHealth; }
    int ammo()    const;
    int maxAmmo() const;

    float mouseSensitivity() const noexcept { return m_mouseSensitivity; }

    //! DebugUI のスライダ直結用の生ポインタ
    float* mouseSensitivityPtr()   noexcept { return &m_mouseSensitivity; }

    void setHealth(float health) noexcept { m_health = health; }

    //!@}
    //----------------------------------------------------------
    //! @name   ICombatTarget 実装
    //----------------------------------------------------------
    //!@{

    //! 死亡・無敵中は何も積まない = その間は被弾しない (無敵時間の実装)
    void collectHitColliders(std::vector<CombatHitCollider>& out) override;

    //! 被弾を体力へ反映し PlayerDamagedEvent を発行
    void onHit(const CombatHit& hit) override;

    //!@}

private:

    static constexpr float INVINCIBLE_DURATION = 0.5f;      //!< 被弾後の無敵時間 (秒)
    static constexpr float MOVE_THRESHOLD      = 0.001f;    //!< 移動入力とみなす最小の長さの2乗
    static constexpr float ARENA_HALF_SIZE     = 500.0f;    //!< XZ の移動可能範囲 (±)
    static constexpr float LOOK_PITCH_CLAMP    = 89.0f;     //!< ピッチの上限 (度)
    // TODO: dead constant need to wire it cleanly.
    static constexpr float PLAYER_HIT_RADIUS   = 0.8f;

    void setAiming(bool aiming) noexcept { m_isAiming = aiming; }
    void takeDamage(float amount);

    //! 視点差分を適用し、クランプ後に実際へ適用された差分を返す (武器スウェイ入力用)
    Vector2 applyLookDelta(float deltaYawDegrees, float deltaPitchDegrees);
    void updateMovement(const Vector3& direction, float deltaTime);
    void updateVerticalMovement(float deltaTime);
    void updateInvincibility(float deltaTime);
    void jump();

    //! 実移動距離から水平速度を算出 (前回位置を更新する副作用あり・毎フレーム1回前提)
    float movementSpeed(float deltaTime);

    //! 目線カメラのワールド行列 (ビュー行列の逆行列)
    Matrix createGameplayCameraWorldMatrix() const;

    std::unique_ptr<PlayerWeapon> m_weapon;

    Vector3 m_rootPosition { 0.0f, 0.0f, 0.0f };    //!< 足元位置
    Vector3 m_eyeOffset    { 0.0f, 6.3f, 0.0f };    //!< 足元 -> 目の高さ
    Vector3 m_lastPosition { 0.0f, 0.0f, 0.0f };    //!< 前フレーム位置 (movementSpeed 用)

    float m_speed            = 25.0f;    //!< 移動速度
    float m_lookYaw          = 0.0f;     //!< ヨー (度, 0-360 循環)
    float m_lookPitch        = 0.0f;     //!< ピッチ (度, 正 = 下向き)
    float m_mouseSensitivity = 0.05f;    //!< マウス感度

    float m_health           = 100.0f;   //!< 体力
    float m_maxHealth        = 100.0f;   //!< 最大体力
    float m_invincibleTimer  = 0.0f;     //!< 残り無敵時間 (秒)

    bool m_isGrounded        = true;     //!< 接地中
    bool m_isAiming          = false;    //!< ADS中
    float m_verticalVelocity = 0.0f;     //!< 垂直速度 (ジャンプ/重力)
    float m_jumpForce        = 15.0f;    //!< ジャンプ初速
    float m_gravity          = 40.0f;    //!< 重力加速度
    float m_groundLevel      = 0.0f;     //!< 地面の高さ
};
