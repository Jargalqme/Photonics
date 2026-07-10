//---------------------------------------------------------------------------
//! @file   Dummy.h
//! @brief  射撃練習用ダミー (トレーニングシーン)
//---------------------------------------------------------------------------
#pragma once

#include "Common/Transform.h"
#include "Gameplay/ICombatTarget.h"
#include <DirectXCollision.h>
#include <algorithm>

class RenderCommandQueue;
struct SceneContext;

//===========================================================================
//! 射撃練習用ダミー
//! 被弾でフラッシュ、死亡で一定時間後に同位置へリスポーン
//===========================================================================
class Dummy : public ICombatTarget
{
public:
    explicit Dummy(SceneContext& context);

    //----------------------------------------------------------
    //! @name   ライフサイクル
    //----------------------------------------------------------
    //!@{

    void initialize();

    //! リスポーン待ちと被弾フラッシュの減衰を更新
    void update(float deltaTime);

    //! 縦長キューブを MeshCommand としてキューへ積む
    void submitRender(RenderCommandQueue& queue) const;

    void finalize();

    //! 指定位置に出現させます (mesh 省略時は共有キューブ)
    void spawn(const Vector3& startPos, DirectX::GeometricPrimitive* mesh = nullptr);

    //!@}
    //----------------------------------------------------------
    //! @name   ICombatTarget 実装
    //----------------------------------------------------------
    //!@{

    //! 非アクティブ中は何も積まない = 撃てない
    void collectHitColliders(std::vector<CombatHitCollider>& out) override;

    //! 被弾フラッシュ + 体力減算。死亡でリスポーン予約とイベント発行
    void onHit(const CombatHit& hit) override;

    //!@}
    //----------------------------------------------------------
    //! @name   取得
    //----------------------------------------------------------
    //!@{

    bool isActive() const { return m_active; }

    Vector3 getPosition() const { return m_transform.position; }
    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }

    //!@}

private:
    static constexpr float HIT_FLASH_DURATION    = 0.15f;     //!< 被弾フラッシュの継続時間 (秒)
    static constexpr float DEFAULT_HEALTH        = 100.0f;    //!< 初期体力
    static constexpr float DEFAULT_RESPAWN_DELAY = 1.0f;      //!< 死亡からリスポーンまでの秒数

    // 当たり判定コライダ (collectHitColliders で使用)
    static constexpr float BODY_OFFSET_Y = 1.0f;    //!< 判定球中心の高さ (足元基準)
    static constexpr float BODY_RADIUS   = 1.0f;    //!< 判定球の半径

    void respawn();

    SceneContext* m_context = nullptr;
    DirectX::GeometricPrimitive* m_mesh = nullptr;    //!< 共有メッシュ (所有は MeshCache)

    Vector3 m_spawnPosition = Vector3::Zero;    //!< リスポーン位置

    float m_health = DEFAULT_HEALTH;
    float m_maxHealth = DEFAULT_HEALTH;
    float m_respawnTimer = 0.0f;    //!< リスポーンまでの残り秒数 (非アクティブ中のみ)

    Color m_originalColor = Color(0.95f, 0.92f, 0.78f);    //!< 通常色
    Color m_hitColor = Color(1.0f, 0.55f, 0.0f);           //!< 被弾フラッシュ色
    Color m_color = m_originalColor;                       //!< 現在の表示色
    float m_hitFlashTimer = 0.0f;                          //!< フラッシュの残り秒数

    // 旧 GameObject 由来
    Transform m_transform;
    bool m_active = true;
};
