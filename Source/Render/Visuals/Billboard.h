#pragma once

#include <string>

struct SceneContext;

class Billboard
{
public:
	Billboard(SceneContext& context);
	~Billboard() = default;

	void initialize(const std::wstring& texturePath);

	void render(
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& projection,
		const DirectX::SimpleMath::Vector3& position,
		float size) const;

	void finalize();

private:
	SceneContext* m_context;

	// ShaderCache から借用（非所有）
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader*  m_pixelShader  = nullptr;

	// 自前所有
	com_ptr<ID3D11Buffer>             m_constantBuffer;
	com_ptr<ID3D11ShaderResourceView> m_textureSRV;

	struct alignas(16) BillboardCB
	{
		DirectX::SimpleMath::Matrix  viewProjection;  // 64 bytes
		DirectX::SimpleMath::Vector3 worldPosition;   // 12 bytes
		float                        billboardSize;   //  4 bytes (total: 80)
		DirectX::SimpleMath::Vector3 cameraRight;     // 12 bytes
		float                        pad0;            //  4 bytes (total: 96)
		DirectX::SimpleMath::Vector3 cameraUp;        // 12 bytes
		float                        pad1;            //  4 bytes (total: 112)
	};
};
