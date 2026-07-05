//---------------------------------------------------------------------------
//! @file   StepTimer.h
//! @brief  経過時間を提供するシンプルなタイマー
//---------------------------------------------------------------------------
#pragma once

#include <cmath>
#include <cstdint>
#include <exception>


namespace DX
{
    // アニメーション・シミュレーションの時間管理ヘルパークラス
    class StepTimer
    {
    public:
        StepTimer() noexcept(false) :
            m_elapsedTicks(0),
            m_totalTicks(0),
            m_leftOverTicks(0),
            m_frameCount(0),
            m_framesPerSecond(0),
            m_framesThisSecond(0),
            m_qpcSecondCounter(0),
            m_isFixedTimeStep(false),
            m_targetElapsedTicks(TicksPerSecond / 60)
        {
            if (!QueryPerformanceFrequency(&m_qpcFrequency))
            {
                throw std::exception();
            }

            if (!QueryPerformanceCounter(&m_qpcLastTime))
            {
                throw std::exception();
            }

            // 最大デルタ時間を 1/10 秒に初期化
            m_qpcMaxDelta = static_cast<uint64_t>(m_qpcFrequency.QuadPart / 10);
        }

        // 前回の Update 呼び出しからの経過時間を取得
        uint64_t GetElapsedTicks() const noexcept { return m_elapsedTicks; }
        double GetElapsedSeconds() const noexcept { return TicksToSeconds(m_elapsedTicks); }

        // プログラム開始からの合計時間を取得
        uint64_t GetTotalTicks() const noexcept { return m_totalTicks; }
        double GetTotalSeconds() const noexcept { return TicksToSeconds(m_totalTicks); }

        // プログラム開始からの更新回数を取得
        uint32_t GetFrameCount() const noexcept { return m_frameCount; }

        // 現在のフレームレートを取得
        uint32_t GetFramesPerSecond() const noexcept { return m_framesPerSecond; }

        // 固定 / 可変タイムステップモードを設定
        void SetFixedTimeStep(bool isFixedTimestep) noexcept { m_isFixedTimeStep = isFixedTimestep; }

        // 固定タイムステップモード時の Update 呼び出し間隔を設定
        void SetTargetElapsedTicks(uint64_t targetElapsed) noexcept { m_targetElapsedTicks = targetElapsed; }
        void SetTargetElapsedSeconds(double targetElapsed) noexcept { m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

        // 整数形式では 1 秒 = 10,000,000 ティックとして時間を表す
        static constexpr uint64_t TicksPerSecond = 10000000;

        static constexpr double TicksToSeconds(uint64_t ticks) noexcept { return static_cast<double>(ticks) / TicksPerSecond; }
        static constexpr uint64_t SecondsToTicks(double seconds) noexcept { return static_cast<uint64_t>(seconds * TicksPerSecond); }

        // 意図的な時間の不連続（ブロッキング IO など）の後に呼ぶことで、
        // 固定タイムステップ処理が遅れを取り戻そうと Update を連打するのを防ぐ

        void ResetElapsedTime()
        {
            if (!QueryPerformanceCounter(&m_qpcLastTime))
            {
                throw std::exception();
            }

            m_leftOverTicks = 0;
            m_framesPerSecond = 0;
            m_framesThisSecond = 0;
            m_qpcSecondCounter = 0;
        }

        // タイマー状態を更新し、指定の Update 関数を適切な回数呼び出す
        template<typename TUpdate>
        void Tick(const TUpdate& update)
        {
            // 現在時刻を取得
            LARGE_INTEGER currentTime;

            if (!QueryPerformanceCounter(&currentTime))
            {
                throw std::exception();
            }

            uint64_t timeDelta = static_cast<uint64_t>(currentTime.QuadPart - m_qpcLastTime.QuadPart);

            m_qpcLastTime = currentTime;
            m_qpcSecondCounter += timeDelta;

            // 過大なデルタ時間をクランプ（デバッガーで停止した後など）
            if (timeDelta > m_qpcMaxDelta)
            {
                timeDelta = m_qpcMaxDelta;
            }

            // QPC 単位を正規のティック形式へ変換。直前のクランプによりオーバーフローしない
            timeDelta *= TicksPerSecond;
            timeDelta /= static_cast<uint64_t>(m_qpcFrequency.QuadPart);

            const uint32_t lastFrameCount = m_frameCount;

            if (m_isFixedTimeStep)
            {
                // 固定タイムステップ更新

                // 経過時間が目標値に十分近い（1/4 ミリ秒以内）場合は、時計を目標値ぴったりに
                // クランプする。これは微小で無意味な誤差の蓄積を防ぐため。このクランプが
                // 無いと、60 fps の固定更新を要求したゲームを 59.94 Hz（NTSC）のディスプレイで
                // VSync 有効のまま動かした場合、僅かな誤差が積もっていずれフレーム落ちする。
                // 小さなズレは 0 に丸めて、滑らかに動かし続ける方がよい。

                if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(timeDelta - m_targetElapsedTicks))) < TicksPerSecond / 4000)
                {
                    timeDelta = m_targetElapsedTicks;
                }

                m_leftOverTicks += timeDelta;

                while (m_leftOverTicks >= m_targetElapsedTicks)
                {
                    m_elapsedTicks = m_targetElapsedTicks;
                    m_totalTicks += m_targetElapsedTicks;
                    m_leftOverTicks -= m_targetElapsedTicks;
                    m_frameCount++;

                    update();
                }
            }
            else
            {
                // 可変タイムステップ更新
                m_elapsedTicks = timeDelta;
                m_totalTicks += timeDelta;
                m_leftOverTicks = 0;
                m_frameCount++;

                update();
            }

            // フレームレートを集計
            if (m_frameCount != lastFrameCount)
            {
                m_framesThisSecond++;
            }

            if (m_qpcSecondCounter >= static_cast<uint64_t>(m_qpcFrequency.QuadPart))
            {
                m_framesPerSecond = m_framesThisSecond;
                m_framesThisSecond = 0;
                m_qpcSecondCounter %= static_cast<uint64_t>(m_qpcFrequency.QuadPart);
            }
        }

    private:
        // 元の計時データ（QPC 単位）
        LARGE_INTEGER m_qpcFrequency;
        LARGE_INTEGER m_qpcLastTime;
        uint64_t m_qpcMaxDelta;

        // 変換後の計時データ（正規ティック形式）
        uint64_t m_elapsedTicks;
        uint64_t m_totalTicks;
        uint64_t m_leftOverTicks;

        // フレームレート集計用
        uint32_t m_frameCount;
        uint32_t m_framesPerSecond;
        uint32_t m_framesThisSecond;
        uint64_t m_qpcSecondCounter;

        // 固定タイムステップモード設定用
        bool m_isFixedTimeStep;
        uint64_t m_targetElapsedTicks;
    };
}
