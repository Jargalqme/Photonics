//---------------------------------------------------------------------------
//! @file	PlayerCamera.cpp
//! @brief	プレイヤーカメラ (追従・エイムFOV・シェイク・ビューモデル投影)
//---------------------------------------------------------------------------
#include "pch.h"
#include "PlayerCamera.h"
#include "Gameplay/Player.h"

//---------------------------------------------------------------------------
//! コンストラクタ (ノイズ初期化 + ビューモデル投影の構築)
//---------------------------------------------------------------------------
PlayerCamera::PlayerCamera(DX::DeviceResources* deviceResources)
	: Camera(deviceResources)
{
	m_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_noise.SetFrequency(SHAKE_NOISE_FREQ);

	updateViewmodelProjection();
}

//---------------------------------------------------------------------------
//! 視点追従 -> FOV ブレンド -> シェイク -> 行列再構築
//---------------------------------------------------------------------------
void PlayerCamera::update(const Player& player, float deltaTime)
{
	setRotation(player.lookPitch(), player.lookYaw());

	const float targetFov = player.isAiming() ? m_adsFov : m_hipFov;

	// 指数ブレンド (フレームレート非依存の近似)
	const float blend = std::clamp(m_fovBlendSpeed * deltaTime, 0.0f, 1.0f);
	m_currentFov += (targetFov - m_currentFov) * blend;

	setPosition(player.eyePosition());
	m_fovy = XMConvertToRadians(m_currentFov);

	// シェイク成分を先に確定させる (Camera::update 内の updateViewMatrix が消費)
	updateShake(deltaTime);

	Camera::update();

	updateViewmodelProjection();
}

//---------------------------------------------------------------------------
//! シェイクを開始 (継続中なら強度を半分加算し、時間を延長)
//---------------------------------------------------------------------------
void PlayerCamera::triggerShake(float intensity, float duration) noexcept
{
	if (m_shakeTimer < m_shakeDuration)
	{
		m_shakeIntensity = std::min(m_shakeIntensity + intensity * 0.5f, MAX_COMBINED_SHAKE);
		m_shakeDuration = std::max(m_shakeDuration, duration);
	}
	else
	{
		m_shakeIntensity = intensity;
		m_shakeDuration = duration;
		m_shakeTimer = 0.0f;
	}
}

//---------------------------------------------------------------------------
//! シェイクの回転・オフセットを乗せて LookAt ビューを構築 (override)
//---------------------------------------------------------------------------
void PlayerCamera::updateViewMatrix() noexcept
{
	const Vector3 eyePos = m_position + Vector3(m_shakeOffsetX, m_shakeOffsetY, m_shakeOffsetZ);

	Vector3 forward = m_forward;
	Vector3 up = m_up;

	if (m_currentShakeIntensity > SHAKE_THRESHOLD)
	{
		const Matrix rotation = Matrix::CreateFromYawPitchRoll(
			XMConvertToRadians(m_shakeYaw),
			XMConvertToRadians(m_shakePitch),
			XMConvertToRadians(m_shakeRoll));

		forward = Vector3::TransformNormal(forward, rotation);
		up = Vector3::TransformNormal(up, rotation);
	}

	const Vector3 target = eyePos + forward;
	m_matView = XMMatrixLookAtLH(eyePos, target, up);
}

//---------------------------------------------------------------------------
//! ビューモデル専用投影を再構築 (FOV 固定 -> ADS でも銃が拡大しない)
//---------------------------------------------------------------------------
void PlayerCamera::updateViewmodelProjection() noexcept
{
	m_matViewmodelProj = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(VIEWMODEL_FOV),
		m_aspectRatio,
		VIEWMODEL_NEAR,
		VIEWMODEL_FAR);
}

//---------------------------------------------------------------------------
//! シェイク成分を算出 (2次減衰、チャンネルごとに異なる y でノイズを分離)
//---------------------------------------------------------------------------
void PlayerCamera::updateShake(float deltaTime) noexcept
{
	if (m_shakeTimer < m_shakeDuration)
	{
		m_shakeTimer += deltaTime;
		m_shakeTime += deltaTime * SHAKE_TIME_SCALE;

		const float progress = m_shakeTimer / m_shakeDuration;
		const float falloff = 1.0f - progress;
		m_currentShakeIntensity = m_shakeIntensity * falloff * falloff;

		const float shake = m_currentShakeIntensity;
		m_shakeYaw     = m_shakeMaxYaw    * shake * m_noise.GetNoise(m_shakeTime,   0.0f);
		m_shakePitch   = m_shakeMaxPitch  * shake * m_noise.GetNoise(m_shakeTime, 100.0f);
		m_shakeRoll    = m_shakeMaxRoll   * shake * m_noise.GetNoise(m_shakeTime, 200.0f);
		m_shakeOffsetX = m_shakeMaxOffset * shake * m_noise.GetNoise(m_shakeTime, 300.0f);
		m_shakeOffsetY = m_shakeMaxOffset * shake * m_noise.GetNoise(m_shakeTime, 400.0f);
		m_shakeOffsetZ = m_shakeMaxOffset * shake * m_noise.GetNoise(m_shakeTime, 500.0f) * SHAKE_Z_ATTENUATION;
	}
	else
	{
		m_currentShakeIntensity = 0.0f;
		m_shakeIntensity = 0.0f;
		m_shakeYaw = 0.0f;
		m_shakePitch = 0.0f;
		m_shakeRoll = 0.0f;
		m_shakeOffsetX = 0.0f;
		m_shakeOffsetY = 0.0f;
		m_shakeOffsetZ = 0.0f;
	}
}
