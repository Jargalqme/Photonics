//---------------------------------------------------------------------------
//! @file   DroneCamera.cpp
//! @brief  ドローン追従カメラ (観察 / チェイス切替)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Gameplay/DroneCamera.h"
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

//! 機体の後方 (-Z) かつ上方。全長 13 の機体が画面に収まる距離
const Vector3 DroneCamera::OFFSET = Vector3(0.0f, 5.0f, -18.0f);

//---------------------------------------------------------------------------
//! コンストラクタ
//---------------------------------------------------------------------------
DroneCamera::DroneCamera(DX::DeviceResources* deviceResources)
	: Camera(deviceResources)
{
}

//---------------------------------------------------------------------------
//! 対象を注視して追従します
//---------------------------------------------------------------------------
void DroneCamera::follow(const Vector3& targetPosition, const Quaternion& targetOrientation, float deltaTime)
{
	// チェイス時のみオフセットと上方向を機体姿勢で回す
	const Vector3 offset = m_chaseMode
		? Vector3::Transform(OFFSET, targetOrientation)
		: OFFSET;

	const Vector3 upReference = m_chaseMode
		? Vector3::Transform(Vector3::UnitY, targetOrientation)
		: Vector3::UnitY;

	const Vector3 desired = targetPosition + offset;

	if (m_initialized)
	{
		const float t = std::clamp(FOLLOW_LAG * deltaTime, 0.0f, 1.0f);
		m_position = Vector3::Lerp(m_position, desired, t);
	}
	else
	{
		m_position = desired;
		m_initialized = true;
	}

	// 注視方向
	Vector3 toTarget = targetPosition - m_position;
	if (toTarget.LengthSquared() < 1e-6f)
	{
		toTarget = Vector3::UnitZ;
	}
	toTarget.Normalize();
	m_forward = toTarget;

	// 基底の導出順序は Camera::updateDirectionVectors に揃える
	m_right = upReference.Cross(m_forward);
	if (m_right.LengthSquared() < 1e-6f)
	{
		// 注視方向と上方向が平行 (真上/真下) — 退避軸を使う
		m_right = Vector3::UnitX;
	}
	m_right.Normalize();

	m_up = m_forward.Cross(m_right);
	m_up.Normalize();

	// Camera::update() は使わず、行列だけ再構築する
	updateViewMatrix();
	updateProjectionMatrix();
}
