#pragma once

#include "DeviceResources.h"
#include "Render/Pipeline/SceneRenderer.h"
#include "Render/Pipeline/UIRenderer.h"
#include "Render/Lighting/SceneLighting.h"
#include <memory>

class RenderCommandQueue;
struct ImportedModelCommand;

class Renderer final : noncopyable
{
public:
    Renderer(DX::DeviceResources* deviceResources);
    ~Renderer() = default;

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void OnDeviceLost();

    // Helper to get device resources for other systems
    [[nodiscard]] DX::DeviceResources* GetDeviceResources() { return m_deviceResources; }

    [[nodiscard]] SceneRenderer* GetSceneRenderer() { return m_sceneRenderer.get(); }

    [[nodiscard]] UIRenderer* GetUI() { return m_ui.get(); }

    void BeginScene();
    void EndScene();

    void ExecuteRenderCommands(
        const RenderCommandQueue& queue,
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& projection,
        const DirectX::SimpleMath::Vector3& cameraPosition,
        const SceneLighting& lighting);

    void BeginViewmodelPass();

    void SetRenderResolution(int width, int height);

    [[nodiscard]] int GetRenderWidth() const;
    [[nodiscard]] int GetRenderHeight() const;

    void ApplyPostProcess();

    [[nodiscard]] ID3D11RenderTargetView* GetSceneRTV() { return m_sceneRTV.Get(); }

    void SetIrradianceSRV(ID3D11ShaderResourceView* srv) { m_irradianceSRV = srv; }

private:
    void CreateImportedModelResources();
    void DrawImportedModelCommand(
        const ImportedModelCommand& command,
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& projection,
        const DirectX::SimpleMath::Vector3& cameraPosition,
        const SceneLighting& lighting);

    // Device resources (not owned by this class)
    DX::DeviceResources* m_deviceResources;

    std::unique_ptr<SceneRenderer> m_sceneRenderer;
    std::unique_ptr<UIRenderer>    m_ui;

    com_ptr<ID3D11VertexShader>    m_importedModelVS;
    com_ptr<ID3D11PixelShader>     m_importedModelPS;
    com_ptr<ID3D11InputLayout>     m_importedModelInputLayout;
    com_ptr<ID3D11Buffer>          m_importedModelConstantBuffer;
    com_ptr<ID3D11RasterizerState> m_importedModelSolidRasterizer;
    com_ptr<ID3D11RasterizerState> m_importedModelWireframeRasterizer;
    com_ptr<ID3D11SamplerState>    m_importedModelSampler;
    com_ptr<ID3D11BlendState>      m_importedModelAlphaBlendState;
    com_ptr<ID3D11BlendState>      m_importedModelAdditiveBlendState;

    ID3D11ShaderResourceView* m_irradianceSRV = nullptr;

    com_ptr<ID3D11Texture2D>          m_sceneTexture;
    com_ptr<ID3D11RenderTargetView>   m_sceneRTV;
    com_ptr<ID3D11ShaderResourceView> m_sceneSRV;

    com_ptr<ID3D11Texture2D>        m_sceneDepthTexture;
    com_ptr<ID3D11DepthStencilView> m_sceneDSV;

    int m_renderWidth = 0;
    int m_renderHeight = 0;
};
