//---------------------------------------------------------------------------
//! @file   Weapon.cpp
//! @brief  武器の戦闘状態 (発射レート・弾数・リロード)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Gameplay/Weapon/Weapon.h"
#include "Gameplay/Weapon/WeaponShot.h"
#include "Gameplay/EventBus.h"
#include "Gameplay/EventTypes.h"

//---------------------------------------------------------------------------
//! 弾数・タイマーを初期状態に戻します
//---------------------------------------------------------------------------
void Weapon::initialize()
{
    m_ammo = m_clipSize;
    m_nextShotTime = 0.0f;
    m_reloadTimer = 0.0f;
    m_firing = false;
    m_reloading = false;
}

//---------------------------------------------------------------------------
//! 発射タイマー・リロードを進め、発射条件が揃えば WeaponShot を積む
//---------------------------------------------------------------------------
bool Weapon::update(
    float deltaTime,
    const Vector3& hitScanOrigin,
    const Vector3& hitScanDirection,
    const Vector3& tracerStart,
    std::vector<WeaponShot>& outShots)
{
    if (m_nextShotTime > 0.0f)
    {
        m_nextShotTime -= deltaTime;
    }

    if (m_reloading)
    {
        m_reloadTimer -= deltaTime;
        if (m_reloadTimer <= 0.0f)
        {
            m_ammo = m_clipSize;
            m_reloading = false;
            m_reloadTimer = 0.0f;
        }
    }

    if (m_firing && canFire())
    {
        if (shoot(hitScanOrigin, hitScanDirection, tracerStart, outShots))
        {
            m_nextShotTime = m_fireInterval;
            --m_ammo;
            EventBus::publish(WeaponShotEvent{});
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------
//! トリガーを引きます
//---------------------------------------------------------------------------
void Weapon::startFire()
{
    m_firing = true;
}

//---------------------------------------------------------------------------
//! トリガーを離します
//---------------------------------------------------------------------------
void Weapon::stopFire()
{
    m_firing = false;
}

//---------------------------------------------------------------------------
//! 発射可能かを取得します
//---------------------------------------------------------------------------
bool Weapon::canFire() const
{
    return m_nextShotTime <= 0.0f && m_ammo > 0 && !m_reloading;
}

//---------------------------------------------------------------------------
//! リロードを開始します
//---------------------------------------------------------------------------
void Weapon::reload()
{
    if (m_reloading)
    {
        return;
    }

    if (m_ammo >= m_clipSize)
    {
        return;
    }

    m_reloading = true;
    m_reloadTimer = m_reloadDuration;
}

//---------------------------------------------------------------------------
//! リロードを中断します
//---------------------------------------------------------------------------
void Weapon::cancelReload()
{
    m_reloading = false;
    m_reloadTimer = 0.0f;
}

//---------------------------------------------------------------------------
//! WeaponShot を生成して積む
//---------------------------------------------------------------------------
bool Weapon::shoot(
    const Vector3& hitScanOrigin,
    const Vector3& hitScanDirection,
    const Vector3& tracerStart,
    std::vector<WeaponShot>& outShots)
{
    outShots.push_back(WeaponShot{
        hitScanOrigin,
        hitScanDirection,
        tracerStart,
        m_damage,
        m_maxRange,
        Vector4(2.0f, 4.25f, 5.0f, 1.0f),
    });

    return true;
}
