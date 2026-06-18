#pragma once

#include "GeometricPrimitive.h"
#include <SimpleMath.h>

class Billboard;
class ImportedModel;

enum class BlendMode
{
	Opaque,
	AlphaBlend,
	Additive
};

struct MeshCommand
{
	DirectX::DX11::GeometricPrimitive* mesh = nullptr;
	DirectX::SimpleMath::Matrix  world = DirectX::SimpleMath::Matrix::Identity;
	DirectX::SimpleMath::Color   color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
	BlendMode                    blendMode = BlendMode::Opaque;
	float                        emissiveIntensity = 0.0f;
	bool                         wireframe = false;
};

struct BillboardCommand
{
	const Billboard* billboard = nullptr;
	DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
	float size = 1.0f;
};

struct ImportedModelCommand
{
	const ImportedModel* model = nullptr;
	DirectX::SimpleMath::Matrix world = DirectX::SimpleMath::Matrix::Identity;
	DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
	BlendMode blendMode = BlendMode::Opaque;
	float emissiveIntensity = 0.0f;
	bool wireframe = false;
};
