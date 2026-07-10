//---------------------------------------------------------------------------
//! @file   SceneManager.h
//! @brief  シーン管理 (登録・切替・フェード遷移)
//---------------------------------------------------------------------------
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

//===========================================================================
//! シーン管理
//! 名前 -> シーンの登録テーブル。切替時に exit/enter を呼び分け、
//! transitionTo は黒フェードを挟んで切り替える
//===========================================================================
class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    //! コンテキストを控えます (addScene より先に呼ぶこと)
    void initialize(SceneContext& context);

    //! 全シーンを finalize して破棄します
    void finalize();

    //!@}
    //----------------------------------------------------------
    //! @name   シーン登録・切替
    //----------------------------------------------------------
    //!@{

    //! 登録と同時に scene->initialize を呼びます
    void addScene(const std::string& name, std::unique_ptr<Scene> scene);

    void removeScene(const std::string& name);

    //! 即時切替 (スタックはクリア)。フェード無し版
    void setActiveScene(const std::string& name);

    //! 現シーンをスタックへ退避して切替します
    void pushScene(const std::string& name);

    //! スタックの一番上のシーンへ戻ります
    void popScene();

    //! 黒フェードを挟んで切り替えます (遷移中の再要求は無視)
    void transitionTo(const std::string& sceneName, float duration = 0.5f);

    bool isTransitioning() const { return m_fadingOut || m_fadingIn; }

    //!@}
    //----------------------------------------------------------
    //! @name   更新・描画
    //----------------------------------------------------------
    //!@{

    //! フェード進行 + アクティブシーンの更新
    void update(float deltaTime, InputManager* input);

    //! アクティブシーンの描画 + フェードオーバーレイ
    void render();

    //!@}
    //----------------------------------------------------------
    //! @name   取得
    //----------------------------------------------------------
    //!@{

    Scene* getActiveScene() const { return m_activeScene; }
    Scene* getScene(const std::string& name) const;
    bool hasScene(const std::string& name) const;

    //!@}

private:
    SceneContext* m_context;                                     //!< 共有サービス (非所有)
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;    //!< 登録テーブル (所有)
    Scene* m_activeScene;                                        //!< 現在のシーン (非所有)
    std::stack<Scene*> m_sceneStack;                             //!< push/pop 用の退避スタック

    // フェード遷移
    static constexpr float DEFAULT_FADE_DURATION = 0.5f;
    float m_fadeAlpha = 0.0f;                     //!< 0 = 透明, 1 = 完全暗転
    float m_fadeDuration = DEFAULT_FADE_DURATION; //!< 片道の秒数
    bool m_fadingOut = false;                     //!< 暗転中
    bool m_fadingIn = false;                      //!< 明転中
    std::string m_pendingScene;                   //!< 暗転完了後に切り替える先

    void updateTransition(float deltaTime);
};
