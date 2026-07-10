//---------------------------------------------------------------------------
//! @file   Cubemap.h
//! @brief  キューブマップ (背景描画 + IBL の環境ソース)
//---------------------------------------------------------------------------
#pragma once
#include "DeviceResources.h"

//===========================================================================
//! キューブマップ
//! 環境キューブマップを保持し、フルスクリーン三角形1枚で背景として描く
//! (頂点バッファ無し、SV_VertexID で生成)。
//! 深度無効なので最初に描くこと。cubeSRV は拡散IBLのベイク元にも使う
//===========================================================================
class Cubemap
{
public:
    Cubemap(DX::DeviceResources* deviceResources);

    //! 6面PNGをキューブマップへ合成し、描画ステートを構築します
    void initialize();

    //! 背景を描画します (シーンの最初に呼ぶ)
    void render();

    void finalize();

    //! 環境キューブマップの SRV を取得します (IBL ベイク元)
    ID3D11ShaderResourceView* cubeSRV() const { return m_cubeSRV.Get(); }

private:

    DX::DeviceResources* m_deviceResources;

    com_ptr<ID3D11VertexShader>       m_vertexShader;
    com_ptr<ID3D11PixelShader>        m_pixelShader;
    com_ptr<ID3D11ShaderResourceView> m_cubeSRV;             //!< 6面PNGから合成したキューブマップ
    com_ptr<ID3D11SamplerState>       m_sampler;
    com_ptr<ID3D11DepthStencilState>  m_depthStencilState;   //!< 深度テスト・書き込み無効
    com_ptr<ID3D11RasterizerState>    m_rasterizerState;     //!< カリング無効
};
