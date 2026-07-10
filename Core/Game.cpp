//---------------------------------------------------------------------------
//! @file   Game.cpp
//! @brief  ゲーム本体 — 初期化・メインループ・ウィンドウイベント処理
//---------------------------------------------------------------------------
#include "pch.h"
#include "Game.h"

#include "Render/Pipeline/Renderer.h"
#include "Render/Assets/ShaderCache.h"
#include "Render/Assets/MeshCache.h"
#include "Render/Assets/ImportedModelCache.h"
#include "Source/Services/InputManager.h"
#include "Scenes/SceneManager.h"
#include "Scenes/IntroScene.h"
#include "Scenes/MainMenuScene.h"
#include "Scenes/BossScene.h"
#include "Scenes/TrainingScene.h"
#include "Scenes/VictoryScene.h"
#include "Scenes/GameOverScene.h"

using namespace DirectX;

//===========================================================================
// ライフサイクル
//===========================================================================
Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_input = std::make_unique<InputManager>();
    m_renderer = std::make_unique<Renderer>(m_deviceResources.get());
    m_shaders = std::make_unique<ShaderCache>();
    m_meshes = std::make_unique<MeshCache>();
    m_importedModels = std::make_unique<ImportedModelCache>();
    m_sceneManager = std::make_unique<SceneManager>();
    m_deviceResources->RegisterDeviceNotify(this);   // デバイスロスト通知を受け取る
}

Game::~Game()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//===========================================================================
// 初期化
//===========================================================================
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    // デバイス依存 -> ウィンドウサイズ依存の順でリソースを生成
    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();
    m_renderer->CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
    m_renderer->CreateWindowSizeDependentResources();

    // サブシステム初期化
    m_input->initialize(window);

    m_shaders->initialize(m_deviceResources->GetD3DDevice());
    m_meshes->initialize(m_deviceResources->GetD3DDeviceContext());
    m_importedModels->initialize(
        m_deviceResources->GetD3DDevice(),
        m_deviceResources->GetD3DDeviceContext());
    m_commonStates = std::make_unique<CommonStates>(m_deviceResources->GetD3DDevice());

    // シーンへ配るサービス参照（すべて非所有）
    m_context.device         = m_deviceResources.get();
    m_context.renderer       = m_renderer.get();
    m_context.input          = m_input.get();
    m_context.audio          = nullptr;
    m_context.shaders        = m_shaders.get();
    m_context.meshes         = m_meshes.get();
    m_context.importedModels = m_importedModels.get();
    m_context.commonStates   = m_commonStates.get();

    // シーン登録
    m_sceneManager->initialize(m_context);

    auto introScene = std::make_unique<IntroScene>(m_sceneManager.get());
    m_sceneManager->addScene("Intro", std::move(introScene));

    auto mainMenuScene = std::make_unique<MainMenuScene>(m_sceneManager.get());
    m_sceneManager->addScene("MainMenu", std::move(mainMenuScene));

    auto bossScene = std::make_unique<BossScene>(m_sceneManager.get());
    m_sceneManager->addScene("BossScene", std::move(bossScene));

    auto trainingScene = std::make_unique<TrainingScene>(m_sceneManager.get());
    m_sceneManager->addScene("Training", std::move(trainingScene));

    auto victoryScene = std::make_unique<VictoryScene>(m_sceneManager.get());
    m_sceneManager->addScene("Victory", std::move(victoryScene));

    auto gameOverScene = std::make_unique<GameOverScene>(m_sceneManager.get());
    m_sceneManager->addScene("GameOver", std::move(gameOverScene));

    m_sceneManager->transitionTo("MainMenu");

    // ImGui 初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;   // imgui.ini を出力しない

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 2;
    fontConfig.PixelSnapH = false;
    
    // TODO: need to fix byte-wise wide -> narrow conversion
    const std::wstring fontPath = GetAssetPath(L"Fonts/Roboto-Regular.ttf");
    std::string fontPathUtf8(fontPath.begin(), fontPath.end());

    io.FontDefault = io.Fonts->AddFontFromFileTTF(
        fontPathUtf8.c_str(),
        18.0f,
        &fontConfig);

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());
}

//===========================================================================
// フレーム処理
//===========================================================================
void Game::Tick()
{
    m_timer.Tick([&]()
        {
            Update(m_timer);
        });
    Render();
}

void Game::Update(DX::StepTimer const& timer)
{
    float deltaTime = float(timer.GetElapsedSeconds());

    m_input->update();
    m_sceneManager->update(deltaTime, m_input.get());
}

void Game::Render()
{
    // 順序が契約：シーン描画 -> EndScene（バックバッファへレターボックス合成）-> ImGui。
    // ImGui は合成後のバックバッファへウィンドウ座標で描くため、トーンマップの影響を
    // 受けず、マウス座標もズレない。
    m_renderer->BeginScene();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_sceneManager->render();

    m_renderer->EndScene();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_deviceResources->Present();
}

//===========================================================================
// ウィンドウメッセージ
//===========================================================================
void Game::OnActivated()
{
    // TODO: フォーカス復帰時に入力を再取得 / ポーズ解除
}

void Game::OnDeactivated()
{
    // TODO: フォーカス喪失時にゲームをポーズ
}

void Game::OnSuspending()
{
    // TODO: 最小化 / サスペンド時に一時停止し、一時リソースを解放
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
}

void Game::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
    {
        return;
    }

    CreateWindowSizeDependentResources();
    m_renderer->CreateWindowSizeDependentResources();
}

void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // 既定はフル HD。レンダリングは常に固定 1920x1080 のため、この値なら等倍表示になる
    width  = 1920;
    height = 1080;
}

//===========================================================================
// D3D リソース
//===========================================================================
void Game::CreateDeviceDependentResources()
{
    // 現状 Game 直轄の D3D リソースは無い（Renderer 側が所有）
}

void Game::CreateWindowSizeDependentResources()
{
}

void Game::OnDeviceLost()
{
    ImGui_ImplDX11_InvalidateDeviceObjects();
    m_renderer->OnDeviceLost();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    m_renderer->CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
    m_renderer->CreateWindowSizeDependentResources();

    ImGui_ImplDX11_CreateDeviceObjects();
}
