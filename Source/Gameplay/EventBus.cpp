//=============================================================================
// @brief Typed deferred gameplay event bus implementation.
//=============================================================================
#include "pch.h"
#include "Gameplay/EventBus.h"

#include <cassert>

bool EventBus::m_isDispatching = false;
std::vector<EventBus::Event> EventBus::m_currentQueue;
std::vector<EventBus::Event> EventBus::m_nextQueue;

std::vector<EventBus::Callback<DummyHitEvent>> EventBus::m_dummyHitListeners;
std::vector<EventBus::Callback<DummyDiedEvent>> EventBus::m_dummyDiedListeners;
std::vector<EventBus::Callback<PlayerDamagedEvent>> EventBus::m_playerDamagedListeners;
std::vector<EventBus::Callback<BossDamagedEvent>> EventBus::m_bossDamagedListeners;
std::vector<EventBus::Callback<BossDiedEvent>> EventBus::m_bossDiedListeners;
std::vector<EventBus::Callback<WeaponShotEvent>> EventBus::m_weaponShotListeners;
std::vector<EventBus::Callback<ShotResolvedEvent>> EventBus::m_shotResolvedListeners;
std::vector<EventBus::Callback<WaveChangedEvent>> EventBus::m_waveChangedListeners;

std::vector<EventBus::Callback<DummyHitEvent>> EventBus::m_pendingDummyHitListeners;
std::vector<EventBus::Callback<DummyDiedEvent>> EventBus::m_pendingDummyDiedListeners;
std::vector<EventBus::Callback<PlayerDamagedEvent>> EventBus::m_pendingPlayerDamagedListeners;
std::vector<EventBus::Callback<BossDamagedEvent>> EventBus::m_pendingBossDamagedListeners;
std::vector<EventBus::Callback<BossDiedEvent>> EventBus::m_pendingBossDiedListeners;
std::vector<EventBus::Callback<WeaponShotEvent>> EventBus::m_pendingWeaponShotListeners;
std::vector<EventBus::Callback<ShotResolvedEvent>> EventBus::m_pendingShotResolvedListeners;
std::vector<EventBus::Callback<WaveChangedEvent>> EventBus::m_pendingWaveChangedListeners;

void EventBus::enqueue(Event event)
{
    auto& targetQueue = m_isDispatching ? m_nextQueue : m_currentQueue;

    if (targetQueue.size() == SOFT_WARNING_THRESHOLD)
    {
        OutputDebugStringA("EventBus warning: more than 1000 events queued in one frame.\n");
    }

    if (targetQueue.size() >= HARD_EVENT_CAP)
    {
#ifdef _DEBUG
        assert(false && "EventBus hard cap exceeded.");
#else
        OutputDebugStringA("EventBus dropped event: hard cap exceeded.\n");
#endif
        return;
    }

    targetQueue.push_back(std::move(event));
}

void EventBus::dispatchQueued()
{
    std::vector<Event> eventsToDispatch;
    eventsToDispatch.swap(m_currentQueue);

    m_isDispatching = true;
    for (const auto& event : eventsToDispatch)
    {
        dispatch(event);
    }
    m_isDispatching = false;

    activatePendingSubscribers();

    if (!m_nextQueue.empty())
    {
        m_currentQueue.insert(
            m_currentQueue.end(),
            std::make_move_iterator(m_nextQueue.begin()),
            std::make_move_iterator(m_nextQueue.end()));
        m_nextQueue.clear();
    }
}

void EventBus::dispatch(const Event& event)
{
    std::visit([](const auto& typedEvent)
    {
        dispatchTyped(typedEvent);
    }, event);
}

void EventBus::activatePendingSubscribers()
{
    auto activate = [](auto& active, auto& pending)
    {
        active.insert(
            active.end(),
            std::make_move_iterator(pending.begin()),
            std::make_move_iterator(pending.end()));
        pending.clear();
    };

    activate(m_dummyHitListeners, m_pendingDummyHitListeners);
    activate(m_dummyDiedListeners, m_pendingDummyDiedListeners);
    activate(m_playerDamagedListeners, m_pendingPlayerDamagedListeners);
    activate(m_bossDamagedListeners, m_pendingBossDamagedListeners);
    activate(m_bossDiedListeners, m_pendingBossDiedListeners);
    activate(m_weaponShotListeners, m_pendingWeaponShotListeners);
    activate(m_shotResolvedListeners, m_pendingShotResolvedListeners);
    activate(m_waveChangedListeners, m_pendingWaveChangedListeners);
}

void EventBus::clear()
{
    m_isDispatching = false;
    m_currentQueue.clear();
    m_nextQueue.clear();

    m_dummyHitListeners.clear();
    m_dummyDiedListeners.clear();
    m_playerDamagedListeners.clear();
    m_bossDamagedListeners.clear();
    m_bossDiedListeners.clear();
    m_weaponShotListeners.clear();
    m_shotResolvedListeners.clear();
    m_waveChangedListeners.clear();

    m_pendingDummyHitListeners.clear();
    m_pendingDummyDiedListeners.clear();
    m_pendingPlayerDamagedListeners.clear();
    m_pendingBossDamagedListeners.clear();
    m_pendingBossDiedListeners.clear();
    m_pendingWeaponShotListeners.clear();
    m_pendingShotResolvedListeners.clear();
    m_pendingWaveChangedListeners.clear();
}
