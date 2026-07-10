//---------------------------------------------------------------------------
//! @file   AudioManager.h
//! @brief  オーディオ管理 (SE・SEグループ・BGM)
//---------------------------------------------------------------------------
#pragma once

#include "Audio.h"
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

//===========================================================================
//! オーディオ管理 (DirectXTK Audio の薄いラッパ)
//! SE は fire-and-forget、BGM は単一インスタンスを差し替え。
//! グループ = ランダム再生用のバリエーション束。
//! initialize 失敗 (デバイスなし) でも無音で続行できる
//===========================================================================
class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    // コピー禁止（オーディオシステムは一つだけ）
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    bool initialize();

    //! エンジン内部の更新 (フレームに1回呼ぶ)
    void update();

    void finalize();

    //!@}
    //----------------------------------------------------------
    //! @name   読み込み
    //----------------------------------------------------------
    //!@{

    bool loadSound(const std::string& name, const std::wstring& filepath);

    //! ディレクトリから prefix 一致の .wav をまとめてグループ登録 (名前順)
    bool loadSoundGroupFromDirectory(
        const std::string& groupName,
        const std::wstring& directory,
        const std::wstring& filenamePrefix = L"");

    //! 読込済みの単発SEをグループへ追加します (フォールバック登録用)
    bool addSoundToGroup(const std::string& groupName, const std::string& soundName);

    bool loadMusic(const std::string& name, const std::wstring& filepath);

    //!@}
    //----------------------------------------------------------
    //! @name   再生
    //----------------------------------------------------------
    //!@{

    //! SE を fire-and-forget 再生します (インスタンス管理は TK 任せ)
    void playSound(const std::string& name, float volume = 1.0f, float pitch = 0.0f, float pan = 0.0f);

    //! グループからランダム再生 (直前と同じ変種は避ける)。jitter でピッチ・音量を揺らす
    void playRandomSound(
        const std::string& groupName,
        float volume = 1.0f,
        float pitchJitter = 0.0f,
        float volumeJitter = 0.0f);

    //! BGM を再生します (再生中の曲は停止して差し替え)
    void playMusic(const std::string& name, bool loop = true);

    void stopMusic();

    //!@}
    //----------------------------------------------------------
    //! @name   音量
    //----------------------------------------------------------
    //!@{

    void setMasterVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }

    //! DebugUI スライダ直結用 (変更後は setMasterVolume で BGM へ反映)
    float* getMasterVolumePtr() { return &m_masterVolume; }

    bool isInitialized() const { return m_initialized; }

    //!@}

private:
    // エンジン（宣言順先頭 = 破棄は最後）
    std::unique_ptr<DirectX::AudioEngine> m_audioEngine;

    // サウンドデータ
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> m_sounds;
    std::unordered_map<std::string, std::vector<std::string>> m_soundGroups;    //!< グループ名 -> SE名リスト
    std::unordered_map<std::string, size_t> m_lastGroupVariant;                 //!< 直前に鳴らした変種 (連続再生回避)
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> m_music;

    // 現在再生中のBGMインスタンス（エンジンより先に破棄される位置に置く）
    std::unique_ptr<DirectX::SoundEffectInstance> m_currentMusic;
    std::string m_currentMusicName;

    static constexpr float DEFAULT_MASTER_VOLUME = 0.8f;
    float m_masterVolume = DEFAULT_MASTER_VOLUME;
    bool m_initialized = false;
    std::mt19937 m_rng;    //!< 変種・ジッター用乱数
};
