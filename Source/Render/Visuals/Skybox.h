#pragma once
#include "DeviceResources.h"

class Skybox
{
public:
    Skybox(DX::DeviceResources* deviceResources);

    void initialize();

    void render();

    void finalize();

    ID3D11ShaderResourceView* cubeSRV() const { return m_cubeSRV.Get(); }

private:

    DX::DeviceResources* m_deviceResources;

    com_ptr<ID3D11VertexShader>       m_vertexShader;
    com_ptr<ID3D11PixelShader>        m_pixelShader;
    com_ptr<ID3D11ShaderResourceView> m_cubeSRV;
    com_ptr<ID3D11SamplerState>       m_sampler;
    com_ptr<ID3D11DepthStencilState>  m_depthStencilState;
    com_ptr<ID3D11RasterizerState>    m_rasterizerState;
};
