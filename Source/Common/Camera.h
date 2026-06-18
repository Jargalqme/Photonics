//---------------------------------------------------------------------------
//! @file   Camera.h
//! @brief  カメラ (ビュー行列・投影行列　+　ビュー定数バッファ)
//---------------------------------------------------------------------------
#pragma once

#include "Core/DeviceResources.h"

//===========================================================================
//! カメラ
//===========================================================================
class Camera
{
public:
	//----------------------------------------------------------
	//! @name   初期化
	//----------------------------------------------------------
	//!@{

	//! コンストラクタ
	explicit Camera(DX::DeviceResources* deviceResources);

	//! デストラクタ
	virtual ~Camera() = default;

	//! デバイス依存リソースを生成 (定数バッファ)
	void createDeviceDependentResources();

	//! デバイス依存リソースを解放
	void finalize();

	//! 更新 (現在の状態からビュー行列・投影行列を再構築)
	void update();

	//! カメラ定数を更新し b10 にバインド (VS/PS)
	void updateConstants();

	//!@}
	//----------------------------------------------------------
	//! @name   設定
	//----------------------------------------------------------
	//!@{

	//! 座標を設定
	void setPosition(const Vector3& position) noexcept { m_position = position; }

	//! 回転を設定 (単位:度)
	void setRotation(float pitch, float yaw, float roll = 0.0f) noexcept;

	//! 投影パラメータを設定 (FOV単位:度)
	void setProjection(float fovDegrees, float aspectRatio, float nearZ, float farZ) noexcept;

	//!@}
	//----------------------------------------------------------
	//! @name   取得・参照
	//----------------------------------------------------------
	//!@{

	//! 座標を取得します
	[[nodiscard]] Vector3 position() const noexcept { return m_position; }

	//! 前方ベクトルを取得します
	[[nodiscard]] Vector3 forward()  const noexcept { return m_forward; }

	//! 右方向ベクトルを取得します
	[[nodiscard]] Vector3 right()    const noexcept { return m_right; }

	//! 上方向ベクトルを取得します
	[[nodiscard]] Vector3 up()       const noexcept { return m_up; }

	//! ピッチ角を取得します (単位:度)
	[[nodiscard]] float   pitch()    const noexcept { return m_pitch; }

	//! ヨー角を取得します (単位:度)
	[[nodiscard]] float   yaw()      const noexcept { return m_yaw; }

	//! ロール角を取得します (単位:度)
	[[nodiscard]] float   roll()     const noexcept { return m_roll; }

	//! ビュー行列を取得します
	[[nodiscard]] Matrix  matView()  const noexcept { return m_matView; }

	//! 投影行列を取得します
	[[nodiscard]] Matrix  matProj()  const noexcept { return m_matProj; }

	//!@}

protected:
	//! 方向ベクトル(forward/right/up)をピッチ・ヨー・ロールから再計算
	void updateDirectionVectors() noexcept;

	//! ビュー行列を再構築 (継承先で上書き可能 ─ カメラシェイクなど)
	virtual void updateViewMatrix() noexcept;

	//! 投影行列を再構築
	void updateProjectionMatrix() noexcept;

	// カメラ姿勢
	Vector3 m_position{ 0.0f, 0.0f, 5.0f };    //!< 座標
	Vector3 m_forward { 0.0f, 0.0f, 1.0f };    //!< 前方ベクトル (+Z, yaw=0 の計算値に一致)
	Vector3 m_right   { 1.0f, 0.0f, 0.0f };    //!< 右方向ベクトル
	Vector3 m_up      { 0.0f, 1.0f, 0.0f };    //!< 上方向ベクトル

	float m_pitch{ 0.0f };                     //!< ピッチ角 (度)
	float m_yaw  { 0.0f };                     //!< ヨー角   (度)
	float m_roll { 0.0f };                     //!< ロール角 (度)

	// レンズパラメータ
	float m_fovy       { DirectX::XMConvertToRadians(45.0f) };    //!< 画角        (radian)
	float m_aspectRatio{ 16.0f / 9.0f };                          //!< アスペクト比
	float m_nearZ      { 0.1f };                                  //!< ニアクリップ距離
	float m_farZ       { 1000.0f };                               //!< ファークリップ距離

	// 行列
	Matrix m_matView{ Matrix::Identity };    //!< ビュー行列
	Matrix m_matProj{ Matrix::Identity };    //!< 投影行列

private:
	//----------------------------------------------------------
	//! 定数バッファ構造体 (b10 / HLSL側と同一レイアウト)
	//----------------------------------------------------------
	struct CameraInfo
	{
		Matrix  view;             // ビュー行列
		Matrix  proj;             // 投影行列
		Matrix  invView;          // ビュー逆行列 (深度→ワールド復元用)
		Matrix  invProj;          // 投影逆行列   (深度→ビュー復元用)
		Vector3 cameraPosition;   // カメラのワールド座標
		float   _pad;             // 16B境界: 256 + 12 + 4 = 272
	};

	com_ptr<ID3D11Buffer> m_constantBuffer;    //!< CameraInfo定数バッファ
	DX::DeviceResources*  m_deviceResources;   //!< デバイスリソース (注入)
};
