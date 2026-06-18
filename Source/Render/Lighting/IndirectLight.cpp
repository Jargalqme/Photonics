#include "pch.h"
#include "Render/Lighting/IndirectLight.h"
#include "Render/Pipeline/RenderUtil.h"

IndirectLight::IndirectLight(DX::DeviceResources* deviceResources)
	: m_deviceResources(deviceResources)
{
}

void IndirectLight::initialize(ID3D11ShaderResourceView* environmentCubeSRV, ID3D11SamplerState* linearClampSampler)
{
	createIrradianceResources();
	bakeIrradiance(environmentCubeSRV, linearClampSampler);
}

void IndirectLight::finalize()
{
	m_irradianceCS.Reset();
	m_irradianceUAV.Reset();
	m_irradianceSRV.Reset();
}

void IndirectLight::createIrradianceResources()
{
	auto* device = m_deviceResources->GetD3DDevice();

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = IRRADIANCE_SIZE;
	texDesc.Height = IRRADIANCE_SIZE;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	com_ptr<ID3D11Texture2D> cubeTexture;
	DX::ThrowIfFailed(device->CreateTexture2D(&texDesc, nullptr, cubeTexture.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	DX::ThrowIfFailed(device->CreateShaderResourceView(
		cubeTexture.Get(), &srvDesc, m_irradianceSRV.ReleaseAndGetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = texDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uavDesc.Texture2DArray.MipSlice = 0;
	uavDesc.Texture2DArray.FirstArraySlice = 0;
	uavDesc.Texture2DArray.ArraySize = 6;
	DX::ThrowIfFailed(device->CreateUnorderedAccessView(
		cubeTexture.Get(), &uavDesc, m_irradianceUAV.ReleaseAndGetAddressOf()));
}

void IndirectLight::bakeIrradiance(ID3D11ShaderResourceView* environmentCubeSRV, ID3D11SamplerState* linearClampSampler)
{
	auto* device   = m_deviceResources->GetD3DDevice();
	auto* context  = m_deviceResources->GetD3DDeviceContext();

	m_irradianceCS = RenderUtil::loadCS(device, L"CS_IrradianceConvolution.cso");

	context->CSSetShader(m_irradianceCS.Get(), nullptr, 0);
	context->CSSetShaderResources(0, 1, &environmentCubeSRV);
	context->CSSetSamplers(0, 1, &linearClampSampler);
	context->CSSetUnorderedAccessViews(0, 1, m_irradianceUAV.GetAddressOf(), nullptr);

	context->Dispatch(1, 1, 6);

	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ID3D11ShaderResourceView*  nullSRV = nullptr;
	ID3D11SamplerState*        nullSmp = nullptr;

	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	context->CSSetShaderResources(0, 1, &nullSRV);
	context->CSSetSamplers(0, 1, &nullSmp);
	context->CSSetShader(nullptr, nullptr, 0);
}