#include "pch.h"
#include "Scenes/BossScene.h"
#include "Gameplay/PlayerCamera.h"
#include "DeviceResources.h"
#include "Gameplay/EventBus.h"
#include "Services/InputManager.h"
#include "Render/Pipeline/Renderer.h"
#include "Render/Pipeline/Bloom.h"
#include "Render/Pipeline/SceneRenderer.h"
#include "Render/Assets/MeshCache.h"
#include "Gameplay/Weapon/WeaponShot.h"
#include "Gameplay/EventTypes.h"
#include "Gameplay/Weapon/PlayerWeapon.h"

namespace
{
    constexpr const char* RIFLE_FIRE_GROUP = "rifle_fire";
    constexpr float RIFLE_FIRE_VOLUME = 0.35f;
    constexpr float RIFLE_FIRE_PITCH_JITTER = 0.015f;

    void loadRifleFireAudio(AudioManager& audio)
    {
        if (audio.loadSoundGroupFromDirectory(
            RIFLE_FIRE_GROUP,
            GetAssetPath(L"Audio/Weapons"),
            L"rifle_fire_"))
        {
            return;
        }

        constexpr const char* fallbackName = "rifle_fire_fallback";
        if (audio.loadSound(fallbackName, GetAssetPath(L"Audio/shoot_sfx.wav")))
        {
            audio.addSoundToGroup(RIFLE_FIRE_GROUP, fallbackName);
        }
    }
}

BossScene::BossScene(SceneManager* sceneManager)
    : Scene("BossScene")
    , m_sceneManager(sceneManager)
{
}

BossScene::~BossScene()
{
}

// === 初期化 ===

void BossScene::initialize(SceneContext& context)
{
    Scene::initialize(context);

    m_lighting.keyLight.directionToLight = Vector3(0.35f, 0.85f, -0.35f);
    m_lighting.keyLight.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    m_lighting.keyLight.intensity = 5.0f;

    // カメラ — アスペクト比はバックバッファではなくレンダー解像度から取る
    // （レンダーが 16:9、ウィンドウが 21:9 のとき投影は 16:9 のまま、合成側でレターボックス）
    m_camera = std::make_unique<PlayerCamera>(m_deviceResources);
    float aspectRatio = float(m_renderer->GetRenderWidth()) / float(m_renderer->GetRenderHeight());
    m_camera->setProjection(45.0f, aspectRatio, 0.1f, 1000.0f);
    m_camera->createDeviceDependentResources();

    // プレイヤー
    m_player = std::make_unique<Player>(*m_context);
    m_player->initialize();

    // VFX
    m_particleSystem = std::make_unique<ParticleSystem>(m_deviceResources);
    m_particleSystem->initialize();

    // 弾は低テッセレーションの球メッシュで描く（MeshCache 所有・借用ポインタ）
    m_bulletMesh = m_context->meshes->getSphere(8);

    m_tracers = std::make_unique<Tracers>(*m_context);
    m_tracers->initialize();

    // 敵
    m_boss = std::make_unique<Boss>(*m_context);
    m_boss->initialize();
    m_boss->setParticles(m_particleSystem.get());
    m_boss->setCamera(m_camera.get());

    // オーディオ
    m_audioManager = std::make_unique<AudioManager>();
    m_audioManager->initialize();
    m_audioManager->loadSound("boost", GetAssetPath(L"Audio/boost_sfx.wav").c_str());
    loadRifleFireAudio(*m_audioManager);
    m_audioManager->loadSound("death", GetAssetPath(L"Audio/death_sfx.wav").c_str());
    m_audioManager->loadMusic("Gameplay", GetAssetPath(L"Audio/sample.wav").c_str());

    m_beatTracker = std::make_unique<BeatTracker>();
    m_beatTracker->setBPM(109.0f);
    m_beatTracker->setStartDelay(15.931f);
    m_beatTracker->setSongDuration(115.0f);
    m_beatTracker->setBeatCallback([this](int beat) {
        onBeat(beat);
    });

    // ワールド
    m_grid = std::make_unique<Grid>(*m_context);
    m_grid->initialize();

    m_arenaFloor = std::make_unique<ArenaFloor>(*m_context);
    m_arenaFloor->initialize();

    m_arenaFloor->setBrightness(1.0f);
    m_arenaFloor->setAlpha(1.0f);
    m_arenaFloor->setTransform(
        Vector3(0.0f, 0.0f, 420.0f),
        Vector3::Zero,
        Vector3::One);

    m_skybox = std::make_unique<Skybox>(m_deviceResources);
    m_skybox->initialize();

    m_indirectLight = std::make_unique<IndirectLight>(m_deviceResources);
    m_indirectLight->initialize(m_skybox->cubeSRV(), m_context->commonStates->LinearClamp());

    // UI
    m_gameUI = std::make_unique<GameUI>();
    m_gameUI->initialize(m_deviceResources);  // m_deviceResources は Scene::initialize で設定済み
    m_gameUI->setBeatTracker(m_beatTracker.get());
    m_gameUI->setPlayer(m_player.get());
    m_gameUI->setBoss(m_boss.get());

    m_debugUI = std::make_unique<DebugUI>();
    m_debugUI->setCamera(m_camera.get());
    m_debugUI->setLightCycle(m_player.get());
    m_debugUI->setGrid(m_grid.get());
    m_debugUI->setSkybox(m_skybox.get());
    m_debugUI->setAudioManager(m_audioManager.get());
    m_debugUI->setBeatTracker(m_beatTracker.get());
    m_debugUI->setBulletPool(&m_bulletPool);
    m_debugUI->setBoss(m_boss.get());
    m_debugUI->setBloom(m_renderer->GetSceneRenderer()->getBloom());
    m_debugUI->setExposurePtr(m_camera->exposurePtr());
    m_debugUI->setSceneLighting(&m_lighting);
}

