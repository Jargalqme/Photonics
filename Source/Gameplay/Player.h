#pragma once

#include <DirectXCollision.h>
#include <memory>
#include <vector>

#include "Gameplay/ICombatTarget.h"

struct SceneContext;
struct WeaponShot;
class  InputManager;
class  PlayerWeapon;

class Player : public ICombatTarget
{
public:
    explicit Player(SceneContext& context);
    ~Player();

    void initialize();
    void finalize();
    void reset();
    void clearInputState();

    void update(InputManager& input, float deltaTime, std::vector<WeaponShot>& outShots);

    PlayerWeapon& weapon();
    const PlayerWeapon& weapon() const;

    float   lookYaw()     const noexcept { return m_lookYaw;   }
    float   lookPitch()   const noexcept { return m_lookPitch; }
    Vector3 lookForward() const;
    Vector3 lookRight()   const;

    Vector3  rootPosition() const noexcept { return m_rootPosition; }
    Vector3  eyeOffset()    const noexcept { return m_eyeOffset; }
    Vector3  eyePosition()  const noexcept { return m_rootPosition + m_eyeOffset; }
    Vector3* rootPositionPtr()    noexcept { return &m_rootPosition; }

    void setRootPosition(const Vector3& position) noexcept { m_rootPosition = position; }
    void setEyeOffset   (const Vector3& offset)   noexcept { m_eyeOffset = offset; }

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
    float* mouseSensitivityPtr()   noexcept { return &m_mouseSensitivity; }

    void setHealth(float health) noexcept { m_health = health; }

    void collectHitColliders(std::vector<CombatHitCollider>& out) override;
    void onHit(const CombatHit& hit) override;

private:

    static constexpr float INVINCIBLE_DURATION = 0.5f;
    static constexpr float MOVE_THRESHOLD      = 0.001f;
    static constexpr float ARENA_HALF_SIZE     = 500.0f;
    static constexpr float LOOK_PITCH_CLAMP    = 89.0f;
    // TODO: dead constant need to wire it cleanly.
    static constexpr float PLAYER_HIT_RADIUS   = 0.8f;

    void setAiming(bool aiming) noexcept { m_isAiming = aiming; }
    void takeDamage(float amount);

    Vector2 applyLookDelta(float deltaYawDegrees, float deltaPitchDegrees);
    void updateMovement(const Vector3& direction, float deltaTime);
    void updateVerticalMovement(float deltaTime);
    void updateInvincibility(float deltaTime);
    void jump();

    float movementSpeed(float deltaTime);
    Matrix createGameplayCameraWorldMatrix() const;

    std::unique_ptr<PlayerWeapon> m_weapon;

    Vector3 m_rootPosition { 0.0f, 0.0f, 0.0f };
    Vector3 m_eyeOffset    { 0.0f, 6.3f, 0.0f };
    Vector3 m_lastPosition { 0.0f, 0.0f, 0.0f };

    float m_speed            = 25.0f;
    float m_lookYaw          = 0.0f;
    float m_lookPitch        = 0.0f;
    float m_mouseSensitivity = 0.05f;

    float m_health           = 100.0f;
    float m_maxHealth        = 100.0f;
    float m_invincibleTimer  = 0.0f;

    bool m_isGrounded        = true;
    bool m_isAiming          = false;
    float m_verticalVelocity = 0.0f;
    float m_jumpForce        = 15.0f;
    float m_gravity          = 40.0f;
    float m_groundLevel      = 0.0f;
};
