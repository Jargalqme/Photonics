//---------------------------------------------------------------------------
//! @file	PlayerCamera.h
//! @brief	プレイヤーカメラ (追従・エイムFOV・シェイク・ビューモデル投影・露出)
//---------------------------------------------------------------------------
#pragma once

#include "Common/Camera.h"
#include "ThirdParty/FastNoiseLite.h"

class Player;

//===========================================================================
//! プレイヤーカメラ
//! Player の視点に追従し、ADS の FOV ブレンドとノイズシェイクを乗せる
//===========================================================================
class PlayerCamera : public Camera
{
public:

	PlayerCamera(DX::DeviceResources* deviceResources);

	//! 視点追従 -> FOV ブレンド -> シェイク -> 行列再構築の順で更新します
	void update(const Player& player, float deltaTime);

	//! シェイクを開始します (継続中なら強度を半分加算し、時間を延長)
	void triggerShake(float intensity, float duration) noexcept;

	[[nodiscard]] bool isShaking() const noexcept { return m_currentShakeIntensity > SHAKE_THRESHOLD; }

	//! ビューモデル専用の投影行列を取得します (ADS の FOV 変化と独立)
	[[nodiscard]] Matrix matViewmodelProj() const noexcept { return m_matViewmodelProj; }

	[[nodiscard]] float exposure() const noexcept { return m_exposure; }

	void setExposure(float e) noexcept { m_exposure = e; }

	// DebugUI のスライダ直結用の生ポインタ
	[[nodiscard]] float* exposurePtr()      noexcept { return &m_exposure; }
	[[nodiscard]] float* hipFovDegreesPtr() noexcept { return &m_hipFov; }
	[[nodiscard]] float* adsFovDegreesPtr() noexcept { return &m_adsFov; }
	[[nodiscard]] float* fovBlendSpeedPtr() noexcept { return &m_fovBlendSpeed; }

protected:

	//! シェイクの回転・オフセットを乗せてビュー行列を構築 (Camera::update から呼ばれる)
	void updateViewMatrix() noexcept override;

	//! ビューモデル投影を再構築 (FOV 固定 -> ADS でも銃が拡大しない)
	void updateViewmodelProjection() noexcept;

	//! シェイクの減衰とノイズサンプリング
	void updateShake(float deltaTime) noexcept;

private:

	static constexpr float MIN_CAMERA_HEIGHT = 1.0f;
	static constexpr float SHAKE_THRESHOLD = 0.01f;       //!< これ未満の強度はシェイクなし扱い
	static constexpr float SHAKE_TIME_SCALE = 60.0f;      //!< ノイズのサンプル位置の進行速度
	static constexpr float MAX_COMBINED_SHAKE = 30.0f;    //!< 加算合成時の強度上限
	static constexpr float SHAKE_NOISE_FREQ = 5.0f;       //!< ノイズ周波数
	static constexpr float SHAKE_Z_ATTENUATION = 0.5f;    //!< 前後方向の揺れの減衰率
	static constexpr float VIEWMODEL_FOV = 70.0f;         //!< ビューモデル FOV (度)
	static constexpr float VIEWMODEL_NEAR = 0.01f;        //!< ビューモデル近クリップ
	static constexpr float VIEWMODEL_FAR = 10.0f;         //!< ビューモデル遠クリップ

	float m_hipFov        { 70.0f };    //!< 腰だめ FOV (度)
	float m_adsFov        { 45.0f };    //!< ADS FOV (度)
	float m_currentFov    { 70.0f };    //!< ブレンド中の現在 FOV (度)
	float m_fovBlendSpeed {  8.0f };    //!< FOV ブレンド速度

	Matrix m_matViewmodelProj{ Matrix::Identity };    //!< ビューモデル専用投影行列

	float m_exposure = 0.8f;    //!< トーンマップ露出

	FastNoiseLite m_noise;    //!< シェイク用ノイズ (OpenSimplex2)

	float m_shakeTime = 0.0f;                //!< ノイズのサンプル位置 (加算し続ける)
	float m_shakeIntensity = 0.0f;           //!< 開始時の強度
	float m_shakeDuration = 0.0f;            //!< 継続時間 (秒)
	float m_shakeTimer = 0.0f;               //!< 経過時間 (秒)
	float m_currentShakeIntensity = 0.0f;    //!< 減衰後の現在強度

	// 各軸の最大振れ幅 (強度 1.0 のとき)
	float m_shakeMaxYaw = 0.5f;         //!< ヨー (度)
	float m_shakeMaxPitch = 0.3f;       //!< ピッチ (度)
	float m_shakeMaxRoll = 1.0f;        //!< ロール (度)
	float m_shakeMaxOffset = 0.03f;     //!< 平行移動量

	// 今フレームのシェイク成分 (updateShake が算出 -> updateViewMatrix が消費)
	float m_shakeYaw = 0.0f;
	float m_shakePitch = 0.0f;
	float m_shakeRoll = 0.0f;
	float m_shakeOffsetX = 0.0f;
	float m_shakeOffsetY = 0.0f;
	float m_shakeOffsetZ = 0.0f;
};
