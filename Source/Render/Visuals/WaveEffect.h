//---------------------------------------------------------------------------
//! @file   WaveEffect.h
//! @brief  波エフェクト (波形シェーダーの発光面)
//---------------------------------------------------------------------------
#pragma once

#include "DeviceResources.h"

#include <SimpleMath.h>

struct SceneContext;

//===========================================================================
//! 波エフェクト
//! 波形シェーダー (PS_WaveWorld) を流す装飾用の発光面。
//! 現状はボス戦の背景 (Z 固定の垂直平面) だが、他の形状にも使える想定
//===========================================================================
class WaveEffect
{
public:
    WaveEffect(SceneContext& context);
    ~WaveEffect() = default;

    void initialize();
    void finalize();

    //! 波形アニメーションの時間を進めます
    void update(float deltaTime);

    void render(const DirectX::SimpleMath::Matrix& view,
                const DirectX::SimpleMath::Matrix& projection);

    //! 配置を設定します (回転は度)
    void setTransform(
        const DirectX::SimpleMath::Vector3& position,
        const DirectX::SimpleMath::Vector3& rotationDegrees,
        const DirectX::SimpleMath::Vector3& scale);

    //! 平面の幅を設定します (initialize より前に呼ぶこと — 頂点生成に使う)
    void setSize(float size) { m_size = size; }

    void setSpeed(float speed) { m_speed = speed; }
    void setBrightness(float b) { m_brightness = b; }
    void setAlpha(float a) { m_alpha = a; }

private:
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
    };

    struct WaveEffectCB
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
