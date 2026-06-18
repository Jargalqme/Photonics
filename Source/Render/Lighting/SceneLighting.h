#pragma once
#include <SimpleMath.h>

struct DirectionalLight
{
	// Direction from the shaded surface toward the light.
	// Example: sun above/front/right = normalize(0.35, 0.85, -0.35)
	DirectX::SimpleMath::Vector3 directionToLight =
		DirectX::SimpleMath::Vector3(0.35f, 0.85f, -0.35f);

	DirectX::SimpleMath::Color color =
		DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);

	// HDR direct-light multiplier.
	float intensity = 5.0f;
};

struct AmbientLight
{
	DirectX::SimpleMath::Color color =
		DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);

	// For non-directional fill or diffuse IBL strength.
	float intensity = 1.0f;
};

struct SceneLighting
{
	DirectionalLight keyLight;
	AmbientLight ambient;
};
