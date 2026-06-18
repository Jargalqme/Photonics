#pragma once

#include "Render/Assets/ImportedModel.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ImportedModelCache
{
public:
    void initialize(ID3D11Device* device);
    void finalize();

    const ImportedModel* get(const std::string& path);
    const ImportedModel* getWithAmbientCGMaterial(
        const std::string& path,
        const std::filesystem::path& materialDirectory);
    const ImportedModel* getPBRBoxWithAmbientCGMaterial(
        const std::filesystem::path& materialDirectory,
        DirectX::SimpleMath::Vector2 uvTiling = DirectX::SimpleMath::Vector2(1.0f, 1.0f));

private:
    ID3D11Device* m_device = nullptr;
    std::unordered_map<std::string, std::unique_ptr<ImportedModel>> m_models;
};
