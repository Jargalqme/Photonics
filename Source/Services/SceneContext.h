//=============================================================================
// @brief    サービスコンテキスト　ー　シーンに渡す参照束（CryEngine gEnv）
//=============================================================================
#pragma once

class Game;
class Renderer;
class InputManager;
class AudioManager;
class ShaderCache;
class MeshCache;
class ImportedModelCache;

namespace DX { class DeviceResources; }
// 注意：DirectX::CommonStates は inline namespace DX11 内で定義されているため
// 自前で前方宣言すると新しい型を導入してしまい、本物と曖昧になる。
// pch.h 経由で CommonStates.h が読み込まれるためここでは型可視性を依存する。

struct SceneContext
{
	Game*                  game         = nullptr;   // ウィンドウモード等のオーケストレーション呼び出し用
	DX::DeviceResources*   device       = nullptr;
	Renderer*              renderer     = nullptr;
	InputManager*          input        = nullptr;
	AudioManager*          audio        = nullptr;
	ShaderCache*           shaders      = nullptr;
	MeshCache*             meshes       = nullptr;
	ImportedModelCache*    importedModels = nullptr;
	DirectX::CommonStates* commonStates = nullptr;
};
