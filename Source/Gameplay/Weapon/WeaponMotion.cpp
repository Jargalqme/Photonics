#include "pch.h"
#include "WeaponMotion.h"

namespace
{
	float clamp01(float value)
	{
		return std::clamp(value, 0.0f, 1.0f);
	}

	float lerpFloat(float from, float to, float amount)
	{
		return from + (to - from) * amount;
	}

	Vector3 lerpVector(const Vector3& from, const Vector3& to, float amount)
	{
		return from + (to - from) * amount;
	}
}

void WeaponMotion::update(const WeaponMotionInput& input)
{
	const float deltaTime = std::max(input.deltaTime, 0.0f);

	WeaponMotionOutput base   = computeBasePose(deltaTime, input.isAiming);
	WeaponMotionOutput recoil = computeRecoil(deltaTime);
	WeaponMotionOutput bob    = computeBob(input);
	WeaponMotionOutput sway   = computeSway(input);

	m_composed.position = base.position + recoil.position + bob.position + sway.position;
	m_composed.rotation = base.rotation + recoil.rotation + bob.rotation + sway.rotation;
}

void WeaponMotion::onFire()
{
	m_recoilZ.kick(m_tuning.recoilKickback);
	m_recoilP.kick(m_tuning.recoilPitchDeg);
}

void WeaponMotion::reset()
{
	m_recoilZ.reset();
	m_recoilP.reset();
	m_adsAlpha     = 0.0f;
	m_bobPhase     = 0.0f;
	m_bobAmount    = 0.0f;
	m_swayPosition = {};
	m_swayRotation = {};
	m_composed     = {};
}

WeaponMotionOutput WeaponMotion::computeBasePose(float deltaTime, bool isAiming)
{
	const float target = isAiming ? 1.0f : 0.0f;
	const float step = clamp01(m_tuning.adsBlendSpeed * deltaTime);
	m_adsAlpha = lerpFloat(m_adsAlpha, target, step);

	WeaponMotionOutput output;
	output.position = lerpVector(m_tuning.hipPosition, m_tuning.adsPosition, m_adsAlpha);
	return output;
}

WeaponMotionOutput WeaponMotion::computeRecoil(float deltaTime)
{
	m_recoilZ.update(0.0f, deltaTime);
	m_recoilP.update(0.0f, deltaTime);
	return { Vector3(0.0f, 0.0f, m_recoilZ.x), Vector3(m_recoilP.x, 0.0f, 0.0f) };
}

WeaponMotionOutput WeaponMotion::computeBob(const WeaponMotionInput& input)
{
	constexpr float kMaxSpeed = 15.0f;
	float speedN = std::clamp(input.moveSpeed / kMaxSpeed, 0.0f, 1.0f);
	float target = (input.grounded && speedN > 0.05f) ? speedN : 0.0f;

	float dt = std::max(input.deltaTime, 0.0f);
	m_bobAmount += (target - m_bobAmount) * std::min(1.0f, m_tuning.bobFalloff * dt);
	m_bobPhase += m_tuning.bobFrequency * speedN * dt;

	float x = std::sin(m_bobPhase) * m_tuning.bobAmplitudeX * m_bobAmount;
	float y = std::cos(m_bobPhase * 2.0f) * m_tuning.bobAmplitudeY * m_bobAmount;

	return { Vector3(x, y, 0.0f), Vector3::Zero };
}

WeaponMotionOutput WeaponMotion::computeSway(const WeaponMotionInput& input)
{
	float dt = std::max(input.deltaTime, 0.0f);
	float maxDelta = std::max(m_tuning.swayMaxDeltaDeg, 0.0f);

	float yaw = std::clamp(input.lookDeltaDegrees.x, -maxDelta, maxDelta);
	float pitch = std::clamp(input.lookDeltaDegrees.y, -maxDelta, maxDelta);

	Vector3 targetPos(-yaw * m_tuning.swayPositionGain, -pitch * m_tuning.swayPositionGain, 0.0f);
	Vector3 targetRot(-pitch * m_tuning.swayRotationGain, -yaw * m_tuning.swayRotationGain, 0.0f);

	float step = std::clamp(m_tuning.swayReturnSpeed * dt, 0.0f, 1.0f);
	m_swayPosition += (targetPos - m_swayPosition) * step;
	m_swayRotation += (targetRot - m_swayRotation) * step;

	return { m_swayPosition, m_swayRotation };
}
