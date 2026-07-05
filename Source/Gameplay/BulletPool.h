//---------------------------------------------------------------------------
//! @file   BulletPool.h
//! @brief  敵弾の固定長プール (ボス戦専用)
//---------------------------------------------------------------------------
#pragma once
#include "Gameplay/Bullet.h"

class RenderCommandQueue;

//===========================================================================
//! 敵弾プール
//! 固定長配列を先頭から線形探索して貸し出す。満杯時 acquire は nullptr
//===========================================================================
class BulletPool
{
public:
    //! 非アクティブな弾を確保して初期化します (満杯なら nullptr)
    Bullet* acquire(
        const Vector3& position,
        const Vector3& direction,
        float speed,
        float lifetime,
        float damage,
        const Vector4& color = Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    //! 全弾を更新
    void update(float deltaTime);

    //! 全弾を非アクティブ化します (シーン入場時のリセット用)
    void deactivateAll();

    //! アクティブな弾を球メッシュとしてキューへ積む
    void submitRender(RenderCommandQueue& queue, DirectX::GeometricPrimitive* mesh) const;

    //! 弾配列の先頭を取得します (衝突判定の走査用)
    Bullet* getBullets() { return m_bullets; }

    //! プール容量を取得します
    int getMaxBullets() const { return MAX_BULLETS; }

    //! アクティブな弾数を数えます (デバッグUI用)
    int countActive() const;

private:
    static constexpr int MAX_BULLETS = 256;    //!< プール容量 (全システム共通の上限)
    Bullet m_bullets[MAX_BULLETS];             //!< 弾の実体 (固定長・再割り当てなし)
};
