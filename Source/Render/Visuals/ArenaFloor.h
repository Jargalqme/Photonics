#pragma once

#include "DeviceResources.h"

#include <SimpleMath.h>

struct SceneContext;

class ArenaFloor
{
public:
    ArenaFloor(SceneContext& context);
    ~ArenaFloor() = default;

    void initialize();
    void finalize();
    void update(float deltaTime);
    void render(const DirectX::SimpleMath::Matrix& view,
                const DirectX::SimpleMath::Matrix& projection);

    void setTransform(
        const DirectX::SimpleMath::Vector3& position,
        const DirectX::SimpleMath::Vector3& rotationDegrees,
        const DirectX::SimpleMath::Vector3& scale);
    void setSize(float size) { m_size = size; }
    void setSpeed(float speed) { m_speed = speed; }
    void setBrightness(float b) { m_brightness = b; }
    void setAlpha(float a) { m_alpha = a; }

private:
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
    };

    struct ArenaFloorCB
    {
        DirectX::XMFLOAT4X4 worldViewProjection;
        float time;
        float speed;
        float brightness;
        float alpha;
    };

    SceneContext* m_context;

    com_ptr<ID3D11Buffer>             m_vertexBuffer;
    com_ptr<ID3D11Buffer>             m_indexBuffer;
    com_ptr<ID3D11Buffer>             m_constantBuffer;
    com_ptr<ID3D11VertexShader>       m_vertexShader;
    com_ptr<ID3D11PixelShader>        m_pixelShader;
    com_ptr<ID3D11InputLayout>        m_inputLayout;

    float m_time = 0.0f;
    float m_size = 500.0f;
    float m_speed = 0.8f;
    float m_brightness = 0.6f;
    float m_alpha = 0.8f;
    DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector3 m_rotationDegrees = DirectX::SimpleMath::Vector3::Zero;
    DirectX::SimpleMath::Vector3 m_scale = DirectX::SimpleMath::Vector3::One;
};
