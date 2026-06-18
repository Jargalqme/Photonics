#include "pch.h"
#include "Scenes/MainMenuScene.h"
#include "Scenes/SceneManager.h"
#include "Services/InputManager.h"
#include "Render/Pipeline/Renderer.h"
#include "Game.h"

#include "Render/Pipeline/Bloom.h"
#include "Render/Pipeline/UIRenderer.h"
#include "Render/Visuals/Grid.h"
#include "Render/Pipeline/SceneRenderer.h"
#include "Render/Skinning/AnimationPlayer.h"
#include "Render/Skinning/SkinnedModel.h"
#include "Render/Skinning/SkinnedModelImporter.h"
#include "Render/Skinning/SkinnedRenderer.h"
#include "Render/Skinning/Skeleton.h"

#include <cmath>
#include <iterator>
#include <string>

namespace
{
    struct ResolutionOption
    {
        int width;
        int height;
        const wchar_t* label;
    };

    constexpr ResolutionOption RESOLUTIONS[] = {
        { 1920, 1080, L"1920 x 1080" },
        { 1920, 1200, L"1920 x 1200" },
    };

    constexpr int RESOLUTION_COUNT = static_cast<int>(std::size(RESOLUTIONS));

    const char* BoolText(bool value)
    {
        return value ? "yes" : "no";
    }

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

    void TraceSkinnedModelLoad(const SkinnedModelData& data, int32_t appendedClips)
    {
        TraceLine("[MainMenu] Skinned model loaded: " + data.sourcePath.generic_string());
        TraceLine("[MainMenu]   vertices=" + std::to_string(data.vertices.size())
            + " indices=" + std::to_string(data.indices.size())
            + " submeshes=" + std::to_string(data.submeshes.size())
            + " materials=" + std::to_string(data.materials.size())
            + " textures=" + std::to_string(data.textures.size())
            + " bones=" + std::to_string(data.bones.size())
            + " clips=" + std::to_string(data.clips.size())
            + " idleAppended=" + std::to_string(appendedClips));

        for (size_t i = 0; i < data.textures.size(); ++i)
        {
            const SkinnedTextureData& tex = data.textures[i];
            TraceLine("[MainMenu]   texture[" + std::to_string(i) + "] "
                + tex.name
                + " bytes=" + std::to_string(tex.bytes.size())
                + " size=" + std::to_string(tex.width) + "x" + std::to_string(tex.height)
                + " compressed=" + BoolText(tex.compressed)
                + " srgb=" + BoolText(tex.srgb));
        }

        for (size_t i = 0; i < data.materials.size(); ++i)
        {
            const SkinnedMaterial& mat = data.materials[i];
            std::string textureName = "none";
            if (mat.baseColorTextureIndex >= 0 &&
                static_cast<size_t>(mat.baseColorTextureIndex) < data.textures.size())
            {
                textureName = data.textures[mat.baseColorTextureIndex].name;
            }

            TraceLine("[MainMenu]   material[" + std::to_string(i) + "]"
                + " baseColor=("
                + std::to_string(mat.baseColor.x) + ", "
                + std::to_string(mat.baseColor.y) + ", "
                + std::to_string(mat.baseColor.z) + ", "
                + std::to_string(mat.baseColor.w) + ")"
                + " baseColorTextureIndex=" + std::to_string(mat.baseColorTextureIndex)
                + " texture=" + textureName);
        }
    }

    Matrix MakeRotationDegrees(const Vector3& rotationDegrees)
    {
        return Matrix::CreateFromYawPitchRoll(
            XMConvertToRadians(rotationDegrees.y),
            XMConvertToRadians(rotationDegrees.x),
            XMConvertToRadians(rotationDegrees.z));
    }

