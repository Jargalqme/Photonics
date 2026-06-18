#include "pch.h"
#include "Gameplay/Weapon/PlayerWeapon.h"
#include "Gameplay/Weapon/WeaponShot.h"
#include "Gameplay/Weapon/Weapon.h"
#include "Gameplay/Weapon/WeaponRifle.h"
#include "Render/Pipeline/RenderCommandQueue.h"
#include "Services/SceneContext.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

PlayerWeapon::PlayerWeapon(SceneContext& context)
    : m_context(&context)
    , m_combat(std::make_unique<WeaponRifle>())
{
}

PlayerWeapon::~PlayerWeapon() = default;

void PlayerWeapon::initialize()
{
    m_model.loadRifle(*m_context, RIFLE_MODEL_PATH);
    m_combat->initialize();
}

void PlayerWeapon::finalize()
{
    m_model.finalize();
}

void PlayerWeapon::reset()
{
    m_motion.reset();
    m_combat->initialize();
}

void PlayerWeapon::clearInputState()
{
    m_combat->stopFire();
}

void PlayerWeapon::update(const PlayerWeaponFrame& frame, std::vector<WeaponShot>& outShots)
{
    applyInputToCombat(frame);
    m_motion.update(buildMotionInput(frame));

    const Vector3 tracerStart = getMuzzlePosition(frame.cameraWorld);
    if (m_combat->update(
        frame.deltaTime,
        frame.hitScanOrigin,
        frame.hitScanDirection,
        tracerStart,
        outShots))
    {
        m_motion.onFire();
    }
}

void PlayerWeapon::render(RenderCommandQueue& queue, const Matrix& view) const
{
    const Matrix cameraWorld = view.Invert();
    m_model.submit(queue, buildModelRootWorld(cameraWorld));
}

bool PlayerWeapon::isReloading() const
{
    return m_combat->isReloading();
}

int PlayerWeapon::getAmmo() const
{
    return m_combat->getAmmoCount();
}

int PlayerWeapon::getMaxAmmo() const
{
    return m_combat->getClipSize();
}

void PlayerWeapon::applyInputToCombat(const PlayerWeaponFrame& frame)
{
    if (frame.fireHeld)
    {
        m_combat->startFire();
    }
    else
    {
        m_combat->stopFire();
    }

    if (frame.reloadPressed)
    {
        m_combat->reload();
    }
}

WeaponMotionInput PlayerWeapon::buildMotionInput(const PlayerWeaponFrame& frame) const
{
    WeaponMotionInput input;
    input.deltaTime = frame.deltaTime;
    input.lookDeltaDegrees = frame.viewDeltaDegrees;
    input.moveSpeed = frame.moveSpeed;
    input.grounded = frame.isGrounded;
    input.isAiming = frame.adsHeld;
    return input;
}

Matrix PlayerWeapon::buildModelRootWorld(const Matrix& cameraWorld) const
{
    const WeaponMotionOutput animation = m_motion.getMotionOutput();
    const Vector3 rotationRadians(
        XMConvertToRadians(animation.rotation.x),
        XMConvertToRadians(animation.rotation.y),
        XMConvertToRadians(animation.rotation.z));

    const Matrix animationRotation = Matrix::CreateFromYawPitchRoll(
        rotationRadians.y,
        rotationRadians.x,
        rotationRadians.z);

    return animationRotation
        * Matrix::CreateTranslation(animation.position)
        * cameraWorld;
}

Vector3 PlayerWeapon::getMuzzlePosition(const Matrix& cameraWorld) const
{
    const Matrix rootWorld = buildModelRootWorld(cameraWorld);
    const Matrix modelWorld = m_model.buildModelWorldMatrix(rootWorld);
    return Vector3::Transform(m_model.getMuzzleLocalPosition(), modelWorld);
}
