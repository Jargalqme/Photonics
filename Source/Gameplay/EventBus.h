//=============================================================================
// @brief Typed deferred gameplay event bus.
//=============================================================================
#pragma once

#include "Gameplay/EventTypes.h"

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

class EventBus
{
public:
    template <typename EventT>
    using Callback = std::function<void(const EventT&)>;

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

    template <typename EventT>
    static void publish(const EventT& event)
    {
        enqueue(Event{ event });
    }

    static void dispatchQueued();
    static void clear();

private:
    using Event = std::variant<
        DummyHitEvent,
        DummyDiedEvent,
        PlayerDamagedEvent,
        BossDamagedEvent,
        BossDiedEvent,
        WeaponShotEvent,
        ShotResolvedEvent,
        WaveChangedEvent>;

    static constexpr size_t SOFT_WARNING_THRESHOLD = 1000;
    static constexpr size_t HARD_EVENT_CAP = 10000;

    static void enqueue(Event event);
    static void dispatch(const Event& event);
    static void activatePendingSubscribers();

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

    template <typename EventT>
    static void dispatchTyped(const EventT& event)
    {
        for (const auto& callback : listeners<EventT>())
        {
            callback(event);
        }
    }

    static bool m_isDispatching;
    static std::vector<Event> m_currentQueue;
    static std::vector<Event> m_nextQueue;

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
