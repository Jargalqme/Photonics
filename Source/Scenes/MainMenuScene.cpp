//---------------------------------------------------------------------------
//! @file   MainMenuScene.cpp
//! @brief  メインメニューシーン
//---------------------------------------------------------------------------
#include "pch.h"
#include "Scenes/MainMenuScene.h"
#include "Scenes/SceneManager.h"
#include "Services/InputManager.h"
#include "Render/Pipeline/Renderer.h"

#include "Render/Pipeline/Bloom.h"
#include "Render/Pipeline/UIRenderer.h"
#include "Render/Visuals/Grid.h"
#include "Render/Pipeline/SceneRenderer.h"

#include <cmath>
#include <string>

namespace
{
    // --- renderTuningPanel 用のデバッグ出力ヘルパ ---

    void TraceLine(const std::string& text)
    {
        OutputDebugStringA(text.c_str());
        OutputDebugStringA("\n");
    }

    std::string FormatVector3(const Vector3& value)
    {
        return std::to_string(value.x) + ", "
            + std::to_string(value.y) + ", "
            + std::to_string(value.z);
    }

    //---------------------------------------------------------------------------
    //! 回転角 (度) からカメラの前方・上ベクトルを構築
    //---------------------------------------------------------------------------
    void BuildCameraBasis(
        const Vector3& rotationDegrees,
        Vector3& outForward,
        Vector3& outUp)
    {
        const float pitchRad = XMConvertToRadians(rotationDegrees.x);
        const float yawRad = XMConvertToRadians(rotationDegrees.y);
        const float rollRad = XMConvertToRadians(rotationDegrees.z);

        outForward.x = std::sin(yawRad) * std::cos(pitchRad);
        outForward.y = -std::sin(pitchRad);
        outForward.z = std::cos(yawRad) * std::cos(pitchRad);
        outForward.Normalize();

        const Vector3 worldUp{ 0.0f, 1.0f, 0.0f };
        Vector3 right = worldUp.Cross(outForward);
        if (right.LengthSquared() <= 1e-6f)
        {
            right = Vector3::Right;
        }
        else
        {
            right.Normalize();
        }

        outUp = outForward.Cross(right);
        outUp.Normalize();

        if (std::fabs(rotationDegrees.z) > 0.001f)
        {
            const Matrix rollMatrix = Matrix::CreateFromAxisAngle(outForward, rollRad);
            outUp = Vector3::Transform(outUp, rollMatrix);
            outUp.Normalize();
        }
    }

    // === メニューレイアウト（UIRenderer の 1920x1080 基準座標） ===

    constexpr float MENU_CENTER_X = 480.0f;
    constexpr float ROOT_TITLE_CENTER_Y = 400.0f;
    constexpr float ROOT_BUTTONS_TOP_Y = 490.0f;
    constexpr float BUTTON_WIDTH = 280.0f;
    constexpr float BUTTON_HEIGHT = 44.0f;
    constexpr float BUTTON_SPACING = 12.0f;

    const DirectX::XMVECTORF32 MENU_TEXT_ACTIVE   = { { { 0.000f, 1.000f, 1.000f, 1.0f } } };
    const DirectX::XMVECTORF32 MENU_TEXT_INACTIVE = { { { 0.588f, 0.784f, 0.784f, 1.0f } } };
    const DirectX::XMVECTORF32 MENU_TITLE_COLOR   = { { { 0.000f, 1.000f, 1.000f, 1.0f } } };

    RECT MakeRefRect(float x, float y, float width, float height)
    {
        return RECT{
            static_cast<LONG>(x),
            static_cast<LONG>(y),
            static_cast<LONG>(x + width),
            static_cast<LONG>(y + height) };
    }

