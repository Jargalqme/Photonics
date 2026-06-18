//---------------------------------------------------------------------------
//! @file   typedef.h
//! @brief  型定義
//---------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <memory>

//===========================================================================
//! @name   基本型定義
//===========================================================================
using s8  = std::int8_t;      //!<  8ビット符号付き整数
using u8  = std::uint8_t;     //!<  8ビット符号無し整数
using s16 = std::int16_t;     //!< 16ビット符号付き整数
using u16 = std::uint16_t;    //!< 16ビット符号無し整数
using s32 = std::int32_t;     //!< 32ビット符号付き整数
using u32 = std::uint32_t;    //!< 32ビット符号無し整数
using s64 = std::int64_t;     //!< 64ビット符号付き整数
using u64 = std::uint64_t;    //!< 64ビット符号無し整数

//===========================================================================
// COMオブジェクト
//===========================================================================
#include <wrl/client.h>

template <typename T>
using com_ptr = Microsoft::WRL::ComPtr<T>;

//===========================================================================
//! @name   ユーティリティクラス
//===========================================================================
//!@{

//! コピー禁止 / move禁止クラス
class noncopyable
{
protected:
    noncopyable()                              = default;
    ~noncopyable()                             = default;
    noncopyable(const noncopyable&)            = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

//! ポインター参照クラス
template <typename T>
class ptr_view
{
public:
    //! コンストラクタ
    ptr_view(T& ref) noexcept
        : ptr_(&ref)
    {
    }

    //! コンストラクタ
    ptr_view(T* p) noexcept
        : ptr_(p)
    {
    }

    //! コンストラクタ
    ptr_view(std::unique_ptr<T>& p) noexcept
        : ptr_(p.get())
    {
    }

    //! コンストラクタ
    ptr_view(std::shared_ptr<T>& p) noexcept
        : ptr_(p.get())
    {
    }

    //! デストラクタ
    ~ptr_view() noexcept = default;

    //! 透過アクセス演算子
    T* operator->() noexcept { return ptr_; }

    //! 透過アクセス演算子 (const版)
    const T* operator->() const noexcept { return ptr_; }

    //! 生ポインタを取得します
    T* get() noexcept { return ptr_; }

    //! ポインターの有無を取得します
    operator bool() const noexcept { return ptr_ != nullptr; }

private:
    T* ptr_;    //!< ポインター
};

//!@}
