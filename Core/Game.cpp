#include "pch.h"
#include "Game.h"
#include "Render/Assets/AssimpModelImporter.h"

#include <algorithm>
#include <cwchar>

extern void ExitGame() noexcept;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    struct Resolution
    {
        int width;
        int height;
    };

    constexpr Resolution kDefaultResolution{ 1920, 1080 };
    constexpr Resolution kSupportedResolutions[] = {
        { 1920, 1080 },
        { 1920, 1200 },
    };

    bool IsSupportedResolution(int width, int height) noexcept
    {
        for (const auto& resolution : kSupportedResolutions)
        {
            if (resolution.width == width && resolution.height == height)
            {
                return true;
            }
        }

        return false;
    }

    void GetSettingsPath(wchar_t (&path)[MAX_PATH]) noexcept
    {
        path[0] = L'\0';

        DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", path, MAX_PATH);
        if (length > 0 && length < MAX_PATH)
        {
            if (wcscat_s(path, L"\\Photonics") == 0)
            {
                CreateDirectoryW(path, nullptr);

                if (wcscat_s(path, L"\\settings.ini") == 0)
                {
                    return;
                }
            }
        }

        wcscpy_s(path, L"photonics_settings.ini");
    }

    Resolution LoadResolutionSettings() noexcept
    {
        wchar_t path[MAX_PATH] = {};
        GetSettingsPath(path);

        const int width = static_cast<int>(GetPrivateProfileIntW(
            L"Display",
            L"Width",
            kDefaultResolution.width,
            path));

        const int height = static_cast<int>(GetPrivateProfileIntW(
            L"Display",
            L"Height",
            kDefaultResolution.height,
            path));

        if (IsSupportedResolution(width, height))
        {
            return { width, height };
        }

        return kDefaultResolution;
    }

    void SaveResolutionSettings(int width, int height) noexcept
    {
        wchar_t path[MAX_PATH] = {};
        GetSettingsPath(path);

        wchar_t value[16] = {};

        swprintf_s(value, L"%d", width);
        WritePrivateProfileStringW(L"Display", L"Width", value, path);

        swprintf_s(value, L"%d", height);
        WritePrivateProfileStringW(L"Display", L"Height", value, path);
    }
}

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_input = std::make_unique<InputManager>();
    m_renderer = std::make_unique<Renderer>(m_deviceResources.get());
    m_shaders = std::make_unique<ShaderCache>();
    m_meshes = std::make_unique<MeshCache>();
    m_importedModels = std::make_unique<ImportedModelCache>();
    m_sceneManager = std::make_unique<SceneManager>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();
    m_renderer->CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
    m_renderer->CreateWindowSizeDependentResources();

    m_input->initialize(window);

    m_shaders->initialize(m_deviceResources->GetD3DDevice());
    m_meshes->initialize(m_deviceResources->GetD3DDeviceContext());
    m_importedModels->initialize(m_deviceResources->GetD3DDevice());
    m_commonStates = std::make_unique<CommonStates>(m_deviceResources->GetD3DDevice());

    m_context.game = this;
    m_context.device = m_deviceResources.get();
    m_context.renderer = m_renderer.get();
    m_context.input = m_input.get();
    m_context.audio = nullptr;
    m_context.shaders = m_shaders.get();
    m_context.meshes = m_meshes.get();
    m_context.importedModels = m_importedModels.get();
    m_context.commonStates = m_commonStates.get();

#ifdef _DEBUG
    const AssimpModelReport rifleReport =
        AssimpModelImporter::Inspect("Assets/Weapons/Rifle/rifle.glb");
    AssimpModelImporter::DumpReport(rifleReport);
    AssimpModelImporter::WriteReportFile(rifleReport, "assimp_model_report.txt");
#endif

    m_sceneManager->initialize(m_context);

    auto introScene = std::make_unique<IntroScene>(m_sceneManager.get());
    m_sceneManager->addScene("Intro", std::move(introScene));

    auto mainMenuScene = std::make_unique<MainMenuScene>(m_sceneManager.get());
    m_sceneManager->addScene("MainMenu", std::move(mainMenuScene));

    auto bossScene = std::make_unique<BossScene>(m_sceneManager.get());
    m_sceneManager->addScene("BossScene", std::move(bossScene));

    auto trainingScene = std::make_unique<TrainingScene>(m_sceneManager.get());
    m_sceneManager->addScene("Training", std::move(trainingScene));

    auto victoryScene = std::make_unique<VictoryScene>(m_sceneManager.get());
    m_sceneManager->addScene("Victory", std::move(victoryScene));

    auto gameOverScene = std::make_unique<GameOverScene>(m_sceneManager.get());
    m_sceneManager->addScene("GameOver", std::move(gameOverScene));

    m_sceneManager->transitionTo("MainMenu");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 2;
    fontConfig.PixelSnapH = false;
    
    const std::wstring fontPath = GetAssetPath(L"Fonts/Roboto-Regular.ttf");
    std::string fontPathUtf8(fontPath.begin(), fontPath.end());

    io.FontDefault = io.Fonts->AddFontFromFileTTF(
        fontPathUtf8.c_str(),
        18.0f,
        &fontConfig);

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());
}