    RECT RootButtonRect(int index)
    {
        const float y = ROOT_BUTTONS_TOP_Y + index * (BUTTON_HEIGHT + BUTTON_SPACING);
        return MakeRefRect(MENU_CENTER_X - BUTTON_WIDTH * 0.5f, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    }

    bool ContainsPoint(const RECT& rect, const Vector2& point)
    {
        return point.x >= rect.left && point.x < rect.right
            && point.y >= rect.top && point.y < rect.bottom;
    }

    void DrawMenuTextItem(
        UIRenderer* ui,
        const std::wstring& text,
        const RECT& rect,
        bool isActive)
    {
        const float x = static_cast<float>(rect.left);
        const float y = static_cast<float>(rect.top);
        const float width = static_cast<float>(rect.right - rect.left);
        const float height = static_cast<float>(rect.bottom - rect.top);

        const float textInsetX = 0.0f;
        const float maxTextWidth = width - textInsetX;
        const XMFLOAT2 textSize = ui->measureText(text, FontType::Menu);
        float textScale = isActive ? 1.08f : 1.0f;
        if (textSize.x * textScale > maxTextWidth && textSize.x > 0.0f)
        {
            textScale = maxTextWidth / textSize.x;
        }

        ui->drawText(text,
            XMFLOAT2(x + textInsetX, y + (height - textSize.y * textScale) * 0.5f),
            isActive ? MENU_TEXT_ACTIVE : MENU_TEXT_INACTIVE,
            isActive ? 1.0f : 0.62f, FontType::Menu, textScale);
    }

}

//---------------------------------------------------------------------------
//! コンストラクタ
//---------------------------------------------------------------------------
MainMenuScene::MainMenuScene(SceneManager* sceneManager)
    : Scene("MainMenu")
    , m_sceneManager(sceneManager)
    , m_selectedIndex(0)
{
}

MainMenuScene::~MainMenuScene()
{
}

//===========================================================================
// シーンライフサイクル
//===========================================================================

//---------------------------------------------------------------------------
//! グリッド背景とメニュー音楽を構築します
//---------------------------------------------------------------------------
void MainMenuScene::initialize(SceneContext& context)
{
    Scene::initialize(context);

    m_grid = std::make_unique<Grid>(*m_context);
    m_grid->initialize();
    m_grid->setLineColor(Color(2.0f, 0.4f, 2.7f, 1.0f));
    m_grid->setBaseColor(Color(0.0f, 0.0f, 0.0f, 1.0f));

    m_audioManager = std::make_unique<AudioManager>();
    m_audioManager->initialize();
    m_audioManager->loadMusic("menu", GetAssetPath(L"Audio/menu_music.wav").c_str());
}

//---------------------------------------------------------------------------
//! カーソル表示・音楽再生・ブルーム有効化
//---------------------------------------------------------------------------
void MainMenuScene::enter()
{
    Scene::enter();
    m_state = State::Root;
    m_selectedIndex = 0;
    if (m_context && m_context->input)
    {
        m_context->input->setCursorVisible(true);
    }

    if (m_audioManager)
    {
        m_audioManager->playMusic("menu", true);
    }

    if (m_renderer && m_renderer->GetSceneRenderer())
    {
        m_renderer->GetSceneRenderer()->setActiveCamera(nullptr);
        m_renderer->GetSceneRenderer()->getBloom()->setEnabled(true);
    }
}

//---------------------------------------------------------------------------
//! 音楽停止・ブルーム無効化
//---------------------------------------------------------------------------
void MainMenuScene::exit()
{
    Scene::exit();

    if (m_audioManager)
    {
        m_audioManager->stopMusic();
    }

    if (m_renderer && m_renderer->GetSceneRenderer())
    {
        m_renderer->GetSceneRenderer()->getBloom()->setEnabled(false);
        m_renderer->GetSceneRenderer()->setActiveCamera(nullptr);
    }
}

void MainMenuScene::finalize()
{
    if (m_grid)
    {
        m_grid->finalize();
        m_grid.reset();
    }

}

//===========================================================================
// 更新
//===========================================================================

//---------------------------------------------------------------------------
//! 現在のページ (Root / Settings) へ入力を配ります
//---------------------------------------------------------------------------
void MainMenuScene::update(float deltaTime, InputManager* input)
{
    (void)deltaTime;

    if (m_audioManager)
    {
        m_audioManager->update();
    }

    if (m_grid)
    {
        m_grid->update();
    }

    if (m_state == State::Root)
    {
        updateRoot(input);
    }
    else
    {
        updateSettings(input);
    }
}

//---------------------------------------------------------------------------
//! 項目一覧の操作 (キー上下 + Enter / マウスホバー + クリック / Esc = 終了)
//---------------------------------------------------------------------------
void MainMenuScene::updateRoot(InputManager* input)
{
    if (input->isKeyPressed(Keyboard::Keys::Up) || input->isKeyPressed(Keyboard::Keys::W))
    {
        m_selectedIndex = (m_selectedIndex - 1 + ROOT_ITEM_COUNT) % ROOT_ITEM_COUNT;
    }

    if (input->isKeyPressed(Keyboard::Keys::Down) || input->isKeyPressed(Keyboard::Keys::S))
    {
        m_selectedIndex = (m_selectedIndex + 1) % ROOT_ITEM_COUNT;
    }

    if (input->isKeyPressed(Keyboard::Keys::Enter) || input->isKeyPressed(Keyboard::Keys::Space))
    {
        activateRootItem(m_selectedIndex);
        return;
    }

    if (input->isKeyPressed(Keyboard::Keys::Escape))
    {
        PostQuitMessage(0);
        return;
    }

    // マウス: ホバーで選択、クリックで決定
    const Vector2 refMouse = toRefSpace(input->getMousePosition());
    for (int i = 0; i < ROOT_ITEM_COUNT; i++)
    {
        if (ContainsPoint(RootButtonRect(i), refMouse))
        {
            m_selectedIndex = i;
            if (input->isLeftMousePressed())
            {
                activateRootItem(i);
            }
            break;
        }
    }
}

//---------------------------------------------------------------------------
//! 選択項目を実行します (開始 / 訓練 / 終了)
//---------------------------------------------------------------------------
void MainMenuScene::activateRootItem(int index)
{
    switch (index)
    {
    case 0:
        m_sceneManager->transitionTo("BossScene");
        break;
    case 1:
        m_sceneManager->transitionTo("Training");
        break;
    // TODO(settings): 設定が入ったら　options　項目を復活
    case 2:
        PostQuitMessage(0);
        break;
    default:
        break;
    }
}

//---------------------------------------------------------------------------
//! 設定ページの操作 (Esc で戻るのみ)
//---------------------------------------------------------------------------
void MainMenuScene::updateSettings(InputManager* input)
{
    // TODO(settings): 設定ページは空シェル
    if (input->isKeyPressed(Keyboard::Keys::Escape))
    {
        m_state = State::Root;
        m_selectedIndex = 0;
    }
}

//---------------------------------------------------------------------------
//! ウィンドウ座標 -> 1920x1080 基準座標 (レターボックス分を補正)
//---------------------------------------------------------------------------
Vector2 MainMenuScene::toRefSpace(const Vector2& windowPos) const
{
    return m_renderer ? m_renderer->WindowToRef(windowPos) : windowPos;
}

//===========================================================================
// 描画
//===========================================================================

//---------------------------------------------------------------------------
//! 固定カメラで3D背景を描き、現在のページのUIを重ねます
//---------------------------------------------------------------------------
void MainMenuScene::render()
{
    const int rw = m_renderer->GetRenderWidth();
    const int rh = m_renderer->GetRenderHeight();
    const float aspect = (rh > 0) ? (static_cast<float>(rw) / static_cast<float>(rh)) : 1.0f;

    Vector3 cameraForward;
    Vector3 cameraUp;
    BuildCameraBasis(m_menuCameraRotationDegrees, cameraForward, cameraUp);
    const Matrix view = XMMatrixLookAtLH(
        m_menuCameraPosition,
        m_menuCameraPosition + cameraForward,
        cameraUp);
    const Matrix proj = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(m_menuCameraFovDegrees), aspect, 1.0f, 1000.0f);

