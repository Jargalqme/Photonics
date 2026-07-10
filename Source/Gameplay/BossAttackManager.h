//---------------------------------------------------------------------------
//! @file   BossAttackManager.h
//! @brief  ボス攻撃パターン管理 (行動チェイン)
//---------------------------------------------------------------------------
#pragma once
#include <SimpleMath.h>
#include <vector>

class BulletPool;

//===========================================================================
//! ボス攻撃パターン管理
//! 予告 -> 攻撃 -> 隙 の3ステージを行動チェインとして循環させる
//===========================================================================
class BossAttackManager
{
private:
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;

public:
    //! プールを接続しフェーズ1のチェインを構築します (Boss::activate から呼ばれる)
    void initialize(BulletPool* pool);

    //! バースト連射のティック + 行動チェインのステージ進行
    void update(float deltaTime);

    void setPosition(const Vector3& pos) { m_position = pos; }
    void setPlayerTarget(const Vector3* target) { m_playerTarget = target; }

    //! フェーズに応じた行動チェインへ差し替えます (先頭から仕切り直し)
    void setPhase(int phase);

private:
    // --- 行動チェイン型 ---
    enum class Pattern { Rain, Burst, Torus };              //!< 攻撃パターンの種類
    enum class Stage   { Telegraph, Attack, Recovery };     //!< 予告 / 発射中 / 隙

    //! チェインの1手 (パターン + 予告/隙の長さ)
    struct Maneuver
    {
        Pattern pattern;
        float   telegraphDuration;    //!< 予告時間 (秒)
        float   recoveryDuration;     //!< 攻撃後の隙 (秒)
    };

    // --- 弾パラメータ ---
    static constexpr float BULLET_LIFETIME = 5.0f;
    static constexpr float BULLET_DAMAGE   = 15.0f;

    // フォーリングレイン
    static constexpr float RAIN_SPAWN_HEIGHT    = 12.0f;
    static constexpr int   RAIN_RING_COUNT      = 30;

    // 展開フェーズ
    static constexpr float RAIN_EXPAND_FAST     = 10.0f;
    static constexpr float RAIN_EXPAND_MEDIUM   = 7.0f;
    static constexpr float RAIN_EXPAND_SLOW     = 4.0f;
    static constexpr float RAIN_EXPAND_DURATION = 2.5f;

    // 落下フェーズ
    static constexpr float RAIN_FALL_SPEED      = 6.0f;
    static constexpr float RAIN_FALL_SPREAD     = 0.05f;    // 落下中の横広がり
    static constexpr float RAIN_TOTAL_LIFETIME  = 12.0f;    // 展開 + 落下の合計

    static constexpr int   BURST_BULLET_COUNT = 15;
    static constexpr float BURST_INTERVAL     = 0.06f;
    static constexpr float BURST_BULLET_SPEED = 35.0f;

    static constexpr int   TORUS_BULLET_COUNT = 30;
    static constexpr float TORUS_SPEED        = 10.0f;
    static constexpr float TORUS_SPAWN_HEIGHT = 0.5f;

    // 攻撃パターン別色 (1.0 超の HDR 値 = ブルームで発光)
    static constexpr Vector4 RAIN_COLOR  = Vector4(1.70f, 0.15f, 2.80f, 1.0f); // violet
    static constexpr Vector4 BURST_COLOR = Vector4(2.75f, 0.06f, 0.08f, 1.0f); // red
    static constexpr Vector4 TORUS_COLOR = Vector4(0.55f, 2.60f, 0.20f, 1.0f); // acid green

    // バースト連射状態
    bool m_burstActive   = false;    //!< 連射中フラグ
    int m_burstRemaining = 0;        //!< 残り発射数
    float m_burstTimer   = 0.0f;     //!< 次の1発までの秒数

    // チェイン状態
    std::vector<Maneuver> m_chain;          //!< 現フェーズの行動チェイン (循環)
    int   m_chainIndex = 0;                 //!< 現在の手
    Stage m_stage      = Stage::Telegraph;  //!< 現在のステージ
    float m_stageTimer = 0.0f;              //!< ステージの残り秒数

    // --- 攻撃メソッド ---
    void firePattern(Pattern p);
    float attackDurationFor(Pattern p) const;
    std::vector<Maneuver> buildChainForPhase(int phase) const;

    void fireRain();
    void fireRainRing(int count, float expandSpeed, float angleOffset);
    void fireAimedBurst();
    void fireBurstBullet();
    void fireTorus();

    // --- メンバ ---
    BulletPool* m_bulletPool      = nullptr;    //!< 弾プール (非所有)
    const Vector3* m_playerTarget = nullptr;    //!< プレイヤー位置 (非所有・Boss 経由で配線)
    Vector3 m_position;                         //!< ボスの現在位置 (毎フレーム Boss が設定)
};
