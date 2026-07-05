//=============================================================================
// @brief    サービスコンテキスト　ー　シーンに渡す参照束（CryEngine gEnv）
//=============================================================================
#pragma once

class Renderer;
class InputManager;
class AudioManager;
class ShaderCache;
class MeshCache;
class ImportedModelCache;

namespace DX { class DeviceResources; }

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
