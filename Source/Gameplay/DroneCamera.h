//---------------------------------------------------------------------------
//! @file   DroneCamera.h
//! @brief  ドローン追従カメラ (観察 / チェイス切替)
//---------------------------------------------------------------------------
#pragma once

#include "Common/Camera.h"

//===========================================================================
//! ドローン追従カメラ
//! 観察モード   = オフセットをワールド固定 -> 機体の回転がそのまま見える
//! チェイスモード = オフセットを機体姿勢で回す -> ゲーム的だが回転は見えにくい
//===========================================================================
class DroneCamera : public Camera
{
public:
	explicit DroneCamera(DX::DeviceResources* deviceResources);

	//! 対象を注視して追従します。
	//! Camera::update() は呼びません — updateDirectionVectors() が
	//! オイラー角から m_forward を再計算し、ここで設定した値を壊すため
	void follow(const Vector3& targetPosition, const Quaternion& targetOrientation, float deltaTime);

	void setChaseMode(bool enabled) noexcept { m_chaseMode = enabled; }
	[[nodiscard]] bool isChaseMode() const noexcept { return m_chaseMode; }

	//! 次回の follow で補間せず即座に配置します (モード切替・視点切替時)
	void snapNext() noexcept { m_initialized = false; }

private:
	static const Vector3 OFFSET;                 //!< 注視点からのオフセット (後方 + 上方)
	static constexpr float FOLLOW_LAG = 6.0f;    //!< 位置追従の速さ (大きいほど機敏)

	bool m_chaseMode   = false;    //!< false = 観察モード
	bool m_initialized = false;    //!< 初回は補間せず即座に配置
};
