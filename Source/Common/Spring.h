//---------------------------------------------------------------------------
//! @file   Spring.h
//! @brief  数値バネ (半陰的オイラー)
//---------------------------------------------------------------------------
// @Reference: https://github.com/TheAllenChou/numeric-springing
#pragma once


//===========================================================================
//! 1次元バネ
//! update で目標値へ減衰追従、kick でインパルス (リコイル等の起動)
//===========================================================================
struct Spring1D
{
    float x     = 0.0f;   //!< 現在値
    float v     = 0.0f;   //!< 速度
    float zeta  = 1.0f;   //!< 減衰比 (1.0=臨界減衰、<1.0=オーバーシュート)
    float omega = 20.0f;  //!< 角周波数 [rad/s] (2π * Hz)

    //! 目標値 xt へ1ステップ進める (半陰的 — dt が大きくても発散しない)
    void update(float xt, float dt)
    {
        const float f      = 1.0f + 2.0f * dt * zeta * omega;
        const float omega2 = omega * omega;
        const float detInv = 1.0f / (f + dt * dt * omega2);
        const float detX   = x * f + dt * v + dt * dt * omega2 * xt;
        const float detV   = v + dt * omega2 * xt - dt * omega2 * x;
        x = detX * detInv;
        v = detV * detInv;
    }

    //! 速度へインパルスを加えます
    void kick(float impulse) { v += impulse; }

    void reset() { x = 0.0f; v = 0.0f; }
};
