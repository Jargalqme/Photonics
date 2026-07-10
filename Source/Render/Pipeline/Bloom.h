//---------------------------------------------------------------------------
//! @file   Bloom.h
//! @brief  ブルーム ポストエフェクト
//---------------------------------------------------------------------------
#pragma once

#include "Core/DeviceResources.h"

//===========================================================================
//! ブルーム
//! 高輝度部分を抽出 -> ミップチェーンでダウン/アップサンプル -> シーンと合成
//! 入出力とも HDR (結果は getOutputSRV() で取得)
//===========================================================================
class Bloom
{
public:
    Bloom(DX::DeviceResources* deviceResources);
    ~Bloom() = default;

    void createDeviceDependentResources();
    void createRenderTargets(int width, int height);
    void finalize();

    //! シーンにブルームを適用します (結果は内部の合成RTへ)
    void render(ID3D11ShaderResourceView* sceneSRV);

    ID3D11ShaderResourceView* getOutputSRV() { return m_compositeSRV.Get(); }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    // ImGuiスライダー用ポインタ
    bool* getEnabledPtr() { return &m_enabled; }
    float* getThresholdPtr() { return &m_threshold; }
    float* getKneePtr() { return &m_knee; }
    float* getIntensityPtr() { return &m_intensity; }
    float* getUpsampleScalePtr() { return &m_upsampleScale; }

private:
    static constexpr int MIP_COUNT = 7;    //!< ミップチェーン段数

    void renderFullscreenPass(ID3D11PixelShader* ps,
        ID3D11RenderTargetView* outputRTV,
        ID3D11ShaderResourceView* input0,
        ID3D11ShaderResourceView* input1,
        UINT width, UINT height);

    DX::DeviceResources* m_deviceResources;

    UINT m_targetWidth = 0;
    UINT m_targetHeight = 0;

    // ブルームミップチェーン (各レベルは前の半分の解像度)
    com_ptr<ID3D11Texture2D>          m_mipTextures[MIP_COUNT];
    com_ptr<ID3D11RenderTargetView>   m_mipRTVs[MIP_COUNT];
    com_ptr<ID3D11ShaderResourceView> m_mipSRVs[MIP_COUNT];
    UINT m_mipWidths[MIP_COUNT] = {};
    UINT m_mipHeights[MIP_COUNT] = {};

    // 合成出力（フル解像度、HDR）
    com_ptr<ID3D11Texture2D>          m_compositeTexture;
    com_ptr<ID3D11RenderTargetView>   m_compositeRTV;
    com_ptr<ID3D11ShaderResourceView> m_compositeSRV;

    // シェーダー
    com_ptr<ID3D11VertexShader> m_fullscreenVS;
    com_ptr<ID3D11PixelShader>  m_prefilterPS;
    com_ptr<ID3D11PixelShader>  m_downsamplePS;
    com_ptr<ID3D11PixelShader>  m_upsamplePS;
    com_ptr<ID3D11PixelShader>  m_compositePS;

    // 共有リソース
    com_ptr<ID3D11Buffer>       m_constantBuffer;
    com_ptr<ID3D11SamplerState> m_sampler;
    com_ptr<ID3D11BlendState>   m_additiveBlend;

    // パラメータ
    bool  m_enabled = false;
    float m_threshold = 1.0f;
    float m_knee = 0.1f;
    float m_intensity = 1.0f;
    float m_upsampleScale = 1.0f;

    //! HLSL 側の定数バッファとレイアウト一致 (16バイト境界)
    struct BloomParamsCB
    {
        DirectX::XMFLOAT2 texelSize;
        float sampleScale;
        float padding1;
        DirectX::XMFLOAT4 threshold;
        float bloomIntensity;
        DirectX::XMFLOAT3 padding2;
    };
};
