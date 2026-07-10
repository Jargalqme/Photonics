//---------------------------------------------------------------------------
//! @file   MeshCache.h
//! @brief  GeometricPrimitive 共有キャッシュ
//---------------------------------------------------------------------------
#pragma once

#include <unordered_map>
#include <string>
#include <memory>

//===========================================================================
//! メッシュキャッシュ
//! GeometricPrimitive を名前キーで共有する (初回アクセス時に遅延生成)
//===========================================================================
class MeshCache
{
public:
	void initialize(ID3D11DeviceContext* context);
	void finalize();

	// 単位形状 (呼び出し側が Transform.scale でリサイズ)
	DirectX::GeometricPrimitive* getCube();          //!< 1 x 1 x 1
	DirectX::GeometricPrimitive* getCylinder();      //!< 高さ 1、直径 1
	DirectX::GeometricPrimitive* getCone();          //!< 高さ 1、直径 1
	DirectX::GeometricPrimitive* getOctahedron();    //!< 半径 1
	DirectX::GeometricPrimitive* getIcosahedron();   //!< 半径 1
	DirectX::GeometricPrimitive* getSphere(int tessellation = 16);  //!< 直径 1 (半径 r にするには scale = r * 2)

	// 比率固定 (直径と断面太さの比が固定のため均等スケール推奨)
	DirectX::GeometricPrimitive* getTorus(float diameter, float thickness, int tessellation = 32);

private:
	ID3D11DeviceContext* m_context = nullptr;
	std::unordered_map<std::string, std::unique_ptr<DirectX::GeometricPrimitive>> m_meshes;
};