    Matrix MakeTransformMatrix(
        const Vector3& position,
        const Vector3& rotationDegrees,
        const Vector3& scale)
    {
        return Matrix::CreateScale(scale) *
            MakeRotationDegrees(rotationDegrees) *
            Matrix::CreateTranslation(position);
    }

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
    constexpr float SETTINGS_TITLE_CENTER_Y = 410.0f;
    constexpr float SETTINGS_ROWS_TOP_Y = 500.0f;
    constexpr float BUTTON_WIDTH = 280.0f;
    constexpr float BUTTON_HEIGHT = 44.0f;
    constexpr float BUTTON_SPACING = 12.0f;
    constexpr float RESOLUTION_ROW_WIDTH = 560.0f;

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

    RECT SettingsButtonRect(int index)
    {
        // 行 0 は解像度行（幅広）、APPLY の前に余白を 1 つ挟む
        const float width = (index == 0) ? RESOLUTION_ROW_WIDTH : BUTTON_WIDTH;
        const float extraGap = (index > 0) ? 8.0f : 0.0f;
        const float y = SETTINGS_ROWS_TOP_Y + index * (BUTTON_HEIGHT + BUTTON_SPACING) + extraGap;
        return MakeRefRect(MENU_CENTER_X - width * 0.5f, y, width, BUTTON_HEIGHT);
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

MainMenuScene::MainMenuScene(SceneManager* sceneManager)
    : Scene("MainMenu")
    , m_sceneManager(sceneManager)
    , m_selectedIndex(0)
{
}

MainMenuScene::~MainMenuScene()
{
}

void MainMenuScene::initialize(SceneContext& context)
{
    Scene::initialize(context);

    m_grid = std::make_unique<Grid>(*m_context);
    m_grid->initialize();
    m_grid->setLineColor(Color(2.0f, 0.4f, 2.7f, 1.0f));
    m_grid->setBaseColor(Color(0.0f, 0.0f, 0.0f, 1.0f));

    SkinnedModelData data;
    const auto modelPath = GetAssetPath(L"Characters/MenuGuy/model3.fbx");
    if (SkinnedModelImporter::LoadSkinnedModelData(modelPath, data))
    {
        size_t selectedClipIndex = data.clips.empty() ? 0 : data.clips.size() - 1;
        bool hasSelectedClip = !data.clips.empty();

        const int32_t appended = 0;
        TraceSkinnedModelLoad(data, appended);

        m_menuCharacter = std::make_unique<SkinnedModel>();
        if (!m_menuCharacter->initialize(m_deviceResources->GetD3DDevice(), std::move(data)))
        {
            m_menuCharacter.reset();
        }

        if (m_menuCharacter)
        {
            m_skinnedRenderer = std::make_unique<SkinnedRenderer>();
            if (!m_skinnedRenderer->initialize(m_deviceResources->GetD3DDevice()))
            {
                m_skinnedRenderer.reset();
            }

            m_skeleton = std::make_unique<Skeleton>();
            m_skeleton->build(m_menuCharacter->data());

            m_animationPlayer = std::make_unique<AnimationPlayer>();
            const auto& clips = m_menuCharacter->clips();
            if (hasSelectedClip && selectedClipIndex < clips.size())
            {
                m_animationPlayer->setClip(&clips[selectedClipIndex]);
                m_animationPlayer->setLoop(true);
            }
            else if (!clips.empty())
            {
                m_animationPlayer->setClip(&clips.back());
                m_animationPlayer->setLoop(true);
            }
        }
    }

    m_audioManager = std::make_unique<AudioManager>();
    m_audioManager->initialize();
    m_audioManager->loadMusic("menu", GetAssetPath(L"Audio/menu_music.wav").c_str());
}

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
    if (m_skinnedRenderer)
    {
        m_skinnedRenderer->finalize();
        m_skinnedRenderer.reset();
    }

    if (m_menuCharacter)
    {
        m_menuCharacter->finalize();
        m_menuCharacter.reset();
    }

    if (m_skeleton)
    {
        m_skeleton->finalize();
        m_skeleton.reset();
    }

    m_animationPlayer.reset();

    if (m_grid)
    {
        m_grid->finalize();
        m_grid.reset();
    }

}

void MainMenuScene::update(float deltaTime, InputManager* input)
{
    if (m_audioManager)
    {
        m_audioManager->update();
    }

    if (m_grid)
    {
        m_grid->update();
    }

    if (m_animationPlayer)
    {
        m_animationPlayer->update(deltaTime);
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
    case 2:
        enterSettings();
        break;
    case 3:
        PostQuitMessage(0);
        break;
    default:
        break;
    }
}

void MainMenuScene::updateSettings(InputManager* input)
{
    if (input->isKeyPressed(Keyboard::Keys::Up) || input->isKeyPressed(Keyboard::Keys::W))
    {
        m_selectedIndex = (m_selectedIndex - 1 + SETTINGS_ITEM_COUNT) % SETTINGS_ITEM_COUNT;
    }

    if (input->isKeyPressed(Keyboard::Keys::Down) || input->isKeyPressed(Keyboard::Keys::S))
    {
        m_selectedIndex = (m_selectedIndex + 1) % SETTINGS_ITEM_COUNT;
    }

    const bool leftPressed = input->isKeyPressed(Keyboard::Keys::Left) || input->isKeyPressed(Keyboard::Keys::A);
    const bool rightPressed = input->isKeyPressed(Keyboard::Keys::Right) || input->isKeyPressed(Keyboard::Keys::D);
    if ((leftPressed || rightPressed) && m_selectedIndex == 0)
    {
        const int delta = rightPressed ? 1 : -1;
        m_pendingResolutionIndex = (m_pendingResolutionIndex + delta + RESOLUTION_COUNT) % RESOLUTION_COUNT;
    }

    if (input->isKeyPressed(Keyboard::Keys::Enter) || input->isKeyPressed(Keyboard::Keys::Space))
    {
        activateSettingsItem(m_selectedIndex);
        return;
    }

    if (input->isKeyPressed(Keyboard::Keys::Escape))
    {
        m_state = State::Root;
        m_selectedIndex = 2;
        return;
    }

    // マウス: ホバーで選択、クリックで決定
    const Vector2 refMouse = toRefSpace(input->getMousePosition());
    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++)
    {
        if (ContainsPoint(SettingsButtonRect(i), refMouse))
        {
            m_selectedIndex = i;
            if (input->isLeftMousePressed())
            {
                activateSettingsItem(i);
            }
            break;
        }
    }
}

