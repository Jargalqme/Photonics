#include "pch.h"
#include "Scenes/TrainingScene.h"
#include "Gameplay/PlayerCamera.h"
#include "DeviceResources.h"
#include "Gameplay/Weapon/WeaponShot.h"
#include "Gameplay/EventBus.h"
#include "Gameplay/EventTypes.h"
#include "Gameplay/Weapon/PlayerWeapon.h"
#include "Render/Pipeline/Renderer.h"
#include "Render/Pipeline/Bloom.h"
#include "Render/Pipeline/SceneRenderer.h"
#include "Services/InputManager.h"
#include "Render/Assets/ImportedModelCache.h"
#include <imgui.h>
#include <filesystem>
#include <limits>

namespace
{
    constexpr const char* RIFLE_FIRE_GROUP = "rifle_fire";
    constexpr float RIFLE_FIRE_VOLUME = 0.35f;
    constexpr float RIFLE_FIRE_PITCH_JITTER = 0.015f;
    constexpr const wchar_t* ENVIRONMENT_LAYOUT_PATH = L"Environment/environment.json";

#ifdef _DEBUG
    constexpr bool SHOW_IMPORTED_RIFLE_WORLD_PREVIEW = false;
    constexpr const char* IMPORTED_RIFLE_PREVIEW_PATH = "Assets/Weapons/Rifle/rifle.glb";
    constexpr float IMPORTED_RIFLE_PREVIEW_TARGET_LENGTH = 2.5f;
    const Vector3 IMPORTED_RIFLE_PREVIEW_POSITION(0.0f, 1.2f, 6.0f);

    Matrix makeImportedRiflePreviewWorld(const ImportedModel& model)
    {
        const auto& vertices = model.data().vertices;
        if (vertices.empty())
        {
            return Matrix::CreateTranslation(IMPORTED_RIFLE_PREVIEW_POSITION);
        }

        const float maxFloat = std::numeric_limits<float>::max();
        Vector3 minBounds(maxFloat, maxFloat, maxFloat);
        Vector3 maxBounds(-maxFloat, -maxFloat, -maxFloat);

        for (const auto& vertex : vertices)
        {
            minBounds.x = std::min(minBounds.x, vertex.position.x);
            minBounds.y = std::min(minBounds.y, vertex.position.y);
            minBounds.z = std::min(minBounds.z, vertex.position.z);
            maxBounds.x = std::max(maxBounds.x, vertex.position.x);
            maxBounds.y = std::max(maxBounds.y, vertex.position.y);
            maxBounds.z = std::max(maxBounds.z, vertex.position.z);
        }

        const Vector3 size = maxBounds - minBounds;
        const float longestSide = std::max({ size.x, size.y, size.z });
        const float scale = longestSide > 0.001f
            ? IMPORTED_RIFLE_PREVIEW_TARGET_LENGTH / longestSide
            : 1.0f;
        const Vector3 center = (minBounds + maxBounds) * 0.5f;

        return Matrix::CreateTranslation(-center)
            * Matrix::CreateScale(scale)
            * Matrix::CreateRotationY(XMConvertToRadians(180.0f))
            * Matrix::CreateTranslation(IMPORTED_RIFLE_PREVIEW_POSITION);
    }
#endif

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

    std::string getAssetPathString(const wchar_t* relativePath)
    {
        return std::filesystem::path(GetAssetPath(relativePath)).string();
    }
}

TrainingScene::TrainingScene(SceneManager* sceneManager)
    : Scene("Training")
    , m_sceneManager(sceneManager)
{
}

TrainingScene::~TrainingScene() = default;

// === 初期化 ===

