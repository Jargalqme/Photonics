//---------------------------------------------------------------------------
//! @file   RenderCommandQueue.h
//! @brief  描画コマンドキュー
//---------------------------------------------------------------------------
#pragma once

#include "Render/Pipeline/RenderCommand.h"

#include <vector>

//===========================================================================
//! 描画コマンドキュー
//! シーンが submit したコマンドを不透明/半透明バケットに振り分けて保持する
//! (毎フレーム clear -> submit -> 描画側がバケット単位で消費)
//===========================================================================
class RenderCommandQueue
{
public:
	void clear();

	//! blendMode で振り分けて積みます (Billboard は常に半透明)
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
