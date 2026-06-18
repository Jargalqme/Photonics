//=============================================================================
// @brief    Scene-level rendering orchestration.
//           Owns post-process effects (Bloom, FinalComposite) and exposes a
//           single render entry point. Sits between the low-level Renderer
//           (frame orchestration) and the gameplay scenes.
//           Pattern modeled after Hazel's SceneRenderer at appropriate scale.
//=============================================================================
#pragma once

#include "Core/DeviceResources.h"
#include <memory>

class Bloom;
class PlayerCamera;
class FinalCompositePass;

class SceneRenderer
{
public:
    SceneRenderer(DX::DeviceResources* deviceResources);
    ~SceneRenderer();

    SceneRenderer(SceneRenderer&&) = default;
    SceneRenderer& operator=(SceneRenderer&&) = default;
    SceneRenderer(SceneRenderer const&) = delete;
    SceneRenderer& operator=(SceneRenderer const&) = delete;

    void createDeviceDependentResources();
    void createWindowSizeDependentResources(int width, int height);
    void onDeviceLost();

    // Run the post-process chain. Source is the HDR scene SRV;
    // destination is the BGRA backbuffer RTV.
    void renderPostProcess(
        ID3D11ShaderResourceView* sceneSRV,
        ID3D11RenderTargetView* backbuffer);

    // Expose Bloom for DebugUI sliders. Owned, non-transferring.
    Bloom* getBloom() { return m_bloom.get(); }

    // Borrowed, non-owning. Scenes call this once at enter() so the
    // composite pass can read exposure (and future imaging properties)
    // from the active camera each frame.
    void setActiveCamera(const PlayerCamera* camera) { m_activeCamera = camera; }

private:
    DX::DeviceResources* m_deviceResources;
    std::unique_ptr<Bloom>              m_bloom;
    std::unique_ptr<FinalCompositePass> m_finalComposite;
    const PlayerCamera*                 m_activeCamera = nullptr;
};
