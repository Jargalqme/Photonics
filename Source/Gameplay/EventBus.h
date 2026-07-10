//---------------------------------------------------------------------------
//! @file   EventBus.h
//! @brief  型付き遅延イベントバス
//---------------------------------------------------------------------------
#pragma once

#include "Gameplay/EventTypes.h"

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

//===========================================================================
//! 型付き遅延イベントバス (static)
//! publish は積むだけ -> シーンが dispatchQueued で一括配信。
//! 配信中に積まれたイベントは次回配信、配信中の subscribe は配信後に有効化
//===========================================================================
class EventBus
{
public:
    template <typename EventT>
    using Callback = std::function<void(const EventT&)>;

    //! 購読を登録します (配信中はイテレータ無効化を避けるため保留リストへ)
    template <typename EventT>
    static void subscribe(Callback<EventT> callback)
    {
        if (m_isDispatching)
        {
            pendingListeners<EventT>().push_back(std::move(callback));
            return;
        }

        listeners<EventT>().push_back(std::move(callback));
    }

    //! イベントを積みます (即時配信ではない)
    template <typename EventT>
    static void publish(const EventT& event)
    {
        enqueue(Event{ event });
    }

    //! 積まれたイベントを一括配信します (フレームに1回呼ぶ)
    static void dispatchQueued();

    //! 全キュー・全購読を破棄します (シーン遷移時)
    static void clear();

private:
    //! 対応イベント一覧 (追加時は listeners / pendingListeners の分岐と .cpp の実体定義にも追加)
    using Event = std::variant<
        DummyHitEvent,
        DummyDiedEvent,
        PlayerDamagedEvent,
        BossDamagedEvent,
        BossDiedEvent,
        WeaponShotEvent,
        ShotResolvedEvent,
        WaveChangedEvent>;

    static constexpr size_t SOFT_WARNING_THRESHOLD = 1000;     //!< 超過で警告ログ (詰まりの早期発見)
    static constexpr size_t HARD_EVENT_CAP = 10000;            //!< 超過でイベント破棄 (暴走ガード)

    static void enqueue(Event event);
    static void dispatch(const Event& event);
    static void activatePendingSubscribers();

    //! イベント型 -> 対応するリスナー配列へのマップ (コンパイル時分岐)
    template <typename EventT>
    static std::vector<Callback<EventT>>& listeners()
    {
        if constexpr (std::is_same_v<EventT, DummyHitEvent>)
        {
            return m_dummyHitListeners;
        }
        else if constexpr (std::is_same_v<EventT, DummyDiedEvent>)
        {
            return m_dummyDiedListeners;
        }
        else if constexpr (std::is_same_v<EventT, PlayerDamagedEvent>)
        {
            return m_playerDamagedListeners;
        }
        else if constexpr (std::is_same_v<EventT, BossDamagedEvent>)
        {
            return m_bossDamagedListeners;
        }
        else if constexpr (std::is_same_v<EventT, BossDiedEvent>)
        {
            return m_bossDiedListeners;
        }
        else if constexpr (std::is_same_v<EventT, WeaponShotEvent>)
        {
            return m_weaponShotListeners;
        }
        else if constexpr (std::is_same_v<EventT, ShotResolvedEvent>)
        {
            return m_shotResolvedListeners;
        }
        else if constexpr (std::is_same_v<EventT, WaveChangedEvent>)
        {
            return m_waveChangedListeners;
        }
        else
        {
            static_assert(sizeof(EventT) == 0, "Unsupported EventBus event type");
        }
    }

    //! listeners() の保留リスト版 (配信中の subscribe 先)
    template <typename EventT>
    static std::vector<Callback<EventT>>& pendingListeners()
    {
        if constexpr (std::is_same_v<EventT, DummyHitEvent>)
        {
            return m_pendingDummyHitListeners;
        }
        else if constexpr (std::is_same_v<EventT, DummyDiedEvent>)
        {
            return m_pendingDummyDiedListeners;
        }
        else if constexpr (std::is_same_v<EventT, PlayerDamagedEvent>)
        {
            return m_pendingPlayerDamagedListeners;
        }
        else if constexpr (std::is_same_v<EventT, BossDamagedEvent>)
        {
            return m_pendingBossDamagedListeners;
        }
        else if constexpr (std::is_same_v<EventT, BossDiedEvent>)
        {
            return m_pendingBossDiedListeners;
        }
        else if constexpr (std::is_same_v<EventT, WeaponShotEvent>)
        {
            return m_pendingWeaponShotListeners;
        }
        else if constexpr (std::is_same_v<EventT, ShotResolvedEvent>)
        {
            return m_pendingShotResolvedListeners;
        }
        else if constexpr (std::is_same_v<EventT, WaveChangedEvent>)
        {
            return m_pendingWaveChangedListeners;
        }
        else
        {
            static_assert(sizeof(EventT) == 0, "Unsupported EventBus event type");
        }
    }

    //! 型が確定したイベントを全リスナーへ配ります
    template <typename EventT>
    static void dispatchTyped(const EventT& event)
    {
        for (const auto& callback : listeners<EventT>())
        {
            callback(event);
        }
    }

    static bool m_isDispatching;                //!< 配信中フラグ (この間の publish/subscribe は保留扱い)
    static std::vector<Event> m_currentQueue;   //!< 次の dispatchQueued で配信する分
    static std::vector<Event> m_nextQueue;      //!< 配信中に積まれた分 (次回へ持ち越し)

    static std::vector<Callback<DummyHitEvent>> m_dummyHitListeners;
    static std::vector<Callback<DummyDiedEvent>> m_dummyDiedListeners;
    static std::vector<Callback<PlayerDamagedEvent>> m_playerDamagedListeners;
    static std::vector<Callback<BossDamagedEvent>> m_bossDamagedListeners;
    static std::vector<Callback<BossDiedEvent>> m_bossDiedListeners;
    static std::vector<Callback<WeaponShotEvent>> m_weaponShotListeners;
    static std::vector<Callback<ShotResolvedEvent>> m_shotResolvedListeners;
    static std::vector<Callback<WaveChangedEvent>> m_waveChangedListeners;

    static std::vector<Callback<DummyHitEvent>> m_pendingDummyHitListeners;
    static std::vector<Callback<DummyDiedEvent>> m_pendingDummyDiedListeners;
    static std::vector<Callback<PlayerDamagedEvent>> m_pendingPlayerDamagedListeners;
    static std::vector<Callback<BossDamagedEvent>> m_pendingBossDamagedListeners;
    static std::vector<Callback<BossDiedEvent>> m_pendingBossDiedListeners;
    static std::vector<Callback<WeaponShotEvent>> m_pendingWeaponShotListeners;
    static std::vector<Callback<ShotResolvedEvent>> m_pendingShotResolvedListeners;
    static std::vector<Callback<WaveChangedEvent>> m_pendingWaveChangedListeners;
};
