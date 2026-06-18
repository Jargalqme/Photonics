#include "pch.h"
#include "AudioManager.h"

#include <algorithm>
#include <filesystem>

using namespace DirectX;

AudioManager::AudioManager()
    : m_rng(std::random_device{}())
{
}

AudioManager::~AudioManager()
{
    finalize();
}

// === 初期化・終了 ===

bool AudioManager::initialize()
{
    if (m_initialized)
    {
        return true;
    }

    AUDIO_ENGINE_FLAGS flags = AudioEngine_Default;

#ifdef _DEBUG
    flags |= AudioEngine_Debug;
#endif

    try
    {
        m_audioEngine = std::make_unique<AudioEngine>(flags);
    }
    catch (const std::exception& e)
    {
        // オーディオデバイスなし — 無音で続行可能
        OutputDebugStringA("AudioManager: Failed to create engine - ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        return false;
    }

    m_initialized = true;
    OutputDebugStringA("AudioManager: Initialized successfully\n");
    return true;
}

void AudioManager::update()
{
    if (!m_audioEngine)
    {
        return;
    }

    // デバイスロスト検出（ヘッドホン抜き等）
    if (m_audioEngine->IsCriticalError())
    {
        OutputDebugStringA("AudioManager: Critical error, attempting reset\n");
    }

    m_audioEngine->Update();
}

void AudioManager::finalize()
{
    if (!m_initialized)
    {
        return;
    }

    // BGM停止
    if (m_currentMusic)
    {
        m_currentMusic->Stop();
        m_currentMusic.reset();
    }

    // サウンドデータ解放
    m_sounds.clear();
    m_soundGroups.clear();
    m_lastGroupVariant.clear();
    m_music.clear();

    // エンジン解放
    m_audioEngine.reset();

    m_initialized = false;
    OutputDebugStringA("AudioManager: Shutdown complete\n");
}

// === 読み込み ===

bool AudioManager::loadSound(const std::string& name, const std::wstring& filepath)
{
    if (!m_audioEngine)
    {
        return false;
    }

    try
    {
        m_sounds[name] = std::make_unique<SoundEffect>(m_audioEngine.get(), filepath.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA("AudioManager: Failed to load sound - ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        return false;
    }
}

bool AudioManager::loadSoundGroupFromDirectory(
    const std::string& groupName,
    const std::wstring& directory,
    const std::wstring& filenamePrefix)
{
    if (!m_audioEngine)
    {
        return false;
    }

    namespace fs = std::filesystem;

    std::vector<fs::path> files;
    const fs::path directoryPath(directory);
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath))
    {
        return false;
    }

    for (const auto& entry : fs::directory_iterator(directoryPath))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const fs::path path = entry.path();
        const std::wstring extension = path.extension().wstring();
        if (extension != L".wav" && extension != L".WAV")
        {
            continue;
        }

        const std::wstring filename = path.filename().wstring();
        if (!filenamePrefix.empty() && filename.rfind(filenamePrefix, 0) != 0)
        {
            continue;
        }

        files.push_back(path);
    }

    std::sort(files.begin(), files.end());

    auto& group = m_soundGroups[groupName];
    group.clear();

    for (const auto& path : files)
    {
        const std::string soundName = groupName + "_" + path.stem().string();
        if (loadSound(soundName, path.wstring()))
        {
            group.push_back(soundName);
        }
    }

    if (group.empty())
    {
        m_soundGroups.erase(groupName);
        m_lastGroupVariant.erase(groupName);
        return false;
    }

    m_lastGroupVariant.erase(groupName);
    return true;
}

bool AudioManager::addSoundToGroup(const std::string& groupName, const std::string& soundName)
{
    if (m_sounds.find(soundName) == m_sounds.end())
    {
        return false;
    }

    auto& group = m_soundGroups[groupName];
    if (std::find(group.begin(), group.end(), soundName) == group.end())
    {
        group.push_back(soundName);
    }

    m_lastGroupVariant.erase(groupName);
    return true;
}

bool AudioManager::loadMusic(const std::string& name, const std::wstring& filepath)
{
    if (!m_audioEngine)
    {
        return false;
    }

    try
    {
        m_music[name] = std::make_unique<SoundEffect>(m_audioEngine.get(), filepath.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA("AudioManager: Failed to load music - ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        return false;
    }
}

// === 再生 ===

void AudioManager::playSound(const std::string& name, float volume, float pitch, float pan)
{
    if (!m_audioEngine)
    {
        return;
    }

    auto it = m_sounds.find(name);
    if (it == m_sounds.end())
    {
        OutputDebugStringA("AudioManager: Sound not found - ");
        OutputDebugStringA(name.c_str());
        OutputDebugStringA("\n");
        return;
    }

    // Fire and Forget — DirectXTKが内部でインスタンス管理
    it->second->Play(volume * m_masterVolume, pitch, pan);
}

void AudioManager::playRandomSound(
    const std::string& groupName,
    float volume,
    float pitchJitter,
    float volumeJitter)
{
    auto groupIt = m_soundGroups.find(groupName);
    if (groupIt == m_soundGroups.end() || groupIt->second.empty())
    {
        OutputDebugStringA("AudioManager: Sound group not found - ");
        OutputDebugStringA(groupName.c_str());
        OutputDebugStringA("\n");
        return;
    }

    const auto& group = groupIt->second;
    std::uniform_int_distribution<size_t> variantDist(0, group.size() - 1);
    size_t variant = variantDist(m_rng);

    auto lastIt = m_lastGroupVariant.find(groupName);
    if (group.size() > 1 && lastIt != m_lastGroupVariant.end() && variant == lastIt->second)
    {
        variant = (variant + 1) % group.size();
    }
    m_lastGroupVariant[groupName] = variant;

    float finalPitch = 0.0f;
    if (pitchJitter > 0.0f)
    {
        std::uniform_real_distribution<float> pitchDist(-pitchJitter, pitchJitter);
        finalPitch = pitchDist(m_rng);
    }

    float finalVolume = volume;
    if (volumeJitter > 0.0f)
    {
        std::uniform_real_distribution<float> volumeDist(1.0f - volumeJitter, 1.0f);
        finalVolume *= volumeDist(m_rng);
    }

    playSound(group[variant], finalVolume, finalPitch, 0.0f);
}

void AudioManager::playMusic(const std::string& name, bool loop)
{
    if (!m_audioEngine)
    {
        return;
    }

    auto it = m_music.find(name);
    if (it == m_music.end())
    {
        OutputDebugStringA("AudioManager: Music not found - ");
        OutputDebugStringA(name.c_str());
        OutputDebugStringA("\n");
        return;
    }

    // 現在のBGMを停止
    stopMusic();

    // インスタンス作成して再生
    m_currentMusic = it->second->CreateInstance();
    m_currentMusicName = name;

    m_currentMusic->SetVolume(m_masterVolume);
    m_currentMusic->Play(loop);
}

void AudioManager::stopMusic()
{
    if (m_currentMusic)
    {
        m_currentMusic->Stop();
        m_currentMusic.reset();
        m_currentMusicName.clear();
    }
}

// === 音量 ===

void AudioManager::setMasterVolume(float volume)
{
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));

    if (m_currentMusic)
    {
        m_currentMusic->SetVolume(m_masterVolume);
    }
}
