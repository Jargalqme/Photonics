//---------------------------------------------------------------------------
//! @file   Player.cpp
//! @brief  プレイヤー (FPS視点・移動・体力)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Gameplay/Player.h"
#include "Gameplay/EventBus.h"
#include "Gameplay/EventTypes.h"
#include "Gameplay/Weapon/PlayerWeapon.h"
#include "Services/SceneContext.h"
#include "Services/InputManager.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
    //---------------------------------------------------------------------------
    //! WASD を移動入力へ変換 (y = 前後, x = 左右)
    //---------------------------------------------------------------------------
    Vector2 playerReadMoveInput(const InputManager& input)
    {
        Vector2 move = Vector2::Zero;

        if (input.isKeyDown(Keyboard::Keys::W))
        {
            move.y += 1.0f;
        }
        if (input.isKeyDown(Keyboard::Keys::S))
        {
            move.y -= 1.0f;
        }
        if (input.isKeyDown(Keyboard::Keys::A))
        {
            move.x -= 1.0f;
        }
        if (input.isKeyDown(Keyboard::Keys::D))
        {
            move.x += 1.0f;
        }

        return move;
    }

    //---------------------------------------------------------------------------
    //! 視線基準の水平移動方向を合成
    //---------------------------------------------------------------------------
    Vector3 playerMovementDirection(Player& player, const Vector2& move)
    {
        Vector3 moveForward = player.lookForward();
        moveForward.y = 0.0f;
        moveForward.Normalize();

        Vector3 moveRight = player.lookRight();
        moveRight.y = 0.0f;
        moveRight.Normalize();

        return moveForward * move.y + moveRight * move.x;
    }
}

//---------------------------------------------------------------------------
//! コンストラクタ (武器を生成)
//---------------------------------------------------------------------------
Player::Player(SceneContext& context)
    : m_weapon(std::make_unique<PlayerWeapon>(context))
{
}

Player::~Player() = default;

//---------------------------------------------------------------------------
//! 武器を初期化します
//---------------------------------------------------------------------------
void Player::initialize()
{
    m_weapon->initialize();
}

//---------------------------------------------------------------------------
//! 視点差分を適用 (ヨーは 0-360 で循環、ピッチはクランプ)
//! 戻り値 = 実際に適用された差分 (武器スウェイ入力用)
//---------------------------------------------------------------------------
Vector2 Player::applyLookDelta(float deltaYaw, float deltaPitch)
{
    const float previousPitch = m_lookPitch;

    m_lookYaw += deltaYaw;
    if (m_lookYaw > 360.0f) { m_lookYaw -= 360.0f; }
    if (m_lookYaw < 0.0f)   { m_lookYaw += 360.0f; }

    m_lookPitch += deltaPitch;
    if (m_lookPitch > LOOK_PITCH_CLAMP)  { m_lookPitch = LOOK_PITCH_CLAMP; }
    if (m_lookPitch < -LOOK_PITCH_CLAMP) { m_lookPitch = -LOOK_PITCH_CLAMP;}

    return Vector2(deltaYaw, m_lookPitch - previousPitch);
}

//---------------------------------------------------------------------------
//! 水平移動 + アリーナ境界クランプ
//---------------------------------------------------------------------------
void Player::updateMovement(const Vector3& direction, float deltaTime)
{
    Vector3 moveDir = direction;
    moveDir.y = 0.0f;
    if (moveDir.LengthSquared() > MOVE_THRESHOLD)
    {
        moveDir.Normalize();
        m_rootPosition += moveDir * m_speed * deltaTime;
    }

    m_rootPosition.x = std::clamp(m_rootPosition.x, -ARENA_HALF_SIZE, ARENA_HALF_SIZE);
    m_rootPosition.z = std::clamp(m_rootPosition.z, -ARENA_HALF_SIZE, ARENA_HALF_SIZE);
}

