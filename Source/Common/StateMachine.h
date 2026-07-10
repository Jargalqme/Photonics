//---------------------------------------------------------------------------
//! @file   StateMachine.h
//! @brief  汎用ステートマシン (コールバック登録式)
//---------------------------------------------------------------------------
#pragma once
#include <functional>
#include <unordered_map>

//! 1ステート分のコールバック束 (未設定の関数はスキップされる)
struct StateCallbacks
{
    std::function<void()> enter;
    std::function<void(float)> update;
    std::function<void()> exit;
};

//===========================================================================
//! 汎用ステートマシン
//! enum 等をキーに enter/update/exit を登録し、changeState が呼び分ける
//===========================================================================
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

    //! 現ステートの exit -> 新ステートの enter (初回は enter のみ)
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
    bool m_hasState = false;    //!< changeState が一度でも呼ばれたか
};
