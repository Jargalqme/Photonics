#pragma once
#include <functional>

class BeatTracker
{
public:
    using BeatCallback = std::function<void(int beat)>;

    BeatTracker() = default;

    void update(float deltaTime);

    void reset();

    void setBPM(float bpm);
    void setSongDuration(float seconds) { m_songDuration = seconds; }
    void setStartDelay(float seconds) { m_startDelay = seconds; }
    void setBeatCallback(BeatCallback callback) { m_beatCallback = callback; }

    float getBPM() const { return m_bpm; }
    float getSongDuration() const { return m_songDuration; }
    int getBeat() const { return m_beat; }
    float getBeatProgress() const;
    float getElapsedTime() const { return m_elapsedTime; }
    bool isSongComplete() const;

    bool isOnBeat(float threshold = 0.1f) const;

private:
    void onBeat();

    // === ビート設定 ===
    static constexpr float DEFAULT_BPM           = 120.0f;
    static constexpr float DEFAULT_BEAT_INTERVAL  = 0.5f;   // 60 / 120 BPM

    float m_bpm = DEFAULT_BPM;
    float m_songDuration = 0.0f;
    float m_startDelay = 0.0f;
    bool m_active = false;

    float m_timeSinceLastBeat = 0.0f;
    float m_beatInterval = DEFAULT_BEAT_INTERVAL;
    float m_elapsedTime = 0.0f;
    int m_beat = 1;
    BeatCallback m_beatCallback;
};
