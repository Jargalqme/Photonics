#pragma once

#include "DeviceResources.h"
#include <memory>
#include <vector>

class MenuBackground
{
public:
    MenuBackground(DX::DeviceResources* deviceResources);
    ~MenuBackground() = default;

    MenuBackground(MenuBackground&&) = default;
    MenuBackground& operator=(MenuBackground&&) = default;

    MenuBackground(MenuBackground const&) = delete;
    MenuBackground& operator=(MenuBackground const&) = delete;

    void initialize();
    void update(float deltaTime);
    void render(int width, int height);
    void onDeviceLost();

    // シェーダーパラメータ設定
    void setSpeed(float speed) { m_speed = speed; }
    void setPatternScale(float scale) { m_patternScale = scale; }
    void setWarpIntensity(float intensity) { m_warpIntensity = intensity; }
    void setBrightness(float brightness) { m_brightness = brightness; }
    void setChromaticOffset(float offset) { m_chromaticOffset = offset; }
    void setColorTint(float r, float g, float b) { m_colorTint = { r, g, b }; }
    void setVignetteStrength(float strength) { m_vignetteStrength = strength; }

    enum class ShaderType
    {
        Warping,   // ワーピング（イントロ用）
        Fractal,   // フラクタル（Kishimisu風）
        Wave       // 波形（メニュー用）
    };
    void setShaderType(ShaderType type);

private:
    // フルスクリーン四角形頂点（クリップ空間座標 + UV）
    struct MenuVertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 uv;
    };

    // HLSL 定数バッファと一致（48バイト、アラインメント済み）
    struct MenuBackgroundCB
    {
        float Time;
        DirectX::XMFLOAT2 Resolution;
        float Speed;
        float PatternScale;
        float WarpIntensity;
        float Brightness;
        float ChromaticOffset;
        DirectX::XMFLOAT3 ColorTint;
        float VignetteStrength;
    };

    DX::DeviceResources* m_deviceResources;
    float m_time;

    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;

    // 頂点データ
    std::vector<MenuVertex> m_vertices;
    size_t m_vertexCount;

    // 内部初期化
    void createFullscreenQuad();
    void createShaders();
    void createDeviceDependentResources();

    // シェーダーパラメータ（調整可能）
    float m_speed = 1.0f;
    float m_patternScale = 9.0f;
    float m_warpIntensity = 1.0f;
    float m_brightness = 0.01f;
    float m_chromaticOffset = 0.07f;
    float m_vignetteStrength = 0.0f;
    DirectX::XMFLOAT3 m_colorTint = { 1.0f, 1.0f, 1.0f };

    ShaderType m_shaderType = ShaderType::Fractal;

    // ピクセルシェーダー（3種類）
    com_ptr<ID3D11PixelShader> m_pixelShaderWarping;
    com_ptr<ID3D11PixelShader> m_pixelShaderFractal;
    com_ptr<ID3D11PixelShader> m_pixelShaderWave;
};
