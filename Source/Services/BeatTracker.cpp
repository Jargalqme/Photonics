#include "pch.h"
#include "BeatTracker.h"

// === 更新 ===

void BeatTracker::update(float deltaTime)
{
    if (m_bpm <= 0.0f)
    {
        return;
    }

    m_elapsedTime += deltaTime;

    // 開始ディレイ中はビートをカウントしない
    if (m_elapsedTime < m_startDelay)
    {
        return;
    }

    // 開始マーク
    if (!m_active)
    {
        m_active = true;
        m_timeSinceLastBeat = 0.0f;
    }

    m_timeSinceLastBeat += deltaTime;

    while (m_timeSinceLastBeat >= m_beatInterval)
    {
        m_timeSinceLastBeat -= m_beatInterval;
        onBeat();
    }
}

void BeatTracker::reset()
{
    m_timeSinceLastBeat = 0.0f;
    m_elapsedTime = 0.0f;
    m_beat = 1;
    m_active = false;
}

void BeatTracker::setBPM(float bpm)
{
    if (bpm > 0.0f)
    {
        m_bpm = bpm;
        m_beatInterval = 60.0f / m_bpm;
    }
}

// === クエリ ===

float BeatTracker::getBeatProgress() const
{
    if (m_beatInterval <= 0.0f)
    {
        return 0.0f;
    }
    return m_timeSinceLastBeat / m_beatInterval;
}

bool BeatTracker::isOnBeat(float threshold) const
{
    float timeToNext = m_beatInterval - m_timeSinceLastBeat;
    return (m_timeSinceLastBeat < threshold) || (timeToNext < threshold);
}

bool BeatTracker::isSongComplete() const
{
    return m_songDuration > 0.0f && m_elapsedTime >= m_songDuration;
}

void BeatTracker::onBeat()
{
    m_beat++;

    if (m_beatCallback)
    {
        m_beatCallback(m_beat);
    }
}
