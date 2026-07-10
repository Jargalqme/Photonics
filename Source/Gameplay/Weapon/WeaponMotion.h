//---------------------------------------------------------------------------
//! @file   WeaponMotion.h
//! @brief  武器の手続きモーション (ADS・リコイル・ボブ・スウェイ)
//---------------------------------------------------------------------------
#pragma once
#include "Common/Spring.h"
#include "SimpleMath.h"

//! 1フレーム分のモーション入力
struct WeaponMotionInput
{
	float deltaTime          { 0.0f  };
	float moveSpeed          { 0.0f  };    //!< 水平移動速度 (ワールド単位/秒)
	bool  grounded           { true  };    //!< 接地中 (空中はボブ停止)
	bool  isAiming           { false };
	Vector2 lookDeltaDegrees {};           //!< 実際に適用された視点差分 (度)
};

//! モーション出力 (カメラローカル姿勢: +X右, +Y上, +Z前方)
struct WeaponMotionOutput
{
	Vector3 position {};    //!< 位置オフセット
	Vector3 rotation {};    //!< 回転 (度)
};

//! モーションのチューニング値 (DebugUI スライダ直結)
struct WeaponMotionTuning
{
	Vector3 hipPosition   { 0.30f, -0.25f, 0.70f };     //!< 腰だめ時のカメラローカル位置
	Vector3 adsPosition   { 0.00f, -0.085f, 0.35f };    //!< ADS時のカメラローカル位置
	float   adsBlendSpeed { 10.0f };                    //!< 腰だめ <-> ADS の補間速度

	// リコイル - 発射時のバネへのインパルス
	float recoilKickback{ -8.0f };			//!< Z後退の初速 (負 = 手前へ)
	float recoilPitchDeg{ -24.0f };			//!< ピッチ跳ね上げの初速 (負 = 上向き)

	// ボブ - 歩行によるサイン波の揺れ
	float bobFrequency { 8.0f };			//!< 揺れの速さ (rad/s)
	float bobAmplitudeX{ 0.02f };			//!< 横揺れ幅
	float bobAmplitudeY{ 0.015f };			//!< 縦揺れ幅
	float bobFalloff{ 5.0f };				//!< 停止時の減衰速度

	// スウェイ - 視点移動に武器が遅れて追従
	float swayPositionGain{ 0.003f };		//!< 視点1度あたりの位置ドリフト
	float swayRotationGain{ 0.18f };		//!< 視点1度あたりの傾き (度)
	float swayReturnSpeed{ 12.0f };			//!< 追従で戻る速さ
	float swayMaxDeltaDeg{ 20.0f };			//!< 入力視点差分のクランプ (度)
};

//===========================================================================
//! 武器の手続きモーション
//! 基本姿勢 (ADSブレンド) + リコイル + ボブ + スウェイ を毎フレーム加算合成
//===========================================================================
class WeaponMotion
{
public:
	//! 4要素を計算して加算合成します
	void update(const WeaponMotionInput& input);

	//! リコイルのバネへキックを与えます (発射したフレームに呼ぶ)
	void onFire();

	//! 全状態を初期姿勢へ戻します
	void reset();

	WeaponMotionOutput  getMotionOutput() const { return m_composed; }
	WeaponMotionTuning* getMotionTuningPtr()    { return &m_tuning; }

private:
	WeaponMotionOutput computeBasePose(float deltaTime, bool isAiming);
	WeaponMotionOutput computeRecoil(float deltaTime);
	WeaponMotionOutput computeBob(const WeaponMotionInput& input);
	WeaponMotionOutput computeSway(const WeaponMotionInput& input);

	Spring1D m_recoilZ{};                                 //!< Z後退バネ
	Spring1D m_recoilP{.zeta = 0.6f, .omega = 30.0f};     //!< ピッチバネ (硬め・速い収束)

	float m_adsAlpha  = 0.0f;     //!< ADSブレンド係数 (0 = 腰だめ, 1 = ADS)
	float m_bobPhase  = 0.0f;     //!< ボブの位相 (rad)
	float m_bobAmount = 0.0f;     //!< ボブの強さ (0-1, 移動速度へ追従)

	Vector3 m_swayPosition = {};    //!< スウェイの現在位置オフセット
	Vector3 m_swayRotation = {};    //!< スウェイの現在傾き (度)

	WeaponMotionTuning m_tuning;      //!< チューニング値 (DebugUI 直結)
	WeaponMotionOutput m_composed;    //!< 合成結果 (getMotionOutput で返す)
};
