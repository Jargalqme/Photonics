//---------------------------------------------------------------------------
//! @file   Drone.cpp
//! @brief  クォータニオン制御ドローン (回転デモ)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Gameplay/Drone.h"
#include "Render/Assets/ImportedModelCache.h"
#include "Render/Pipeline/RenderCommand.h"
#include "Render/Pipeline/RenderCommandQueue.h"
#include "Services/InputManager.h"
#include "Services/SceneContext.h"
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

const Vector3 Drone::START_POSITION = Vector3(0.0f, 5.0f, 10.0f);

namespace
{
	//! ship.glb の機首を機体軸 +Z へ合わせる補正。
	//! 初回描画で機首の向きを目視確認してから確定させる。
	//! Matrix は constexpr にできないためヘッダの定数群とは別に置く
	const Matrix MESH_CORRECTION = Matrix::Identity;

	//---------------------------------------------------------------------------
	//! 回転入力を読み取ります (x = ピッチ, y = ヨー, z = ロール / 各 -1..+1)
	//! WASD = ピッチ・ヨー / 左右矢印 = ロール (操縦権の切替はシーン側)
	//---------------------------------------------------------------------------
	Vector3 droneReadRotationInput(const InputManager& input)
	{
		Vector3 axis = Vector3::Zero;

		// +X 回転は機首下げ (Z -> -Y)
		if (input.isKeyDown(Keyboard::Keys::W))
		{
			axis.x += 1.0f;
		}
		if (input.isKeyDown(Keyboard::Keys::S))
		{
			axis.x -= 1.0f;
		}

		// +Y 回転は機首右 (Z -> +X)
		if (input.isKeyDown(Keyboard::Keys::D))
		{
			axis.y += 1.0f;
		}
		if (input.isKeyDown(Keyboard::Keys::A))
		{
			axis.y -= 1.0f;
		}

		// +Z 回転は左バンク (右翼 +X が +Y へ上がる)
		if (input.isKeyDown(Keyboard::Keys::Left))
		{
			axis.z += 1.0f;
		}
		if (input.isKeyDown(Keyboard::Keys::Right))
		{
			axis.z -= 1.0f;
		}

		return axis;
	}

	//---------------------------------------------------------------------------
	//! スロットル入力を読み取ります (上矢印 = +1 加速, 下矢印 = -1 ブレーキ)
	//---------------------------------------------------------------------------
	float droneReadThrottleInput(const InputManager& input)
	{
		float throttle = 0.0f;

		if (input.isKeyDown(Keyboard::Keys::Up))
		{
			throttle += 1.0f;
		}
		if (input.isKeyDown(Keyboard::Keys::Down))
		{
			throttle -= 1.0f;
		}

		return throttle;
	}
}

//---------------------------------------------------------------------------
//! コンストラクタ
//---------------------------------------------------------------------------
Drone::Drone(SceneContext& context)
	: m_context(&context)
{
}

//---------------------------------------------------------------------------
//! 共有モデルを取得します
//---------------------------------------------------------------------------
void Drone::initialize()
{
	m_model = m_context->importedModels->get(MODEL_PATH);
	reset();
}

//---------------------------------------------------------------------------
//! 姿勢と速度を初期値へ戻します
//---------------------------------------------------------------------------
void Drone::reset()
{
	m_orientation = Quaternion::Identity;
	m_position    = START_POSITION;
	m_speed       = 0.0f;
}

//---------------------------------------------------------------------------
//! 入力を機体軸回転として積み、機首方向へ前進させます
//! 操縦権がないときは入力を読まず、現在の姿勢・速度のまま慣性飛行を続ける
//---------------------------------------------------------------------------
void Drone::update(float deltaTime, bool controlled)
{
	const InputManager& input = *m_context->input;

	if (controlled && input.isKeyPressed(Keyboard::Keys::Home))
	{
		reset();
		return;
	}

	// --- 機体軸まわりの回転量を作る ---
	// 入力を機体軸の角速度ベクトルとして扱い、その合成軸まわりの
	// 単一回転にする。3軸へ分解して掛けないので並び順に依存しない
	const Vector3 axis = controlled ? droneReadRotationInput(input) : Vector3::Zero;

	const Vector3 omega(
		axis.x * PITCH_RATE * deltaTime,
		axis.y * YAW_RATE   * deltaTime,
		axis.z * ROLL_RATE  * deltaTime);

	const float angle = omega.Length();

	Quaternion delta = Quaternion::Identity;
	if (angle > ANGLE_EPSILON)
	{
		delta = Quaternion::CreateFromAxisAngle(omega / angle, angle);
	}

	// delta を左に置く = 機体軸で回してから既存姿勢でワールドへ移す。
	// 逆順 (m_orientation * delta) はワールド軸回転になり、
	// 機首を上げた後のヨーがロールに見えてしまう
	m_orientation = delta * m_orientation;

	// 毎フレームの積算で単位長からずれるため正規化する
	m_orientation.Normalize();

	// --- 前進 ---
	const float throttle = controlled ? droneReadThrottleInput(input) : 0.0f;
	m_speed = std::clamp(m_speed + throttle * ACCELERATION * deltaTime, MIN_SPEED, MAX_SPEED);

	m_position += getForward() * m_speed * deltaTime;
}

//---------------------------------------------------------------------------
//! ImportedModelCommand としてキューへ積む
//---------------------------------------------------------------------------
void Drone::submitRender(RenderCommandQueue& queue) const
{
	if (!m_model)
	{
		return;
	}

	ImportedModelCommand command;
	command.model = m_model;

	// S * (メッシュ補正) * R * T
	// 補正は姿勢より左 = モデル空間で先に機首を合わせてから姿勢で回す
	command.world =
		Matrix::CreateScale(MODEL_SCALE) *
		MESH_CORRECTION *
		Matrix::CreateFromQuaternion(m_orientation) *
		Matrix::CreateTranslation(m_position);

	command.color = Color(1.0f, 1.0f, 1.0f);
	queue.submit(command);
}

//---------------------------------------------------------------------------
//! 終了処理 (モデルは非所有なので参照を切るだけ)
//---------------------------------------------------------------------------
void Drone::finalize()
{
	m_model = nullptr;
}
