#pragma once

#include <memory>
#include <SimpleMath.h>
#include <vector>

#include "Gameplay/Weapon/WeaponMotion.h"
#include "Gameplay/Weapon/WeaponModel.h"

class RenderCommandQueue;
class Weapon;
struct SceneContext;
struct WeaponShot;

struct PlayerWeaponFrame
{
    float deltaTime = 0.0f;
    DirectX::SimpleMath::Vector3 hitScanOrigin = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector3 hitScanDirection = DirectX::SimpleMath::Vector3::UnitZ;
    DirectX::SimpleMath::Vector2 viewDeltaDegrees = DirectX::SimpleMath::Vector2::Zero;
    float moveSpeed = 0.0f;
    bool fireHeld = false;
    bool adsHeld = false;
    bool reloadPressed = false;
    bool isGrounded = true;
    DirectX::SimpleMath::Matrix cameraWorld = DirectX::SimpleMath::Matrix::Identity;
};

class PlayerWeapon
{
public:
    explicit PlayerWeapon(SceneContext& context);
    ~PlayerWeapon();

    void initialize();
    void finalize();
    void reset();
    void clearInputState();

    void update(const PlayerWeaponFrame& frame, std::vector<WeaponShot>& outShots);
    void render(RenderCommandQueue& queue, const DirectX::SimpleMath::Matrix& view) const;

    bool isReloading() const;
    int getAmmo() const;
    int getMaxAmmo() const;

    WeaponMotionTuning* getMotionTuning() { return m_motion.getMotionTuningPtr(); }
    bool hasRifleModel() const { return m_model.hasRifle(); }
    RifleModelSettings& getRifleModelSettings() { return m_model.rifleSettings(); }
    const RifleModelSettings& getRifleModelSettings() const { return m_model.rifleSettings(); }
    void resetRifleModelSettings() { m_model.resetRifleSettings(); }

private:
    static constexpr const char* RIFLE_MODEL_PATH = "Assets/Weapons/Rifle/rifle.glb";

    void applyInputToCombat(const PlayerWeaponFrame& frame);
    WeaponMotionInput buildMotionInput(const PlayerWeaponFrame& frame) const;
    DirectX::SimpleMath::Matrix buildModelRootWorld(
        const DirectX::SimpleMath::Matrix& cameraWorld) const;
    DirectX::SimpleMath::Vector3 getMuzzlePosition(
        const DirectX::SimpleMath::Matrix& cameraWorld) const;

    SceneContext* m_context = nullptr;
    std::unique_ptr<Weapon> m_combat;
    WeaponMotion m_motion;
    WeaponModel m_model;
};
