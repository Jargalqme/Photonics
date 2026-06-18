#include "pch.h"
#include "Render/Assets/ShaderCache.h"
#include "Render/Pipeline/RenderUtil.h"

using Microsoft::WRL::ComPtr;

void ShaderCache::initialize(ID3D11Device* device)
{
	m_device = device;
}

void ShaderCache::finalize()
{
	m_vs.clear();
	m_ps.clear();
	m_device = nullptr;
}

// === VS ===

ID3D11VertexShader* ShaderCache::getVS(const std::wstring& filename)
{
	auto it = m_vs.find(filename);
	if (it != m_vs.end())
	{
		return it->second.shader.Get();
	}

	VSEntry& entry = m_vs[filename];
	entry.shader = RenderUtil::loadVS(m_device, filename, &entry.blob);
	return entry.shader.Get();
}

ID3DBlob* ShaderCache::getVSBlob(const std::wstring& filename)
{
	getVS(filename);  // 副作用：未ロードなら今ロード（ヒット時は no-op）
	return m_vs[filename].blob.Get();
}

// === PS ===

ID3D11PixelShader* ShaderCache::getPS(const std::wstring& filename)
{
	auto it = m_ps.find(filename);
	if (it != m_ps.end())
	{
		return it->second.Get();
	}

	auto shader = RenderUtil::loadPS(m_device, filename);
	auto inserted = m_ps.emplace(filename, std::move(shader));
	return inserted.first->second.Get();
}
