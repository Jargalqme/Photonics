//---------------------------------------------------------------------------
//! @file	PlayerCamera.h
//! @brief	プレーヤーカメラ（カメラ　+　追従/エイム/シェイク/ビューモデル/エクスポージャ）
//---------------------------------------------------------------------------
#pragma once

#include "Common/Camera.h"
#include "ThirdParty/FastNoiseLite.h"

class Player;

//===========================================================================
//! プレーヤーカメラ
//===========================================================================
class PlayerCamera : public Camera
{
public:

	PlayerCamera(DX::DeviceResources* deviceResources);

	void update(const Player& player, float deltaTime);

	void triggerShake(float intensity, float duration) noexcept;

	[[nodiscard]] bool isShaking() const noexcept { return m_currentShakeIntensity > SHAKE_THRESHOLD; }

	[[nodiscard]] Matrix matViewmodelProj() const noexcept { return m_matViewmodelProj; }

	[[nodiscard]] float exposure() const noexcept { return m_exposure; }

	void setExposure(float e) noexcept { m_exposure = e; }

	[[nodiscard]] float* exposurePtr()      noexcept { return &m_exposure; }
	[[nodiscard]] float* hipFovDegreesPtr() noexcept { return &m_hipFov; }
	[[nodiscard]] float* adsFovDegreesPtr() noexcept { return &m_adsFov; }
	[[nodiscard]] float* fovBlendSpeedPtr() noexcept { return &m_fovBlendSpeed; }

protected:

	void updateViewMatrix() noexcept override;

	void updateViewmodelProjection() noexcept;

	void updateShake(float deltaTime) noexcept;

private:

	static constexpr float MIN_CAMERA_HEIGHT = 1.0f;
	static constexpr float SHAKE_THRESHOLD = 0.01f;
	static constexpr float SHAKE_TIME_SCALE = 60.0f;
	static constexpr float MAX_COMBINED_SHAKE = 30.0f;
	static constexpr float SHAKE_NOISE_FREQ = 5.0f;
	static constexpr float SHAKE_Z_ATTENUATION = 0.5f;
	static constexpr float VIEWMODEL_FOV = 70.0f;
	static constexpr float VIEWMODEL_NEAR = 0.01f;
	static constexpr float VIEWMODEL_FAR = 10.0f;

	float m_hipFov        { 70.0f };
	float m_adsFov        { 45.0f };
	float m_currentFov    { 70.0f };
	float m_fovBlendSpeed {  8.0f };

	Matrix m_matViewmodelProj{ Matrix::Identity };

	float m_exposure = 0.8f;

	FastNoiseLite m_noise;

	float m_shakeTime = 0.0f;
	float m_shakeIntensity = 0.0f;
	float m_shakeDuration = 0.0f;
	float m_shakeTimer = 0.0f;
	float m_currentShakeIntensity = 0.0f;

	float m_shakeMaxYaw = 0.5f;
	float m_shakeMaxPitch = 0.3f;
	float m_shakeMaxRoll = 1.0f;
	float m_shakeMaxOffset = 0.03f;

	float m_shakeYaw = 0.0f;
	float m_shakePitch = 0.0f;
	float m_shakeRoll = 0.0f;
	float m_shakeOffsetX = 0.0f;
	float m_shakeOffsetY = 0.0f;
	float m_shakeOffsetZ = 0.0f;
};
