#pragma once

#include <vector>

struct SceneContext;

class Tracers
{
public:
	Tracers(SceneContext& context);
	~Tracers() = default;

	void initialize();

	void spawn(
		const DirectX::SimpleMath::Vector3& start,
		const DirectX::SimpleMath::Vector3& end,
		const DirectX::SimpleMath::Vector4& color);

	void update(float deltaTime);

	void render(
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& projection,
		const DirectX::SimpleMath::Vector3& cameraPos);

	void finalize();

private:
	SceneContext* m_context;

	// ShaderCache から借用（非所有）
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader*  m_pixelShader  = nullptr;

	// 自前所有
	com_ptr<ID3D11Buffer> m_constantBuffer;

	// アクティブなトレーサー1本分のデータ
	struct Tracer
	{
		DirectX::SimpleMath::Vector3 start;
		DirectX::SimpleMath::Vector3 end;
		DirectX::SimpleMath::Vector4 color;
		float                        life;  // [0,1] — 1=新鮮, 0=消滅
		bool                         fresh;
	};
	std::vector<Tracer> m_tracers;

	// TracerVS/TracerPS と一致する CB レイアウト（128 bytes）
	struct alignas(16) TracerCB
	{
		DirectX::SimpleMath::Matrix  viewProjection;   // 64 bytes
		DirectX::SimpleMath::Vector3 beamStart;        // 12 bytes
		float                        beamWidth;        //  4 bytes (total: 80)
		DirectX::SimpleMath::Vector3 beamEnd;          // 12 bytes
		float                        beamLife;         //  4 bytes (total: 96)
		DirectX::SimpleMath::Vector4 beamColor;        // 16 bytes (total: 112)
		DirectX::SimpleMath::Vector3 cameraPosition;   // 12 bytes
		float                        _pad;             //  4 bytes (total: 128) — HLSL float3 末尾パディング合わせ
	};
};
