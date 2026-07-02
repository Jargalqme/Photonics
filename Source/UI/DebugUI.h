#pragma once
#include "Common/Transform.h"

class Player;
class Grid;
class Skybox;
class AudioManager;
class BeatTracker;
class PlayerCamera;
class InputManager;
class BulletPool;
class Boss;
class Bloom;
struct SceneLighting;

class WaveSurface;

class DebugUI
{
public:
    DebugUI() = default;

    void setCamera(PlayerCamera* camera) { m_camera = camera; }
    void setLightCycle(Player* player) { m_player = player; }
    void setGrid(Grid* grid) { m_grid = grid; }
    void setSkybox(Skybox* skybox) { m_skybox = skybox; }
    void setAudioManager(AudioManager* audio) { m_audioManager = audio; }
    void setBeatTracker(BeatTracker* music) { m_beatTracker = music; }
    void setFireRatePtr(float* fireRate) { m_fireRate = fireRate; }
    void setInputManager(InputManager* input) { m_input = input; }
    void setBulletPool(BulletPool* pool) { m_bulletPool = pool; }
    void setBoss(Boss* boss) { m_boss = boss; }
    void setBloom(Bloom* bloom) { m_bloom = bloom; }
    void setExposurePtr(float* exposure) { m_exposure = exposure; }
    void setSceneLighting(SceneLighting* lighting) { m_lighting = lighting; }

    void setWaveSurface(WaveSurface* wave) { m_waveSurface = wave; }

    void render();

private:
    bool beginDebugWindow();
    void endDebugWindow();

    void drawHeader();
    void drawCameraPanel();
    void drawWeaponPanel();
    void drawRenderingPanel();
    void drawCombatPanel();
    void drawAudioPanel();
    void drawCompositionOverlay();

    PlayerCamera* m_camera = nullptr;
    Player* m_player = nullptr;
    Grid* m_grid = nullptr;
    Skybox* m_skybox = nullptr;
    AudioManager* m_audioManager = nullptr;
    BeatTracker* m_beatTracker = nullptr;
    float* m_fireRate = nullptr;
    InputManager* m_input = nullptr;
    BulletPool* m_bulletPool = nullptr;
    Boss* m_boss = nullptr;
    Bloom* m_bloom = nullptr;
    float* m_exposure = nullptr;
    SceneLighting* m_lighting = nullptr;

    bool m_showThirds = false;

    WaveSurface* m_waveSurface = nullptr;
};
