//---------------------------------------------------------------------------
//! @file   SceneLighting.h
//! @brief  シーンライティング (ライトパラメータ束)
//---------------------------------------------------------------------------
#pragma once
#include <SimpleMath.h>

//! 平行光源
struct DirectionalLight
{
	//! 面からライトへ向かう方向 (例: 上/前/右の太陽 = normalize(0.35, 0.85, -0.35))
	DirectX::SimpleMath::Vector3 directionToLight =
		DirectX::SimpleMath::Vector3(0.35f, 0.85f, -0.35f);

	DirectX::SimpleMath::Color color =
		DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);

	float intensity = 5.0f;    //!< HDR直接光の倍率
};

//! 環境光
struct AmbientLight
{
	DirectX::SimpleMath::Color color =
		DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);

	float intensity = 1.0f;    //!< 無指向フィル / 拡散IBLの強さ
};

//===========================================================================
//! シーンライティング (シーンが所有し、描画時に Renderer へ渡す)
//===========================================================================
struct SceneLighting
{
	DirectionalLight keyLight;
	AmbientLight ambient;
};
