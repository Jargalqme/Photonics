#pragma once

#include <unordered_map>
#include <string>

class ShaderCache
{
public:
	void initialize(ID3D11Device* device);
	void finalize();

	// 同じ .cso を二度ロードしない（既にあればキャッシュ返却）
	ID3D11VertexShader* getVS(const std::wstring& filename);
	ID3D11PixelShader*  getPS(const std::wstring& filename);

	// 入力レイアウト作成用にバイトコード blob も公開
	ID3DBlob* getVSBlob(const std::wstring& filename);

private:
	struct VSEntry
	{
		com_ptr<ID3D11VertexShader> shader;
		com_ptr<ID3DBlob>           blob;
	};

	ID3D11Device* m_device = nullptr;
	std::unordered_map<std::wstring, VSEntry>                    m_vs;
	std::unordered_map<std::wstring, com_ptr<ID3D11PixelShader>> m_ps;
};
