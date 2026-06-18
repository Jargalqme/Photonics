#include "pch.h"
#include "Scenes/IntroScene.h"
#include "Scenes/SceneManager.h"
#include "Render/Pipeline/Renderer.h"

// === 生成・破棄 ===

IntroScene::IntroScene(SceneManager* sceneManager)
    : Scene("Intro")
    , m_sceneManager(sceneManager)
{
}

IntroScene::~IntroScene()
{
}

// === シーンライフサイクル ===

void IntroScene::initialize(SceneContext& context)
{
    Scene::initialize(context);

    // ワーピング背景シェーダー（暗め、色収差あり）
    m_background = std::make_unique<MenuBackground>(m_deviceResources);
    m_background->initialize();
    m_background->setShaderType(MenuBackground::ShaderType::Warping);
    m_background->setSpeed(1.0f);
    m_background->setPatternScale(9.0f);
    m_background->setWarpIntensity(1.0f);
    m_background->setBrightness(0.01f);
    m_background->setChromaticOffset(0.07f);
}

void IntroScene::enter()
{
    Scene::enter();
    m_introState = 0;
    m_introTimer = 0.0f;
    m_quoteAlpha = 0.0f;
}

void IntroScene::exit()
{
    Scene::exit();
}

void IntroScene::finalize()
{
    if (m_background)
    {
        m_background->onDeviceLost();
    }
}

// === 更新 ===

void IntroScene::update(float deltaTime, InputManager* input)
{
    (void)input;

    if (m_background)
    {
        m_background->update(deltaTime);
    }

    m_introTimer += deltaTime;

    // ステートマシン: 待機 → フェードイン → 保持 → フェードアウト → メインメニュー
    switch (m_introState)
    {
    case 0:  // 待機（シェーダー背景のみ表示）
        if (m_introTimer >= WAIT_TIME)
        {
            m_introState = 1;
            m_introTimer = 0.0f;
        }
        break;

    case 1:  // 引用テキストフェードイン
        m_quoteAlpha = std::min(m_introTimer / QUOTE_FADE_IN_TIME, 1.0f);
        if (m_introTimer >= QUOTE_FADE_IN_TIME)
        {
            m_introState = 2;
            m_introTimer = 0.0f;
            m_quoteAlpha = 1.0f;
        }
        break;

    case 2:  // 引用テキスト保持
        if (m_introTimer >= QUOTE_HOLD_TIME)
        {
            m_introState = 3;
            m_introTimer = 0.0f;
        }
        break;

    case 3:  // 引用テキストフェードアウト → メインメニューへ遷移
        m_quoteAlpha = std::max(1.0f - (m_introTimer / QUOTE_FADE_OUT_TIME), 0.0f);
        if (m_introTimer >= QUOTE_FADE_OUT_TIME)
        {
            m_sceneManager->transitionTo("MainMenu");
        }
        break;
    }
}

// === 描画 ===

void IntroScene::render()
{
    if (m_background)
    {
        m_background->render(m_renderer->GetRenderWidth(), m_renderer->GetRenderHeight());
    }

    // 引用テキスト（画面下部中央に表示）
    if (m_quoteAlpha > 0.0f)
    {
        auto size = m_deviceResources->GetOutputSize();
        float centerX = (size.right - size.left) * 0.5f;
        float centerY = (size.bottom - size.top) * 0.9f;

        UIRenderer* ui = m_renderer->GetUI();
        ui->begin();
        ui->drawTextCentered(
            m_quoteText,
            XMFLOAT2(centerX, centerY),
            Colors::White,
            m_quoteAlpha,
            FontType::Quote
        );
        ui->end();
    }
}
