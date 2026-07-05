//---------------------------------------------------------------------------
//! @file   DeviceResources.cpp
//! @brief  Direct3D 11 デバイスとスワップチェーンのラッパー
//---------------------------------------------------------------------------
#include "pch.h"
#include "DeviceResources.h"

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
#if defined(_DEBUG)
    // SDKレイヤーのサポートを確認
    inline bool SdkLayersAvailable() noexcept
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,       // NULL ドライバー
            nullptr,
            D3D11_CREATE_DEVICE_DEBUG,  // デバッグレイヤーをサポートするデバイスを作成
            nullptr,
            0,
            D3D11_SDK_VERSION,
            nullptr,
            nullptr,
            nullptr
            );

        return SUCCEEDED(hr);
    }
#endif

    inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt) noexcept
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
        default:                                return fmt;
        }
    }

    inline long ComputeIntersectionArea(
        long ax1, long ay1, long ax2, long ay2,
        long bx1, long by1, long bx2, long by2) noexcept
    {
        return std::max(0l, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0l, std::min(ay2, by2) - std::max(ay1, by1));
    }
}

// DeviceResourcesのコンストラクタ
DeviceResources::DeviceResources(
    DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    UINT backBufferCount,
    D3D_FEATURE_LEVEL minFeatureLevel,
    unsigned int flags) noexcept :
        m_screenViewport{},
        m_backBufferFormat(backBufferFormat),
        m_depthBufferFormat(depthBufferFormat),
        m_backBufferCount(backBufferCount),
        m_d3dMinFeatureLevel(minFeatureLevel),
        m_window(nullptr),
        m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
        m_outputSize{0, 0, 1, 1},
        m_colorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709),
        m_options(flags | c_FlipPresent),
        m_deviceNotify(nullptr)
{
}

