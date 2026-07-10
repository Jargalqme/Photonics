//---------------------------------------------------------------------------
//! @file   InputManager.h
//! @brief  入力管理 (キーボード・マウス)
//---------------------------------------------------------------------------
#pragma once

#include <Keyboard.h>
#include <Mouse.h>
#include <SimpleMath.h>
#include <memory>

//===========================================================================
//! 入力管理 (DirectXTK Keyboard / Mouse の薄いラッパ)
//! Down = 押下中、Pressed / Released = このフレームのエッジ。
//! カーソル表示とマウスモード (絶対/相対) は setCursorVisible で連動する
//===========================================================================
class InputManager
{
public:
    InputManager();
    ~InputManager() = default;

    void initialize(HWND window);

    //! 状態スナップショットとエッジトラッカーを更新します (フレームに1回)
    void update();

    //----------------------------------------------------------
    //! @name   キーボード
    //----------------------------------------------------------
    //!@{

    bool isKeyDown     (DirectX::Keyboard::Keys key) const;
    bool isKeyPressed  (DirectX::Keyboard::Keys key) const;
    bool isKeyReleased (DirectX::Keyboard::Keys key) const;

    //!@}
    //----------------------------------------------------------
    //! @name   マウス
    //----------------------------------------------------------
    //!@{

    //! カーソル位置を取得します (絶対モードのみ有効)
    DirectX::SimpleMath::Vector2 getMousePosition() const;

    //! 視点用の移動量を取得します (モード切替直後の1フレームは 0 を返す)
    DirectX::SimpleMath::Vector2 getMouseDelta();

    bool isLeftMouseDown     () const;
    bool isRightMouseDown    () const;
    bool isMiddleMouseDown   () const;
    bool isLeftMousePressed  () const;
    bool isRightMousePressed () const;

    //!@}
    //----------------------------------------------------------
    //! @name   カーソル・モード
    //----------------------------------------------------------
    //!@{

    //! カーソル表示とマウスモードを連動切替します (表示 = 絶対, 非表示 = 相対)
    void setCursorVisible(bool visible);

    bool isCursorVisible() const { return m_cursorVisible; }
    bool isSystemCursorVisible() const;

    void setMouseMode(DirectX::Mouse::Mode mode);

    //!@}

private:
    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse>    m_mouse;
    std::unique_ptr<DirectX::Keyboard::KeyboardStateTracker> m_keyboardTracker;    //!< Pressed/Released のエッジ検出
    std::unique_ptr<DirectX::Mouse::ButtonStateTracker>      m_mouseTracker;

    DirectX::Keyboard::State m_keyboardState;
    DirectX::Mouse::State    m_mouseState;
    DirectX::Mouse::State    m_prevMouseState;    //!< 絶対モードの差分計算用

    DirectX::Mouse::Mode m_currentMouseMode = DirectX::Mouse::MODE_RELATIVE;
    bool m_cursorVisible         = false;    //!< 論理カーソル状態 (表示 = 絶対モード)
    bool m_discardNextMouseDelta = false;    //!< モード切替直後のゴミ差分を1回捨てる
};
