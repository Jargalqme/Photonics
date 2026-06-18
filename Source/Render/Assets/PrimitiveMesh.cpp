#include "pch.h"
#include "Render/Assets/PrimitiveMesh.h"

#include <algorithm>
#include <iterator>

using DirectX::SimpleMath::Color;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;

namespace
{
	constexpr float kMinDimension     = 0.001f;
	constexpr float kDefaultRoughness = 0.5f;

	float SanitizeDimension(float value)
	{
		return std::max(value, kMinDimension);
	}

	Vector2 SanitizeTiling(Vector2 tiling)
	{
		tiling.x = std::max(tiling.x, kMinDimension);
		tiling.y = std::max(tiling.y, kMinDimension);
		return tiling;
	}

	void AddDefaultMaterial(ImportedModelData& data)
	{
		ImportedMaterial material;
		material.name = "PrimitiveDefault";
		material.baseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
		material.metallicFactor = 0.0f;
		material.roughnessFactor = kDefaultRoughness;
		data.materials.push_back(material);
	}

	void AddSingleSubmesh(ImportedModelData& data, const char* name)
	{
		ImportedSubmesh submesh;
		submesh.name = name;
		submesh.baseVertex = 0;
		submesh.startIndex = 0;
		submesh.indexCount = static_cast<uint32_t>(data.indices.size());
		submesh.materialIndex = 0;
		data.submeshes.push_back(submesh);
	}

	void AddFace(
		ImportedModelData& data,
		const Vector3& center,
		const Vector3& halfU,
		const Vector3& halfV,
		const Vector3& normal,
		const Vector3& tangent,
		const Vector2& uvTiling)
	{
		const uint32_t baseIndex = static_cast<uint32_t>(data.vertices.size());

		const Vector3 topLeft     = center - halfU + halfV;
		const Vector3 topRight    = center + halfU + halfV;
		const Vector3 bottomRight = center + halfU - halfV;
		const Vector3 bottomLeft  = center - halfU - halfV;

		ImportedModelVertex vertices[4];
		vertices[0].position = topLeft;
		vertices[0].normal   = normal;
		vertices[0].tangent  = tangent;
		vertices[0].texcoord = Vector2(0.0f, 0.0f);

		vertices[1].position = topRight;
		vertices[1].normal   = normal;
		vertices[1].tangent  = tangent;
		vertices[1].texcoord = Vector2(uvTiling.x, 0.0f);

		vertices[2].position = bottomRight;
		vertices[2].normal   = normal;
		vertices[2].tangent  = tangent;
		vertices[2].texcoord = Vector2(uvTiling.x, uvTiling.y);

		vertices[3].position = bottomLeft;
		vertices[3].normal   = normal;
		vertices[3].tangent  = tangent;
		vertices[3].texcoord = Vector2(0.0f, uvTiling.y);

		data.vertices.insert(data.vertices.end(), std::begin(vertices), std::end(vertices));

		// Clockwise winding for the current D3D11 imported-model rasterizer.
		data.indices.push_back(baseIndex + 0);
		data.indices.push_back(baseIndex + 2);
		data.indices.push_back(baseIndex + 1);

		data.indices.push_back(baseIndex + 0);
		data.indices.push_back(baseIndex + 3);
		data.indices.push_back(baseIndex + 2);
	}
}

namespace PrimitiveMesh
{
	ImportedModelData CreatePBRQuad(float width, float height, Vector2 uvTiling)
	{
		width    = SanitizeDimension(width);
		height   = SanitizeDimension(height);
		uvTiling = SanitizeTiling(uvTiling);

		ImportedModelData data;
		data.vertices.reserve(4);
		data.indices.reserve(6);

		AddFace(
			data,
			Vector3::Zero,
			Vector3(width * 0.5f, 0.0f, 0.0f),
			Vector3(0.0f, height * 0.5f, 0.0f),
			Vector3(0.0f, 0.0f, 1.0f),
			Vector3(1.0f, 0.0f, 0.0f),
			uvTiling);

		AddSingleSubmesh(data, "Quad");
		AddDefaultMaterial(data);

		return data;
	}

	ImportedModelData CreatePBRBox(float width, float height, float depth, Vector2 uvTiling)
	{
		width    = SanitizeDimension(width);
		height   = SanitizeDimension(height);
		depth    = SanitizeDimension(depth);
		uvTiling = SanitizeTiling(uvTiling);

		ImportedModelData data;
		data.vertices.reserve(24);
		data.indices.reserve(36);

		const float halfWidth  = width  * 0.5f;
		const float halfHeight = height * 0.5f;
		const float halfDepth  = depth  * 0.5f;

		// +Z
		AddFace(
			data,
			Vector3(0.0f, 0.0f, halfDepth),
			Vector3(halfWidth, 0.0f, 0.0f),
			Vector3(0.0f, halfHeight, 0.0f),
			Vector3(0.0f, 0.0f, 1.0f),
			Vector3(1.0f, 0.0f, 0.0f),
			uvTiling);

		// -Z
		AddFace(
			data,
			Vector3(0.0f, 0.0f, -halfDepth),
			Vector3(-halfWidth, 0.0f, 0.0f),
			Vector3(0.0f, halfHeight, 0.0f),
			Vector3(0.0f, 0.0f, -1.0f),
			Vector3(-1.0f, 0.0f, 0.0f),
			uvTiling);

		// +X
		AddFace(
			data,
			Vector3(halfWidth, 0.0f, 0.0f),
			Vector3(0.0f, 0.0f, -halfDepth),
			Vector3(0.0f, halfHeight, 0.0f),
			Vector3(1.0f, 0.0f, 0.0f),
			Vector3(0.0f, 0.0f, -1.0f),
			uvTiling);

		// -X
		AddFace(
			data,
			Vector3(-halfWidth, 0.0f, 0.0f),
			Vector3(0.0f, 0.0f, halfDepth),
			Vector3(0.0f, halfHeight, 0.0f),
			Vector3(-1.0f, 0.0f, 0.0f),
			Vector3(0.0f, 0.0f, 1.0f),
			uvTiling);

		// +Y
		AddFace(
			data,
			Vector3(0.0f, halfHeight, 0.0f),
			Vector3(halfWidth, 0.0f, 0.0f),
			Vector3(0.0f, 0.0f, -halfDepth),
			Vector3(0.0f, 1.0f, 0.0f),
			Vector3(1.0f, 0.0f, 0.0f),
			uvTiling);

		// -Y
		AddFace(
			data,
			Vector3(0.0f, -halfHeight, 0.0f),
			Vector3(halfWidth, 0.0f, 0.0f),
			Vector3(0.0f, 0.0f, halfDepth),
			Vector3(0.0f, -1.0f, 0.0f),
			Vector3(1.0f, 0.0f, 0.0f),
			uvTiling);

		AddSingleSubmesh(data, "Box");
		AddDefaultMaterial(data);

		return data;
	}
}
