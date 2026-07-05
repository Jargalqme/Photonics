//---------------------------------------------------------------------------
//! @file   ICombatTarget.h
//! @brief  被弾側インターフェース (ヒット球の提供 + ダメージ受信)
//---------------------------------------------------------------------------
#pragma once

#include <DirectXCollision.h>
#include <SimpleMath.h>
#include <vector>

class ICombatTarget;

//! 命中部位 (部位別ダメージ倍率の識別用)
enum class HitPart
{
    Body,
    Head,
    Core,
    Armor,
    WeakPoint
};

//! ヒットコライダ (ターゲットが collectHitColliders で提出する当たり判定球)
struct CombatHitCollider
{
    ICombatTarget*           target           = nullptr;    //!< 被弾通知先 (非所有)
    HitPart                  part             = HitPart::Body;    //!< 部位
    DirectX::BoundingSphere  bounds;                         //!< 判定球 (ワールド空間)
    float                    damageMultiplier = 1.0f;        //!< 部位別ダメージ倍率
};

//! 命中結果 (onHit に渡される確定情報)
struct CombatHit
{
    HitPart                       part        = HitPart::Body;                        //!< 命中部位
    DirectX::SimpleMath::Vector3  point       = DirectX::SimpleMath::Vector3::Zero;   //!< 命中点
    DirectX::SimpleMath::Vector3  direction   = DirectX::SimpleMath::Vector3::Zero;   //!< 弾・レイの進行方向
    float                         rawDamage   = 0.0f;                                 //!< 倍率適用前ダメージ
    float                         finalDamage = 0.0f;                                 //!< 倍率適用後ダメージ
};

//===========================================================================
//! 被弾側インターフェース
//! Player / Dummy / Boss が実装。武器側ではなく「撃たれる側」の抽象
//===========================================================================
class ICombatTarget
{
public:
    virtual ~ICombatTarget() = default;

    //! 現フレームの当たり判定球を提出します (無敵中などは何も積まない)
    virtual void collectHitColliders(std::vector<CombatHitCollider>& out) = 0;

    //! 命中確定時に呼ばれます (ダメージ適用・イベント発行)
    virtual void onHit(const CombatHit& hit) = 0;
};