#pragma region Frame Update
void Game::Tick()
{
    m_timer.Tick([&]()
        {
            Update(m_timer);
        });
    Render();
}

void Game::Update(DX::StepTimer const& timer)
{
    float deltaTime = float(timer.GetElapsedSeconds());

    m_input->update();
    m_sceneManager->update(deltaTime, m_input.get());
}
#pragma endregion

#pragma region Frame Render
void Game::Render()
{
    m_renderer->BeginScene();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_sceneManager->render();

    m_renderer->EndScene();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_deviceResources->Present();
}
#pragma endregion

#pragma region Message Handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
}

void Game::applyResolution(int width, int height)
{
    if (!IsSupportedResolution(width, height))
    {
        return;
    }

    ResizeWindowedClient(width, height);
    SaveResolutionSettings(width, height);
}

void Game::ResizeWindowedClient(int width, int height)
{
    HWND hwnd = m_deviceResources->GetWindow();
    if (!hwnd)
    {
        return;
    }

    ShowWindow(hwnd, SW_RESTORE);

    const auto style = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE));
    const auto exStyle = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_EXSTYLE));

    RECT windowRect = { 0, 0, width, height };
    const BOOL hasMenu = GetMenu(hwnd) != nullptr;

    using AdjustWindowRectExForDpiFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto adjustForDpi = user32
        ? reinterpret_cast<AdjustWindowRectExForDpiFn>(
            GetProcAddress(user32, "AdjustWindowRectExForDpi"))
        : nullptr;
    auto getDpiForWindow = user32
        ? reinterpret_cast<GetDpiForWindowFn>(
            GetProcAddress(user32, "GetDpiForWindow"))
        : nullptr;

    if (adjustForDpi && getDpiForWindow)
    {
        adjustForDpi(&windowRect, style, hasMenu, exStyle, getDpiForWindow(hwnd));
    }
    else
    {
        AdjustWindowRectEx(&windowRect, style, hasMenu, exStyle);
    }

    const int windowWidth = windowRect.right - windowRect.left;
    const int windowHeight = windowRect.bottom - windowRect.top;

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFO);

    RECT workRect = {};
    if (GetMonitorInfo(monitor, &monitorInfo))
    {
        workRect = monitorInfo.rcWork;
    }
    else
    {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workRect, 0);
    }

    const int workWidth = workRect.right - workRect.left;
    const int workHeight = workRect.bottom - workRect.top;
    const int x = workRect.left + std::max(0, (workWidth - windowWidth) / 2);
    const int y = workRect.top + std::max(0, (workHeight - windowHeight) / 2);

    SetWindowPos(hwnd, nullptr,
        x, y,
        windowWidth, windowHeight,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    using DwmGetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD);
    constexpr DWORD DwmwaExtendedFrameBounds = 9;

    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    auto getDwmWindowAttribute = dwm
        ? reinterpret_cast<DwmGetWindowAttributeFn>(
            GetProcAddress(dwm, "DwmGetWindowAttribute"))
        : nullptr;

    RECT visibleFrame = {};
    RECT actualWindow = {};
    if (getDwmWindowAttribute
        && SUCCEEDED(getDwmWindowAttribute(
            hwnd,
            DwmwaExtendedFrameBounds,
            &visibleFrame,
            sizeof(visibleFrame)))
        && GetWindowRect(hwnd, &actualWindow))
    {
        const int visibleWidth = visibleFrame.right - visibleFrame.left;
        const int visibleHeight = visibleFrame.bottom - visibleFrame.top;
        const int visibleX = workRect.left + std::max(0, (workWidth - visibleWidth) / 2);
        const int visibleY = workRect.top + std::max(0, (workHeight - visibleHeight) / 2);

        const int correctedX = actualWindow.left + (visibleX - visibleFrame.left);
        const int correctedY = actualWindow.top + (visibleY - visibleFrame.top);

        if (correctedX != actualWindow.left || correctedY != actualWindow.top)
        {
            SetWindowPos(hwnd, nullptr,
                correctedX, correctedY,
                0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    if (dwm)
    {
        FreeLibrary(dwm);
    }

    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);
    OnWindowSizeChanged(clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top);
}

void Game::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
    {
        return;
    }

    CreateWindowSizeDependentResources();
    m_renderer->CreateWindowSizeDependentResources();
}

void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    const Resolution resolution = LoadResolutionSettings();
    width = resolution.width;
    height = resolution.height;
}
#pragma endregion

#pragma region Direct3D Resources
void Game::CreateDeviceDependentResources()
{
}

void Game::CreateWindowSizeDependentResources()
{
}

void Game::OnDeviceLost()
{
    ImGui_ImplDX11_InvalidateDeviceObjects();
    m_renderer->OnDeviceLost();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    m_renderer->CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
    m_renderer->CreateWindowSizeDependentResources();

    ImGui_ImplDX11_CreateDeviceObjects();
}
#pragma endregion
