//---------------------------------------------------------------------------
//! @file   PlayerWeapon.h
//! @brief  プレイヤー武器 (戦闘・モーション・モデルの統合)
//---------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>
#include <vector>

#include "Gameplay/Weapon/Weapon.h"
#include "Gameplay/Weapon/WeaponFrame.h"
#include "Gameplay/Weapon/WeaponMotion.h"
#include "Gameplay/Weapon/WeaponModel.h"

class RenderCommandQueue;
struct SceneContext;
struct WeaponShot;

//===========================================================================
//! プレイヤー武器
//! 戦闘 (Weapon)・モーション (WeaponMotion)・見た目 (WeaponModel) を束ね、
//! Player の1フレーム入力を各要素へ配るファサード
//===========================================================================
class PlayerWeapon
{
public:
    explicit PlayerWeapon(SceneContext& context);

    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    //! 武器モデルをロードし弾数を初期化します
    void initialize();

    void finalize();

    //! モーションと弾数を開始状態へ戻します
    void reset();

    //! 押しっぱなし入力を解除します (マウス解放時)
    void clearInputState();

    //!@}
    //----------------------------------------------------------
    //! @name   更新・描画
    //----------------------------------------------------------
    //!@{

    //! 入力反映 -> モーション更新 -> 発射判定。発射したフレームはリコイルを起動
    void update(const PlayerWeaponFrame& frame, std::vector<WeaponShot>& outShots);

    //! ビュー行列から目線カメラを復元し、モーション適用済みの武器モデルを積む
    void render(RenderCommandQueue& queue, const DirectX::SimpleMath::Matrix& view) const;

    //!@}
    //----------------------------------------------------------
    //! @name   取得 (HUD 用)
    //----------------------------------------------------------
    //!@{

    bool isReloading() const;
    int getAmmo() const;
    int getMaxAmmo() const;

    //!@}
    //----------------------------------------------------------
    //! @name   DebugUI 連携
    //----------------------------------------------------------
    //!@{

    //! モーションチューニングのスライダ直結用ポインタ
    WeaponMotionTuning* getMotionTuning() { return m_motion.getMotionTuningPtr(); }

    bool hasRifleModel() const { return m_model.hasRifle(); }
    RifleModelSettings& getRifleModelSettings() { return m_model.rifleSettings(); }
    const RifleModelSettings& getRifleModelSettings() const { return m_model.rifleSettings(); }
    void resetRifleModelSettings() { m_model.resetRifleSettings(); }

    //!@}

private:
    static constexpr const char* RIFLE_MODEL_PATH = "Assets/Weapons/Rifle/twintap.glb";    //!< 装備する武器モデル

    void applyInputToCombat(const PlayerWeaponFrame& frame);

    //! モーション出力 (カメラローカル姿勢) をカメラワールドへ合成
    DirectX::SimpleMath::Matrix buildModelRootWorld(
        const DirectX::SimpleMath::Matrix& cameraWorld) const;

    //! マズルのワールド位置を取得します (トレーサー始点用)
    DirectX::SimpleMath::Vector3 getMuzzlePosition(
        const DirectX::SimpleMath::Matrix& cameraWorld) const;

    SceneContext* m_context = nullptr;
    Weapon m_combat;          //!< 戦闘状態 (発射レート・弾数・リロード)
    WeaponMotion m_motion;    //!< 手続きモーション (ADS・リコイル・ボブ・スウェイ)
    WeaponModel m_model;      //!< 一人称モデル (ビューモデル)
};
