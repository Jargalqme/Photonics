//---------------------------------------------------------------------------
//! @file   Scene.h
//! @brief  シーン基底クラス
//---------------------------------------------------------------------------
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

//===========================================================================
//! シーン基底クラス
//! ライフサイクル: initialize (登録時に1回) -> enter/exit (切替の度) -> finalize
//===========================================================================
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

    //! コンテキストの参照を控えます (オーバーライド側は必ず基底を呼ぶこと)
    virtual void initialize(SceneContext& context)
    {
        m_context = &context;
        m_deviceResources = context.device;
        m_renderer = context.renderer;
    }

    //! アクティブ化 (オーバーライド側は必ず基底を呼ぶこと。exit も同様)
    virtual void enter() { m_isActive = true; }
    virtual void exit() { m_isActive = false; }
    virtual void finalize() {}
    virtual void update(float dt, InputManager* input) = 0;
    virtual void render() = 0;

    const std::string& getName() const { return m_name; }
    bool isActive() const { return m_isActive; }

protected:
    std::string m_name;                          //!< シーン名 (SceneManager の登録キーとは別)
    bool m_isActive;                             //!< enter/exit で更新 (false 中は update されない)
    SceneContext* m_context;                     //!< 共有サービス (非所有)
    DX::DeviceResources* m_deviceResources;      //!< デバイスリソース (非所有)
    Renderer* m_renderer;                        //!< レンダラ (非所有)
};
