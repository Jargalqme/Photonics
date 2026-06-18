#include "pch.h"
#include "Render/Pipeline/RenderCommandQueue.h"

void RenderCommandQueue::clear()
{
	m_opaqueMeshes.clear();
	m_transparentMeshes.clear();
	m_opaqueImportedModels.clear();
	m_transparentImportedModels.clear();
	m_transparentBillboards.clear();
}

void RenderCommandQueue::submit(const MeshCommand& command)
{
	if (command.blendMode == BlendMode::Opaque)
	{
		m_opaqueMeshes.push_back(command);
	}
	else
	{
		m_transparentMeshes.push_back(command);
	}
}

void RenderCommandQueue::submit(const BillboardCommand& command)
{
	m_transparentBillboards.push_back(command);
}

void RenderCommandQueue::submit(const ImportedModelCommand& command)
{
	if (command.blendMode == BlendMode::Opaque)
	{
		m_opaqueImportedModels.push_back(command);
	}
	else
	{
		m_transparentImportedModels.push_back(command);
	}
}
