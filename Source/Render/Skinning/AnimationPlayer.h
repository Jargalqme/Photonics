#pragma once

#include "SkinnedModelData.h"

#include <SimpleMath.h>
#include <cstdint>

class Skeleton;

// Drives Skeleton from an AnimationClip. Advances a playback time each frame,
// samples TRS curves with linear/slerp interpolation, composes a local
// transform per animated bone, and writes them into the skeleton via
// setLocalTransform. Bones with no channel in the active clip keep their
// bind-pose locals (which Skeleton::resetToBindPose seeds on initialization
// and we re-seed every frame before sampling, so clip switches don't leak
// stale poses from previous bones).
class AnimationPlayer
{
public:
    void setClip(const AnimationClip* clip);

    void setLoop(bool loop) { m_loop = loop; }

    void update(float deltaTime);

    // Samples the clip at the current time, writes per-bone local transforms
    // into skeleton, then asks skeleton to compose the palette.
    void apply(Skeleton& skeleton) const;

private:
    static DirectX::SimpleMath::Vector3 sampleVector(
        const std::vector<PositionKey>& keys, float tTicks);
    static DirectX::SimpleMath::Vector3 sampleVector(
        const std::vector<ScaleKey>& keys, float tTicks);
    static DirectX::SimpleMath::Quaternion sampleQuat(
        const std::vector<RotationKey>& keys, float tTicks);

    const AnimationClip* m_clip = nullptr;
    float m_timeSeconds = 0.0f;
    bool  m_loop  = true;
};
