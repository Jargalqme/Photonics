#include "pch.h"
#include "Skeleton.h"

#include <algorithm>

bool Skeleton::build(const SkinnedModelData& data)
{
    finalize();
    const size_t n = data.bones.size();
    if (n == 0) { return false; }

    m_parentIndex.resize(n);
    m_localBindTransform.resize(n);
    m_offsetMatrix.resize(n);
    m_localTransforms.resize(n);
    m_worldTransforms.resize(n);
    m_palette.resize(n);

    for (size_t i = 0; i < n; ++i)
    {
        m_parentIndex[i]        = data.bones[i].parentIndex;
        m_localBindTransform[i] = data.bones[i].localBindTransform;
        m_offsetMatrix[i]       = data.bones[i].offsetMatrix;
    }
    m_globalInverseTransform = data.globalInverseTransform;

    // Topological sort: iterate until every bone whose parent is already
    // ordered is appended. Bones with no parent (parentIndex == INVALID) go
    // first. O(n^2) worst-case but n ~ 50-100, so fine.
    m_topoOrder.reserve(n);
    std::vector<bool> placed(n, false);
    while (m_topoOrder.size() < n)
    {
        const size_t startCount = m_topoOrder.size();
        for (size_t i = 0; i < n; ++i)
        {
            if (placed[i]) { continue; }
            const int32_t parent = m_parentIndex[i];
            const bool parentReady =
                parent == INVALID_BONE_INDEX ||
                placed[static_cast<size_t>(parent)];
            if (parentReady)
            {
                m_topoOrder.push_back(static_cast<int32_t>(i));
                placed[i] = true;
            }
        }
        if (m_topoOrder.size() == startCount)
        {
            // Cycle in the hierarchy (or dangling parent index). Bail out
            // gracefully by appending the rest in original order so we don't
            // hang. Animation may look wrong but the renderer won't crash.
            for (size_t i = 0; i < n; ++i)
            {
                if (!placed[i]) { m_topoOrder.push_back(static_cast<int32_t>(i)); }
            }
            break;
        }
    }

    resetToBindPose();
    return true;
}

void Skeleton::finalize()
{
    m_parentIndex.clear();
    m_localBindTransform.clear();
    m_offsetMatrix.clear();
    m_topoOrder.clear();
    m_localTransforms.clear();
    m_worldTransforms.clear();
    m_palette.clear();
    m_globalInverseTransform = DirectX::SimpleMath::Matrix::Identity;
}

void Skeleton::resetToBindPose()
{
    for (size_t i = 0; i < m_localTransforms.size(); ++i)
    {
        m_localTransforms[i] = m_localBindTransform[i];
    }
}

void Skeleton::setLocalTransform(int32_t boneIndex, const DirectX::SimpleMath::Matrix& m)
{
    if (boneIndex < 0 || static_cast<size_t>(boneIndex) >= m_localTransforms.size())
    {
        return;
    }
    m_localTransforms[boneIndex] = m;
}

const DirectX::SimpleMath::Matrix& Skeleton::localBindTransform(int32_t boneIndex) const
{
    static const DirectX::SimpleMath::Matrix identity =
        DirectX::SimpleMath::Matrix::Identity;

    if (boneIndex < 0 || static_cast<size_t>(boneIndex) >= m_localBindTransform.size())
    {
        return identity;
    }
    return m_localBindTransform[boneIndex];
}

void Skeleton::computePalette()
{
    for (int32_t bi : m_topoOrder)
    {
        const int32_t parent = m_parentIndex[bi];
        if (parent == INVALID_BONE_INDEX)
        {
            m_worldTransforms[bi] = m_localTransforms[bi];
        }
        else
        {
            m_worldTransforms[bi] =
                m_localTransforms[bi] * m_worldTransforms[parent];
        }
    }

    for (size_t i = 0; i < m_palette.size(); ++i)
    {
        m_palette[i] =
            m_offsetMatrix[i] * m_worldTransforms[i] * m_globalInverseTransform;
    }
}
