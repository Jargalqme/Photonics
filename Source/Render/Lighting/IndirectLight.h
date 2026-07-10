//---------------------------------------------------------------------------
//! @file   IndirectLight.h
//! @brief  拡散IBL (放射照度キューブの実行時ベイク)
//---------------------------------------------------------------------------
#pragma once
#include "DeviceResources.h"

//===========================================================================
//! 拡散IBL
//! シーン初期化時に環境キューブから 32x32x6 の放射照度キューブを
//! コンピュートシェーダーで1回ベイクする
//! (オフラインベイク移行の A/B 検証後に退役予定 — S2.2)
//===========================================================================
class IndirectLight
{
public:
	IndirectLight(DX::DeviceResources* deviceResources);

	//! リソース作成 + ベイクを1回実行します
	void initialize(ID3D11ShaderResourceView* environmentCubeSRV, ID3D11SamplerState* linearClampSampler);

	void finalize();

	//! ベイク済み放射照度キューブの SRV を取得します
	ID3D11ShaderResourceView* irradianceSRV() const { return m_irradianceSRV.Get(); }

private:

	void createIrradianceResources();

	void bakeIrradiance(ID3D11ShaderResourceView* environmentCubeSRV, ID3D11SamplerState* linearClampSampler);

	static constexpr UINT IRRADIANCE_SIZE = 32;    //!< 面あたりの解像度

	DX::DeviceResources* m_deviceResources;

	com_ptr<ID3D11ShaderResourceView>  m_irradianceSRV;    //!< 読み取り用 (TEXTURECUBE ビュー)
	com_ptr<ID3D11UnorderedAccessView> m_irradianceUAV;    //!< 書き込み用 (TEXTURE2DARRAY ビュー)
	com_ptr<ID3D11ComputeShader>       m_irradianceCS;
};
