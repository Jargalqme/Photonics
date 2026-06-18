#pragma once
#include "Common/Spring.h"
#include "SimpleMath.h"

struct WeaponMotionInput
{
	float deltaTime          { 0.0f  };
	// Horizontal player speed in world units per second.
	float moveSpeed          { 0.0f  };
	bool  grounded           { true  };
	bool  isAiming           { false };
	Vector2 lookDeltaDegrees {};
};

struct WeaponMotionOutput
{
	// Camera-local pose: +X right, +Y up, +Z forward.
	Vector3 position {};
	Vector3 rotation {};
};

struct WeaponMotionTuning
{
	Vector3 hipPosition   { 0.30f, -0.25f, 0.70f };
	Vector3 adsPosition   { 0.00f, -0.085f, 0.35f };
	float   adsBlendSpeed { 10.0f };

	// Recoil - impulse on fire
	float recoilKickback{ -8.0f };			// Z pushback velocity
	float recoilPitchDeg{ -24.0f };			// pitch-up velocity

	// Bob - sine wave from walking
	float bobFrequency { 8.0f };			// how fast the cycle runs (rad/s)
	float bobAmplitudeX{ 0.02f };			// horizontal sway distance
	float bobAmplitudeY{ 0.015f };			// vertical   sway distance
	float bobFalloff{ 5.0f };				// fade speed when stop

	// Sway - weapon trails behind camera rotation
	float swayPositionGain{ 0.003f };		// how far the gun drifts per degree of look
	float swayRotationGain{ 0.18f };		// how much the gun tilts per degree of look
	float swayReturnSpeed{ 12.0f };			// how fast it catches back up
	float swayMaxDeltaDeg{ 20.0f };			// clamp on incoming look delta
};

class WeaponMotion
{
public:
	void update(const WeaponMotionInput& input);
	void onFire();
	void reset();

	WeaponMotionOutput  getMotionOutput() const { return m_composed; }
	WeaponMotionTuning* getMotionTuningPtr()    { return &m_tuning; }

private:
	WeaponMotionOutput computeBasePose(float deltaTime, bool isAiming);
	WeaponMotionOutput computeRecoil(float deltaTime);
	WeaponMotionOutput computeBob(const WeaponMotionInput& input);
	WeaponMotionOutput computeSway(const WeaponMotionInput& input);

	Spring1D m_recoilZ{};
	Spring1D m_recoilP{.zeta = 0.6f, .omega = 30.0f};

	float m_adsAlpha  = 0.0f;
	float m_bobPhase  = 0.0f;
	float m_bobAmount = 0.0f;

	Vector3 m_swayPosition = {};
	Vector3 m_swayRotation = {};

	WeaponMotionTuning m_tuning;
	WeaponMotionOutput m_composed;
};