void MainMenuScene::activateSettingsItem(int index)
{
    if (index == 0)
    {
        m_pendingResolutionIndex = (m_pendingResolutionIndex + 1) % RESOLUTION_COUNT;
    }
    else if (index == 1)
    {
        applySettings();
    }
    else if (index == 2)
    {
        m_state = State::Root;
        m_selectedIndex = 2;
    }
}

Vector2 MainMenuScene::toRefSpace(const Vector2& windowPos) const
{
    // マウスはウィンドウピクセル座標、メニューは 1920x1080 基準座標
    UIRenderer* ui = m_renderer ? m_renderer->GetUI() : nullptr;
    if (!ui || ui->scaleX() <= 0.0f || ui->scaleY() <= 0.0f)
    {
        return windowPos;
    }
    return Vector2(windowPos.x / ui->scaleX(), windowPos.y / ui->scaleY());
}

void MainMenuScene::enterSettings()
{
    m_state = State::Settings;
    m_selectedIndex = 0;

    const int currentW = m_context->renderer->GetRenderWidth();
    const int currentH = m_context->renderer->GetRenderHeight();

    m_pendingResolutionIndex = 0;
    for (int i = 0; i < RESOLUTION_COUNT; i++)
    {
        if (RESOLUTIONS[i].width == currentW && RESOLUTIONS[i].height == currentH)
        {
            m_pendingResolutionIndex = i;
            break;
        }
    }
}

void MainMenuScene::applySettings()
{
    m_context->game->applyResolution(
        RESOLUTIONS[m_pendingResolutionIndex].width,
        RESOLUTIONS[m_pendingResolutionIndex].height);
}

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
    renderMenuCharacter(view, proj);

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