//---------------------------------------------------------------------------
//! 入力 -> 視点 -> 移動 -> ジャンプ -> 武器の順で1フレーム分を反映
//---------------------------------------------------------------------------
void Player::update(InputManager& input, float deltaTime, std::vector<WeaponShot>& outShots)
{
    const Vector2 mouseDelta = input.getMouseDelta();
    const Vector2 viewDeltaDegrees = applyLookDelta(
        mouseDelta.x * m_mouseSensitivity,
        mouseDelta.y * m_mouseSensitivity);

    const bool fireHeld = input.isLeftMouseDown();
    const bool adsHeld = input.isRightMouseDown();
    const bool reloadPressed = input.isKeyPressed(Keyboard::Keys::R);
    setAiming(adsHeld);

    const Vector2 move = playerReadMoveInput(input);
    updateMovement(playerMovementDirection(*this, move), deltaTime);

    if (input.isKeyPressed(Keyboard::Keys::Space))
    {
        jump();
    }

    updateInvincibility(deltaTime);
    updateVerticalMovement(deltaTime);

    PlayerWeaponFrame weaponFrame;
    weaponFrame.deltaTime = deltaTime;
    weaponFrame.hitScanOrigin = eyePosition();
    weaponFrame.hitScanDirection = lookForward();
    weaponFrame.viewDeltaDegrees = viewDeltaDegrees;
    weaponFrame.moveSpeed = movementSpeed(deltaTime);
    weaponFrame.fireHeld = fireHeld;
    weaponFrame.adsHeld = adsHeld;
    weaponFrame.reloadPressed = reloadPressed;
    weaponFrame.isGrounded = m_isGrounded;
    weaponFrame.cameraWorld = createGameplayCameraWorldMatrix();
    m_weapon->update(weaponFrame, outShots);
}

//---------------------------------------------------------------------------
//! 押しっぱなし入力を解除します
//---------------------------------------------------------------------------
void Player::clearInputState()
{
    m_weapon->clearInputState();
    setAiming(false);
}

//---------------------------------------------------------------------------
//! 武器を取得します
//---------------------------------------------------------------------------
PlayerWeapon& Player::weapon()
{
    return *m_weapon;
}

//---------------------------------------------------------------------------
//! 武器を取得します (const版)
//---------------------------------------------------------------------------
const PlayerWeapon& Player::weapon() const
{
    return *m_weapon;
}

//---------------------------------------------------------------------------
//! 実移動距離から水平速度を算出します (前回位置を更新)
//---------------------------------------------------------------------------
float Player::movementSpeed(float deltaTime)
{
    if (deltaTime <= 0.0f || m_speed <= 0.0f)
    {
        m_lastPosition = m_rootPosition;
        return 0.0f;
    }

    Vector3 delta = m_rootPosition - m_lastPosition;
    delta.y = 0.0f;

    m_lastPosition = m_rootPosition;

    return delta.Length() / deltaTime;
}

//---------------------------------------------------------------------------
//! 接地中のみジャンプ
//---------------------------------------------------------------------------
void Player::jump()
{
    if (m_isGrounded)
    {
        m_verticalVelocity = m_jumpForce;
        m_isGrounded = false;
    }
}

//---------------------------------------------------------------------------
//! 無敵タイマーを減算
//---------------------------------------------------------------------------
void Player::updateInvincibility(float deltaTime)
{
    if (m_invincibleTimer > 0.0f)
    {
        m_invincibleTimer -= deltaTime;
        if (m_invincibleTimer < 0.0f)
        {
            m_invincibleTimer = 0.0f;
        }
    }
}

//---------------------------------------------------------------------------
//! 重力による落下と着地判定
//---------------------------------------------------------------------------
void Player::updateVerticalMovement(float deltaTime)
{
    if (!m_isGrounded)
    {
        m_verticalVelocity -= m_gravity * deltaTime;
        m_rootPosition.y += m_verticalVelocity * deltaTime;

        if (m_rootPosition.y <= m_groundLevel)
        {
            m_rootPosition.y = m_groundLevel;
            m_verticalVelocity = 0.0f;
            m_isGrounded = true;
        }
    }
}

