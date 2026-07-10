//---------------------------------------------------------------------------
//! @file   SceneContext.h
//! @brief  サービスコンテキスト (シーンに渡す参照束)
//---------------------------------------------------------------------------
#pragma once

class Renderer;
class InputManager;
class AudioManager;
class ShaderCache;
class MeshCache;
class ImportedModelCache;

namespace DX { class DeviceResources; }

//===========================================================================
//! シーンに渡す共有サービスの参照束 (CryEngine の gEnv 相当)
//! 全て非所有 — 実体の寿命は Game が持つ
//===========================================================================
struct SceneContext
{
	DX::DeviceResources*   device       = nullptr;
	Renderer*              renderer     = nullptr;
	InputManager*          input        = nullptr;
	AudioManager*          audio        = nullptr;
	ShaderCache*           shaders      = nullptr;
	MeshCache*             meshes       = nullptr;
	ImportedModelCache*    importedModels = nullptr;
	DirectX::CommonStates* commonStates = nullptr;
};
