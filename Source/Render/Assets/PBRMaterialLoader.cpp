#include "pch.h"
#include "Render/Assets/PBRMaterialLoader.h"
#include <fstream>

namespace
{
	void ReportError(std::string* outError, const std::string& message)
	{
		if (outError)
		{
			*outError = message;
		}
	}

	std::filesystem::path FindTextureFile(const std::filesystem::path& directory, const char* name)
	{
		const std::filesystem::path jpg = directory / (std::string(name) + ".jpg");

		if (std::filesystem::exists(jpg))
		{
			return jpg;
		}

		return {};
	}

	bool ReadBinaryFile(const std::filesystem::path& path, std::vector<uint8_t>& outBytes)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			return false;
		}

		const std::ifstream::pos_type fileSize = file.tellg();
		if (fileSize <= 0)
		{
			return false;
		}

		outBytes.resize(static_cast<size_t>(fileSize));
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(outBytes.data()), static_cast<std::streamsize>(fileSize));
		return file.good();
	}

	int32_t AppendTexture(
		ImportedModelData& data,
		const std::filesystem::path& path,
		bool srgb,
		bool forceRGBA32)
	{
		if (path.empty())
		{
			return IMPORTED_TEXTURE_NONE;
		}

		ImportedTextureData texture;
		texture.name = path.filename().generic_string();
		texture.source = path.generic_string();
		texture.formatHint = path.extension().string();
		texture.compressed = true;
		texture.srgb = srgb;
		texture.forceRGBA32 = forceRGBA32;

		if (!ReadBinaryFile(path, texture.bytes))
		{
			return IMPORTED_TEXTURE_NONE;
		}

		const int32_t index = static_cast<int32_t>(data.textures.size());
		data.textures.push_back(std::move(texture));
		return index;
	}
}

bool PBRMaterialLoader::ApplyAmbientCGTextureSet(
	ImportedModelData& data,
	uint32_t materialIndex,
	const std::filesystem::path& materialDirectory,
	std::string* outError)
{
	if (!std::filesystem::is_directory(materialDirectory))
	{
		ReportError(outError, "Material directory does not exist: " + materialDirectory.generic_string());
		return false;
	}

	if (materialIndex >= data.materials.size())
	{
		ReportError(outError, "Material index is out of range.");
		return false;
	}

	const std::filesystem::path colorPath = FindTextureFile(materialDirectory, "Color");
	const std::filesystem::path normalPath = FindTextureFile(materialDirectory, "NormalDX");
	const std::filesystem::path roughnessPath = FindTextureFile(materialDirectory, "Roughness");

	if (colorPath.empty() || normalPath.empty() || roughnessPath.empty())
	{
		ReportError(outError, "Expected Color, NormalDX, and Roughness textures.");
		return false;
	}

	const int32_t colorIndex = AppendTexture(data, colorPath, true, false);
	const int32_t normalIndex = AppendTexture(data, normalPath, false, true);
	const int32_t roughnessIndex = AppendTexture(data, roughnessPath, false, true);

	if (colorIndex == IMPORTED_TEXTURE_NONE ||
		normalIndex == IMPORTED_TEXTURE_NONE ||
		roughnessIndex == IMPORTED_TEXTURE_NONE)
	{
		ReportError(outError, "Failed to read one or more required PBR textures.");
		return false;
	}

	ImportedMaterial& material = data.materials[materialIndex];
	if (material.name.empty())
	{
		material.name = materialDirectory.filename().generic_string();
	}

	material.baseColorTexture      = colorPath.generic_string();
	material.baseColorTextureIndex = colorIndex;
	material.normalTextureIndex    = normalIndex;
	material.roughnessTextureIndex = roughnessIndex;

	material.metallicRoughnessTextureIndex = IMPORTED_TEXTURE_NONE;
	material.roughnessFactor = 1.0f;
	material.metallicFactor  = 0.0f;

	const int32_t metalnessIndex = AppendTexture(
		data,
		FindTextureFile(materialDirectory, "Metalness"),
		false,
		true);
	material.metalnessTextureIndex = metalnessIndex;
	material.metallicFactor = metalnessIndex != IMPORTED_TEXTURE_NONE ? 1.0f : 0.0f;


	material.ambientOcclusionTextureIndex = AppendTexture(data, FindTextureFile(materialDirectory, "AmbientOcclusion"), false, true);
	material.heightTextureIndex           = AppendTexture(data, FindTextureFile(materialDirectory, "Displacement"), false, true);

	return true;
}