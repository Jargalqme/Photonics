#pragma once

#include "Scenes/Scene.h"

#include "Gameplay/Player.h"
#include "Gameplay/Boss.h"
#include "Gameplay/BulletPool.h"
#include "Gameplay/ICombatTarget.h"
#include "Gameplay/CombatSystem.h"
#include "Render/Visuals/Grid.h"
#include "Render/Visuals/Skybox.h"
#include "Render/Lighting/IndirectLight.h"
#include "Render/Visuals/ArenaFloor.h"
#include "Render/Visuals/BulletRenderer.h"
#include "Render/Visuals/ParticleSystem.h"
#include "Render/Pipeline/RenderCommandQueue.h"
#include "Render/Lighting/SceneLighting.h"
#include "Render/Visuals/Tracers.h"
#include "Services/AudioManager.h"
#include "Services/BeatTracker.h"
#include "UI/GameUI.h"
#include "UI/DebugUI.h"
#include "Scenes/SceneManager.h"

#include <cstdint>
#include <memory>
#include <vector>

class PlayerCamera;
class ImportedModel;

class BossScene : public Scene
{
public:
    BossScene(SceneManager* sceneManager);
    ~BossScene() override;

    void initialize(SceneContext& context) override;
    void enter() override;
    void exit() override;
    void finalize() override;

    void update(float deltaTime, InputManager* input) override;
    void render() override;

private:
    static constexpr int   IMPACT_PARTICLE_COUNT   = 30;
    static constexpr float IMPACT_PARTICLE_SPEED   = 6.0f;
    static constexpr float IMPACT_PARTICLE_LIFE    = 0.4f;
    static constexpr float IMPACT_PARTICLE_SPREAD  = 0.8f;

    static constexpr int   DEATH_PARTICLE_COUNT    = 200;
    static constexpr float DEATH_PARTICLE_SPEED    = 8.0f;
    static constexpr float DEATH_PARTICLE_LIFE     = 1.5f;
    static constexpr float DEATH_PARTICLE_SPREAD   = 1.0f;

    static constexpr float KILL_SHAKE_INTENSITY       = 0.15f;
    static constexpr float KILL_SHAKE_DURATION        = 0.2f;
    static constexpr float PLAYER_HIT_SHAKE_INTENSITY = 0.35f;
    static constexpr float PLAYER_HIT_SHAKE_DURATION  = 0.15f;

    static constexpr int   BOSS_HIT_PARTICLE_COUNT  = 24;
    static constexpr float BOSS_HIT_PARTICLE_SPEED  = 6.0f;
    static constexpr float BOSS_HIT_PARTICLE_LIFE   = 0.4f;
    static constexpr float BOSS_HIT_PARTICLE_SPREAD = 0.8f;

    static constexpr int   BOSS_DEATH_PARTICLE_COUNT  = 150;
    static constexpr float BOSS_DEATH_PARTICLE_SPEED  = 12.0f;
    static constexpr float BOSS_DEATH_PARTICLE_LIFE   = 2.0f;
    static constexpr float BOSS_DEATH_PARTICLE_SPREAD = 1.0f;
    static constexpr float BOSS_DEATH_SHAKE_INTENSITY = 1.0f;
    static constexpr float BOSS_DEATH_SHAKE_DURATION  = 1.0f;

    SceneManager* m_sceneManager = nullptr;

    std::unique_ptr<PlayerCamera> m_camera;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<Boss>   m_boss;

    std::vector<ICombatTarget*> m_shotTargets;
    std::vector<ICombatTarget*> m_bulletTargets;
    BulletPool   m_bulletPool;
    CombatSystem m_combatSystem;

    std::unique_ptr<ParticleSystem> m_particleSystem;
    std::unique_ptr<BulletRenderer> m_bulletRenderer;
    std::unique_ptr<Tracers> m_tracers;
    RenderCommandQueue m_renderQueue;
    SceneLighting m_lighting;

    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<BeatTracker> m_beatTracker;

    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<ArenaFloor> m_arenaFloor;
    std::unique_ptr<Skybox> m_skybox;
    std::unique_ptr<IndirectLight> m_indirectLight;

#ifdef _DEBUG
    const ImportedModel* m_pbrSmokeModel = nullptr;
#endif

    std::unique_ptr<GameUI> m_gameUI;
    std::unique_ptr<DebugUI> m_debugUI;
    bool m_debugMode = false;

    float m_hitstopTimer = 0.0f;

    void onBeat(int beat);

    void renderWorld(const Matrix& view, const Matrix& proj, const Vector3& camPos);
    void renderEffects(const Matrix& view, const Matrix& proj, const Vector3& camPos);
    void renderViewmodel(const Matrix& view, const Vector3& camPos);
    void renderUI(const Matrix& view, const Matrix& proj);
};
