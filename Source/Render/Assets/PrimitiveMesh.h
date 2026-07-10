//---------------------------------------------------------------------------
//! @file   PrimitiveMesh.h
//! @brief  PBR 用プリミティブメッシュ生成 (ImportedModelData を組み立てる)
//---------------------------------------------------------------------------
#pragma once

#include <cstdint>

#include "Render/Assets/ImportedModel.h"

namespace PrimitiveMesh
{
	//! 原点中心・XY 平面のクアッド (+Z 向き)
	//! uvTiling で表面に書き込む UV 範囲を指定
	ImportedModelData CreatePBRQuad(
		float width = 1.0f,
		float height = 1.0f,
		DirectX::SimpleMath::Vector2 uvTiling = DirectX::SimpleMath::Vector2(1.0f, 1.0f));

	//! 原点中心のボックス
	//! 面ごとに頂点を複製し、法線/接線/UV を PBR サンプリング用に正しく持つ
	ImportedModelData CreatePBRBox(
		float width = 1.0f,
		float height = 1.0f,
		float depth = 1.0f,
		DirectX::SimpleMath::Vector2 uvTiling = DirectX::SimpleMath::Vector2(1.0f, 1.0f));
}
