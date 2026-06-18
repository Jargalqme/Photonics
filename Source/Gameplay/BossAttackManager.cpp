#include "pch.h"
#include "Gameplay/BossAttackManager.h"
#include "Gameplay/BulletPool.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void BossAttackManager::initialize(BulletPool* pool)
{
    m_bulletPool = pool;
    setPhase(1);
}

void BossAttackManager::setPhase(int phase)
{
    m_chain = buildChainForPhase(phase);
    m_chainIndex = 0;
    m_stage = Stage::Telegraph;
    m_stageTimer = m_chain.empty() ? 0.0f : m_chain[0].telegraphDuration;
}

void BossAttackManager::update(float deltaTime)
{
    // バースト連射のチク
    if (m_burstActive)
    {
        m_burstTimer -= deltaTime;
        if (m_burstTimer <= 0.0f)
        {
            fireBurstBullet();
            m_burstRemaining--;
            if (m_burstRemaining <= 0)
            {
                m_burstActive = false;
            }
            else
            {
                m_burstTimer = BURST_INTERVAL;
            }
        }
    }

    // 行動チェインのステージ進行
    if (m_chain.empty())
    {
        return;
    }

    m_stageTimer -= deltaTime;
    if (m_stageTimer > 0.0f)
    {
        return;
    }

    const Maneuver& m = m_chain[m_chainIndex];
    switch (m_stage)
    {
    case Stage::Telegraph:
        firePattern(m.pattern);
        m_stage = Stage::Attack;
        m_stageTimer = attackDurationFor(m.pattern);
        break;

    case Stage::Attack:
        m_stage = Stage::Recovery;
        m_stageTimer = m.recoveryDuration;
        break;

    case Stage::Recovery:
        m_chainIndex = (m_chainIndex + 1) % static_cast<int>(m_chain.size());
        m_stage = Stage::Telegraph;
        m_stageTimer = m_chain[m_chainIndex].telegraphDuration;
        break;
    }
}

void BossAttackManager::firePattern(Pattern p)
{
    switch (p)
    {
    case Pattern::Rain:  fireRain();        break;
    case Pattern::Burst: fireAimedBurst();  break;
    case Pattern::Torus: fireTorus();       break;
    }
}

float BossAttackManager::attackDurationFor(Pattern p) const
{
    switch (p)
    {
    case Pattern::Burst: return BURST_BULLET_COUNT * BURST_INTERVAL;  // 連射の長さ
    case Pattern::Rain:
    case Pattern::Torus:
    default:             return 0.0f;                                 // 一括スポーン — 即次へ
    }
}

std::vector<BossAttackManager::Maneuver>
BossAttackManager::buildChainForPhase(int phase) const
{
    switch (phase)
    {
    case 1:  // 単発ずつ — プレイヤーがリズムを学ぶ
        return {
            { Pattern::Rain,  1.5f, 2.0f },
            { Pattern::Burst, 0.8f, 1.5f },
        };
    case 2:  // トーラス追加 + リカバリ短縮
        return {
            { Pattern::Rain,  1.2f, 1.0f },
            { Pattern::Torus, 0.8f, 0.8f },
            { Pattern::Burst, 0.6f, 1.0f },
        };
    case 3:  // 最小リカバリで重ね撃ち
        return {
            { Pattern::Rain,  0.8f, 0.4f },
            { Pattern::Torus, 0.6f, 0.3f },
            { Pattern::Burst, 0.4f, 0.5f },
        };
    }
    return {};
}

void BossAttackManager::fireRain()
{
    if (!m_bulletPool)
    {
        return;
    }

    fireRainRing(RAIN_RING_COUNT, RAIN_EXPAND_SLOW, 0.0f);
    fireRainRing(RAIN_RING_COUNT, RAIN_EXPAND_MEDIUM, 0.3f);
    fireRainRing(RAIN_RING_COUNT, RAIN_EXPAND_FAST, 0.6f);
}

void BossAttackManager::fireRainRing(int count, float expandSpeed, float angleOffset)
{
    Vector3 spawnPos = m_position + Vector3(0.0f, RAIN_SPAWN_HEIGHT, 0.0f);

    float angleStep = XM_2PI / static_cast<float>(count);

    for (int i = 0; i < count; i++)
    {
        float angle = angleStep * i + angleOffset;

        Vector3 expandDir(cosf(angle), 0.0f, sinf(angle));
        expandDir.Normalize();

        Bullet* bullet = m_bulletPool->acquire(
            spawnPos,
            expandDir,
            expandSpeed,
            RAIN_TOTAL_LIFETIME,
            BULLET_DAMAGE,
            RAIN_COLOR
        );

        if (bullet)
        {
            Vector3 fallDir = expandDir * RAIN_FALL_SPREAD;
            fallDir.y = -1.0f;
            fallDir.Normalize();

            bullet->setPhaseSwitch(RAIN_EXPAND_DURATION, fallDir, RAIN_FALL_SPEED);
        }
    }
}

void BossAttackManager::fireAimedBurst()
{
    m_burstActive = true;
    m_burstRemaining = BURST_BULLET_COUNT;
    m_burstTimer = 0.0f;  // 最初の弾は即発射
}

void BossAttackManager::fireBurstBullet()
{
    if (!m_bulletPool || !m_playerTarget)
    {
        return;
    }

    Vector3 dir = *m_playerTarget - m_position;
    dir.Normalize();

    m_bulletPool->acquire(
        m_position,
        dir,
        BURST_BULLET_SPEED,
        BULLET_LIFETIME,
        BULLET_DAMAGE,
        BURST_COLOR
    );
}

void BossAttackManager::fireTorus()
{
    if (!m_bulletPool)
    {
        return;
    }

    Vector3 spawnPos = Vector3(m_position.x, TORUS_SPAWN_HEIGHT, m_position.z);

    float angleStep = XM_2PI / static_cast<float>(TORUS_BULLET_COUNT);

    for (int i = 0; i < TORUS_BULLET_COUNT; i++)
    {
        float angle = angleStep * i;
        Vector3 dir(cosf(angle), 0.0f, sinf(angle));

        m_bulletPool->acquire(
            spawnPos,
            dir,
            TORUS_SPEED,
            BULLET_LIFETIME,
            BULLET_DAMAGE,
            TORUS_COLOR
        );
    }
}
