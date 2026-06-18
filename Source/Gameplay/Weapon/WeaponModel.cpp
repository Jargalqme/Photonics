#include "pch.h"
#include "Gameplay/Weapon/WeaponModel.h"
#include "Render/Assets/ImportedModel.h"
#include "Render/Assets/ImportedModelCache.h"
#include "Render/Pipeline/RenderCommandQueue.h"
#include "Services/SceneContext.h"
#include <limits>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
    struct ImportedModelBounds
    {
        Vector3 center = Vector3::Zero;
        float longestSide = 1.0f;
    };

    ImportedModelBounds calculateImportedModelBounds(const ImportedModel& model)
    {
        const auto& vertices = model.data().vertices;
        if (vertices.empty())
        {
            return ImportedModelBounds{};
        }

        const float maxFloat = std::numeric_limits<float>::max();
        Vector3 minBounds(maxFloat, maxFloat, maxFloat);
        Vector3 maxBounds(-maxFloat, -maxFloat, -maxFloat);

        for (const auto& vertex : vertices)
        {
            minBounds.x = std::min(minBounds.x, vertex.position.x);
            minBounds.y = std::min(minBounds.y, vertex.position.y);
            minBounds.z = std::min(minBounds.z, vertex.position.z);
            maxBounds.x = std::max(maxBounds.x, vertex.position.x);
            maxBounds.y = std::max(maxBounds.y, vertex.position.y);
            maxBounds.z = std::max(maxBounds.z, vertex.position.z);
        }

        const Vector3 size = maxBounds - minBounds;
        const float longestSide = std::max({ size.x, size.y, size.z });
        ImportedModelBounds bounds;
        bounds.center = (minBounds + maxBounds) * 0.5f;
        bounds.longestSide = longestSide > 0.001f ? longestSide : 1.0f;
        return bounds;
    }

    Matrix createImportedRifleLocalMatrix(
        const Vector3& center,
        float longestSide,
        const RifleModelSettings& settings)
    {
        const float scale = settings.targetLength / std::max(longestSide, 0.001f);
        const Vector3 rotationRadians(
            XMConvertToRadians(settings.rotationDegrees.x),
            XMConvertToRadians(settings.rotationDegrees.y),
            XMConvertToRadians(settings.rotationDegrees.z));

        return Matrix::CreateTranslation(-center)
            * Matrix::CreateScale(scale)
            * Matrix::CreateFromYawPitchRoll(
                rotationRadians.y,
                rotationRadians.x,
                rotationRadians.z)
            * Matrix::CreateTranslation(settings.position);
    }
}

bool WeaponModel::loadRifle(SceneContext& context, const std::string& path)
{
    m_importedRifle = nullptr;
    m_importedRifleCenter = Vector3::Zero;
    m_importedRifleLongestSide = 1.0f;

    if (!context.importedModels)
    {
        return false;
    }

    const ImportedModel* model = context.importedModels->get(path);
    if (!model)
    {
        return false;
    }

    m_importedRifle = model;
    const ImportedModelBounds bounds = calculateImportedModelBounds(*model);
    m_importedRifleCenter = bounds.center;
    m_importedRifleLongestSide = bounds.longestSide;
    return true;
}

void WeaponModel::submit(RenderCommandQueue& queue, const Matrix& rootWorld) const
{
    if (m_importedRifle)
    {
        ImportedModelCommand command;
        command.model = m_importedRifle;
        command.world = buildModelWorldMatrix(rootWorld);
        command.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        queue.submit(command);
        return;
    }
}

Matrix WeaponModel::buildModelWorldMatrix(const Matrix& rootWorld) const
{
    if (!m_importedRifle)
    {
        return rootWorld;
    }

    return createImportedRifleLocalMatrix(
        m_importedRifleCenter,
        m_importedRifleLongestSide,
        m_rifleSettings) * rootWorld;
}

Vector3 WeaponModel::getMuzzleLocalPosition() const
{
    if (m_importedRifle)
    {
        if (const ImportedModelNode* muzzle = m_importedRifle->findNamedNode("VM_Muzzle"))
        {
            return muzzle->position;
        }
    }

    return Vector3(0.0f, 0.0f, kFallbackMuzzleZ);
}

void WeaponModel::finalize()
{
    m_importedRifle = nullptr;
    m_importedRifleCenter = Vector3::Zero;
    m_importedRifleLongestSide = 1.0f;
}

void WeaponModel::resetRifleSettings()
{
    m_rifleSettings = RifleModelSettings{};
}
