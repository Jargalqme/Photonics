//---------------------------------------------------------------------------
//! @file   SceneManager.cpp
//! @brief  シーン管理 (登録・切替・フェード遷移)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Scenes/SceneManager.h"
#include "Scenes/Scene.h"
#include "Render/Pipeline/Renderer.h"
#include "Services/SceneContext.h"
#include <cassert>

//---------------------------------------------------------------------------
//! コンストラクタ
//---------------------------------------------------------------------------
SceneManager::SceneManager()
    : m_context(nullptr)
    , m_activeScene(nullptr)
{
}

//---------------------------------------------------------------------------
//! デストラクタ (finalize 忘れの保険)
//---------------------------------------------------------------------------
SceneManager::~SceneManager()
{
    finalize();
}

//===========================================================================
// 初期化・終了
//===========================================================================

//---------------------------------------------------------------------------
//! コンテキストを控えます
//---------------------------------------------------------------------------
void SceneManager::initialize(SceneContext& context)
{
    m_context = &context;
}

//---------------------------------------------------------------------------
//! 全シーンを finalize して破棄します
//---------------------------------------------------------------------------
void SceneManager::finalize()
{
    // アクティブシーンを終了
    if (m_activeScene)
    {
        m_activeScene->exit();
        m_activeScene = nullptr;
    }

    // 全シーンを終了処理
    for (auto& pair : m_scenes)
    {
        pair.second->finalize();
    }

    m_scenes.clear();

    while (!m_sceneStack.empty())
    {
        m_sceneStack.pop();
    }
}

//===========================================================================
// シーン登録・切替
//===========================================================================

//---------------------------------------------------------------------------
//! 登録と同時に scene->initialize を呼びます
//---------------------------------------------------------------------------
void SceneManager::addScene(const std::string& name, std::unique_ptr<Scene> scene)
{
    assert(scene != nullptr && "Cannot add null scene");
    assert(m_context != nullptr && "SceneManager::initialize must be called before addScene");

    scene->initialize(*m_context);
    m_scenes[name] = std::move(scene);
}

//---------------------------------------------------------------------------
//! シーンを登録から外して破棄します (アクティブなら先に exit)
//---------------------------------------------------------------------------
void SceneManager::removeScene(const std::string& name)
{
    auto it = m_scenes.find(name);
    if (it != m_scenes.end())
    {
        if (m_activeScene == it->second.get())
        {
            m_activeScene->exit();
            m_activeScene = nullptr;
        }

        it->second->finalize();
        m_scenes.erase(it);
    }
}

//---------------------------------------------------------------------------
//! 即時切替 (フェード無し)
//---------------------------------------------------------------------------
void SceneManager::setActiveScene(const std::string& name)
{
    auto it = m_scenes.find(name);
    if (it != m_scenes.end())
    {
        // 現在のシーンを終了
        if (m_activeScene)
        {
            m_activeScene->exit();
        }

        // 直接切替時はスタックをクリア
        while (!m_sceneStack.empty())
        {
            m_sceneStack.pop();
        }

        // 新シーンを開始
        m_activeScene = it->second.get();
        m_activeScene->enter();
    }
}

//---------------------------------------------------------------------------
//! 現シーンをスタックへ退避して切替します
//---------------------------------------------------------------------------
void SceneManager::pushScene(const std::string& name)
{
    auto it = m_scenes.find(name);
    if (it != m_scenes.end())
    {
        // 現在のシーンを一時停止してスタックへ
        if (m_activeScene)
        {
            m_activeScene->exit();
            m_sceneStack.push(m_activeScene);
        }

        m_activeScene = it->second.get();
        m_activeScene->enter();
    }
}

//---------------------------------------------------------------------------
//! スタックの一番上のシーンへ戻ります
//---------------------------------------------------------------------------
void SceneManager::popScene()
{
    if (!m_sceneStack.empty())
    {
        if (m_activeScene)
        {
            m_activeScene->exit();
        }

        // 前のシーンに戻る
        m_activeScene = m_sceneStack.top();
        m_sceneStack.pop();

        if (m_activeScene)
        {
            m_activeScene->enter();
        }
    }
}

//===========================================================================
// フェード遷移
//===========================================================================

//---------------------------------------------------------------------------
//! 黒フェードを挟んで切り替えます (遷移中の再要求は無視)
//---------------------------------------------------------------------------
void SceneManager::transitionTo(const std::string& sceneName, float duration)
{
    // 遷移中は新しい遷移を開始しない
    if (m_fadingOut || m_fadingIn)
    {
        return;
    }

    if (!hasScene(sceneName))
    {
        return;
    }

    m_pendingScene = sceneName;
    m_fadeDuration = duration;
    m_fadingOut = true;
    m_fadeAlpha = 0.0f;
}

//---------------------------------------------------------------------------
//! フェードを進めます (完全暗転の瞬間にシーン切替)
//---------------------------------------------------------------------------
void SceneManager::updateTransition(float deltaTime)
{
    if (m_fadingOut)
    {
        // 暗転へ
        m_fadeAlpha += deltaTime / m_fadeDuration;
        if (m_fadeAlpha >= 1.0f)
        {
            m_fadeAlpha = 1.0f;

            // 完全に暗転した状態でシーン切替
            setActiveScene(m_pendingScene);
            m_pendingScene.clear();

            m_fadingOut = false;
            m_fadingIn = true;
        }
    }
    else if (m_fadingIn)
    {
        // 明転へ
        m_fadeAlpha -= deltaTime / m_fadeDuration;
        if (m_fadeAlpha <= 0.0f)
        {
            m_fadeAlpha = 0.0f;
            m_fadingIn = false;
        }
    }
}

//===========================================================================
// 更新・描画
//===========================================================================

//---------------------------------------------------------------------------
//! フェード進行 + アクティブシーンの更新
//---------------------------------------------------------------------------
void SceneManager::update(float deltaTime, InputManager* input)
{
    updateTransition(deltaTime);

    if (m_activeScene && m_activeScene->isActive())
    {
        m_activeScene->update(deltaTime, input);
    }
}

//---------------------------------------------------------------------------
//! アクティブシーンの描画 + フェードオーバーレイ
//---------------------------------------------------------------------------
void SceneManager::render()
{
    if (m_activeScene)
    {
        m_activeScene->render();
    }

    // フェードオーバーレイ描画
    if (m_fadeAlpha > 0.0f)
    {
        UIRenderer* ui = m_context->renderer->GetUI();
        ui->begin();
        ui->drawRect(ui->fullscreenRect(), DirectX::Colors::Black, m_fadeAlpha);
        ui->end();
    }
}

//---------------------------------------------------------------------------
//! 名前でシーンを取得します (未登録なら nullptr)
//---------------------------------------------------------------------------
Scene* SceneManager::getScene(const std::string& name) const
{
    auto it = m_scenes.find(name);
    return (it != m_scenes.end()) ? it->second.get() : nullptr;
}

//---------------------------------------------------------------------------
//! シーンが登録済みかを取得します
//---------------------------------------------------------------------------
bool SceneManager::hasScene(const std::string& name) const
{
    return m_scenes.find(name) != m_scenes.end();
}
