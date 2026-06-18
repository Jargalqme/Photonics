#include "pch.h"
#include "AnimationPlayer.h"

#include "Skeleton.h"

#include <algorithm>

namespace
{
    // Find the index of the keyframe whose time <= t. Returns 0 if t precedes
    // every key; returns keys.size()-1 if t follows every key.
    template <typename KeyT>
    size_t findKeyIndex(const std::vector<KeyT>& keys, float t)
    {
        if (keys.size() <= 1) { return 0; }
        for (size_t i = 0; i + 1 < keys.size(); ++i)
        {
            if (t < keys[i + 1].time) { return i; }
        }
        return keys.size() - 1;
    }

    float interpolant(float a, float b, float t)
    {
        const float range = b - a;
        return (range > 1e-6f) ? ((t - a) / range) : 0.0f;
    }

    void DecomposeBindLocal(
        const DirectX::SimpleMath::Matrix& bindLocal,
        DirectX::SimpleMath::Vector3& scale,
        DirectX::SimpleMath::Quaternion& rotation,
        DirectX::SimpleMath::Vector3& translation)
    {
        DirectX::SimpleMath::Matrix copy = bindLocal;
        if (!copy.Decompose(scale, rotation, translation))
        {
            scale = DirectX::SimpleMath::Vector3::One;
            rotation = DirectX::SimpleMath::Quaternion::Identity;
            translation = DirectX::SimpleMath::Vector3::Zero;
        }
    }
}

void AnimationPlayer::setClip(const AnimationClip* clip)
{
    m_clip = clip;
    m_timeSeconds = 0.0f;
}

void AnimationPlayer::update(float deltaTime)
{
    if (!m_clip || m_clip->ticksPerSecond <= 0.0f) { return; }

    m_timeSeconds += deltaTime;

    const float durationSeconds = m_clip->duration / m_clip->ticksPerSecond;
    if (durationSeconds <= 0.0f) { m_timeSeconds = 0.0f; return; }

    if (m_loop)
    {
        m_timeSeconds = std::fmod(m_timeSeconds, durationSeconds);
        if (m_timeSeconds < 0.0f) { m_timeSeconds += durationSeconds; }
    }
    else
    {
        m_timeSeconds = std::clamp(m_timeSeconds, 0.0f, durationSeconds);
    }
}

DirectX::SimpleMath::Vector3 AnimationPlayer::sampleVector(
    const std::vector<PositionKey>& keys, float tTicks)
{
    if (keys.empty()) { return DirectX::SimpleMath::Vector3::Zero; }
    const size_t i = findKeyIndex(keys, tTicks);
    if (i + 1 >= keys.size()) { return keys.back().value; }
    const float u = interpolant(keys[i].time, keys[i + 1].time, tTicks);
    return DirectX::SimpleMath::Vector3::Lerp(keys[i].value, keys[i + 1].value, u);
}

DirectX::SimpleMath::Vector3 AnimationPlayer::sampleVector(
    const std::vector<ScaleKey>& keys, float tTicks)
{
    if (keys.empty()) { return DirectX::SimpleMath::Vector3::One; }
    const size_t i = findKeyIndex(keys, tTicks);
    if (i + 1 >= keys.size()) { return keys.back().value; }
    const float u = interpolant(keys[i].time, keys[i + 1].time, tTicks);
    return DirectX::SimpleMath::Vector3::Lerp(keys[i].value, keys[i + 1].value, u);
}

DirectX::SimpleMath::Quaternion AnimationPlayer::sampleQuat(
    const std::vector<RotationKey>& keys, float tTicks)
{
    if (keys.empty()) { return DirectX::SimpleMath::Quaternion::Identity; }
    const size_t i = findKeyIndex(keys, tTicks);
    if (i + 1 >= keys.size()) { return keys.back().value; }
    const float u = interpolant(keys[i].time, keys[i + 1].time, tTicks);
    return DirectX::SimpleMath::Quaternion::Slerp(keys[i].value, keys[i + 1].value, u);
}

void AnimationPlayer::apply(Skeleton& skeleton) const
{
    // Re-seed bind pose so bones with no channel in this clip stay at rest.
    skeleton.resetToBindPose();

    if (!m_clip) { skeleton.computePalette(); return; }

    const float tTicks = m_timeSeconds * m_clip->ticksPerSecond;

    for (const AnimationChannel& channel : m_clip->channels)
    {
        if (channel.boneIndex == INVALID_BONE_INDEX) { continue; }

        DirectX::SimpleMath::Vector3 bindScale;
        DirectX::SimpleMath::Quaternion bindRotation;
        DirectX::SimpleMath::Vector3 bindTranslation;
        DecomposeBindLocal(
            skeleton.localBindTransform(channel.boneIndex),
            bindScale,
            bindRotation,
            bindTranslation);

        const DirectX::SimpleMath::Vector3 pos = channel.positions.empty()
            ? bindTranslation : sampleVector(channel.positions, tTicks);
        const DirectX::SimpleMath::Quaternion rot = channel.rotations.empty()
            ? bindRotation : sampleQuat(channel.rotations, tTicks);
        const DirectX::SimpleMath::Vector3 scl = channel.scales.empty()
            ? bindScale : sampleVector(channel.scales, tTicks);

        const DirectX::SimpleMath::Matrix local =
            DirectX::SimpleMath::Matrix::CreateScale(scl) *
            DirectX::SimpleMath::Matrix::CreateFromQuaternion(rot) *
            DirectX::SimpleMath::Matrix::CreateTranslation(pos);

        skeleton.setLocalTransform(channel.boneIndex, local);
    }

    skeleton.computePalette();
}
