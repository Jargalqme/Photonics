#pragma once

#include <filesystem>
#include <string>

struct ImportedModelData;

class AssimpModelImporter
{
public:
    static bool LoadImportedModelData(
        const std::filesystem::path& path,
        ImportedModelData& outData,
        std::string* outError = nullptr);

private:
    static std::filesystem::path ResolvePath(const std::filesystem::path& path);
};
