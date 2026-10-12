//---------------------------------------------------------------------------
//! @file   StateMachine.h
//! @brief  汎用ステートマシン (コールバック登録式)
//!
//! Game Programming Patterns「State」章を意図的に変形したもの。
//! State クラス階層ではなく enum キー + コールバック束のテーブルで
//! 同じ enter/update/exit 分離を実現する (書籍コードの移植ではない)
//---------------------------------------------------------------------------
#pragma once
#include <cassert>
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
    //! 未登録ステートへの遷移はアサート (operator[] だと空ステートが無言で挿入されるため find で検証)
    void changeState(T newState)
    {
        const auto next = m_states.find(newState);
        assert(next != m_states.end() && "StateMachine: changeState to unregistered state");
        if (next == m_states.end())
        {
            return;    // Release では現ステート維持 (exit 済みの宙ぶらりんを作らない)
        }

        if (m_hasState)
        {
            const auto current = m_states.find(m_currentState);
            if (current != m_states.end() && current->second.exit)
            {
                current->second.exit();
            }
        }

        m_currentState = newState;
        m_hasState = true;

        if (next->second.enter)
        {
            next->second.enter();
        }
    }

    void update(float dt)
    {
        if (!m_hasState)
        {
            return;
        }

        const auto it = m_states.find(m_currentState);
        if (it != m_states.end() && it->second.update)
        {
            it->second.update(dt);
        }
    }

    //! 初回 changeState 前の呼び出しはアサート (T{} を黙って返すと enum の先頭値と区別できない)
    T getCurrentState() const
    {
        assert(m_hasState && "StateMachine: getCurrentState before first changeState");
        return m_currentState;
    }

    bool hasState() const { return m_hasState; }

private:
    std::unordered_map<T, StateCallbacks> m_states;
    T m_currentState{};
    bool m_hasState = false;    //!< changeState が一度でも呼ばれたか
};