// Direct3Dデバイスを設定し、そのデバイス及びデバイスコンテキストへのハンドルを格納します
void DeviceResources::CreateDeviceResources()
{
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    if (SdkLayersAvailable())
    {
        // デバッグビルドでは SDK レイヤーによるデバッグを有効化
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
    else
    {
        OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
    }
#endif

    CreateFactory();

    // ボーダーレスフルスクリーン向けにテアリング対応の有無を確認
    if (m_options & c_AllowTearing)
    {
        BOOL allowTearing = FALSE;

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = m_dxgiFactory.As(&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            m_options &= ~c_AllowTearing;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
    }

    // FLIP スワップエフェクト非対応の OS では HDR を無効化
    if (m_options & c_EnableHDR)
    {
        ComPtr<IDXGIFactory5> factory5;
        if (FAILED(m_dxgiFactory.As(&factory5)))
        {
            m_options &= ~c_EnableHDR;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: HDR swap chains not supported");
#endif
        }
    }

    // 非対応 OS では FLIP を無効化
    if (m_options & c_FlipPresent)
    {
        ComPtr<IDXGIFactory4> factory4;
        if (FAILED(m_dxgiFactory.As(&factory4)))
        {
            m_options &= ~c_FlipPresent;
#ifdef _DEBUG
            OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
        }
    }

    // このアプリが対応する DirectX ハードウェア機能レベル
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    UINT featLevelCount = 0;
    for (; featLevelCount < static_cast<UINT>(std::size(s_featureLevels)); ++featLevelCount)
    {
        if (s_featureLevels[featLevelCount] < m_d3dMinFeatureLevel)
            break;
    }

    if (!featLevelCount)
    {
        throw std::out_of_range("minFeatureLevel too high");
    }

    ComPtr<IDXGIAdapter1> adapter;
    GetHardwareAdapter(adapter.GetAddressOf());

    // Direct3D 11 デバイスと対応するコンテキストを作成
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = E_FAIL;
    if (adapter)
    {
        hr = D3D11CreateDevice(
            adapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            creationFlags,
            s_featureLevels,
            featLevelCount,
            D3D11_SDK_VERSION,
            device.GetAddressOf(),  // 作成されたデバイスを受け取る
            &m_d3dFeatureLevel,     // 実際に得られた機能レベルを受け取る
            context.GetAddressOf()  // 即時コンテキストを受け取る
            );
    }
#if defined(NDEBUG)
    else
    {
        throw std::runtime_error("No Direct3D hardware device found");
    }
#else
    if (FAILED(hr))
    {
        // 初期化に失敗した場合は WARP デバイス（ソフトウェアラスタライザ）へフォールバック。
        // WARP の詳細: http://go.microsoft.com/fwlink/?LinkId=286690
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP, // ハードウェアデバイスの代わりに WARP デバイスを作成
            nullptr,
            creationFlags,
            s_featureLevels,
            featLevelCount,
            D3D11_SDK_VERSION,
            device.GetAddressOf(),
            &m_d3dFeatureLevel,
            context.GetAddressOf()
            );

        if (SUCCEEDED(hr))
        {
            OutputDebugStringA("Direct3D Adapter - WARP\n");
        }
    }
#endif

    ThrowIfFailed(hr);

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    ThrowIfFailed(device.As(&m_d3dDevice));
    ThrowIfFailed(context.As(&m_d3dContext));
    ThrowIfFailed(context.As(&m_d3dAnnotation));
}

// ウィンドウサイズが変わるたびに作り直しが必要なリソース群
void DeviceResources::CreateWindowSizeDependentResources()
{
    if (!m_window)
    {
        throw std::logic_error("Call SetWindow with a valid Win32 window handle");
    }

    // 以前のウィンドウサイズ固有のバインドを解除
    m_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_d3dRenderTargetView.Reset();
    m_d3dDepthStencilView.Reset();
    m_renderTarget.Reset();
    m_depthStencil.Reset();
    m_d3dContext->Flush();

    // レンダーターゲットサイズ（ピクセル）を決定
    const UINT backBufferWidth = std::max<UINT>(static_cast<UINT>(m_outputSize.right - m_outputSize.left), 1u);
    const UINT backBufferHeight = std::max<UINT>(static_cast<UINT>(m_outputSize.bottom - m_outputSize.top), 1u);
    const DXGI_FORMAT backBufferFormat = (m_options & (c_FlipPresent | c_AllowTearing | c_EnableHDR)) ? NoSRGB(m_backBufferFormat) : m_backBufferFormat;

    if (m_swapChain)
    {
        // スワップチェーンが既にあればリサイズ
        HRESULT hr = m_swapChain->ResizeBuffers(
            m_backBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
            );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            // デバイスが失われた場合はデバイスとスワップチェーンの再作成が必要
            HandleDeviceLost();

            // HandleDeviceLost がこのメソッドへ再入して新しいデバイスを正しく構築するため、
            // ここで処理を打ち切る
            return;
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }
    else
    {
        // スワップチェーンの記述子を作成
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = m_backBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = (m_options & (c_FlipPresent | c_AllowTearing | c_EnableHDR)) ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Win32 ウィンドウからスワップチェーンを作成
        ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr, m_swapChain.ReleaseAndGetAddressOf()
            ));

        // このクラスは排他フルスクリーン非対応。DXGI が ALT+ENTER に反応しないようにする
        ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // HDR 用の色空間設定
    UpdateColorSpace();

    // バックバッファのレンダーターゲットビューを作成
    ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(m_renderTarget.ReleaseAndGetAddressOf())));

    CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, m_backBufferFormat);
    ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(
        m_renderTarget.Get(),
        &renderTargetViewDesc,
        m_d3dRenderTargetView.ReleaseAndGetAddressOf()
        ));

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // 3D 描画用のデプスステンシルビューを作成（必要な場合のみ）
        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            m_depthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1, // 配列サイズは 1
            1, // ミップレベルは 1
            D3D11_BIND_DEPTH_STENCIL
            );

        ThrowIfFailed(m_d3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            m_depthStencil.ReleaseAndGetAddressOf()
            ));

        ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(
            m_depthStencil.Get(),
            nullptr,
            m_d3dDepthStencilView.ReleaseAndGetAddressOf()
            ));
    }

    // 3D 描画ビューポートをウィンドウ全体に設定
    m_screenViewport = { 0.0f, 0.0f, static_cast<float>(backBufferWidth), static_cast<float>(backBufferHeight), 0.f, 1.f };
}

