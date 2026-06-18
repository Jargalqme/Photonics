#pragma once

#include <cstdint>

#include "Render/Assets/ImportedModel.h"

namespace PrimitiveMesh
{
	// Quad centered at the origin on the XY plane, facing +Z.
	// uvTiling controls the UV range written across the surface.
	ImportedModelData CreatePBRQuad(
		float width = 1.0f,
		float height = 1.0f,
		DirectX::SimpleMath::Vector2 uvTiling = DirectX::SimpleMath::Vector2(1.0f, 1.0f));

	// Box centered at the origin. Each face has unique vertices so normals,
	// tangents, and UVs are correct for PBR texture sampling.
	ImportedModelData CreatePBRBox(
		float width = 1.0f,
		float height = 1.0f,
		float depth = 1.0f,
		DirectX::SimpleMath::Vector2 uvTiling = DirectX::SimpleMath::Vector2(1.0f, 1.0f));
}