// === シーン遷移 ===

void BossScene::enter()
{
    Scene::enter();
    EventBus::clear();
    m_debugMode = false;
    if (m_context && m_context->input)
    {
        m_context->input->setCursorVisible(false);
    }

    // UI が自前で購読する（clear 後に再登録する必要があるためここで呼ぶ）
    m_gameUI->subscribeEvents();

    // 敵ヒット時 → 衝撃パーティクル
    EventBus::subscribe<DummyHitEvent>([this](const DummyHitEvent& event) {
        m_particleSystem->emit(event.position, Vector4(1.0f, 0.8f, 0.2f, 1.0f),
            IMPACT_PARTICLE_COUNT, IMPACT_PARTICLE_SPEED,
            IMPACT_PARTICLE_LIFE, IMPACT_PARTICLE_SPREAD);
    });

    EventBus::subscribe<WeaponShotEvent>([this](const WeaponShotEvent&) {
        m_audioManager->playRandomSound(RIFLE_FIRE_GROUP, RIFLE_FIRE_VOLUME, RIFLE_FIRE_PITCH_JITTER);
    });

    // ヒットスキャン解決時 → トレーサー描画
    EventBus::subscribe<ShotResolvedEvent>([this](const ShotResolvedEvent& event) {
        m_tracers->spawn(event.tracerStart, event.hitPoint, event.color);
    });

    // 敵撃破時 → 爆発パーティクル + シェイク + 効果音
    EventBus::subscribe<DummyDiedEvent>([this](const DummyDiedEvent& event) {
        m_particleSystem->emit(event.position, Vector4(1.0f, 0.3f, 0.1f, 1.0f),
            DEATH_PARTICLE_COUNT, DEATH_PARTICLE_SPEED,
            DEATH_PARTICLE_LIFE, DEATH_PARTICLE_SPREAD);
        m_camera->triggerShake(KILL_SHAKE_INTENSITY, KILL_SHAKE_DURATION);
        m_audioManager->playSound("death", 0.2f);
    });

    EventBus::subscribe<PlayerDamagedEvent>([this](const PlayerDamagedEvent&) {
        m_camera->triggerShake(PLAYER_HIT_SHAKE_INTENSITY, PLAYER_HIT_SHAKE_DURATION);
    });

    EventBus::subscribe<BossDamagedEvent>([this](const BossDamagedEvent& event) {
        m_particleSystem->emit(event.position, Vector4(0.8f, 0.0f, 1.0f, 1.0f),
            BOSS_HIT_PARTICLE_COUNT, BOSS_HIT_PARTICLE_SPEED,
            BOSS_HIT_PARTICLE_LIFE, BOSS_HIT_PARTICLE_SPREAD);
    });

    EventBus::subscribe<BossDiedEvent>([this](const BossDiedEvent& event) {
        m_particleSystem->emit(event.position, Vector4(0.8f, 0.0f, 1.0f, 1.0f),
            BOSS_DEATH_PARTICLE_COUNT, BOSS_DEATH_PARTICLE_SPEED,
            BOSS_DEATH_PARTICLE_LIFE, BOSS_DEATH_PARTICLE_SPREAD);
        m_camera->triggerShake(BOSS_DEATH_SHAKE_INTENSITY, BOSS_DEATH_SHAKE_DURATION);
        m_audioManager->playSound("death", 0.3f);
    });

    // ゲーム状態リセット
    m_player->reset();

    m_boss->deactivate();

    m_bulletPool.deactivateAll();

    // Boss starts directly; Stage 4 replaces this with the intro FSM.
    m_boss->setPlayerTarget(m_player->rootPositionPtr());
    m_boss->setBulletPool(&m_bulletPool);
    m_boss->activate();
    EventBus::publish(WaveChangedEvent{ 3 });

    // ヒットスキャンターゲット構築 — Boss のみ（敵弾は Player へ直接解決）
    m_shotTargets.clear();
    m_shotTargets.push_back(m_boss.get());

    // オーディオリセット
    m_beatTracker->reset();
    m_audioManager->playMusic("Gameplay");

    // ブルーム有効化（exit() で disable されるため毎エントリで再有効化）
    m_renderer->GetSceneRenderer()->getBloom()->setEnabled(true);

    m_renderer->GetSceneRenderer()->setActiveCamera(m_camera.get());
}

