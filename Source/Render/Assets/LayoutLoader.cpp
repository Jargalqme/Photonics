#include "pch.h"
#include "Render/Assets/LayoutLoader.h"
#include "Render/Assets/ImportedModelCache.h"
#include "Render/Assets/MeshCache.h"
#include "Services/SceneContext.h"
#include "ThirdParty/json.hpp"
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
    struct PrimitiveBinding
    {
        GeometricPrimitive* primitive = nullptr;
        float mayaScaleCorrection = 1.0f;
        std::string canonicalType;
    };

    std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    std::string readString(const nlohmann::json& object, const char* fieldName)
    {
        const auto it = object.find(fieldName);
        if (it != object.end() && it->is_string())
        {
            return it->get<std::string>();
        }
        return {};
    }

    bool readBool(const nlohmann::json& object, const char* fieldName, bool fallback)
    {
        const auto it = object.find(fieldName);
        if (it != object.end() && it->is_boolean())
        {
            return it->get<bool>();
        }
        return fallback;
    }

    float readFloat(const nlohmann::json& object, const char* fieldName, float fallback)
    {
        const auto it = object.find(fieldName);
        if (it != object.end() && it->is_number())
        {
            return it->get<float>();
        }
        return fallback;
    }

    const nlohmann::json* findArrayField(
        const nlohmann::json& object,
        std::initializer_list<const char*> fieldNames,
        std::size_t minSize = 3)
    {
        for (const char* fieldName : fieldNames)
        {
            const auto it = object.find(fieldName);
            if (it != object.end() && it->is_array() && it->size() >= minSize)
            {
                return &(*it);
            }
        }
        return nullptr;
    }

    Vector3 readVector3(
        const nlohmann::json& object,
        std::initializer_list<const char*> fieldNames,
        const Vector3& fallback)
    {
        const nlohmann::json* values = findArrayField(object, fieldNames);
        if (!values)
        {
            return fallback;
        }

        return Vector3(
            (*values)[0].get<float>(),
            (*values)[1].get<float>(),
            (*values)[2].get<float>());
    }

    Vector2 readVector2(
        const nlohmann::json& object,
        std::initializer_list<const char*> fieldNames,
        const Vector2& fallback)
    {
        const nlohmann::json* values = findArrayField(object, fieldNames, 2);
        if (!values || values->size() < 2)
        {
            return fallback;
        }

        return Vector2(
            (*values)[0].get<float>(),
            (*values)[1].get<float>());
    }

    Color readColor(const nlohmann::json& object)
    {
        const nlohmann::json* values = findArrayField(object, { "color", "colour" });
        if (!values)
        {
            return Color(1.0f, 1.0f, 1.0f, 1.0f);
        }

        const float alpha = values->size() >= 4 ? (*values)[3].get<float>() : 1.0f;
        return Color(
            (*values)[0].get<float>(),
            (*values)[1].get<float>(),
            (*values)[2].get<float>(),
            alpha);
    }

    std::string namePrefix(const std::string& name)
    {
        const std::string::size_type pathSeparator = name.find_last_of("|/");
        const std::string shortName = pathSeparator == std::string::npos
            ? name
            : name.substr(pathSeparator + 1);

        const std::string::size_type separator = shortName.find('_');
        return toLower(separator == std::string::npos
            ? shortName
            : shortName.substr(0, separator));
    }

    bool isMarkerType(const std::string& rawType)
    {
        const std::string type = toLower(rawType);
        return type == "socket"
            || type == "marker"
            || type == "player"
            || type == "player_start"
            || type == "playerstart"
            || type == "spawn"
            || type == "entry"
            || type == "entry_gate"
            || type == "exit"
            || type == "exit_gate";
    }

    PrimitiveBinding resolvePrimitive(SceneContext& context, const std::string& rawType)
    {
        PrimitiveBinding binding;
        if (!context.meshes)
        {
            return binding;
        }

        const std::string type = toLower(rawType);
        if (type == "box" || type == "cube" || type == "platform" || type == "wall"
            || type == "floor" || type == "ceiling" || type == "block")
        {
            binding.primitive = context.meshes->getCube();
            binding.canonicalType = "box";
        }
        else if (type == "cylinder" || type == "pillar" || type == "column")
        {
            binding.primitive = context.meshes->getCylinder();
            binding.mayaScaleCorrection = 2.0f;
            binding.canonicalType = "cylinder";
        }
        else if (type == "sphere")
        {
            binding.primitive = context.meshes->getSphere();
            binding.mayaScaleCorrection = 2.0f;
            binding.canonicalType = "sphere";
        }
        else if (type == "torus" || type == "ring")
        {
            binding.primitive = context.meshes->getTorus(1.0f, 0.333f);
            binding.mayaScaleCorrection = 2.0f;
            binding.canonicalType = "torus";
        }
        else if (type == "cone")
        {
            binding.primitive = context.meshes->getCone();
            binding.mayaScaleCorrection = 2.0f;
            binding.canonicalType = "cone";
        }
        else if (type == "octahedron" || type == "octa")
        {
            binding.primitive = context.meshes->getOctahedron();
            binding.canonicalType = "octahedron";
        }
        else if (type == "icosahedron" || type == "icosa")
        {
            binding.primitive = context.meshes->getIcosahedron();
            binding.canonicalType = "icosahedron";
        }

        return binding;
    }

    Transform readTransform(
        const nlohmann::json& object,
        float scaleCorrection,
        bool mirrorX,
        float worldScale)
    {
        Transform transform;

        Vector3 position = readVector3(object, { "position", "pos" }, Vector3::Zero);
        Vector3 rotationDegrees = readVector3(object, { "rotation", "rot" }, Vector3::Zero);
        Vector3 scale = readVector3(object, { "scale", "size" }, Vector3::One);

        if (mirrorX)
        {
            position.x = -position.x;
            rotationDegrees.y = -rotationDegrees.y;
            rotationDegrees.z = -rotationDegrees.z;
        }

        transform.position = position * worldScale;
        transform.rotation = Vector3(
            XMConvertToRadians(rotationDegrees.x),
            XMConvertToRadians(rotationDegrees.y),
            XMConvertToRadians(rotationDegrees.z));
        transform.scale = scale * scaleCorrection * worldScale;
        return transform;
    }

    void appendMarker(
        const nlohmann::json& object,
        bool mirrorX,
        float worldScale,
        PrimitiveLayout& outLayout)
    {
        PrimitiveLayoutMarker marker;
        marker.name = readString(object, "name");
        marker.type = readString(object, "type");
        if (marker.type.empty())
        {
            marker.type = readString(object, "marker");
        }
        if (marker.type.empty())
        {
            marker.type = namePrefix(marker.name);
        }
        marker.localTransform = readTransform(object, 1.0f, mirrorX, worldScale);
        outLayout.markers.push_back(std::move(marker));
    }

    std::filesystem::path resolvePBRMaterialDirectory(const std::string& materialName)
    {
        if (materialName.empty())
        {
            return {};
        }

        return std::filesystem::path(GetAssetPath(L"Textures/PBR")) / materialName;
    }

    void appendPrimitiveObject(
        SceneContext& context,
        const nlohmann::json& object,
        bool defaultMayaScaleCorrection,
        bool mirrorX,
        float worldScale,
        PrimitiveLayout& outLayout)
    {
        const std::string name = readString(object, "name");
        const std::string semanticType = readString(object, "type");
        const std::string prefix = namePrefix(name);

        std::string primitiveCandidate = readString(object, "primitive");
        if (primitiveCandidate.empty())
        {
            primitiveCandidate = semanticType;
        }

        PrimitiveBinding binding = resolvePrimitive(context, primitiveCandidate);
        if (!binding.primitive && !prefix.empty() && prefix != toLower(primitiveCandidate))
        {
            binding = resolvePrimitive(context, prefix);
        }

        if (!binding.primitive)
        {
            if (isMarkerType(primitiveCandidate) || isMarkerType(semanticType) || isMarkerType(prefix))
            {
                appendMarker(object, mirrorX, worldScale, outLayout);
            }
            return;
        }

        bool applyMayaScaleCorrection = defaultMayaScaleCorrection;
        applyMayaScaleCorrection = readBool(
            object,
            "apply_maya_primitive_scale_correction",
            applyMayaScaleCorrection);

        float scaleCorrection = applyMayaScaleCorrection ? binding.mayaScaleCorrection : 1.0f;
        scaleCorrection = readFloat(object, "scale_correction", scaleCorrection);

        PrimitiveLayoutPart part;
        part.primitive = binding.primitive;
        part.localTransform = readTransform(object, scaleCorrection, mirrorX, worldScale);
        part.color = readColor(object);
        part.name = name;
        part.type = semanticType.empty() ? binding.canonicalType : semanticType;
        part.primitiveType = binding.canonicalType;
        part.collidable = readBool(object, "collidable", true);

        const std::string pbrMaterial = readString(object, "pbr_material");
        if (!pbrMaterial.empty() && binding.canonicalType == "box" && context.importedModels)
        {
            const Vector2 uvTiling = readVector2(
                object,
                { "pbr_uv_tiling", "uv_tiling", "texture_tiling" },
                Vector2(1.0f, 1.0f));

            part.importedModel = context.importedModels->getPBRBoxWithAmbientCGMaterial(
                resolvePBRMaterialDirectory(pbrMaterial),
                uvTiling);
        }

        outLayout.parts.push_back(std::move(part));
    }

    bool defaultMayaScaleCorrectionFor(const nlohmann::json& data, bool legacyPartsFormat)
    {
        const std::string scaleSpace = toLower(readString(data, "scale_space"));
        if (scaleSpace == "maya")
        {
            return true;
        }
        if (scaleSpace == "engine")
        {
            return false;
        }

        return readBool(data, "apply_maya_primitive_scale_correction", legacyPartsFormat);
    }
}

