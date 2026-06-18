#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ImportedModelData;

struct AssimpMeshReport
{
    std::string name;
    uint32_t vertexCount = 0;
    uint32_t faceCount = 0;
    uint32_t materialIndex = 0;
    bool hasNormals = false;
    bool hasTexCoords = false;
    bool hasTangents = false;
};

struct AssimpMaterialReport
{
    std::string name;
    std::array<float, 4> baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::string baseColorTexture;
    std::string diffuseTexture;
};

struct AssimpNamedNodeReport
{
    std::string name;
    std::array<float, 3> position = { 0.0f, 0.0f, 0.0f };
    uint32_t meshCount = 0;
};

struct AssimpModelReport
{
    bool loaded = false;
    std::filesystem::path requestedPath;
    std::filesystem::path resolvedPath;
    std::string error;
    uint32_t nodeCount = 0;
    uint32_t meshCount = 0;
    uint32_t materialCount = 0;
    uint32_t textureCount = 0;
    std::vector<AssimpMeshReport> meshes;
    std::vector<AssimpMaterialReport> materials;
    std::vector<AssimpNamedNodeReport> namedNodes;
};

class AssimpModelImporter
{
public:
    static AssimpModelReport Inspect(const std::filesystem::path& path);
    static bool LoadImportedModelData(
        const std::filesystem::path& path,
        ImportedModelData& outData,
        std::string* outError = nullptr);
    static void DumpReport(const AssimpModelReport& report);
    static bool WriteReportFile(
        const AssimpModelReport& report,
        const std::filesystem::path& path);

private:
    static std::filesystem::path ResolvePath(const std::filesystem::path& path);
};
