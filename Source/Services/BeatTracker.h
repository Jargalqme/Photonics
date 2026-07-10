//---------------------------------------------------------------------------
//! @file   BeatTracker.h
//! @brief  音楽ビート追跡
//---------------------------------------------------------------------------
#pragma once
#include <functional>

//===========================================================================
//! 音楽ビート追跡
//! BPM から拍タイミングを計算し、拍毎にコールバックを呼ぶ
//! (音声解析ではなく時計ベース — 曲の BPM と開始ディレイを手動設定)
//===========================================================================
class BeatTracker
{
public:
    using BeatCallback = std::function<void(int beat)>;

    BeatTracker() = default;

    //! 経過時間を進め、拍を跨いだ回数だけコールバックを発火
    void update(float deltaTime);

    //! 曲の頭から仕切り直します (曲再生と同時に呼ぶ)
    void reset();

    //----------------------------------------------------------
    //! @name   設定
    //----------------------------------------------------------
    //!@{

    void setBPM(float bpm);
    void setSongDuration(float seconds) { m_songDuration = seconds; }

    //! 曲頭の無音・導入部をスキップする秒数
    void setStartDelay(float seconds) { m_startDelay = seconds; }

    void setBeatCallback(BeatCallback callback) { m_beatCallback = callback; }

    //!@}
    //----------------------------------------------------------
    //! @name   取得
    //----------------------------------------------------------
    //!@{

    float getBPM() const { return m_bpm; }
    float getSongDuration() const { return m_songDuration; }
    int getBeat() const { return m_beat; }

    //! 拍内の進行度 0-1 を取得します (グリッドパルス等の演出用)
    float getBeatProgress() const;

    float getElapsedTime() const { return m_elapsedTime; }
    bool isSongComplete() const;

    //! 拍の前後 threshold 秒以内かを取得します (リズム入力判定用)
    bool isOnBeat(float threshold = 0.1f) const;

    //!@}

private:
    void onBeat();

    // ビート設定
    static constexpr float DEFAULT_BPM           = 120.0f;
    static constexpr float DEFAULT_BEAT_INTERVAL  = 0.5f;   //!< 60 / 120 BPM

    float m_bpm = DEFAULT_BPM;
    float m_songDuration = 0.0f;      //!< 曲の長さ (秒, 0 = 完了判定なし)
    float m_startDelay = 0.0f;        //!< ビート開始までの導入秒数
    bool m_active = false;            //!< ディレイ明けフラグ

    float m_timeSinceLastBeat = 0.0f; //!< 直前の拍からの経過秒数
    float m_beatInterval = DEFAULT_BEAT_INTERVAL;    //!< 拍間隔 (秒) = 60 / BPM
    float m_elapsedTime = 0.0f;       //!< reset からの総経過秒数
    int m_beat = 1;                   //!< 現在の拍番号 (1始まり)
    BeatCallback m_beatCallback;
};
