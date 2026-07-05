//---------------------------------------------------------------------------
//! @file   DeviceResources.h
//! @brief  Direct3D 11 デバイスとスワップチェーンのラッパー
//---------------------------------------------------------------------------
#pragma once

namespace DX
{
    // DeviceResources の所有者がデバイスのロスト / 再作成の通知を受け取るためのインターフェース
    interface IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~IDeviceNotify() = default;
    };

    // DirectX デバイスリソース全体を管理する
    class DeviceResources
    {
    public:
        static constexpr unsigned int c_FlipPresent  = 0x1;
        static constexpr unsigned int c_AllowTearing = 0x2;
        static constexpr unsigned int c_EnableHDR    = 0x4;

        DeviceResources(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
                        DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
                        UINT backBufferCount = 2,
                        D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_10_0,
                        unsigned int flags = c_FlipPresent) noexcept;
        ~DeviceResources() = default;

        DeviceResources(DeviceResources&&) = default;
        DeviceResources& operator= (DeviceResources&&) = default;

        DeviceResources(DeviceResources const&) = delete;
        DeviceResources& operator= (DeviceResources const&) = delete;

        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void SetWindow(HWND window, int width, int height) noexcept;
        bool WindowSizeChanged(int width, int height);
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify) noexcept { m_deviceNotify = deviceNotify; }
        void Present();
        void UpdateColorSpace();

        // デバイス情報アクセサ
        RECT GetOutputSize() const noexcept { return m_outputSize; }

        // Direct3D アクセサ
        auto                    GetD3DDevice() const noexcept           { return m_d3dDevice.Get(); }
        auto                    GetD3DDeviceContext() const noexcept    { return m_d3dContext.Get(); }
        auto                    GetSwapChain() const noexcept           { return m_swapChain.Get(); }
        auto                    GetDXGIFactory() const noexcept         { return m_dxgiFactory.Get(); }
        HWND                    GetWindow() const noexcept              { return m_window; }
        D3D_FEATURE_LEVEL       GetDeviceFeatureLevel() const noexcept  { return m_d3dFeatureLevel; }
        ID3D11Texture2D*        GetRenderTarget() const noexcept        { return m_renderTarget.Get(); }
        ID3D11Texture2D*        GetDepthStencil() const noexcept        { return m_depthStencil.Get(); }
        ID3D11RenderTargetView*	GetRenderTargetView() const noexcept    { return m_d3dRenderTargetView.Get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const noexcept    { return m_d3dDepthStencilView.Get(); }
        DXGI_FORMAT             GetBackBufferFormat() const noexcept    { return m_backBufferFormat; }
        DXGI_FORMAT             GetDepthBufferFormat() const noexcept   { return m_depthBufferFormat; }
        D3D11_VIEWPORT          GetScreenViewport() const noexcept      { return m_screenViewport; }
        UINT                    GetBackBufferCount() const noexcept     { return m_backBufferCount; }
        DXGI_COLOR_SPACE_TYPE   GetColorSpace() const noexcept          { return m_colorSpace; }
        unsigned int            GetDeviceOptions() const noexcept       { return m_options; }

        // パフォーマンスイベント（PIX マーカー）
        void PIXBeginEvent(_In_z_ const wchar_t* name)
        {
            m_d3dAnnotation->BeginEvent(name);
        }

        void PIXEndEvent()
        {
            m_d3dAnnotation->EndEvent();
        }

        void PIXSetMarker(_In_z_ const wchar_t* name)
        {
            m_d3dAnnotation->SetMarker(name);
        }

    private:
        void CreateFactory();
        void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);

        // Direct3D オブジェクト
        com_ptr<IDXGIFactory2>               m_dxgiFactory;
        com_ptr<ID3D11Device1>               m_d3dDevice;
        com_ptr<ID3D11DeviceContext1>        m_d3dContext;
        com_ptr<IDXGISwapChain1>             m_swapChain;
        com_ptr<ID3DUserDefinedAnnotation>   m_d3dAnnotation;

        // Direct3D 描画オブジェクト（3D 描画に必須）
        com_ptr<ID3D11Texture2D>             m_renderTarget;
        com_ptr<ID3D11Texture2D>             m_depthStencil;
        com_ptr<ID3D11RenderTargetView>      m_d3dRenderTargetView;
        com_ptr<ID3D11DepthStencilView>      m_d3dDepthStencilView;
        D3D11_VIEWPORT                       m_screenViewport;

        // Direct3D プロパティ
        DXGI_FORMAT                          m_backBufferFormat;
        DXGI_FORMAT                          m_depthBufferFormat;
        UINT                                 m_backBufferCount;
        D3D_FEATURE_LEVEL                    m_d3dMinFeatureLevel;

        // キャッシュ済みデバイスプロパティ
        HWND                                 m_window;
        D3D_FEATURE_LEVEL                    m_d3dFeatureLevel;
        RECT                                 m_outputSize;

        // HDR 対応
        DXGI_COLOR_SPACE_TYPE                m_colorSpace;

        // DeviceResources オプション（上のフラグ参照）
        unsigned int                         m_options;

        // IDeviceNotify は DeviceResources の所有者なので、生ポインタのまま保持してよい
        IDeviceNotify*                       m_deviceNotify;
    };
}