bool LayoutLoader::loadLayout(
    SceneContext& context,
    const std::string& jsonPath,
    PrimitiveLayout& outLayout)
{
    outLayout = PrimitiveLayout{};

    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        return false;
    }

    nlohmann::json data;
    try
    {
        file >> data;
    }
    catch (const nlohmann::json::exception&)
    {
        return false;
    }

    outLayout.name = readString(data, "name");
    outLayout.root = readString(data, "root");
    outLayout.worldScale = readFloat(data, "world_scale", readFloat(data, "layout_scale", 1.0f));

    const bool hasObjects = data.contains("objects") && data["objects"].is_array();
    const bool hasParts = data.contains("parts") && data["parts"].is_array();
    if (!hasObjects && !hasParts)
    {
        return false;
    }

    const bool legacyPartsFormat = !hasObjects && hasParts;
    const bool mirrorX = readBool(data, "mirror_x", legacyPartsFormat);
    const bool applyMayaScaleCorrection = defaultMayaScaleCorrectionFor(data, legacyPartsFormat);
    const float worldScale = outLayout.worldScale;

    const nlohmann::json& primitives = hasObjects ? data["objects"] : data["parts"];
    for (const auto& object : primitives)
    {
        if (object.is_object())
        {
            appendPrimitiveObject(context, object, applyMayaScaleCorrection, mirrorX, worldScale, outLayout);
        }
    }

    if (data.contains("markers") && data["markers"].is_array())
    {
        for (const auto& marker : data["markers"])
        {
            if (marker.is_object())
            {
                appendMarker(marker, mirrorX, worldScale, outLayout);
            }
        }
    }

    return true;
}

bool LayoutLoader::loadPrimitiveLayout(
    SceneContext& context,
    const std::string& jsonPath,
    std::vector<PrimitiveLayoutPart>& outParts)
{
    outParts.clear();

    PrimitiveLayout layout;
    if (!loadLayout(context, jsonPath, layout))
    {
        return false;
    }

    outParts = std::move(layout.parts);
    return true;
}
