//---------------------------------------------------------------------------
//! @file   CombatSystem.h
//! @brief  戦闘解決 (ヒットスキャン + 敵弾 vs プレイヤー)
//---------------------------------------------------------------------------
#pragma once

#include "Gameplay/ICombatTarget.h"

#include <vector>

class BulletPool;
class Player;
struct WeaponShot;

//===========================================================================
//! 戦闘解決システム
//! - ヒットスキャン: WeaponShot のレイ vs ターゲット群 (最近傍ヒット)
//! - 敵弾:           弾球 vs プレイヤーのヒット球 (ボス戦専用)
//===========================================================================
class CombatSystem
{
public:
    CombatSystem() = default;

    //! ヒットスキャンを解決 (コライダ収集 -> レイ判定 -> onHit 通知)
    void update(
        std::vector<ICombatTarget*>& shotTargets,
        std::vector<WeaponShot>& weaponShots);

    //! 敵弾はプレイヤーにしか当たらないため直接解決する (ボス戦専用)
    void resolveBullets(BulletPool& bullets, Player& player);

private:
    //! ターゲット群からヒットコライダを収集
    void collectColliders(std::vector<ICombatTarget*>& targets, std::vector<CombatHitCollider>& out);

    //! レイ vs 球で最近傍ヒットを確定し、トレーサーイベントを発行
    void resolveWeaponShots(std::vector<WeaponShot>& shots);

    std::vector<CombatHitCollider> m_shotColliders;      //!< ヒットスキャン対象 (フレーム毎に再収集)
    std::vector<CombatHitCollider> m_bulletColliders;    //!< 敵弾対象 = プレイヤー (無敵中は空)
};
