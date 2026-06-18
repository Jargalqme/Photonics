#pragma once

#include "Render/Pipeline/RenderCommand.h"

#include <vector>

class RenderCommandQueue
{
public:
	void clear();
	void submit(const MeshCommand& command);
	void submit(const BillboardCommand& command);
	void submit(const ImportedModelCommand& command);

	const std::vector<MeshCommand>& opaqueMeshes() const { return m_opaqueMeshes; }
	const std::vector<MeshCommand>& transparentMeshes() const { return m_transparentMeshes; }
	const std::vector<ImportedModelCommand>& opaqueImportedModels() const { return m_opaqueImportedModels; }
	const std::vector<ImportedModelCommand>& transparentImportedModels() const { return m_transparentImportedModels; }
	const std::vector<BillboardCommand>& transparentBillboards() const { return m_transparentBillboards; }

private:
	std::vector<MeshCommand> m_opaqueMeshes;
	std::vector<MeshCommand> m_transparentMeshes;
	std::vector<ImportedModelCommand> m_opaqueImportedModels;
	std::vector<ImportedModelCommand> m_transparentImportedModels;
	std::vector<BillboardCommand> m_transparentBillboards;
};