// Win32 ウィンドウの作成（再作成）時に呼ばれる
void DeviceResources::SetWindow(HWND window, int width, int height) noexcept
{
    m_window = window;

    m_outputSize.left = m_outputSize.top = 0;
    m_outputSize.right = static_cast<long>(width);
    m_outputSize.bottom = static_cast<long>(height);
}

// Win32 ウィンドウのサイズ変更時に呼ばれる
bool DeviceResources::WindowSizeChanged(int width, int height)
{
    if (!m_window)
        return false;

    RECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = static_cast<long>(width);
    newRc.bottom = static_cast<long>(height);
    if (newRc.right == m_outputSize.right && newRc.bottom == m_outputSize.bottom)
    {
        // HDR 用の色空間設定
        UpdateColorSpace();

        return false;
    }

    m_outputSize = newRc;
    CreateWindowSizeDependentResources();
    return true;
}

// 全デバイスリソースを再作成し、現在の状態へ復元する
void DeviceResources::HandleDeviceLost()
{
    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeviceLost();
    }

    m_d3dDepthStencilView.Reset();
    m_d3dRenderTargetView.Reset();
    m_renderTarget.Reset();
    m_depthStencil.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dAnnotation.Reset();

#ifdef _DEBUG
    {
        ComPtr<ID3D11Debug> d3dDebug;
        if (SUCCEEDED(m_d3dDevice.As(&d3dDebug)))
        {
            d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY);
        }
    }
#endif

    m_d3dDevice.Reset();
    m_dxgiFactory.Reset();

    CreateDeviceResources();
    CreateWindowSizeDependentResources();

    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeviceRestored();
    }
}

// スワップチェーンの内容を画面へ表示する
void DeviceResources::Present()
{
    HRESULT hr = E_FAIL;
    if (m_options & c_AllowTearing)
    {
        // 同期間隔 0 の場合、対応環境では常にテアリングを使うことが推奨されている
        hr = m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
    else
    {
        // 第 1 引数により DXGI は VSync までブロックし、アプリは次の VSync まで眠る。
        // 画面に表示されないフレームの描画にサイクルを浪費しないため
        hr = m_swapChain->Present(1, 0);
    }

    // レンダーターゲットの内容を破棄。
    // 全面を描き直す場合にのみ有効な操作（ダーティ矩形やスクロール矩形を使うなら削除すること）
    m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

    if (m_d3dDepthStencilView)
    {
        // デプスステンシルの内容を破棄
        m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());
    }

    // 切断やドライバー更新でデバイスが失われた場合は、全デバイスリソースを再作成する
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
            static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr));
        OutputDebugStringA(buff);
#endif
        HandleDeviceLost();
    }
    else
    {
        ThrowIfFailed(hr);

        if (!m_dxgiFactory->IsCurrent())
        {
            UpdateColorSpace();
        }
    }
}

void DeviceResources::CreateFactory()
{
#if defined(_DEBUG) && !defined(__MINGW32__)
    bool debugDXGI = false;
    {
        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
        {
            debugDXGI = true;

            ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80,
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
        }
    }

    if (!debugDXGI)
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));
}

