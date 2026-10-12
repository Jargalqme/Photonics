//---------------------------------------------------------------------------
//! @file   Drone.h
//! @brief  クォータニオン制御ドローン (回転デモ)
//---------------------------------------------------------------------------
#pragma once

class RenderCommandQueue;
class ImportedModel;
struct SceneContext;

//===========================================================================
//! クォータニオン制御ドローン
//! 姿勢をオイラー角ではなく Quaternion で保持し、機体軸まわりに回転する。
//! ジンバルロックが起きないことの実演が目的 — 空力計算は行わない
//===========================================================================
class Drone
{
public:
	explicit Drone(SceneContext& context);

	//----------------------------------------------------------
	//! @name   ライフサイクル
	//----------------------------------------------------------
	//!@{

	//! 共有モデルを取得して初期姿勢へ戻します
	void initialize();

	//! 入力を機体軸回転として積み、機首方向へ前進させます
	//! @param controlled 操縦権 (false のときは入力を読まず慣性飛行)
	void update(float deltaTime, bool controlled);

	//! ImportedModelCommand としてキューへ積む
	void submitRender(RenderCommandQueue& queue) const;

	void finalize();

	//!@}
	//----------------------------------------------------------
	//! @name   取得
	//----------------------------------------------------------
	//!@{

	Vector3    getPosition()    const { return m_position; }
	Quaternion getOrientation() const { return m_orientation; }
	float      getSpeed()       const { return m_speed; }

	//! 機首方向 (機体軸 +Z をワールドへ変換)
	Vector3 getForward() const { return Vector3::Transform(Vector3::UnitZ, m_orientation); }

	//! 機体上方向 (追従カメラ用)
	Vector3 getUp() const { return Vector3::Transform(Vector3::UnitY, m_orientation); }

	//!@}

private:
	//! 姿勢と速度を初期値へ戻します (デモのやり直し用)
	void reset();

	// 角速度 (ラジアン/秒)
	static constexpr float PITCH_RATE = 1.5f;    //!< 上下 (機体軸 X)
	static constexpr float YAW_RATE   = 1.5f;    //!< 左右 (機体軸 Y)
	static constexpr float ROLL_RATE  = 2.0f;    //!< 傾き (機体軸 Z)

	// 速度
	static constexpr float ACCELERATION = 8.0f;     //!< スロットル加速度 (m/秒^2)
	static constexpr float MAX_SPEED    = 30.0f;    //!< 最高速度 (m/秒)
	static constexpr float MIN_SPEED    = 0.0f;     //!< 後退はしない (0 = 空中静止)

	// 表示
	static constexpr float MODEL_SCALE = 1.0f;    //!< ship.glb の素のスケール (要目視調整)

	static constexpr float ANGLE_EPSILON = 1e-6f;    //!< これ未満の回転量は無視 (軸の正規化でゼロ除算を避ける)

	static constexpr const char* MODEL_PATH = "Assets/Characters/ship.glb";

	static const Vector3 START_POSITION;    //!< 初期位置 (Vector3 は constexpr 不可)

	SceneContext*        m_context = nullptr;
	const ImportedModel* m_model   = nullptr;    //!< 共有モデル (所有は ImportedModelCache)

	Quaternion m_orientation = Quaternion::Identity;    //!< 機体姿勢 (毎フレーム正規化)
	Vector3    m_position    = Vector3::Zero;
	float      m_speed       = 0.0f;                    //!< 機首方向の速さ (m/秒)
};
