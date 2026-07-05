//---------------------------------------------------------------------------
//! @file   Bullet.h
//! @brief  敵弾 (ボス戦専用・プール管理)
//---------------------------------------------------------------------------
#pragma once
#include <SimpleMath.h>
#include <DirectXCollision.h>

using namespace DirectX::SimpleMath;

class RenderCommandQueue;

//===========================================================================
//! 敵弾
//! プールから貸し出され、寿命切れ・地面到達・命中で非アクティブに戻る
//===========================================================================
class Bullet
{
public:
    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    //! プールから取り出した弾を初期化する (Nystrom Object Pool パターン)
    void initialize(
        const Vector3& position,
        const Vector3& direction,
        float speed,
        float lifetime,
        float damage,
        const Vector4& color = Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    //! 移動・寿命・フェーズ切り替えを更新
    void update(float deltaTime);

    //! 球メッシュを MeshCommand としてキューへ積む (見た目の半径 = 衝突半径)
    void submitRender(RenderCommandQueue& queue, DirectX::GeometricPrimitive* mesh) const;

    //!@}
    //----------------------------------------------------------
    //! @name   プール管理
    //----------------------------------------------------------
    //!@{

    //! アクティブ状態を取得します
    bool isActive() const { return m_active; }

    //! 非アクティブ化してプールへ返します
    void deactivate() { m_active = false; }

    //!@}
    //----------------------------------------------------------
    //! @name   取得・設定
    //----------------------------------------------------------
    //!@{

    //! 命中時ダメージを取得します
    float getDamage() const { return m_damage; }

    //! 座標を取得します
    Vector3 getPosition() const { return m_position; }

    //! 進行方向を取得します
    Vector3 getDirection() const { return m_direction; }

    //! 衝突判定球を取得します
    const DirectX::BoundingSphere& getBoundingSphere() const { return m_boundingSphere; }

    //! 指定秒数後に方向と速度を切り替える (レイン弾の展開 -> 落下)
    void setPhaseSwitch(float delay, const Vector3& newDirection, float newSpeed);

    //!@}

private:
    static constexpr float COLLISION_RADIUS   = 0.3f;   //!< 衝突半径 (描画サイズも兼用)
    static constexpr float SPEED_RAMP_TIME    = 0.3f;   //!< イージング完了までの時間
    static constexpr float MIN_SPEED_RATIO    = 0.4f;   //!< 最低速度の割合
    static constexpr float EMISSIVE_INTENSITY = 1.0f;   //!< ブルームに乗せる発光倍率

    bool m_active = false;                  //!< プール貸し出し中フラグ

    Vector3 m_position  = Vector3::Zero;    //!< 座標
    Vector3 m_direction = Vector3::Zero;    //!< 進行方向 (正規化済み)
    float m_lifetime = 0.0f;                //!< 残り寿命 (秒)
    float m_damage   = 0.0f;                //!< 命中時ダメージ

    float m_maxSpeed = 0.0f;                //!< 最高速度
    float m_minSpeed = 0.0f;                //!< 発射直後の最低速度
    float m_age      = 0.0f;                //!< 発射からの経過時間 (秒)

    // フェーズ切り替え (レイン弾: 横展開 -> 落下)
    float   m_phaseTimer = 0.0f;            //!< 切り替えまでの残り秒数
    Vector3 m_phaseDirection;               //!< 切り替え後の方向
    float   m_phaseSpeed = 0.0f;            //!< 切り替え後の速度
    bool    m_hasPhaseSwitch = false;       //!< 切り替え予約あり

    Vector4 m_color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);    //!< HDR色 (ブルームで発光)
    DirectX::BoundingSphere m_boundingSphere = { XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f };    //!< 衝突判定球
};
