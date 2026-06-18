#pragma once
#include <functional>
#include <unordered_map>

struct StateCallbacks
{
    std::function<void()> enter;
    std::function<void(float)> update;
    std::function<void()> exit;
};

template <typename T>
class StateMachine
{
public:
    void addState(T state,
        std::function<void()> enterFn,
        std::function<void(float)> updateFn,
        std::function<void()> exitFn)
    {
        m_states[state] = { enterFn, updateFn, exitFn };
    }

    void changeState(T newState)
    {
        if (m_hasState && m_states[m_currentState].exit)
        {
            m_states[m_currentState].exit();
        }

        m_currentState = newState;
        m_hasState = true;

        if (m_states[m_currentState].enter)
        {
            m_states[m_currentState].enter();
        }
    }

    void update(float dt)
    {
        if (m_hasState && m_states[m_currentState].update)
        {
            m_states[m_currentState].update(dt);
        }
    }

    T getCurrentState() const { return m_currentState; }
    bool hasState() const { return m_hasState; }

private:
    std::unordered_map<T, StateCallbacks> m_states;
    T m_currentState{};
    bool m_hasState = false;
};
