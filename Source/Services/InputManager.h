#pragma once

#include <Keyboard.h>
#include <Mouse.h>
#include <SimpleMath.h>
#include <memory>

class InputManager
{
public:
    InputManager();
    ~InputManager() = default;

    void initialize(HWND window);
    void update();

    bool isKeyDown     (DirectX::Keyboard::Keys key) const;
    bool isKeyPressed  (DirectX::Keyboard::Keys key) const;
    bool isKeyReleased (DirectX::Keyboard::Keys key) const;

    DirectX::SimpleMath::Vector2 getMousePosition() const;
    DirectX::SimpleMath::Vector2 getMouseDelta();

    bool isLeftMouseDown     () const;
    bool isRightMouseDown    () const;
    bool isMiddleMouseDown   () const;
    bool isLeftMousePressed  () const;
    bool isRightMousePressed () const;

    void setCursorVisible(bool visible);
    bool isCursorVisible() const { return m_cursorVisible; }
    bool isSystemCursorVisible() const;

    void setMouseMode(DirectX::Mouse::Mode mode);

private:
    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse>    m_mouse;
    std::unique_ptr<DirectX::Keyboard::KeyboardStateTracker> m_keyboardTracker;
    std::unique_ptr<DirectX::Mouse::ButtonStateTracker>      m_mouseTracker;

    DirectX::Keyboard::State m_keyboardState;
    DirectX::Mouse::State    m_mouseState;
    DirectX::Mouse::State    m_prevMouseState;

    DirectX::Mouse::Mode m_currentMouseMode = DirectX::Mouse::MODE_RELATIVE;
    bool m_cursorVisible         = false;
    bool m_discardNextMouseDelta = false;
};
