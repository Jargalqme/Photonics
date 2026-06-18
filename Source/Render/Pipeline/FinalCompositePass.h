//=============================================================================
// @brief    Final composite step. Applies exposure, tonemap (ACES Hill), and
//           gamma correction to the linear HDR scene (or post-bloom result),
//           writing the LDR output into the BGRA UNORM backbuffer.
//=============================================================================
#pragma once

#include "Core/DeviceResources.h"

class FinalCompositePass
{
public:
    FinalCompositePass(DX::DeviceResources* deviceResources);
    ~FinalCompositePass() = default;

    void createDeviceDependentResources();
    void finalize();

    // Exposure is owned by Camera. Caller supplies it per frame.
    void process(
        ID3D11ShaderResourceView* inputSRV,
        ID3D11RenderTargetView* outputRTV,
        float exposure);

private:
    struct FinalCompositeCB
    {
        float exposure;
        DirectX::XMFLOAT3 padding;
    };

    DX::DeviceResources* m_deviceResources;

    com_ptr<ID3D11VertexShader> m_vertexShader;
    com_ptr<ID3D11PixelShader>  m_pixelShader;
    com_ptr<ID3D11SamplerState> m_sampler;
    com_ptr<ID3D11Buffer>       m_constantBuffer;
};