void BossScene::exit()
{
    Scene::exit();
    EventBus::clear();
    m_audioManager->stopMusic();

    if (m_renderer)
    {
        m_renderer->GetSceneRenderer()->getBloom()->setEnabled(false);
        m_renderer->GetSceneRenderer()->setActiveCamera(nullptr);
    }
}

void BossScene::finalize()
{
    // GPU リソース解放
    if (m_grid)           { m_grid->finalize(); }
    if (m_arenaFloor)     { m_arenaFloor->finalize(); }
    if (m_skybox)         { m_skybox->finalize(); }
    if (m_indirectLight)  { m_indirectLight->finalize(); }
    if (m_player)         { m_player->finalize(); }
    if (m_boss)           { m_boss->finalize(); }
    if (m_particleSystem) { m_particleSystem->finalize(); }
    if (m_tracers)        { m_tracers->finalize(); }
    m_camera->finalize();

    // 借用ポインタを nullptr 化（実体は MeshCache が所有）
    m_bulletMesh = nullptr;

    // オブジェクト破棄
    m_particleSystem.reset();
    m_tracers.reset();
    m_camera.reset();
    m_player.reset();
    m_boss.reset();
    m_grid.reset();
    m_arenaFloor.reset();
    m_skybox.reset();
    m_indirectLight.reset();
    m_beatTracker.reset();
    m_audioManager.reset();
    m_debugUI.reset();
}

// === 更新 ===

