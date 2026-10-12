//---------------------------------------------------------------------------
//! @file   BossScene.cpp
//! @brief  ボス戦シーン
//---------------------------------------------------------------------------
#include "pch.h"
#include "Scenes/BossScene.h"
#include "Gameplay/PlayerCamera.h"
#include "DeviceResources.h"
#include "Services/InputManager.h"
#include "Render/Pipeline/Renderer.h"
#include "Render/Pipeline/Bloom.h"
#include "Render/Pipeline/SceneRenderer.h"
#include "Render/Assets/MeshCache.h"
#include "Gameplay/Weapon/WeaponShot.h"
#include "Gameplay/Weapon/PlayerWeapon.h"

namespace
{
    constexpr const char* RIFLE_FIRE_GROUP = "rifle_fire";
    constexpr float RIFLE_FIRE_VOLUME = 0.35f;
    constexpr float RIFLE_FIRE_PITCH_JITTER = 0.015f;

    const Vector4 PLAYER_TRACER_COLOR(2.0f, 4.25f, 5.0f, 1.0f);    //!< トレーサーのHDR色 (ブルームで発光)

    //---------------------------------------------------------------------------
    //! 発射音グループをロード (バリエーション不在時は単発SEへフォールバック)
    //---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
//! コンストラクタ
//---------------------------------------------------------------------------
BossScene::BossScene(SceneManager* sceneManager)
    : Scene("BossScene")
    , m_sceneManager(sceneManager)
{
}

BossScene::~BossScene()
{
}

//===========================================================================
// 初期化
//===========================================================================

//---------------------------------------------------------------------------
//! 全サブシステムを構築します (ライティング -> カメラ -> プレイヤー -> VFX -> 敵 -> オーディオ -> ワールド -> UI)
//---------------------------------------------------------------------------
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

    m_synchronizer = std::make_unique<Synchronizer>();
    m_synchronizer->setBPM(109.0f);
    m_synchronizer->setStartDelay(15.931f);
    m_synchronizer->setSongDuration(115.0f);
    m_synchronizer->setBeatCallback([this](int beat) {
        onBeat(beat);
    });

    // ワールド
    m_grid = std::make_unique<Grid>(*m_context);
    m_grid->initialize();

    m_waveEffect = std::make_unique<WaveEffect>(*m_context);
    m_waveEffect->initialize();

    m_waveEffect->setBrightness(1.0f);
    m_waveEffect->setAlpha(1.0f);
    m_waveEffect->setTransform(
        Vector3(0.0f, 0.0f, 420.0f),
        Vector3::Zero,
        Vector3::One);

    m_cubemap = std::make_unique<Cubemap>(m_deviceResources);
    m_cubemap->initialize();

    m_indirectLight = std::make_unique<IndirectLight>(m_deviceResources);
    m_indirectLight->initialize(m_cubemap->cubeSRV(), m_context->commonStates->LinearClamp());

    // UI
    m_gameUI = std::make_unique<GameUI>();
    m_gameUI->initialize(m_deviceResources);  // m_deviceResources は Scene::initialize で設定済み
    m_gameUI->setSynchronizer(m_synchronizer.get());
    m_gameUI->setPlayer(m_player.get());
    m_gameUI->setBoss(m_boss.get());

    m_debugUI = std::make_unique<DebugUI>();
    m_debugUI->setCamera(m_camera.get());
    m_debugUI->setLightCycle(m_player.get());
    m_debugUI->setGrid(m_grid.get());
    m_debugUI->setCubemap(m_cubemap.get());
    m_debugUI->setAudioManager(m_audioManager.get());
    m_debugUI->setSynchronizer(m_synchronizer.get());
    m_debugUI->setBulletPool(&m_bulletPool);
    m_debugUI->setBoss(m_boss.get());
    m_debugUI->setBloom(m_renderer->GetSceneRenderer()->getBloom());
    m_debugUI->setExposurePtr(m_camera->exposurePtr());
    m_debugUI->setSceneLighting(&m_lighting);

    // バトルフロー FSM — 登録は一度きり。enter() が changeState(intro) でリセットする
    m_battleFSM.addState(BattlePhase::intro,
        [this]()
        {
            m_introTimer = 0.0f;
            m_gameUI->setShowWaveIndicator(true);    // 中央上の "BOSS" バナー (wave==3)
        },
        [this](float dt) { updateIntro(dt); },
        nullptr);

    m_battleFSM.addState(BattlePhase::fight,
        [this]() { m_gameUI->setShowWaveIndicator(false); },
        [this](float dt) { updateFight(dt); },
        nullptr);

    // 勝敗は enter で遷移を1回だけ発火 (transitionTo はフェード完了まで遅延実行)
    m_battleFSM.addState(BattlePhase::victory,
        [this]() { m_sceneManager->transitionTo("Victory"); },
        nullptr,
        nullptr);