// 最初に利用可能なハードウェアアダプターを取得する。
// 見つからない場合は *ppAdapter に nullptr が設定される
void DeviceResources::GetHardwareAdapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    HRESULT hr = m_dxgiFactory.As(&factory6);
    if (SUCCEEDED(hr))
    {
        for (UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
            adapterIndex++)
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Basic Render Driver アダプターは選ばない
                continue;
            }

        #ifdef _DEBUG
            wchar_t buff[256] = {};
            swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
            OutputDebugStringW(buff);
        #endif

            break;
        }
    }

    if (!adapter)
    {
        for (UINT adapterIndex = 0;
            SUCCEEDED(m_dxgiFactory->EnumAdapters1(
                adapterIndex,
                adapter.ReleaseAndGetAddressOf()));
            adapterIndex++)
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Basic Render Driver アダプターは選ばない
                continue;
            }

#ifdef _DEBUG
            wchar_t buff[256] = {};
            swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
            OutputDebugStringW(buff);
#endif

            break;
        }
    }

    *ppAdapter = adapter.Detach();
}

// HDR 出力に対応するため、スワップチェーンの色空間を設定する
void DeviceResources::UpdateColorSpace()
{
    if (!m_dxgiFactory)
        return;

    if (!m_dxgiFactory->IsCurrent())
    {
        // 出力情報は DXGI ファクトリにキャッシュされる。古くなっていたらファクトリを作り直す
        CreateFactory();
    }

    DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    bool isDisplayHDR10 = false;

    if (m_swapChain)
    {
        // HDR 対応の検出には、この時点でアプリに対応する主要 DXGI 出力の色空間を
        // （ウィンドウとディスプレイの交差判定で）調べる必要がある

        // アプリウィンドウの矩形範囲を取得
        RECT windowBounds;
        if (!GetWindowRect(m_window, &windowBounds))
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "GetWindowRect");

        const long ax1 = windowBounds.left;
        const long ay1 = windowBounds.top;
        const long ax2 = windowBounds.right;
        const long ay2 = windowBounds.bottom;

        ComPtr<IDXGIOutput> bestOutput;
        long bestIntersectArea = -1;

        ComPtr<IDXGIAdapter> adapter;
        for (UINT adapterIndex = 0;
            SUCCEEDED(m_dxgiFactory->EnumAdapters(adapterIndex, adapter.ReleaseAndGetAddressOf()));
            ++adapterIndex)
        {
            ComPtr<IDXGIOutput> output;
            for (UINT outputIndex = 0;
                SUCCEEDED(adapter->EnumOutputs(outputIndex, output.ReleaseAndGetAddressOf()));
                ++outputIndex)
            {
                // 現在の出力の矩形範囲を取得
                DXGI_OUTPUT_DESC desc;
                ThrowIfFailed(output->GetDesc(&desc));
                const auto& r = desc.DesktopCoordinates;

                // 交差面積を計算
                const long intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, r.left, r.top, r.right, r.bottom);
                if (intersectArea > bestIntersectArea)
                {
                    bestOutput.Swap(output);
                    bestIntersectArea = intersectArea;
                }
            }
        }

        if (bestOutput)
        {
            ComPtr<IDXGIOutput6> output6;
            if (SUCCEEDED(bestOutput.As(&output6)))
            {
                DXGI_OUTPUT_DESC1 desc;
                ThrowIfFailed(output6->GetDesc1(&desc));

                if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                {
                    // ディスプレイ出力は HDR10
                    isDisplayHDR10 = true;
                }
            }
        }
    }

    if ((m_options & c_EnableHDR) && isDisplayHDR10)
    {
        switch (m_backBufferFormat)
        {
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            // アプリケーション側で HDR10 信号を生成する
            colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            // システム側で HDR10 信号を生成し、アプリはリニア値を使う
            colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
            break;

        default:
            break;
        }
    }

    m_colorSpace = colorSpace;

    ComPtr<IDXGISwapChain3> swapChain3;
    if (m_swapChain && SUCCEEDED(m_swapChain.As(&swapChain3)))
    {
        UINT colorSpaceSupport = 0;
        if (SUCCEEDED(swapChain3->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))
            && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        {
            ThrowIfFailed(swapChain3->SetColorSpace1(colorSpace));
        }
    }
}
