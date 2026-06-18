#pragma once

#include "Common/Transform.h"
#include "GeometricPrimitive.h"
#include <SimpleMath.h>
#include <string>
#include <vector>

class ImportedModel;
struct SceneContext;

struct PrimitiveLayoutPart
{
    DirectX::DX11::GeometricPrimitive* primitive = nullptr;
    const ImportedModel* importedModel = nullptr;
    Transform localTransform;
    DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
    std::string name;
    std::string type;
    std::string primitiveType;
    bool collidable = true;
};

struct PrimitiveLayoutMarker
{
    Transform localTransform;
    std::string name;
    std::string type;
};

struct PrimitiveLayout
{
    std::string name;
    std::string root;
    float worldScale = 1.0f;
    std::vector<PrimitiveLayoutPart> parts;
    std::vector<PrimitiveLayoutMarker> markers;
};

class LayoutLoader
{
public:
    static bool loadLayout(
        SceneContext& context,
        const std::string& jsonPath,
        PrimitiveLayout& outLayout);

    static bool loadPrimitiveLayout(
        SceneContext& context,
        const std::string& jsonPath,
        std::vector<PrimitiveLayoutPart>& outParts);
};