    renderMenuWorld(view, proj);

    if (m_state == State::Root)
    {
        renderRoot();
    }
    else
    {
        renderSettings();
    }
    
    // Tuning panel in main menu scene
    //renderTuningPanel();
}

//---------------------------------------------------------------------------
//! 3D背景 (グリッドのみ)
//---------------------------------------------------------------------------
void MainMenuScene::renderMenuWorld(
    const Matrix& view,
    const Matrix& proj)
{
    if (m_grid)
    {
        m_grid->render(view, proj);
    }
}

//---------------------------------------------------------------------------
//! カメラ・ブルーム調整用 ImGui パネル (通常は呼び出しをコメントアウト)
//---------------------------------------------------------------------------
void MainMenuScene::renderTuningPanel()
{
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(390.0f, 390.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Main Menu Tuning", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    Bloom* bloom = nullptr;
    if (m_renderer && m_renderer->GetSceneRenderer())
    {
        bloom = m_renderer->GetSceneRenderer()->getBloom();
    }

    if (ImGui::Button("Reset"))
    {
        m_menuCameraPosition = Vector3(0.0f, 24.0f, -110.0f);
        m_menuCameraRotationDegrees = Vector3(-3.0f, 0.0f, 0.0f);
        m_menuCameraFovDegrees = 45.0f;

        if (bloom)
        {
            bloom->setEnabled(true);
            *bloom->getThresholdPtr() = 1.0f;
            *bloom->getKneePtr() = 0.1f;
            *bloom->getIntensityPtr() = 1.0f;
            *bloom->getUpsampleScalePtr() = 1.0f;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Log Values"))
    {
        TraceLine("[MainMenu] Tuning camera position=(" + FormatVector3(m_menuCameraPosition)
            + ") rotationDegrees=(" + FormatVector3(m_menuCameraRotationDegrees)
            + ") fov=" + std::to_string(m_menuCameraFovDegrees));
        if (bloom)
        {
            TraceLine("[MainMenu] Tuning bloom enabled=" + std::to_string(bloom->isEnabled() ? 1 : 0)
                + " threshold=" + std::to_string(*bloom->getThresholdPtr())
                + " knee=" + std::to_string(*bloom->getKneePtr())
                + " intensity=" + std::to_string(*bloom->getIntensityPtr())
                + " upsampleScale=" + std::to_string(*bloom->getUpsampleScalePtr()));
        }
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3("Position##Camera", &m_menuCameraPosition.x, 0.25f, -500.0f, 500.0f, "%.2f");
        ImGui::DragFloat3("Rotation##Camera", &m_menuCameraRotationDegrees.x, 0.1f, -180.0f, 180.0f, "%.2f deg");
        ImGui::SliderFloat("FOV##Camera", &m_menuCameraFovDegrees, 20.0f, 100.0f, "%.1f deg");
    }

    if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (bloom)
        {
            ImGui::Checkbox("Enabled##Bloom", bloom->getEnabledPtr());
            ImGui::SliderFloat("Threshold##Bloom", bloom->getThresholdPtr(), 0.0f, 5.0f, "%.2f");
            ImGui::SliderFloat("Knee##Bloom", bloom->getKneePtr(), 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Intensity##Bloom", bloom->getIntensityPtr(), 0.0f, 5.0f, "%.2f");
            ImGui::SliderFloat("Upsample Scale##Bloom", bloom->getUpsampleScalePtr(), 0.25f, 3.0f, "%.2f");
        }
        else
        {
            ImGui::TextDisabled("Unavailable");
        }
    }

    ImGui::End();
}

//---------------------------------------------------------------------------
//! タイトル + 項目一覧
//---------------------------------------------------------------------------
void MainMenuScene::renderRoot()
{
    UIRenderer* ui = m_renderer->GetUI();
    ui->begin();

    ui->drawTextCentered(L"P H O T O N X",
        XMFLOAT2(MENU_CENTER_X, ROOT_TITLE_CENTER_Y),
        MENU_TITLE_COLOR, 1.0f, FontType::Title);

    const wchar_t* menuItems[ROOT_ITEM_COUNT] = { L"start game", L"workshop", L"exit" };
    for (int i = 0; i < ROOT_ITEM_COUNT; i++)
    {
        DrawMenuTextItem(ui, menuItems[i], RootButtonRect(i), i == m_selectedIndex);
    }

    ui->end();
}

void MainMenuScene::renderSettings()
{
    // TODO(settings): 空シェル
}
