//---------------------------------------------------------------------------
//! @file   EventTypes.h
//! @brief  ゲームプレイイベントのペイロード定義
//---------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>

//! ダミーが被弾した
struct DummyHitEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
};

//! ダミーが破壊された
struct DummyDiedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
};

//! プレイヤーが被弾した
struct PlayerDamagedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
    float damage = 0.0f;
    float health = 0.0f;       //!< 適用後の体力
    float maxHealth = 0.0f;
};

//! ボスが被弾した
struct BossDamagedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
    float damage = 0.0f;
    float health = 0.0f;       //!< 適用後の体力
    float maxHealth = 0.0f;
};

//! ボスが倒された
struct BossDiedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
};

//! プレイヤーが発砲した (ペイロードなし・発砲SEのトリガー)
struct WeaponShotEvent
{
};

//! ヒットスキャンが解決された (トレーサー描画用)
struct ShotResolvedEvent
{
    DirectX::SimpleMath::Vector3 tracerStart = DirectX::SimpleMath::Vector3::Zero;    //!< トレーサー始点 (銃口)
    DirectX::SimpleMath::Vector3 hitPoint = DirectX::SimpleMath::Vector3::Zero;       //!< 着弾点 (未命中時は最大射程点)
    DirectX::SimpleMath::Vector4 color = DirectX::SimpleMath::Vector4::Zero;          //!< トレーサー色 (HDR)
};

//! ウェーブが切り替わった (GameUI の表示用)
struct WaveChangedEvent
{
    int waveNumber = 0;
};