void TrainingScene::initialize(SceneContext& context)
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

    m_audioManager = std::make_unique<AudioManager>();
    m_audioManager->initialize();
    loadRifleFireAudio(*m_audioManager);

    // VFX
    m_particleSystem = std::make_unique<ParticleSystem>(m_deviceResources);
    m_particleSystem->initialize();

    m_bulletRenderer = std::make_unique<BulletRenderer>(m_deviceResources);
    m_bulletRenderer->initialize();

    m_tracers = std::make_unique<Tracers>(*m_context);
    m_tracers->initialize();

    // ワールド
    m_grid = std::make_unique<Grid>(*m_context);
    m_grid->initialize();
    LayoutLoader::loadLayout(
        *m_context,
        getAssetPathString(ENVIRONMENT_LAYOUT_PATH),
        m_environmentLayout);

    // ダミー
    m_dummy = std::make_unique<Dummy>(*m_context);
    m_dummy->initialize();

    // UI
    m_gameUI = std::make_unique<GameUI>();
    m_gameUI->initialize(m_deviceResources);
    m_gameUI->setPlayer(m_player.get());
    m_gameUI->setDummy(m_dummy.get());
    m_gameUI->setShowWaveIndicator(false);

    m_debugUI = std::make_unique<DebugUI>();
    m_debugUI->setCamera(m_camera.get());
    m_debugUI->setLightCycle(m_player.get());
    m_debugUI->setGrid(m_grid.get());
    m_debugUI->setBulletPool(&m_bulletPool);
    m_debugUI->setBloom(m_renderer->GetSceneRenderer()->getBloom());
    m_debugUI->setExposurePtr(m_camera->exposurePtr());
    m_debugUI->setAudioManager(m_audioManager.get());
    m_debugUI->setSceneLighting(&m_lighting);
}

// === シーン遷移 ===

void TrainingScene::enter()
{
    Scene::enter();
    EventBus::clear();
    m_debugMode = false;
    if (m_context && m_context->input)
    {
        m_context->input->setCursorVisible(false);
    }

    m_gameUI->subscribeEvents();

    // ダミー命中 → 衝撃パーティクル
    EventBus::subscribe<DummyHitEvent>([this](const DummyHitEvent& event) {
        m_particleSystem->emit(event.position, Vector4(1.0f, 0.8f, 0.2f, 1.0f),
            IMPACT_PARTICLE_COUNT, IMPACT_PARTICLE_SPEED,
            IMPACT_PARTICLE_LIFE, IMPACT_PARTICLE_SPREAD);
    });

    // Weapon shot audio.
    EventBus::subscribe<WeaponShotEvent>([this](const WeaponShotEvent&) {
        m_audioManager->playRandomSound(RIFLE_FIRE_GROUP, RIFLE_FIRE_VOLUME, RIFLE_FIRE_PITCH_JITTER);
    });

    // ヒットスキャン解決 → トレーサー
    EventBus::subscribe<ShotResolvedEvent>([this](const ShotResolvedEvent& event) {
        m_tracers->spawn(event.tracerStart, event.hitPoint, event.color);
    });

    // ダミー撃破 → 爆発パーティクル + シェイク
    EventBus::subscribe<DummyDiedEvent>([this](const DummyDiedEvent& event) {
        m_particleSystem->emit(event.position, Vector4(1.0f, 0.3f, 0.1f, 1.0f),
            DEATH_PARTICLE_COUNT, DEATH_PARTICLE_SPEED,
            DEATH_PARTICLE_LIFE, DEATH_PARTICLE_SPREAD);
        m_camera->triggerShake(KILL_SHAKE_INTENSITY, KILL_SHAKE_DURATION);
    });

    // プレイヤーリセット
    m_player->reset();

    // 弾プールを念のためクリア（前シーン残留分対策）
    for (int i = 0; i < m_bulletPool.getMaxBullets(); i++)
    {
        m_bulletPool.getBullets()[i].deactivate();
    }

    // ダミーをスポーン
    m_dummy->spawn(Vector3(0.0f, 0.0f, DUMMY_SPAWN_Z));

    // 戦闘ターゲット = ダミーのみ。Player を含めない = 永久無敵。
    m_shotTargets.clear();
    m_shotTargets.push_back(m_dummy.get());
    m_bulletTargets.clear();

    // ブルーム有効化（exit() で disable されるため毎エントリで再有効化）
    m_renderer->GetSceneRenderer()->getBloom()->setEnabled(true);

    m_renderer->GetSceneRenderer()->setActiveCamera(m_camera.get());
}

void TrainingScene::exit()
{
    Scene::exit();
    EventBus::clear();

    if (m_renderer)
    {
        m_renderer->GetSceneRenderer()->getBloom()->setEnabled(false);
        m_renderer->GetSceneRenderer()->setActiveCamera(nullptr);
    }
}

