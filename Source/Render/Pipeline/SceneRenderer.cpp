#include "pch.h"
#include "Render/Pipeline/SceneRenderer.h"
#include "Render/Pipeline/Bloom.h"
#include "Render/Pipeline/FinalCompositePass.h"
#include "Gameplay/PlayerCamera.h"

SceneRenderer::SceneRenderer(DX::DeviceResources* deviceResources)
    : m_deviceResources(deviceResources)
    , m_bloom(std::make_unique<Bloom>(deviceResources))
    , m_finalComposite(std::make_unique<FinalCompositePass>(deviceResources))
{
}

// Destructor defined here so unique_ptr<Bloom> / unique_ptr<FinalCompositePass>
// see the full types. Header forward-declares only.
SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::createDeviceDependentResources()
{
    m_bloom->createDeviceDependentResources();
    m_finalComposite->createDeviceDependentResources();
}

void SceneRenderer::createWindowSizeDependentResources(int width, int height)
{
    m_bloom->createWindowSizeDependentResources(width, height);
    // FinalCompositePass has no window-size-dependent resources.
}

void SceneRenderer::onDeviceLost()
{
    m_bloom->finalize();
    m_finalComposite->finalize();
}

void SceneRenderer::renderPostProcess(
    ID3D11ShaderResourceView* sceneSRV,
    ID3D11RenderTargetView* backbuffer)
{
    // Pull exposure from the active camera. Scenes are expected to have
    // called setActiveCamera() at enter(); fallback to 1.0 (identity) if
    // no camera was set so the screen never goes black.
    const float exposure = m_activeCamera ? m_activeCamera->exposure() : 1.0f;

    // Bloom is the only effect that toggles. The final composite pass is
    // always the last step that tonemaps the HDR scene (or post-bloom
    // result) into the BGRA backbuffer.
    if (m_bloom->isEnabled())
    {
        m_bloom->render(sceneSRV);
        m_finalComposite->process(m_bloom->getOutputSRV(), backbuffer, exposure);
    }
    else
    {
        m_finalComposite->process(sceneSRV, backbuffer, exposure);
    }
}
