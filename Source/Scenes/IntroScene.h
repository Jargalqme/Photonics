#pragma once
#include "Scenes/Scene.h"
#include "Render/Visuals/MenuBackground.h"

class SceneManager;

class IntroScene : public Scene
{
public:
    IntroScene(SceneManager* sceneManager);
    ~IntroScene() override;

    void initialize(SceneContext& context) override;
    void enter() override;
    void exit() override;
    void finalize() override;

    void update(float deltaTime, InputManager* input) override;
    void render() override;

private:
    SceneManager* m_sceneManager;
    std::unique_ptr<MenuBackground> m_background;

    // イントロステートマシン
    int m_introState = 0;
    float m_introTimer = 0.0f;
    float m_quoteAlpha = 0.0f;

    // タイミング定数
    static constexpr float WAIT_TIME           = 3.0f;
    static constexpr float QUOTE_FADE_IN_TIME  = 1.0f;
    static constexpr float QUOTE_HOLD_TIME     = 3.0f;
    static constexpr float QUOTE_FADE_OUT_TIME = 1.0f;

    const wchar_t* m_quoteText = L"The Battle Between Light and Darkness";
};
