//---------------------------------------------------------------------------
//! @file   Cubemap.cpp
//! @brief  キューブマップ (背景描画 + IBL の環境ソース)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Render/Visuals/Cubemap.h"
#include "Render/Pipeline/RenderUtil.h"
#include "DDSTextureLoader.h"

using namespace DirectX;

//---------------------------------------------------------------------------
//! コンストラクタ
//---------------------------------------------------------------------------
Cubemap::Cubemap(DX::DeviceResources* deviceResources)
	: m_deviceResources(deviceResources)
{
}

//===========================================================================
// 初期化・終了
//===========================================================================

//---------------------------------------------------------------------------
//! ベイク済みDDS読み込み -> 描画ステート構築
//---------------------------------------------------------------------------
void Cubemap::initialize()
{
	auto device = m_deviceResources->GetD3DDevice();

	// ベイク済み環境キューブマップ読み込み (fp16, 6面 x 11ミップ)
	// DDSヘッダーがキューブ構造・フォーマット・ミップ数を持つため、
	// テクスチャと TextureCube SRV を一括生成できる (手書きの desc は不要)
	DX::ThrowIfFailed(CreateDDSTextureFromFile(
		device,
		L"Assets/Textures/Cubemap/skybox_red.dds",
		nullptr,
		m_cubeSRV.ReleaseAndGetAddressOf()));

	// サンプラー (線形フィルタリング、クランプ)
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	DX::ThrowIfFailed(device->CreateSamplerState(&sampDesc, m_sampler.ReleaseAndGetAddressOf()));

	// シェーダー読み込み
	m_vertexShader = RenderUtil::loadVS(device, L"VS_Cubemap.cso");
	m_pixelShader  = RenderUtil::loadPS(device, L"PS_Cubemap.cso");

	// 深度ステンシル：テストなし、書き込みなし (背景は深度に参加しない)
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DX::ThrowIfFailed(device->CreateDepthStencilState(
		&dsDesc, m_depthStencilState.ReleaseAndGetAddressOf()));

	// ラスタライザー：カリングなし (フルスクリーン三角形は z=1 のため深度クリップも無効)
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.DepthClipEnable = FALSE;
	DX::ThrowIfFailed(device->CreateRasterizerState(
		&rsDesc, m_rasterizerState.ReleaseAndGetAddressOf()));
}

void Cubemap::finalize()
{
	m_vertexShader.Reset();
	m_pixelShader.Reset();
	m_cubeSRV.Reset();
	m_sampler.Reset();
	m_depthStencilState.Reset();
	m_rasterizerState.Reset();
}

//===========================================================================
// 描画
//===========================================================================

//---------------------------------------------------------------------------
//! フルスクリーン三角形で背景を描画します
//---------------------------------------------------------------------------
void Cubemap::render()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// パイプライン設定 (頂点バッファ・入力レイアウト不使用)
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	// キューブマップテクスチャとサンプラーをバインド
	context->PSSetShaderResources(0, 1, m_cubeSRV.GetAddressOf());
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

	context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
	context->RSSetState(m_rasterizerState.Get());

	// フルスクリーン三角形描画
	context->Draw(3, 0);

	// ステート復元
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->PSSetShaderResources(0, 1, &nullSRV);
	context->OMSetDepthStencilState(nullptr, 0);
	context->RSSetState(nullptr);
}
