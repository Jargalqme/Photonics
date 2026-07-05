//---------------------------------------------------------------------------
//! @file   Game.h
//! @brief  ゲーム本体 — メインループと全サブシステムの所有者
//---------------------------------------------------------------------------
#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Source/Services/SceneContext.h"

#include <memory>

class Renderer;
class InputManager;
class ShaderCache;
class MeshCache;
class ImportedModelCache;
class SceneManager;

class Game final : public DX::IDeviceNotify
{
public:
	Game() noexcept(false);
	~Game();

	Game(Game const&) = delete;
	Game& operator=(Game const&) = delete;
	Game(Game&&) = delete;
	Game& operator=(Game&&) = delete;

	//! ウィンドウ作成後の一括初期化（D3D -> サブシステム -> シーン登録 -> ImGui）
	void Initialize(HWND window, int width, int height);

	//! 1フレーム進める（更新 + 描画）メッセージの無い間、呼ばれ続ける
	void Tick();

	// IDeviceNotify — デバイスロスト / 復帰
	void OnDeviceLost() override;
	void OnDeviceRestored() override;

	// ウィンドウメッセージ（Main.cpp の WndProc から呼ばれる）
	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowMoved();
	void OnDisplayChange();
	void OnWindowSizeChanged(int width, int height);

	//! 既定のクライアント領域サイズを取得します
	void GetDefaultSize(int& width, int& height) const noexcept;

private:
	void Update(DX::StepTimer const& timer);
	void Render();

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	// 基盤 — デバイスは全サブシステムから参照されるため先頭（構築が最初・破棄が最後）
	std::unique_ptr<DX::DeviceResources>   m_deviceResources;
	DX::StepTimer                          m_timer;

	// サブシステム
	std::unique_ptr<Renderer>              m_renderer;
	std::unique_ptr<InputManager>          m_input;
	std::unique_ptr<ShaderCache>           m_shaders;
	std::unique_ptr<MeshCache>             m_meshes;
	std::unique_ptr<ImportedModelCache>    m_importedModels;
	std::unique_ptr<DirectX::CommonStates> m_commonStates;

	// シーン
	std::unique_ptr<SceneManager>          m_sceneManager;
	SceneContext                           m_context;   //!< シーンへ配る非所有サービス参照の束
};
