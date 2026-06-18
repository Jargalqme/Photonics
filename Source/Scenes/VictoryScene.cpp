#include "pch.h"
#include "Scenes/VictoryScene.h"
#include "Scenes/SceneManager.h"
#include "Services/InputManager.h"
#include "Render/Pipeline/Renderer.h"

// === 生成・破棄 ===

VictoryScene::VictoryScene(SceneManager* sceneManager)
    : Scene("Victory")
    , m_sceneManager(sceneManager)
    , m_selectedIndex(0)
{
}

VictoryScene::~VictoryScene()
{
}

// === シーンライフサイクル ===

void VictoryScene::initialize(SceneContext& context)
{
    Scene::initialize(context);

    // SE
    m_audioManager = std::make_unique<AudioManager>();
    m_audioManager->initialize();
    m_audioManager->loadMusic("victory", GetAssetPath(L"Audio/victory_sfx.wav").c_str());

    // 波形シェーダー背景（ゴールド系）
    m_background = std::make_unique<MenuBackground>(m_deviceResources);
    m_background->initialize();
    m_background->setShaderType(MenuBackground::ShaderType::Wave);
    m_background->setColorTint(1.0f, 0.85f, 0.4f);
}

void VictoryScene::enter()
{
    Scene::enter();
    m_selectedIndex = 0;
    if (m_context && m_context->input)
    {
        m_context->input->setCursorVisible(true);
    }

    if (m_audioManager)
    {
        m_audioManager->playMusic("victory", true);
    }
}

void VictoryScene::exit()
{
    Scene::exit();

    if (m_audioManager)
    {
        m_audioManager->stopMusic();
    }
}

void VictoryScene::finalize()
{
    if (m_background)
    {
        m_background->onDeviceLost();
    }
}

// === 更新 ===

void VictoryScene::update(float deltaTime, InputManager* input)
{
    if (m_audioManager)
    {
        m_audioManager->update();
    }
    if (m_background)
    {
        m_background->update(deltaTime);
    }

    // --- ナビゲーション ---
    if (input->isKeyPressed(Keyboard::Keys::Up) || input->isKeyPressed(Keyboard::Keys::W))
    {
        m_selectedIndex--;
        if (m_selectedIndex < 0)
        {
            m_selectedIndex = m_menuItemCount - 1;
        }
    }
    if (input->isKeyPressed(Keyboard::Keys::Down) || input->isKeyPressed(Keyboard::Keys::S))
    {
        m_selectedIndex++;
        if (m_selectedIndex >= m_menuItemCount)
        {
            m_selectedIndex = 0;
        }
    }

    // --- 決定（フェード遷移） ---
    if (input->isKeyPressed(Keyboard::Keys::Enter) || input->isKeyPressed(Keyboard::Keys::Space))
    {
        switch (m_selectedIndex)
        {
        case 0:  // 再プレイ
            m_sceneManager->transitionTo("BossScene");
            break;
        case 1:  // メインメニュー
            m_sceneManager->transitionTo("MainMenu");
            break;
        }
    }
}

// === 描画 ===

void VictoryScene::render()
{
    if (m_background)
    {
        m_background->render(m_renderer->GetRenderWidth(), m_renderer->GetRenderHeight());
    }

    // === ImGui メニュー ===
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    float centerX = windowSize.x * 0.5f;
    float centerY = windowSize.y * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(centerX, centerY), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 350));

    ImGui::Begin("##Victory", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground);

    float windowWidth = ImGui::GetWindowWidth();

    // タイトル
    ImGui::SetWindowFontScale(4.0f);
    const char* title = "VICTORY";
    ImGui::SetCursorPosX((windowWidth - ImGui::CalcTextSize(title).x) / 2.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), title);

    // サブタイトル
    ImGui::SetWindowFontScale(1.5f);
    const char* subtitle = "SYSTEM SECURED";
    ImGui::SetCursorPosX((windowWidth - ImGui::CalcTextSize(subtitle).x) / 2.0f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), subtitle);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    // メニュー項目
    ImGui::SetWindowFontScale(1.8f);

    const char* menuItems[] = { "PLAY AGAIN", "MAIN MENU" };

    for (int i = 0; i < m_menuItemCount; i++)
    {
        const float hitWidth = 280.0f;
        const float hitHeight = ImGui::GetTextLineHeight() + 10.0f;
        ImGui::SetCursorPosX((windowWidth - hitWidth) / 2.0f);
        ImVec2 hitPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(menuItems[i], ImVec2(hitWidth, hitHeight));
        if (ImGui::IsItemHovered())
        {
            m_selectedIndex = i;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_selectedIndex = i;
            switch (m_selectedIndex)
            {
            case 0:
                m_sceneManager->transitionTo("BossScene");
                break;
            case 1:
                m_sceneManager->transitionTo("MainMenu");
                break;
            default:
                break;
            }
        }

        bool isSelected = (i == m_selectedIndex);

        char displayText[64];
        if (isSelected)
        {
            sprintf_s(displayText, "> %s <", menuItems[i]);
        }
        else
        {
            sprintf_s(displayText, "  %s  ", menuItems[i]);
        }

        float textWidth = ImGui::CalcTextSize(displayText).x;
        ImVec2 textPos(hitPos.x + (hitWidth - textWidth) / 2.0f, hitPos.y + 5.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        if (isSelected)
        {
            drawList->AddText(textPos, IM_COL32(255, 217, 0, 255), displayText);
        }
        else
        {
            drawList->AddText(textPos, IM_COL32(102, 102, 102, 255), displayText);
        }

        ImGui::Spacing();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}
