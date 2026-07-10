//---------------------------------------------------------------------------
//! @file   GameUI.h
//! @brief  ゲームHUD (ImGui 直描き)
//---------------------------------------------------------------------------
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

//===========================================================================
//! ゲームHUD
//! 照準・体力バー・弾数・ウェーブ表示を ImGui ForegroundDrawList へ直描き
//! (コンポジット後 = ウィンドウ座標系・トーンマップ外)。参照は全て非所有
//===========================================================================
class GameUI
{
public:
    GameUI();
    ~GameUI() = default;

    void initialize(DX::DeviceResources* deviceResources);

    //! イベント購読を登録します (EventBus::clear 後にシーンの enter から呼ぶ)
    void subscribeEvents();

    //! グロー・パルスの減衰を進めます
    void update(float deltaTime);

    //! HUD 一式を描画します (view/proj はワールド空間バーの投影用)
    void render(const Matrix& view, const Matrix& proj);

    void setBeatTracker(BeatTracker* music) { m_beatTracker = music; }
    void setPlayer(Player* player) { m_player = player; }
    void setDummy(Dummy* dummy) { m_dummy = dummy; }
    void setWaveNumber(int wave) { m_currentWave = wave; }
    void setBoss(Boss* boss) { m_boss = boss; }
    void setShowWaveIndicator(bool show) { m_showWaveIndicator = show; }

    //! ビートフラッシュを起動します (奇数拍 = シアン, 偶数拍 = マゼンタ)
    void triggerBeatFlash(int beat);

private:
    DX::DeviceResources* m_deviceResources = nullptr;

    // ゲーム参照（所有しない）
    BeatTracker* m_beatTracker = nullptr;
    Player* m_player = nullptr;
    Dummy* m_dummy = nullptr;
    Boss* m_boss = nullptr;

    // エッジグロー
    static constexpr float EDGE_GLOW_DECAY = 5.0f;      //!< グローの減衰速度 (/秒)
    static constexpr float BEAT_PULSE_DECAY = 8.0f;     //!< パルスの減衰速度 (/秒)
    float m_edgeGlowAlpha = 0.0f;
    float m_beatPulseAlpha = 0.0f;

    int m_currentWave = 0;              //!< WaveChangedEvent で更新
    bool m_showWaveIndicator = true;    //!< トレーニングでは非表示
    ImU32 m_beatColor = IM_COL32(0, 255, 255, 255);

    // 描画関数
    void drawCrosshair();
    void drawWaveIndicator();
    void drawPlayerHealth();
    void drawBossHealth();
    void drawWeaponHUD();

    //! 敵頭上のワールド空間体力バー (既知バグ: 非16:9 でズレる — fix-it リスト参照)
    void drawDummyHealthBar(const Matrix& view, const Matrix& proj);
};
