//---------------------------------------------------------------------------
//! @file   pch.h
//! @brief  プリコンパイル済みヘッダー — 標準システムインクルード
//---------------------------------------------------------------------------
#pragma once

#include <winsdkver.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0603
#endif
#include <sdkddkver.h>

// C++ 標準のテンプレート版 min/max を使う
#define NOMINMAX

// DirectX アプリに GDI は不要
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// 必要なら <mcx.h> をインクルード
#define NOMCX

// 必要なら <winsvc.h> をインクルード
#define NOSERVICE

// WinHelp は非推奨
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>

#include "Source/Common/TypeDef.h"

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include <DirectXMath.h>
#include <DirectXColors.h>

// DirectXTK
#include "GeometricPrimitive.h"
#include "Effects.h"
#include "CommonStates.h"
#include "SimpleMath.h"
#include "PostProcess.h"
#include "Keyboard.h"
#include "Mouse.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <exception>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <filesystem>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
namespace DX
{
    // COM 例外用ヘルパークラス
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const noexcept override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // D3D API の失敗を例外へ変換するヘルパー
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }
}

// パス設定 — 実行ファイル基準で Shaders/ Assets/ を解決
inline std::wstring GetExecutableDir()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path().wstring() + L"/";
}
inline std::wstring GetShaderPath(const wchar_t* name)
{
    return GetExecutableDir() + L"Shaders/" + name;
}
inline std::wstring GetAssetPath(const wchar_t* name)
{
    return GetExecutableDir() + L"Assets/" + name;
}