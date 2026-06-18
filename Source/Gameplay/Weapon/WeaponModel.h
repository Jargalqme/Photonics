#pragma once

#include <SimpleMath.h>
#include <string>

class ImportedModel;
class RenderCommandQueue;
struct SceneContext;

struct RifleModelSettings
{
    float targetLength = 1.68f;
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3(-0.015f, -0.195f, 0.180f);
    DirectX::SimpleMath::Vector3 rotationDegrees = DirectX::SimpleMath::Vector3(0.0f, 180.0f, 0.0f);
};

// First-person imported weapon model.
class WeaponModel
{
public:
    bool loadRifle(SceneContext& context, const std::string& path);
    void submit(RenderCommandQueue& queue, const DirectX::SimpleMath::Matrix& rootWorld) const;
    DirectX::SimpleMath::Matrix buildModelWorldMatrix(const DirectX::SimpleMath::Matrix& rootWorld) const;
    DirectX::SimpleMath::Vector3 getMuzzleLocalPosition() const;
    void finalize();

    bool hasRifle() const { return m_importedRifle != nullptr; }
    RifleModelSettings& rifleSettings() { return m_rifleSettings; }
    const RifleModelSettings& rifleSettings() const { return m_rifleSettings; }
    void resetRifleSettings();

private:
    const ImportedModel* m_importedRifle = nullptr;
    RifleModelSettings m_rifleSettings;
    DirectX::SimpleMath::Vector3 m_importedRifleCenter = DirectX::SimpleMath::Vector3::Zero;
    float m_importedRifleLongestSide = 1.0f;

    static constexpr float kFallbackMuzzleZ = 0.3f;
};
