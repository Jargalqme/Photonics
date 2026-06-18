#include "pch.h"
#include "InputManager.h"

using DirectX::Keyboard;
using DirectX::Mouse;
using DirectX::SimpleMath::Vector2;

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

void InputManager::initialize(HWND window)
{
    m_mouse->SetWindow(window);
    setCursorVisible(false);
}

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

Vector2 InputManager::getMouseDelta()
{
    if (m_discardNextMouseDelta)
    {
        m_discardNextMouseDelta = false;
        return Vector2::Zero;
    }

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

void InputManager::setCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    setMouseMode(visible ? Mouse::MODE_ABSOLUTE : Mouse::MODE_RELATIVE);
}

bool InputManager::isSystemCursorVisible() const
{
    return m_mouse ? m_mouse->IsVisible() : false;
}

void InputManager::setMouseMode(Mouse::Mode mode)
{
    m_currentMouseMode = mode;
    m_discardNextMouseDelta = true;
    m_mouse->SetMode(mode);
}