//---------------------------------------------------------------------------
//! 死亡・無敵中はコライダを積まない = その間は被弾しない (無敵時間の実装)
//---------------------------------------------------------------------------
void Player::collectHitColliders(std::vector<CombatHitCollider>& out)
{
    if (isDead() || isInvincible())
    {
        return;
    }

    CombatHitCollider c;
    c.target = this;
    c.part = HitPart::Body;
    c.bounds.Center = XMFLOAT3(
        m_rootPosition.x,
        m_rootPosition.y + 1.2f,
        m_rootPosition.z);
    c.bounds.Radius = 1.2f;
    c.damageMultiplier = 1.0f;
    out.push_back(c);
}

//---------------------------------------------------------------------------
//! 被弾を体力へ反映し PlayerDamagedEvent を発行
//---------------------------------------------------------------------------
void Player::onHit(const CombatHit& hit)
{
    if (isDead() || isInvincible())
    {
        return;
    }

    takeDamage(hit.finalDamage);
    EventBus::publish(PlayerDamagedEvent{
        m_rootPosition,
        hit.finalDamage,
        m_health,
        m_maxHealth });
}

//---------------------------------------------------------------------------
//! ダメージを適用し無敵時間を開始します
//---------------------------------------------------------------------------
void Player::takeDamage(float amount)
{
    if (isInvincible())
    {
        return;
    }

    m_health -= amount;
    if (m_health < 0.0f)
    {
        m_health = 0.0f;
    }

    m_invincibleTimer = INVINCIBLE_DURATION;
}

//---------------------------------------------------------------------------
//! 武器を解放します
//---------------------------------------------------------------------------
void Player::finalize()
{
    m_weapon->finalize();
}

//---------------------------------------------------------------------------
//! 目線カメラのワールド行列を構築 (LookAt ビューの逆行列)
//---------------------------------------------------------------------------
Matrix Player::createGameplayCameraWorldMatrix() const
{
    const Vector3 eyePos = eyePosition();
    const Vector3 forward = lookForward();
    const Vector3 right = lookRight();
    Vector3 up = forward.Cross(right);
    up.Normalize();

    const Matrix view = XMMatrixLookAtLH(eyePos, eyePos + forward, up);
    return view.Invert();
}

//---------------------------------------------------------------------------
//! ヨー・ピッチから視線方向を算出 (正のピッチ = 下向き)
//---------------------------------------------------------------------------
Vector3 Player::lookForward() const
{
    const float yawRad = XMConvertToRadians(m_lookYaw);
    const float pitchRad = XMConvertToRadians(m_lookPitch);
    Vector3 forward;
    forward.x = sinf(yawRad) * cosf(pitchRad);
    forward.y = -sinf(pitchRad);
    forward.z = cosf(yawRad) * cosf(pitchRad);
    forward.Normalize();
    return forward;
}

//---------------------------------------------------------------------------
//! 視線の右方向を算出 (worldUp と forward の外積 -> 常に水平)
//---------------------------------------------------------------------------
Vector3 Player::lookRight() const
{
    const Vector3 worldUp(0.0f, 1.0f, 0.0f);
    Vector3 right = worldUp.Cross(lookForward());
    right.Normalize();
    return right;
}

//---------------------------------------------------------------------------
//! リロード中かを取得します
//---------------------------------------------------------------------------
bool Player::isReloading() const
{
    return m_weapon->isReloading();
}

//---------------------------------------------------------------------------
//! 残弾数を取得します
//---------------------------------------------------------------------------
int Player::ammo() const
{
    return m_weapon->getAmmo();
}

//---------------------------------------------------------------------------
//! 装弾数を取得します
//---------------------------------------------------------------------------
int Player::maxAmmo() const
{
    return m_weapon->getMaxAmmo();
}

//---------------------------------------------------------------------------
//! 開始状態へ戻します (位置・体力・視点・武器)
//---------------------------------------------------------------------------
void Player::reset()
{
    m_rootPosition = Vector3(0.0f, 0.0f, -20.0f);
    m_health = m_maxHealth;
    m_invincibleTimer = 0.0f;
    m_isAiming = false;
    m_isGrounded = true;
    m_verticalVelocity = 0.0f;
    m_lookYaw = 0.0f;
    m_lookPitch = 0.0f;
    m_lastPosition = m_rootPosition;
    m_weapon->reset();
}
