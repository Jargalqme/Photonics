//---------------------------------------------------------------------------
//! @file   MainMenuScene.h
//! @brief  メインメニューシーン
//---------------------------------------------------------------------------
#pragma once

#include "Scenes/Scene.h"
#include "Services/AudioManager.h"

#include <SimpleMath.h>
#include <memory>

class SceneManager;
class Grid;

//===========================================================================
//! メインメニューシーン
//! 固定カメラのグリッド3D背景 + 1920x1080 基準のUI。
//! キーボード (W/S・矢印・Enter) とマウス (ホバー・クリック) の両対応
//===========================================================================
class MainMenuScene : public Scene
{
public:
    MainMenuScene(SceneManager* sceneManager);
    ~MainMenuScene() override;

    void initialize(SceneContext& context) override;
    void enter() override;
    void exit() override;
    void finalize() override;

    void update(float deltaTime, InputManager* input) override;
    void render() override;

private:
    enum class State { Root, Settings };    //!< Settings は空シェル (TODO(settings))

    SceneManager* m_sceneManager;
    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<Grid> m_grid;

    State m_state = State::Root;
    int m_selectedIndex = 0;      //!< 選択中の項目 (キーとマウスホバーで共有)

    static constexpr int ROOT_ITEM_COUNT = 3;

    // メニュー用固定カメラ (renderTuningPanel で調整して決めた値)
    DirectX::SimpleMath::Vector3 m_menuCameraPosition{ 0.0f, 24.0f, -110.0f };
    DirectX::SimpleMath::Vector3 m_menuCameraRotationDegrees{ -3.0f, 0.0f, 0.0f };
    float m_menuCameraFovDegrees = 45.0f;

    void updateRoot(InputManager* input);
    void updateSettings(InputManager* input);

    //! 選択項目を実行します (開始 / 訓練 / 終了)
    void activateRootItem(int index);

    //! ウィンドウ座標 -> 1920x1080 基準座標 (レターボックス分を補正)
    DirectX::SimpleMath::Vector2 toRefSpace(
        const DirectX::SimpleMath::Vector2& windowPos) const;

    void renderMenuWorld(
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& proj);

    //! カメラ・ブルーム調整用 ImGui パネル (通常は呼び出しをコメントアウト)
    void renderTuningPanel();

    void renderRoot();
    void renderSettings();
};
