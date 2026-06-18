//=============================================================================
// @brief Typed gameplay event payloads.
//=============================================================================
#pragma once

#include <SimpleMath.h>

struct DummyHitEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
};

struct DummyDiedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
};

struct PlayerDamagedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
    float damage = 0.0f;
    float health = 0.0f;
    float maxHealth = 0.0f;
};

struct BossDamagedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
    float damage = 0.0f;
    float health = 0.0f;
    float maxHealth = 0.0f;
};

struct BossDiedEvent
{
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
};

struct WeaponShotEvent
{
};

struct ShotResolvedEvent
{
    DirectX::SimpleMath::Vector3 tracerStart = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector3 hitPoint = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector4 color = DirectX::SimpleMath::Vector4::Zero;
};

struct WaveChangedEvent
{
    int waveNumber = 0;
};
