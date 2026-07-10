//---------------------------------------------------------------------------
//! @file   RenderCommand.h
//! @brief  描画コマンド定義 (シーン -> キュー投入用の POD)
//---------------------------------------------------------------------------
#pragma once

#include "GeometricPrimitive.h"
#include <SimpleMath.h>

class Billboard;
class ImportedModel;

//! ブレンドモード (キューの不透明/半透明振り分けキー)
enum class BlendMode
{
	Opaque,
	AlphaBlend,
	Additive
};

//! GeometricPrimitive 描画コマンド
//! 注意: GeometricPrimitive::Draw はブレンド/深度を color のアルファで決める
//! (alpha < 1 で AlphaBlend + DepthRead)。blendMode はキュー振り分けにのみ効く
struct MeshCommand
{
	DirectX::DX11::GeometricPrimitive* mesh = nullptr;
	DirectX::SimpleMath::Matrix  world = DirectX::SimpleMath::Matrix::Identity;
	DirectX::SimpleMath::Color   color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
	BlendMode                    blendMode = BlendMode::Opaque;
	float                        emissiveIntensity = 0.0f;
	bool                         wireframe = false;
};

//! ビルボード描画コマンド (常に半透明バケットに積まれる)
struct BillboardCommand
{
	const Billboard* billboard = nullptr;
	DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
	float size = 1.0f;
};

//! インポートモデル描画コマンド
struct ImportedModelCommand
{
	const ImportedModel* model = nullptr;
	DirectX::SimpleMath::Matrix world = DirectX::SimpleMath::Matrix::Identity;
	DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
	BlendMode blendMode = BlendMode::Opaque;
	float emissiveIntensity = 0.0f;
	bool wireframe = false;
};
