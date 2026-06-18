#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <stack>

class Scene;
class InputManager;
class Renderer;
struct SceneContext;
namespace DX { class DeviceResources; }

class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    void initialize(SceneContext& context);

    void finalize();

    void addScene(const std::string& name, std::unique_ptr<Scene> scene);

    void removeScene(const std::string& name);

    void setActiveScene(const std::string& name);

    void pushScene(const std::string& name);

    void popScene();

    void transitionTo(const std::string& sceneName, float duration = 0.5f);

    bool isTransitioning() const { return m_fadingOut || m_fadingIn; }

    void update(float deltaTime, InputManager* input);

    void render();

    Scene* getActiveScene() const { return m_activeScene; }
    Scene* getScene(const std::string& name) const;
    bool hasScene(const std::string& name) const;

private:
    SceneContext* m_context;
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
    Scene* m_activeScene;
    std::stack<Scene*> m_sceneStack;

    // === フェード遷移 ===
    static constexpr float DEFAULT_FADE_DURATION = 0.5f;
    float m_fadeAlpha = 0.0f;
    float m_fadeDuration = DEFAULT_FADE_DURATION;
    bool m_fadingOut = false;
    bool m_fadingIn = false;
    std::string m_pendingScene;

    void updateTransition(float deltaTime);
};
