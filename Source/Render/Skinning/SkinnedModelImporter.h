#pragma once

#include "SkinnedModelData.h"

#include <cstdint>
#include <filesystem>
#include <string>

class SkinnedModelImporter
{
public:
    static bool LoadSkinnedModelData(
        const std::filesystem::path& path,
        SkinnedModelData& outData,
        std::string* outError = nullptr);

    static int32_t AppendClipsFromFile(
        const std::filesystem::path& path,
        SkinnedModelData& existingData,
        std::string* outError = nullptr);

private:
    static std::filesystem::path ResolvePath(const std::filesystem::path& path);
};
