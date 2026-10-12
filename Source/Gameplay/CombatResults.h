//---------------------------------------------------------------------------
//! @file   CombatResults.h
//! @brief  1フレーム分の戦闘結果 (CombatSystem が積み、シーンが演出へ変換する)
//---------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>
#include <vector>

class ICombatTarget;

//! 命中1件分 (誰に当たり、この命中で死亡したか)
//! 位置は持たない - 演出位置はシーンが被弾者本人から取る (ポインタ比較で判別)
struct CombatHitResult
{
    ICombatTarget* target = nullptr;    //!< 被弾者 (非所有)
    bool died = false;                  //!< この命中で死亡した
};

//! トレーサー1本分 (色は演出値のためシーン側が持つ)
struct TracerLine
{
    DirectX::SimpleMath::Vector3 start = DirectX::SimpleMath::Vector3::Zero;    //!< 始点 (銃口)
    DirectX::SimpleMath::Vector3 end = DirectX::SimpleMath::Vector3::Zero;      //!< 終点 (着弾点 / 未命中時は最大射程点)
};

//===========================================================================
//! 1フレーム分の戦闘結果
//! CombatSystem::update が冒頭でクリアして積み、resolveBullets が追記する。
//! シーンは戦闘解決の後に results() で読み、演出 (VFX・SE・シェイク) へ変換する
//===========================================================================
struct CombatResults
{
    std::vector<CombatHitResult> hits;    //!< 適用された命中 (無敵・死亡中で無視された命中は含まない)
    std::vector<TracerLine> tracers;      //!< 発射1発につき1本 (命中の有無を問わず)

    void clear()
    {
        hits.clear();
        tracers.clear();
    }
};