    m_battleFSM.addState(BattlePhase::defeat,
        [this]() { m_sceneManager->transitionTo("GameOver"); },
        nullptr,
        nullptr);
}

//===========================================================================
// シーン遷移
//===========================================================================

//---------------------------------------------------------------------------
//! ゲーム状態リセット + ボス起動
//---------------------------------------------------------------------------
void BossScene::enter()
{
    Scene::enter();
    m_debugMode = false;
    if (m_context && m_context->input)
    {
        m_context->input->setCursorVisible(false);
    }

    // ゲーム状態リセット
    m_player->reset();

    m_boss->deactivate();

    m_bulletPool.deactivateAll();

    m_boss->setPlayerTarget(m_player->rootPositionPtr());
    m_boss->setBulletPool(&m_bulletPool);
    m_boss->activate();
    m_gameUI->setWaveNumber(3);

    // ヒットスキャンターゲット構築 — Boss のみ（敵弾は Player へ直接解決）
    m_shotTargets.clear();
    m_shotTargets.push_back(m_boss.get());

    // オーディオリセット
    m_synchronizer->reset();
    m_audioManager->playMusic("Gameplay");

    // ブルーム有効化（exit() で disable されるため毎エントリで再有効化）
    m_renderer->GetSceneRenderer()->getBloom()->setEnabled(true);

    m_renderer->GetSceneRenderer()->setActiveCamera(m_camera.get());

    m_battleFSM.changeState(BattlePhase::intro);
}

//---------------------------------------------------------------------------
//! 音楽停止 + カメラ・ブルームを返却
//---------------------------------------------------------------------------
void BossScene::exit()
{
    Scene::exit();
    m_audioManager->stopMusic();

    if (m_renderer)
    {
        m_renderer->GetSceneRenderer()->getBloom()->setEnabled(false);
        m_renderer->GetSceneRenderer()->setActiveCamera(nullptr);
    }
}

//---------------------------------------------------------------------------
//! GPU リソース解放 -> 借用ポインタ返却 -> オブジェクト破棄
//---------------------------------------------------------------------------
void BossScene::finalize()
{
    // GPU リソース解放
    if (m_grid)           { m_grid->finalize(); }
    if (m_waveEffect)     { m_waveEffect->finalize(); }
    if (m_cubemap)         { m_cubemap->finalize(); }
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
    m_waveEffect.reset();
    m_cubemap.reset();
    m_indirectLight.reset();
    m_synchronizer.reset();
    m_audioManager.reset();
    m_debugUI.reset();
}

//===========================================================================
// 更新
//===========================================================================

//---------------------------------------------------------------------------
//! 入力 -> バトルフローFSM -> カメラ・サブシステム (フェーズ非依存の演出系)
//---------------------------------------------------------------------------
void BossScene::update(float deltaTime, InputManager* input)
{
    // === 入力 (どのフェーズでも生きている) ===
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
    m_playerInputBlocked = m_debugMode || debugToggledThisFrame;

    // === バトルフロー (ゲームプレイ系統はステート側が回す) ===
    m_battleFSM.update(deltaTime);

    // === カメラ + サブシステム (フェーズに依らず毎フレーム — 勝敗フェード中も演出は進む) ===
    m_camera->update(*m_player, deltaTime);
    m_camera->updateConstants();

    m_gameUI->update(deltaTime);
    m_audioManager->update();
    m_particleSystem->update(deltaTime);
    m_tracers->update(deltaTime);
    m_synchronizer->update(deltaTime);
    m_grid->setBeatPulse(m_synchronizer->getBeatProgress());
    m_grid->update();
    m_waveEffect->update(deltaTime);
}

//---------------------------------------------------------------------------
//! intro: バナー表示中はタイマーを進めるだけ (プレイヤー・ボスとも停止)
//---------------------------------------------------------------------------
void BossScene::updateIntro(float deltaTime)
{
    m_introTimer += deltaTime;
    if (m_introTimer >= INTRO_DURATION)
    {
        m_battleFSM.changeState(BattlePhase::fight);
    }
}

//---------------------------------------------------------------------------
//! fight: ゲームプレイ系統 (正準順: Player → Boss → Combat) + 勝敗ポーリング
//---------------------------------------------------------------------------
void BossScene::updateFight(float deltaTime)
{
    // ボス死亡は系統より先に見る — 撃破フレームの演出 (パーティクル/SE) を
    // 1フレーム描画させてから遷移する (旧 victoryReached の先読みと同じ順序)
    if (m_boss->isDead())
    {
        m_battleFSM.changeState(BattlePhase::victory);
        return;
    }

    // 1フレーム内の射撃インテントは Player 側で積み、Combat 側で解決して空にする
    std::vector<WeaponShot> weaponShots;
    if (!m_playerInputBlocked)
    {
        m_player->update(*m_context->input, deltaTime, weaponShots);
    }

    // 発砲SE (weaponShots は CombatSystem が解決時に空にするため、ここで見る)
    if (!weaponShots.empty())
    {
        m_audioManager->playRandomSound(RIFLE_FIRE_GROUP, RIFLE_FIRE_VOLUME, RIFLE_FIRE_PITCH_JITTER);
    }

    m_boss->update(deltaTime);
    m_bulletPool.update(deltaTime);
    m_combatSystem.update(m_shotTargets, weaponShots);
    m_combatSystem.resolveBullets(m_bulletPool, *m_player);

    applyCombatFeedback(m_combatSystem.results());

    // プレイヤー死亡は解決後に見る — 死んだそのフレームで遷移 (旧コードと同順)
    if (m_player->isDead())
    {
        m_battleFSM.changeState(BattlePhase::defeat);
    }
}

