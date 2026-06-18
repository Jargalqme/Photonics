#include "pch.h"
#include "Render/Assets/MeshCache.h"

using namespace DirectX;

void MeshCache::initialize(ID3D11DeviceContext* context)
{
	m_context = context;
}

void MeshCache::finalize()
{
	m_meshes.clear();
	m_context = nullptr;
}

// === 単位形状 ===

GeometricPrimitive* MeshCache::getCube()
{
	auto& slot = m_meshes["Cube"];
	if (!slot)
	{
		slot = GeometricPrimitive::CreateCube(m_context, 1.0f, false);
	}
	return slot.get();
}

GeometricPrimitive* MeshCache::getCylinder()
{
	auto& slot = m_meshes["Cylinder"];
	if (!slot)
	{
		slot = GeometricPrimitive::CreateCylinder(m_context, 1.0f, 1.0f, 32, false);
	}
	return slot.get();
}

GeometricPrimitive* MeshCache::getCone()
{
	auto& slot = m_meshes["Cone"];
	if (!slot)
	{
		slot = GeometricPrimitive::CreateCone(m_context, 1.0f, 1.0f, 32, false);
	}
	return slot.get();
}

GeometricPrimitive* MeshCache::getOctahedron()
{
	auto& slot = m_meshes["Octahedron"];
	if (!slot)
	{
		slot = GeometricPrimitive::CreateOctahedron(m_context, 1.0f, false);
	}
	return slot.get();
}

GeometricPrimitive* MeshCache::getIcosahedron()
{
	auto& slot = m_meshes["Icosahedron"];
	if (!slot)
	{
		slot = GeometricPrimitive::CreateIcosahedron(m_context, 1.0f, false);
	}
	return slot.get();
}

GeometricPrimitive* MeshCache::getSphere(int tessellation)
{
	const std::string key = "Sphere_" + std::to_string(tessellation);
	auto& slot = m_meshes[key];
	if (!slot)
	{
		slot = GeometricPrimitive::CreateSphere(m_context, 1.0f, tessellation, false);
	}
	return slot.get();
}

// === 比率固定 ===

// 注意：浮動小数のキーは呼び出し側がリテラルを渡す前提（実行時計算した値を使う場合は要再設計）
GeometricPrimitive* MeshCache::getTorus(float majorRadius, float minorRadius, int tessellation)
{
	const std::string key = "Torus_"
		+ std::to_string(majorRadius) + "_"
		+ std::to_string(minorRadius) + "_"
		+ std::to_string(tessellation);

	auto& slot = m_meshes[key];
	if (!slot)
	{
		slot = GeometricPrimitive::CreateTorus(m_context, majorRadius, minorRadius, tessellation, false);
	}
	return slot.get();
}
