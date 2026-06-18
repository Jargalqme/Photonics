#pragma once

#include "Audio.h"
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    // コピー禁止（オーディオシステムは一つだけ）
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    bool initialize();

    void update();

    void finalize();

    bool loadSound(const std::string& name, const std::wstring& filepath);
    bool loadSoundGroupFromDirectory(
        const std::string& groupName,
        const std::wstring& directory,
        const std::wstring& filenamePrefix = L"");
    bool addSoundToGroup(const std::string& groupName, const std::string& soundName);

    bool loadMusic(const std::string& name, const std::wstring& filepath);
    void playSound(const std::string& name, float volume = 1.0f, float pitch = 0.0f, float pan = 0.0f);
    void playRandomSound(
        const std::string& groupName,
        float volume = 1.0f,
        float pitchJitter = 0.0f,
        float volumeJitter = 0.0f);
    void playMusic(const std::string& name, bool loop = true);

    void stopMusic();

    void setMasterVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }
    float* getMasterVolumePtr() { return &m_masterVolume; }

    bool isInitialized() const { return m_initialized; }

private:
    // エンジン（最後に破棄）
    std::unique_ptr<DirectX::AudioEngine> m_audioEngine;

    // サウンドデータ
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> m_sounds;
    std::unordered_map<std::string, std::vector<std::string>> m_soundGroups;
    std::unordered_map<std::string, size_t> m_lastGroupVariant;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> m_music;

    // 現在再生中のBGMインスタンス（最初に破棄）
    std::unique_ptr<DirectX::SoundEffectInstance> m_currentMusic;
    std::string m_currentMusicName;

    static constexpr float DEFAULT_MASTER_VOLUME = 0.8f;
    float m_masterVolume = DEFAULT_MASTER_VOLUME;
    bool m_initialized = false;
    std::mt19937 m_rng;
};