//---------------------------------------------------------------------------
//! ビートコールバック (Synchronizer -> GameUI のフラッシュ)
//---------------------------------------------------------------------------
void BossScene::onBeat(int beat)
{
    m_gameUI->triggerBeatFlash(beat);
}

//---------------------------------------------------------------------------
//! 戦闘結果を演出へ変換します (被弾パーティクル・シェイク・SE・トレーサー)
//! 演出位置は被弾者本人から取る (結果は誰に当たったかだけを持つ)
//---------------------------------------------------------------------------
void BossScene::applyCombatFeedback(const CombatResults& results)
{
    for (const auto& hit : results.hits)
    {
        if (hit.target == m_boss.get())
        {
            m_particleSystem->emit(m_boss->getPosition(), Vector4(0.8f, 0.0f, 1.0f, 1.0f),
                BOSS_HIT_PARTICLE_COUNT, BOSS_HIT_PARTICLE_SPEED,
                BOSS_HIT_PARTICLE_LIFE, BOSS_HIT_PARTICLE_SPREAD);

            if (hit.died)
            {
                m_particleSystem->emit(m_boss->getPosition(), Vector4(0.8f, 0.0f, 1.0f, 1.0f),
                    BOSS_DEATH_PARTICLE_COUNT, BOSS_DEATH_PARTICLE_SPEED,
                    BOSS_DEATH_PARTICLE_LIFE, BOSS_DEATH_PARTICLE_SPREAD);
                m_camera->triggerShake(BOSS_DEATH_SHAKE_INTENSITY, BOSS_DEATH_SHAKE_DURATION);
                m_audioManager->playSound("death", 0.3f);
            }
        }
        else if (hit.target == m_player.get())
        {
            m_camera->triggerShake(PLAYER_HIT_SHAKE_INTENSITY, PLAYER_HIT_SHAKE_DURATION);
        }
    }

    for (const auto& tracer : results.tracers)
    {
        m_tracers->spawn(tracer.start, tracer.end, PLAYER_TRACER_COLOR);
    }
}

//===========================================================================
// 描画
//===========================================================================

//---------------------------------------------------------------------------
//! 拡散IBLを渡してからレンダーパスを順に実行します
//---------------------------------------------------------------------------
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

//===========================================================================
// レンダーパス
//===========================================================================

//---------------------------------------------------------------------------
//! キューブマップ背景 + 波エフェクト + グリッド + ボス
//---------------------------------------------------------------------------
void BossScene::renderWorld(const Matrix& view, const Matrix& proj, const Vector3& camPos)
{
    m_cubemap->render();
    m_waveEffect->render(view, proj);
    m_grid->render(view, proj);

    m_renderQueue.clear();
    m_boss->submitRender(m_renderQueue);
    m_renderer->ExecuteRenderCommands(m_renderQueue, view, proj, camPos, m_lighting);
}

//---------------------------------------------------------------------------
//! 弾・パーティクル・トレーサー
//---------------------------------------------------------------------------
void BossScene::renderEffects(const Matrix& view, const Matrix& proj, const Vector3& camPos)
{
    m_renderQueue.clear();
    m_bulletPool.submitRender(m_renderQueue, m_bulletMesh);
    m_renderer->ExecuteRenderCommands(m_renderQueue, view, proj, camPos, m_lighting);

    m_particleSystem->render(view, proj);
    m_tracers->render(view, proj, camPos);
}

//---------------------------------------------------------------------------
//! 武器ビューモデル (深度クリアで常に最前面)
//---------------------------------------------------------------------------
void BossScene::renderViewmodel(const Matrix& view, const Vector3& camPos)
{
    m_renderer->BeginViewmodelPass();   // 深度クリアして常に最前面に描画

    m_renderQueue.clear();
    m_player->weapon().render(m_renderQueue, view);
    m_renderer->ExecuteRenderCommands(m_renderQueue, view, m_camera->matViewmodelProj(), camPos, m_lighting);
}

//---------------------------------------------------------------------------
//! GameUI + デバッグ (F3 中のみ)
//---------------------------------------------------------------------------
void BossScene::renderUI(const Matrix& view, const Matrix& proj)
{
    m_gameUI->render(view, proj);
    if (m_debugMode && m_debugUI)
    {
        m_debugUI->render();
    }
}
