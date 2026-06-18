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

Player::Player(SceneContext& context)
    : m_weapon(std::make_unique<PlayerWeapon>(context))
{
}

Player::~Player() = default;

void Player::initialize()
{
    m_weapon->initialize();
}

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

void Player::clearInputState()
{
    m_weapon->clearInputState();
    setAiming(false);
}

PlayerWeapon& Player::weapon()
{
    return *m_weapon;
}

const PlayerWeapon& Player::weapon() const
{
    return *m_weapon;
}

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

void Player::jump()
{
    if (m_isGrounded)
    {
        m_verticalVelocity = m_jumpForce;
        m_isGrounded = false;
    }
}

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

void Player::finalize()
{
    m_weapon->finalize();
}

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

Vector3 Player::lookRight() const
{
    const Vector3 worldUp(0.0f, 1.0f, 0.0f);
    Vector3 right = worldUp.Cross(lookForward());
    right.Normalize();
    return right;
}

bool Player::isReloading() const
{
    return m_weapon->isReloading();
}

int Player::ammo() const
{
    return m_weapon->getAmmo();
}

int Player::maxAmmo() const
{
    return m_weapon->getMaxAmmo();
}

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
