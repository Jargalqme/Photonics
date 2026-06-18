#pragma once

#include <string>

namespace RenderUtil
{
	// === 定数バッファ作成（DYNAMIC + CPU_WRITE）===
	template <typename T>
	Microsoft::WRL::ComPtr<ID3D11Buffer> createDynamicConstantBuffer(ID3D11Device* device)
	{
		static_assert(sizeof(T) % 16 == 0,
			"D3D11 constant buffer size must be a multiple of 16 bytes.");
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth      = sizeof(T);
		desc.Usage          = D3D11_USAGE_DYNAMIC;
		desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		DX::ThrowIfFailed(device->CreateBuffer(&desc, nullptr, buffer.ReleaseAndGetAddressOf()));
		return buffer;
	}

	// === 定数バッファ 更新（DYNAMIC + Map/WRITE_DISCARD）===
	// 参考：contract: learn.microsoft.com .../how-to-use-dynamic-resources
	//		framework CommandList::updateSubresource (gpu_command_list.cpp:193)
	template <typename T>
	void updateDynamicConstantBuffer(ID3D11DeviceContext* context,
		const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer,
		const T& data)
	{
		static_assert(sizeof(T) % 16 == 0,
			"D3D11 constant buffer size must be a multiple of 16 bytes.");
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		DX::ThrowIfFailed(context->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
		memcpy(mapped.pData, &data, sizeof(T));		// pData is write-combine memory: write only, never read
		context->Unmap(buffer.Get(), 0);
	}

	// === 静的 頂点バッファ（IMMUTABLE + 初期データ）===
	template <typename T>
	Microsoft::WRL::ComPtr<ID3D11Buffer> createStaticVertexBuffer(
		ID3D11Device* device, const T* data, UINT count)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(T) * count;
		desc.Usage     = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = data;

		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		DX::ThrowIfFailed(device->CreateBuffer(&desc, &init, buffer.ReleaseAndGetAddressOf()));
		return buffer;
	}

	// === 静的 インデックスバッファ（IMMUTABLE + 初期データ）===
	template <typename T>
	Microsoft::WRL::ComPtr<ID3D11Buffer> createStaticIndexBuffer(
		ID3D11Device* device, const T* data, UINT count)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(T) * count;
		desc.Usage     = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = data;

		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		DX::ThrowIfFailed(device->CreateBuffer(&desc, &init, buffer.ReleaseAndGetAddressOf()));
		return buffer;
	}

	// === シェーダ読み込み（.cso 前提、コンパイル済みバイトコード） ===
	// outBlob は入力レイアウト作成用にバイトコードが必要な場合のみ渡す
	Microsoft::WRL::ComPtr<ID3D11VertexShader> loadVS(
		ID3D11Device* device,
		const std::wstring& filename,
		Microsoft::WRL::ComPtr<ID3DBlob>* outBlob = nullptr);

	Microsoft::WRL::ComPtr<ID3D11PixelShader> loadPS(
		ID3D11Device* device,
		const std::wstring& filename);

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> loadCS(
		ID3D11Device* device,
		const std::wstring& filename);
}
