#pragma once
#include "DeviceResources.h"

class IndirectLight
{
public:
	IndirectLight(DX::DeviceResources* deviceResources);

	void initialize(ID3D11ShaderResourceView* environmentCubeSRV, ID3D11SamplerState* linearClampSampler);

	void finalize();

	ID3D11ShaderResourceView* irradianceSRV() const { return m_irradianceSRV.Get(); }

private:

	void createIrradianceResources();

	void bakeIrradiance(ID3D11ShaderResourceView* environmentCubeSRV, ID3D11SamplerState* linearClampSampler);

	static constexpr UINT IRRADIANCE_SIZE = 32;

	DX::DeviceResources* m_deviceResources;

	com_ptr<ID3D11ShaderResourceView>  m_irradianceSRV;
	com_ptr<ID3D11UnorderedAccessView> m_irradianceUAV;
	com_ptr<ID3D11ComputeShader>       m_irradianceCS;
};
