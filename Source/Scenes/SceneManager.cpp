#include "pch.h"
#include "Scenes/SceneManager.h"
#include "Scenes/Scene.h"
#include "Render/Pipeline/Renderer.h"
#include "Services/SceneContext.h"
#include <cassert>

SceneManager::SceneManager()
    : m_context(nullptr)
    , m_activeScene(nullptr)
{
}

SceneManager::~SceneManager()
{
    finalize();
}

// === 初期化・終了 ===

void SceneManager::initialize(SceneContext& context)
{
    m_context = &context;
}

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

// === シーン管理 ===

void SceneManager::addScene(const std::string& name, std::unique_ptr<Scene> scene)
{
    assert(scene != nullptr && "Cannot add null scene");
    assert(m_context != nullptr && "SceneManager::initialize must be called before addScene");

    scene->initialize(*m_context);
    m_scenes[name] = std::move(scene);
}

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

// === フェード遷移 ===

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

// === 更新・描画 ===

void SceneManager::update(float deltaTime, InputManager* input)
{
    updateTransition(deltaTime);

    if (m_activeScene && m_activeScene->isActive())
    {
        m_activeScene->update(deltaTime, input);
    }
}

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

Scene* SceneManager::getScene(const std::string& name) const
{
    auto it = m_scenes.find(name);
    return (it != m_scenes.end()) ? it->second.get() : nullptr;
}

bool SceneManager::hasScene(const std::string& name) const
{
    return m_scenes.find(name) != m_scenes.end();
}
