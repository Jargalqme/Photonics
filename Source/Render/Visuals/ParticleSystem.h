#pragma once
#include "DeviceResources.h"
#include <vector>

class ParticleSystem
{
public:
    ParticleSystem(DX::DeviceResources* deviceResources);
    ~ParticleSystem() = default;

    void initialize();
    void update(float deltaTime);
    void render(const DirectX::SimpleMath::Matrix& view,
                const DirectX::SimpleMath::Matrix& projection);
    void finalize();

    void emit(const DirectX::SimpleMath::Vector3& position,
              const DirectX::SimpleMath::Vector4& color,
              uint32_t count,
              float speed = 5.0f,
              float lifetime = 2.0f,
              float spread = 1.0f);

private:
    // --- 定数 ---
    static constexpr uint32_t MAX_PARTICLES = 4096;
    static constexpr float GRAVITY          = 2.0f;
    static constexpr float UPWARD_BIAS      = 0.5f;
    static constexpr float BASE_SIZE        = 0.15f;
    static constexpr float SIZE_VARIANCE    = 0.1f;

    // HLSL Particle 構造体と一致（64バイト）
    struct Particle
    {
        DirectX::SimpleMath::Vector3 position = {};
        float lifetime = 0.0f;
        DirectX::SimpleMath::Vector3 velocity = {};
        float maxLifetime = 0.0f;
        DirectX::SimpleMath::Vector4 color = {};
        float size = 0.0f;
        float pad[3] = {};
    };

    // HLSL 定数バッファと一致
    struct ParticleCB
    {
        DirectX::SimpleMath::Matrix inverseView;
        DirectX::SimpleMath::Matrix viewProjection;
    };

    // 発生リクエスト（キュー化、update() で処理）
    struct EmitRequest
    {
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector4 color;
        uint32_t count;
        float speed;
        float lifetime;
        float spread;
    };

    DX::DeviceResources* m_deviceResources;
    float m_totalTime = 0.0f;

    std::vector<Particle> m_particles;
    std::vector<EmitRequest> m_emitRequests;

    // GPU リソース
    com_ptr<ID3D11Buffer> m_particleBuffer;
    com_ptr<ID3D11ShaderResourceView> m_particleSRV;
    com_ptr<ID3D11Buffer> m_constantBuffer;

    // シェーダー
    com_ptr<ID3D11VertexShader> m_vertexShader;
    com_ptr<ID3D11PixelShader> m_pixelShader;

    // レンダーステート
    com_ptr<ID3D11BlendState> m_blendState;
    com_ptr<ID3D11DepthStencilState> m_depthStencilState;
    com_ptr<ID3D11RasterizerState> m_rasterizerState;
};