void TrainingScene::finalize()
{
    if (m_grid) { m_grid->finalize(); }
    if (m_player) { m_player->finalize(); }
    if (m_particleSystem) { m_particleSystem->finalize(); }
    if (m_bulletRenderer) { m_bulletRenderer->finalize(); }
    if (m_tracers) { m_tracers->finalize(); }
    if (m_dummy) { m_dummy->finalize(); }
    m_camera->finalize();

    m_dummy.reset();
    m_particleSystem.reset();
    m_bulletRenderer.reset();
    m_tracers.reset();
    m_camera.reset();
    m_player.reset();
    m_grid.reset();
    m_audioManager.reset();
    m_debugUI.reset();
}

// === 更新 ===

void TrainingScene::update(float deltaTime, InputManager* input)
{
    // === 入力 ===
    if (input->isKeyPressed(Keyboard::Keys::Escape))
    {
        m_sceneManager->transitionTo("MainMenu");
        return;
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

    // === ゲームプレイ系統（正準順: Player → Dummy → Combat → Camera）===
    std::vector<WeaponShot> weaponShots;
    if (!m_debugMode && !debugToggledThisFrame)
    {
        m_player->update(*input, deltaTime, weaponShots);
    }

    m_dummy->update(deltaTime);

    m_combatSystem.update(deltaTime, m_shotTargets, m_bulletTargets, m_bulletPool, weaponShots);

    EventBus::dispatchQueued();

    m_camera->update(*m_player, deltaTime);
    m_camera->updateConstants();

    // === サブシステム ===
    m_gameUI->update(deltaTime);
    m_audioManager->update();
    m_particleSystem->update(deltaTime);
    m_bulletRenderer->update(deltaTime);
    m_tracers->update(deltaTime);
    m_grid->update();
}

// === 描画 ===

void TrainingScene::render()
{
    const auto view   = m_camera->matView();
    const auto proj   = m_camera->matProj();
    const auto camPos = m_camera->position();

    renderWorld(view, proj, camPos);      // グリッド + ダミーメッシュ
    renderEffects(view, proj, camPos);    // 弾・パーティクル・トレーサー
    renderViewmodel(view, camPos);        // 深度クリア + 武器ビューモデル
    renderUI(view, proj);                 // GameUI + 訓練パネル + デバッグ
}

// === レンダーパス ===

void TrainingScene::renderWorld(const Matrix& view, const Matrix& proj, const Vector3& camPos)
{
    m_grid->render(view, proj);

    m_renderQueue.clear();
    m_dummy->submitRender(m_renderQueue);

    for (const PrimitiveLayoutPart& part : m_environmentLayout.parts)
    {
        if (part.importedModel)
        {
            ImportedModelCommand command;
            command.model = part.importedModel;
            command.world = part.localTransform.getMatrix();
            command.color = part.color;
            m_renderQueue.submit(command);
        }
        else
        {
            MeshCommand command;
            command.mesh = part.primitive;
            command.world = part.localTransform.getMatrix();
            command.color = part.color;
            m_renderQueue.submit(command);
        }
    }

#ifdef _DEBUG
    if constexpr (SHOW_IMPORTED_RIFLE_WORLD_PREVIEW)
    {
        if (m_context && m_context->importedModels)
        {
            if (const ImportedModel* rifle = m_context->importedModels->get(IMPORTED_RIFLE_PREVIEW_PATH))
            {
                ImportedModelCommand command;
                command.model = rifle;
                command.world = makeImportedRiflePreviewWorld(*rifle);
                command.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
                m_renderQueue.submit(command);
            }
        }
    }
#endif

    m_renderer->ExecuteRenderCommands(m_renderQueue, view, proj, camPos, m_lighting);
}

void TrainingScene::renderEffects(const Matrix& view, const Matrix& proj, const Vector3& camPos)
{
    m_bulletRenderer->render(&m_bulletPool, view, proj, camPos);
    m_particleSystem->render(view, proj);
    m_tracers->render(view, proj, camPos);
}

void TrainingScene::renderViewmodel(const Matrix& view, const Vector3& camPos)
{
    m_renderer->BeginViewmodelPass();   // 深度クリアして常に最前面に描画

    m_renderQueue.clear();
    m_player->weapon().render(m_renderQueue, view);
    m_renderer->ExecuteRenderCommands(m_renderQueue, view, m_camera->matViewmodelProj(), camPos, m_lighting);
}

void TrainingScene::renderUI(const Matrix& view, const Matrix& proj)
{
    m_gameUI->render(view, proj);
    if (m_debugMode && m_debugUI)
    {
        m_debugUI->render();
    }
}
