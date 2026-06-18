#pragma once

#include "SkinnedModelData.h"

#include <SimpleMath.h>
#include <cstdint>
#include <vector>

// Runtime bone hierarchy. Built once per loaded SkinnedModelData. Owns:
//   - A topological order over bones (parent always evaluated before child)
//   - A staging buffer of per-bone local transforms (filled each frame by
//     AnimationPlayer or set to bone.localBindTransform for the rest pose)
//   - A buffer of accumulated world transforms (computed from the locals)
//   - The final palette (globalInverseTransform * world * bone.offsetMatrix)
//     that the renderer uploads to the GPU
class Skeleton
{
public:
    bool build(const SkinnedModelData& data);
    void finalize();

    // Set every local transform back to its bind-time rest pose.
    void resetToBindPose();

    // Set one bone's local transform. AnimationPlayer calls this every frame
    // for each animated bone; unanimated bones keep whatever resetToBindPose
    // wrote last.
    void setLocalTransform(int32_t boneIndex, const DirectX::SimpleMath::Matrix& m);
    const DirectX::SimpleMath::Matrix& localBindTransform(int32_t boneIndex) const;

    // Walks the topological order and composes:
    //   world[i]   = world[parent] * local[i]
    //   palette[i] = globalInverseTransform * world[i] * offsetMatrix[i]
    void computePalette();

    const DirectX::SimpleMath::Matrix* palette() const { return m_palette.data(); }
    uint32_t boneCount() const { return static_cast<uint32_t>(m_localTransforms.size()); }

private:
    // Source data, cached on build(). Indexed by original bone index.
    std::vector<int32_t>                       m_parentIndex;
    std::vector<DirectX::SimpleMath::Matrix>   m_localBindTransform;
    std::vector<DirectX::SimpleMath::Matrix>   m_offsetMatrix;

    // Topological evaluation order (root first). Each entry is an index into
    // the bone arrays above; iterating m_topoOrder guarantees parent before
    // child when computing world transforms.
    std::vector<int32_t>                       m_topoOrder;

    // Per-frame buffers.
    std::vector<DirectX::SimpleMath::Matrix>   m_localTransforms;
    std::vector<DirectX::SimpleMath::Matrix>   m_worldTransforms;
    std::vector<DirectX::SimpleMath::Matrix>   m_palette;

    DirectX::SimpleMath::Matrix                m_globalInverseTransform =
        DirectX::SimpleMath::Matrix::Identity;
};
