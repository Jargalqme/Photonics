#pragma once

#include "Scenes/Scene.h"
#include "Services/AudioManager.h"

#include <SimpleMath.h>
#include <memory>

class SceneManager;
class Grid;

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
    enum class State { Root, Settings };

    SceneManager* m_sceneManager;
    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<Grid> m_grid;

    State m_state = State::Root;
    int m_selectedIndex = 0;

    static constexpr int ROOT_ITEM_COUNT = 3;

    DirectX::SimpleMath::Vector3 m_menuCameraPosition{ 0.0f, 24.0f, -110.0f };
    DirectX::SimpleMath::Vector3 m_menuCameraRotationDegrees{ -3.0f, 0.0f, 0.0f };
    float m_menuCameraFovDegrees = 45.0f;

    void updateRoot(InputManager* input);
    void updateSettings(InputManager* input);
    void activateRootItem(int index);
    DirectX::SimpleMath::Vector2 toRefSpace(
        const DirectX::SimpleMath::Vector2& windowPos) const;
    void renderMenuWorld(
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& proj);
    void renderTuningPanel();
    void renderRoot();
    void renderSettings();
};
