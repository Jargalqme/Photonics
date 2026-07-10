//---------------------------------------------------------------------------
//! @file   GameOverScene.h
//! @brief  ゲームオーバーシーン
//---------------------------------------------------------------------------
#pragma once
#include "Scenes/Scene.h"
#include "Services/AudioManager.h"
#include "Render/Visuals/MenuBackground.h"

class SceneManager;

//===========================================================================
//! ゲームオーバーシーン
//! 赤系の波形背景 + ImGui 直描きメニュー (RETRY / MAIN MENU)
//===========================================================================
class GameOverScene : public Scene
{
public:
    GameOverScene(SceneManager* sceneManager);
    ~GameOverScene() override;

    void initialize(SceneContext& context) override;
    void enter() override;
    void exit() override;
    void finalize() override;

    void update(float deltaTime, InputManager* input) override;
    void render() override;

private:
    SceneManager* m_sceneManager;
    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<MenuBackground> m_background;

    // メニューナビゲーション
    int m_selectedIndex = 0;        //!< 選択中の項目 (キーとマウスホバーで共有)
    const int m_menuItemCount = 2;  //!< RETRY, MAIN MENU
};
