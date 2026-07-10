//---------------------------------------------------------------------------
//! @file   InputManager.cpp
//! @brief  入力管理 (キーボード・マウス)
//---------------------------------------------------------------------------
#include "pch.h"
#include "InputManager.h"

using DirectX::Keyboard;
using DirectX::Mouse;
using DirectX::SimpleMath::Vector2;

//---------------------------------------------------------------------------
//! コンストラクタ
//---------------------------------------------------------------------------
InputManager::InputManager()
    : m_keyboard(std::make_unique<Keyboard>())
    , m_mouse(std::make_unique<Mouse>())
    , m_keyboardTracker(std::make_unique<Keyboard::KeyboardStateTracker>())
    , m_mouseTracker(std::make_unique<Mouse::ButtonStateTracker>())
    , m_keyboardState{}
    , m_mouseState{}
    , m_prevMouseState{}
{
}

//---------------------------------------------------------------------------
//! ウィンドウを関連付け、相対モード (カーソル非表示) で開始
//---------------------------------------------------------------------------
void InputManager::initialize(HWND window)
{
    m_mouse->SetWindow(window);
    setCursorVisible(false);
}

//---------------------------------------------------------------------------
//! 状態スナップショットとエッジトラッカーを更新します
//---------------------------------------------------------------------------
void InputManager::update()
{
    m_prevMouseState = m_mouseState;

    m_keyboardState = m_keyboard->GetState();
    m_mouseState = m_mouse->GetState();

    m_keyboardTracker->Update(m_keyboardState);
    m_mouseTracker->Update(m_mouseState);
}

bool InputManager::isKeyDown(Keyboard::Keys key) const
{
    return m_keyboardState.IsKeyDown(key);
}

bool InputManager::isKeyPressed(Keyboard::Keys key) const
{
    return m_keyboardTracker->IsKeyPressed(key);
}

bool InputManager::isKeyReleased(Keyboard::Keys key) const
{
    return m_keyboardTracker->IsKeyReleased(key);
}

Vector2 InputManager::getMousePosition() const
{
    return Vector2(
        static_cast<float>(m_mouseState.x),
        static_cast<float>(m_mouseState.y));
}

//---------------------------------------------------------------------------
//! 視点用の移動量を取得します
//---------------------------------------------------------------------------
Vector2 InputManager::getMouseDelta()
{
    // モード切替直後は座標系が変わりゴミ差分が出るため1回捨てる
    if (m_discardNextMouseDelta)
    {
        m_discardNextMouseDelta = false;
        return Vector2::Zero;
    }

    // 相対モードでは state.x/y が差分そのもの
    if (m_currentMouseMode == Mouse::MODE_RELATIVE)
    {
        return Vector2(
            static_cast<float>(m_mouseState.x),
            static_cast<float>(m_mouseState.y));
    }

    return Vector2(
        static_cast<float>(m_mouseState.x - m_prevMouseState.x),
        static_cast<float>(m_mouseState.y - m_prevMouseState.y));
}

bool InputManager::isLeftMouseDown() const
{
    return m_mouseState.leftButton;
}

bool InputManager::isRightMouseDown() const
{
    return m_mouseState.rightButton;
}

bool InputManager::isMiddleMouseDown() const
{
    return m_mouseState.middleButton;
}

bool InputManager::isLeftMousePressed() const
{
    return m_mouseTracker->leftButton == Mouse::ButtonStateTracker::PRESSED;
}

bool InputManager::isRightMousePressed() const
{
    return m_mouseTracker->rightButton == Mouse::ButtonStateTracker::PRESSED;
}

//---------------------------------------------------------------------------
//! カーソル表示とマウスモードを連動切替します
//---------------------------------------------------------------------------
void InputManager::setCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    setMouseMode(visible ? Mouse::MODE_ABSOLUTE : Mouse::MODE_RELATIVE);
}

bool InputManager::isSystemCursorVisible() const
{
    return m_mouse ? m_mouse->IsVisible() : false;
}

//---------------------------------------------------------------------------
//! マウスモードを切り替えます (直後の1フレームの差分は捨てる)
//---------------------------------------------------------------------------
void InputManager::setMouseMode(Mouse::Mode mode)
{
    m_currentMouseMode = mode;
    m_discardNextMouseDelta = true;
    m_mouse->SetMode(mode);
}
