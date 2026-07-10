//---------------------------------------------------------------------------
//! @file   IntroScene.h
//! @brief  イントロシーン (起動演出)
//---------------------------------------------------------------------------
#pragma once
#include "Scenes/Scene.h"
#include "Render/Visuals/MenuBackground.h"

class SceneManager;

//===========================================================================
//! イントロシーン
//! ワーピング背景の上で引用テキストをフェードさせ、MainMenu へ遷移する
//===========================================================================
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
    int m_introState = 0;         //!< 0=待機 1=フェードイン 2=保持 3=フェードアウト
    float m_introTimer = 0.0f;    //!< 現ステートの経過秒数
    float m_quoteAlpha = 0.0f;    //!< 引用テキストの不透明度

    // タイミング定数 (秒)
    static constexpr float WAIT_TIME           = 3.0f;
    static constexpr float QUOTE_FADE_IN_TIME  = 1.0f;
    static constexpr float QUOTE_HOLD_TIME     = 3.0f;
    static constexpr float QUOTE_FADE_OUT_TIME = 1.0f;

    const wchar_t* m_quoteText = L"The Battle Between Light and Darkness";
};
