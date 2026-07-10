//---------------------------------------------------------------------------
//! @file   BeatTracker.cpp
//! @brief  音楽ビート追跡
//---------------------------------------------------------------------------
#include "pch.h"
#include "BeatTracker.h"

//===========================================================================
// 更新
//===========================================================================

//---------------------------------------------------------------------------
//! 経過時間を進め、拍を跨いだ回数だけ onBeat を発火
//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
//! 曲の頭から仕切り直します
//---------------------------------------------------------------------------
void BeatTracker::reset()
{
    m_timeSinceLastBeat = 0.0f;
    m_elapsedTime = 0.0f;
    m_beat = 1;
    m_active = false;
}

//---------------------------------------------------------------------------
//! BPM を設定します (拍間隔を再計算)
//---------------------------------------------------------------------------
void BeatTracker::setBPM(float bpm)
{
    if (bpm > 0.0f)
    {
        m_bpm = bpm;
        m_beatInterval = 60.0f / m_bpm;
    }
}

//===========================================================================
// クエリ
//===========================================================================

//---------------------------------------------------------------------------
//! 拍内の進行度 0-1 を取得します
//---------------------------------------------------------------------------
float BeatTracker::getBeatProgress() const
{
    if (m_beatInterval <= 0.0f)
    {
        return 0.0f;
    }
    return m_timeSinceLastBeat / m_beatInterval;
}

//---------------------------------------------------------------------------
//! 拍の前後 threshold 秒以内かを取得します
//---------------------------------------------------------------------------
bool BeatTracker::isOnBeat(float threshold) const
{
    float timeToNext = m_beatInterval - m_timeSinceLastBeat;
    return (m_timeSinceLastBeat < threshold) || (timeToNext < threshold);
}

//---------------------------------------------------------------------------
//! 曲が終わったかを取得します (songDuration 未設定なら常に false)
//---------------------------------------------------------------------------
bool BeatTracker::isSongComplete() const
{
    return m_songDuration > 0.0f && m_elapsedTime >= m_songDuration;
}

//---------------------------------------------------------------------------
//! 拍番号を進めてコールバック通知
//---------------------------------------------------------------------------
void BeatTracker::onBeat()
{
    m_beat++;

    if (m_beatCallback)
    {
        m_beatCallback(m_beat);
    }
}