void BossScene::update(float deltaTime, InputManager* input)
{
    // === 入力 ===
    if (input->isKeyPressed(Keyboard::Keys::Escape))
    {
        PostQuitMessage(0);
    }

    bool debugToggledThisFrame = false;
    if (input->isKeyPressed(Keyboard::Keys::F3))
    {
        debugToggledThisFrame = true;
        m_debugMode = !m_debugMode;
        if (m_debugMode)
        {
            input->setCursorVisible(true);
            m_player->clearInputState();
        }
        else
        {
            input->setCursorVisible(false);
        }
    }

    m_debugUI->setInputManager(input);

    // === ゲームプレイ系統（正準順: Player → Boss → Combat → Camera）===
    const bool victoryReached = m_boss->isActivated() && m_boss->isDead();

    // 1フレーム内の射撃インテントは Player 側で積み、Combat 側で解決して空にする
    std::vector<WeaponShot> weaponShots;
    if (!victoryReached)
    {
        if (!m_debugMode && !debugToggledThisFrame)
        {
            m_player->update(*input, deltaTime, weaponShots);
        }

        if (m_boss->isActivated())
        {
            m_boss->update(deltaTime);
        }
        m_bulletPool.update(deltaTime);
        m_combatSystem.update(m_shotTargets, weaponShots);
        m_combatSystem.resolveBullets(m_bulletPool, *m_player);
    }

    EventBus::dispatchQueued();

    if (victoryReached)
    {
        m_sceneManager->transitionTo("Victory");
        return;
    }

    if (m_player->isDead())
    {
        m_sceneManager->transitionTo("GameOver");
        return;
    }

    m_camera->update(*m_player, deltaTime);
    m_camera->updateConstants();

    // === サブシステム（UI / オーディオ / VFX / ワールド）===
    m_gameUI->update(deltaTime);
    m_audioManager->update();
    m_particleSystem->update(deltaTime);
    m_tracers->update(deltaTime);
    m_beatTracker->update(deltaTime);
    m_grid->setBeatPulse(m_beatTracker->getBeatProgress());
    m_grid->update();
    m_arenaFloor->update(deltaTime);
}

// === ヘルパー ===

void BossScene::onBeat(int beat)
{
    m_gameUI->triggerBeatFlash(beat);
}

// === 描画 ===

void BossScene::render()
{
    const auto view   = m_camera->matView();
    const auto proj   = m_camera->matProj();
    const auto camPos = m_camera->position();

    m_renderer->SetIrradianceSRV(m_indirectLight->irradianceSRV());

    renderWorld(view, proj, camPos);      // グリッド + ボスメッシュ
    renderEffects(view, proj, camPos);    // 弾・パーティクル・トレーサー
    renderViewmodel(view, camPos);        // 深度クリア + 武器ビューモデル
    renderUI(view, proj);                 // GameUI + デバッグ
}

// === レンダーパス ===

void BossScene::renderWorld(const Matrix& view, const Matrix& proj, const Vector3& camPos)
{
    m_skybox->render();
    m_arenaFloor->render(view, proj);
    m_grid->render(view, proj);

    m_renderQueue.clear();
    m_boss->submitRender(m_renderQueue);
    m_renderer->ExecuteRenderCommands(m_renderQueue, view, proj, camPos, m_lighting);
}

void BossScene::renderEffects(const Matrix& view, const Matrix& proj, const Vector3& camPos)
{
    m_renderQueue.clear();
    m_bulletPool.submitRender(m_renderQueue, m_bulletMesh);
    m_renderer->ExecuteRenderCommands(m_renderQueue, view, proj, camPos, m_lighting);

    m_particleSystem->render(view, proj);
    m_tracers->render(view, proj, camPos);
}

void BossScene::renderViewmodel(const Matrix& view, const Vector3& camPos)
{
    m_renderer->BeginViewmodelPass();   // 深度クリアして常に最前面に描画

    m_renderQueue.clear();
    m_player->weapon().render(m_renderQueue, view);
    m_renderer->ExecuteRenderCommands(m_renderQueue, view, m_camera->matViewmodelProj(), camPos, m_lighting);
}

void BossScene::renderUI(const Matrix& view, const Matrix& proj)
{
    m_gameUI->render(view, proj);
    if (m_debugMode && m_debugUI)
    {
        m_debugUI->render();
    }
}
