#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Render/Pipeline/Renderer.h"

#include "Source/Services/SceneContext.h"
#include "Source/Services/InputManager.h"

#include "Source/Scenes/SceneManager.h"
#include "Source/Scenes/BossScene.h"
#include "Source/Scenes/TrainingScene.h"
#include "Source/Scenes/IntroScene.h"
#include "Source/Scenes/MainMenuScene.h"
#include "Source/Scenes/VictoryScene.h"
#include "Source/Scenes/GameOverScene.h"

#include "Render/Assets/ShaderCache.h"
#include "Render/Assets/MeshCache.h"
#include "Render/Assets/ImportedModelCache.h"
#include <memory>

class Game final : public DX::IDeviceNotify
{
public:
    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    void Initialize(HWND window, int width, int height);
    void Tick();

    void applyResolution(int width, int height);

    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    void GetDefaultSize(int& width, int& height) const noexcept;

private:
    void Update(DX::StepTimer const& timer);
    void Render();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void ResizeWindowedClient(int width, int height);

    std::unique_ptr<DX::DeviceResources>    m_deviceResources;
    DX::StepTimer                           m_timer;
    SceneContext                            m_context;
    std::unique_ptr<Renderer>               m_renderer;
    std::unique_ptr<InputManager>           m_input;
    std::unique_ptr<ShaderCache>            m_shaders;
    std::unique_ptr<MeshCache>              m_meshes;
    std::unique_ptr<ImportedModelCache>     m_importedModels;
    std::unique_ptr<DirectX::CommonStates>  m_commonStates;
    std::unique_ptr<SceneManager>           m_sceneManager;
};
