#include "pch.h"
#include "Render/Pipeline/RenderUtil.h"

using Microsoft::WRL::ComPtr;

namespace
{
	// .cso をディスクから読み込んで blob を返す（ファイル内ヘルパ）
	ComPtr<ID3DBlob> loadShaderBlob(const std::wstring& filename)
	{
		ComPtr<ID3DBlob> blob;
		DX::ThrowIfFailed(D3DReadFileToBlob(
			GetShaderPath(filename.c_str()).c_str(), blob.GetAddressOf()));
		return blob;
	}
}

namespace RenderUtil
{
	ComPtr<ID3D11VertexShader> loadVS(
		ID3D11Device* device,
		const std::wstring& filename,
		ComPtr<ID3DBlob>* outBlob)
	{
		ComPtr<ID3DBlob> blob = loadShaderBlob(filename);

		ComPtr<ID3D11VertexShader> shader;
		DX::ThrowIfFailed(device->CreateVertexShader(
			blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
			shader.ReleaseAndGetAddressOf()));

		if (outBlob)
		{
			*outBlob = std::move(blob);
		}
		return shader;
	}

	ComPtr<ID3D11PixelShader> loadPS(
		ID3D11Device* device,
		const std::wstring& filename)
	{
		ComPtr<ID3DBlob> blob = loadShaderBlob(filename);

		ComPtr<ID3D11PixelShader> shader;
		DX::ThrowIfFailed(device->CreatePixelShader(
			blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
			shader.ReleaseAndGetAddressOf()));

		return shader;
	}

	ComPtr<ID3D11ComputeShader> loadCS(
		ID3D11Device* device,
		const std::wstring& filename)
	{
		ComPtr<ID3DBlob> blob = loadShaderBlob(filename);

		ComPtr<ID3D11ComputeShader> shader;
		DX::ThrowIfFailed(device->CreateComputeShader(
			blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
			shader.ReleaseAndGetAddressOf()));

		return shader;
	}
}
