#pragma once
#include "DeviceResources.h"
#include <SimpleMath.h>
#include <vector>
#include <memory>

class BeatTracker;
class Player;
class Dummy;
class Boss;

using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
using DirectX::SimpleMath::Matrix;

class GameUI
{
public:
    GameUI();
    ~GameUI() = default;

    void initialize(DX::DeviceResources* deviceResources);

    void subscribeEvents();

    void update(float deltaTime);

    void render(const Matrix& view, const Matrix& proj);

    void setBeatTracker(BeatTracker* music) { m_beatTracker = music; }
    void setPlayer(Player* player) { m_player = player; }
    void setDummy(Dummy* dummy) { m_dummy = dummy; }
    void setWaveNumber(int wave) { m_currentWave = wave; }
    void setBoss(Boss* boss) { m_boss = boss; }
    void setShowWaveIndicator(bool show) { m_showWaveIndicator = show; }

    void triggerBeatFlash(int beat);

private:
    DX::DeviceResources* m_deviceResources = nullptr;

    // === ゲーム参照（所有しない） ===
    BeatTracker* m_beatTracker = nullptr;
    Player* m_player = nullptr;
    Dummy* m_dummy = nullptr;
    Boss* m_boss = nullptr;

    // === エッジグロー ===
    static constexpr float EDGE_GLOW_DECAY = 5.0f;
    static constexpr float BEAT_PULSE_DECAY = 8.0f;
    float m_edgeGlowAlpha = 0.0f;
    float m_beatPulseAlpha = 0.0f;

    int m_currentWave = 0;
    bool m_showWaveIndicator = true;
    ImU32 m_beatColor = IM_COL32(0, 255, 255, 255);

    // === 描画関数 ===
    void drawCrosshair();
    void drawWaveIndicator();
    void drawPlayerHealth();
    void drawBossHealth();
    void drawWeaponHUD();
    void drawDummyHealthBar(const Matrix& view, const Matrix& proj);
};
