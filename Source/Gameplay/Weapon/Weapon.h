//---------------------------------------------------------------------------
//! @file   Weapon.h
//! @brief  武器の戦闘状態 (発射レート・弾数・リロード)
//---------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>
#include <vector>

struct WeaponShot;

//===========================================================================
//! 武器 (ヒットスキャンライフル)
//! 発射インテントを WeaponShot として積むだけ。命中判定は CombatSystem
//===========================================================================
class Weapon
{
public:
    //! 弾数・タイマーを初期状態に戻します
    void initialize();

    //! 発射タイマー・リロードを進め、発射条件が揃えば WeaponShot を積む
    //! @return このフレームに発射したら true (リコイル起動などの契機)
    bool update(
        float deltaTime,
        const DirectX::SimpleMath::Vector3& hitScanOrigin,
        const DirectX::SimpleMath::Vector3& hitScanDirection,
        const DirectX::SimpleMath::Vector3& tracerStart,
        std::vector<WeaponShot>& outShots);

    //----------------------------------------------------------
    //! @name   発射・リロード操作
    //----------------------------------------------------------
    //!@{

    //! トリガーを引きます (押しっぱなしで連射)
    void startFire();

    //! トリガーを離します
    void stopFire();

    //! 発射可能かを取得します (レート・残弾・リロード中でない)
    bool canFire() const;

    //! トリガー状態を取得します
    bool isFiring() const { return m_firing; }

    //! リロードを開始します (満タン時・リロード中は無視)
    void reload();

    //! リロードを中断します
    void cancelReload();

    //! リロード中かを取得します
    bool isReloading() const { return m_reloading; }

    //!@}
    //----------------------------------------------------------
    //! @name   取得
    //----------------------------------------------------------
    //!@{

    //! 残弾数を取得します
    int getAmmoCount() const { return m_ammo; }

    //! マガジン容量を取得します
    int getClipSize() const { return m_clipSize; }

    //! 弾切れかを取得します
    bool outOfAmmo() const { return m_ammo <= 0; }

    //!@}

private:
    //! WeaponShot を生成して積む
    bool shoot(
        const DirectX::SimpleMath::Vector3& hitScanOrigin,
        const DirectX::SimpleMath::Vector3& hitScanDirection,
        const DirectX::SimpleMath::Vector3& tracerStart,
        std::vector<WeaponShot>& outShots);

    // チューニング値（旧 WeaponRifle の設定 — 武器はライフル1種のみ）
    int m_ammo = 0;                  //!< 残弾数
    int m_clipSize = 30;             //!< マガジン容量
    float m_fireInterval = 0.15f;    //!< 発射間隔 (秒)
    float m_nextShotTime = 0.0f;     //!< 次弾発射までの残り秒数
    float m_reloadDuration = 1.5f;   //!< リロード所要時間 (秒)
    float m_reloadTimer = 0.0f;      //!< リロード残り秒数
    float m_damage = 2.5f;           //!< 1発のダメージ
    float m_maxRange = 150.0f;       //!< ヒットスキャン最大距離
    bool m_firing = false;           //!< トリガー押下中
    bool m_reloading = false;        //!< リロード中
};
