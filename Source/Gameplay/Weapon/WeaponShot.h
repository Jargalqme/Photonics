//---------------------------------------------------------------------------
//! @file   WeaponShot.h
//! @brief  発射インテント (武器が積み、CombatSystem が解決して空にする)
//---------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>

//! ヒットスキャン1発分のデータ
struct WeaponShot
{
    DirectX::SimpleMath::Vector3 hitScanOrigin;      //!< レイ原点 (目線位置)
    DirectX::SimpleMath::Vector3 hitScanDirection;   //!< レイ方向 (正規化済み)
    DirectX::SimpleMath::Vector3 tracerStart;        //!< トレーサー描画の始点 (銃口位置)
    float damage;                                    //!< 1発のダメージ
    float maxRange;                                  //!< 最大到達距離
    DirectX::SimpleMath::Vector4 tracerColor;        //!< トレーサーのHDR色
};
