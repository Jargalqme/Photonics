#pragma once

#include "DeviceResources.h"

#include <cstdint>

class BulletPool;

class BulletRenderer
{
public:
    explicit BulletRenderer(DX::DeviceResources* deviceResources);
    ~BulletRenderer() = default;

    void initialize();
    void update(float deltaTime);
    void render(
        BulletPool* bulletPool,
        const DirectX::SimpleMath::Matrix& view,
        const DirectX::SimpleMath::Matrix& projection,
        const DirectX::SimpleMath::Vector3& cameraPosition);
    void finalize();

private:
    static constexpr uint32_t MAX_BULLETS_TO_RENDER = 256;
    static constexpr uint32_t VERTICES_PER_ORB = 6;
    static constexpr float BULLET_ORB_RADIUS = 0.5f;

    struct BulletRenderData
    {
        DirectX::SimpleMath::Vector3 position;
        float speed;
        DirectX::SimpleMath::Vector3 direction;
        float age;
        DirectX::SimpleMath::Vector4 color;
    };

    struct BulletRenderCB
    {
        DirectX::SimpleMath::Matrix viewProjection;
        DirectX::SimpleMath::Vector3 cameraPosition;
        float time;
        float boltLength;
        float boltWidth;
        uint32_t boltCount;
        float padding;
    };

    DX::DeviceResources* m_deviceResources = nullptr;
    float m_time = 0.0f;

    com_ptr<ID3D11Buffer> m_bulletBuffer;
    com_ptr<ID3D11ShaderResourceView> m_bulletSRV;
    com_ptr<ID3D11Buffer> m_constantBuffer;
    BulletRenderData m_bulletData[MAX_BULLETS_TO_RENDER] = {};

    com_ptr<ID3D11VertexShader> m_vertexShader;
    com_ptr<ID3D11PixelShader> m_pixelShader;

    com_ptr<ID3D11BlendState> m_blendState;
    com_ptr<ID3D11DepthStencilState> m_depthStencilState;
    com_ptr<ID3D11RasterizerState> m_rasterizerState;
};