void MainMenuScene::renderMenuWorld(
    const Matrix& view,
    const Matrix& proj)
{
    if (m_grid)
    {
        m_grid->render(view, proj);
    }
}

void MainMenuScene::renderMenuCharacter(
    const Matrix& view,
    const Matrix& proj)
{
    if (!m_menuCharacter || !m_skinnedRenderer)
    {
        return;
    }

    const Matrix world = MakeTransformMatrix(
        m_menuCharacterPosition,
        m_menuCharacterRotationDegrees,
        m_menuCharacterScale);

    const Matrix* palette = nullptr;
    uint32_t paletteCount = 0;
    if (m_skeleton && m_animationPlayer)
    {
        m_animationPlayer->apply(*m_skeleton);
        palette = m_skeleton->palette();
        paletteCount = m_skeleton->boneCount();
    }

    m_skinnedRenderer->draw(
        m_deviceResources->GetD3DDeviceContext(),
        *m_menuCharacter,
        palette,
        paletteCount,
        world,
        view,
        proj,
        Vector4(1.0f, 1.0f, 1.0f, 1.0f),
        Vector4(0.0f, -1.0f, 0.0f, 1.0f));
}

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

        m_menuCharacterPosition = Vector3(55.0f, -60.0f, 280.0f);
        m_menuCharacterRotationDegrees = Vector3::Zero;
        m_menuCharacterScale = Vector3(2.0f, 2.0f, 2.0f);

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
        TraceLine("[MainMenu] Tuning character position=(" + FormatVector3(m_menuCharacterPosition)
            + ") rotationDegrees=(" + FormatVector3(m_menuCharacterRotationDegrees)
            + ") scale=(" + FormatVector3(m_menuCharacterScale) + ")");
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

    if (ImGui::CollapsingHeader("Character", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3("Position##Character", &m_menuCharacterPosition.x, 0.25f, -300.0f, 300.0f, "%.2f");
        ImGui::DragFloat3("Rotation##Character", &m_menuCharacterRotationDegrees.x, 0.1f, -180.0f, 180.0f, "%.2f deg");
        ImGui::DragFloat3("Scale##Character", &m_menuCharacterScale.x, 0.01f, 0.01f, 5.0f, "%.2f");
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

void MainMenuScene::renderRoot()
{
    UIRenderer* ui = m_renderer->GetUI();
    ui->begin();

    ui->drawTextCentered(L"P H O T O N X",
        XMFLOAT2(MENU_CENTER_X, ROOT_TITLE_CENTER_Y),
        MENU_TITLE_COLOR, 1.0f, FontType::Title);

    const wchar_t* menuItems[ROOT_ITEM_COUNT] = { L"start game", L"workshop", L"options", L"exit" };
    for (int i = 0; i < ROOT_ITEM_COUNT; i++)
    {
        DrawMenuTextItem(ui, menuItems[i], RootButtonRect(i), i == m_selectedIndex);
    }

    ui->end();
}

void MainMenuScene::renderSettings()
{
    UIRenderer* ui = m_renderer->GetUI();
    ui->begin();

    ui->drawTextCentered(L"options",
        XMFLOAT2(MENU_CENTER_X, SETTINGS_TITLE_CENTER_Y),
        MENU_TITLE_COLOR, 1.0f, FontType::Title);

    const std::wstring resolutionLabel = RESOLUTIONS[m_pendingResolutionIndex].label;
    const std::wstring resolutionRow = (m_selectedIndex == 0)
        ? L"resolution  <  " + resolutionLabel + L"  >"
        : L"resolution     " + resolutionLabel;

    DrawMenuTextItem(ui, resolutionRow, SettingsButtonRect(0), m_selectedIndex == 0);
    DrawMenuTextItem(ui, L"apply", SettingsButtonRect(1), m_selectedIndex == 1);
    DrawMenuTextItem(ui, L"back", SettingsButtonRect(2), m_selectedIndex == 2);

    ui->end();
}
