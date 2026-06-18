#pragma once

#include <string>
#include "Services/SceneContext.h"

// 前方宣言
class InputManager;
class Renderer;
class Camera;

namespace DX {
    class DeviceResources;
}

/// シーン基底クラス
class Scene
{
public:
    Scene(const std::string& name)
        : m_name(name)
        , m_isActive(false)
        , m_context(nullptr)
        , m_deviceResources(nullptr)
        , m_renderer(nullptr)
    {
    }
    virtual ~Scene() = default;

    virtual void initialize(SceneContext& context)
    {
        m_context = &context;
        m_deviceResources = context.device;
        m_renderer = context.renderer;
    }

    virtual void enter() { m_isActive = true; }
    virtual void exit() { m_isActive = false; }
    virtual void finalize() {}
    virtual void update(float dt, InputManager* input) = 0;
    virtual void render() = 0;

    const std::string& getName() const { return m_name; }
    bool isActive() const { return m_isActive; }

protected:
    std::string m_name;
    bool m_isActive;
    SceneContext* m_context;
    DX::DeviceResources* m_deviceResources;
    Renderer* m_renderer;
};
