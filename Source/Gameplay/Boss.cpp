#include "pch.h"
#include "Gameplay/Boss.h"
#include "Gameplay/BulletPool.h"
#include "Gameplay/EventBus.h"
#include "Gameplay/EventTypes.h"
#include "Gameplay/PlayerCamera.h"
#include "Render/Visuals/ParticleSystem.h"
#include "Services/SceneContext.h"
#include "Render/Assets/MeshCache.h"
#include "Render/Pipeline/RenderCommandQueue.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// === 生成・初期化 ===

Boss::Boss(SceneContext& context)
    : m_context(&context)
    , m_skull(context)
    , m_color(Colors::Black)
    , m_health(BOSS_MAX_HEALTH)
    , m_maxHealth(BOSS_MAX_HEALTH)
    , m_activated(false)
{
}

void Boss::initialize()
{
    buildBoss();
    m_attacks.initialize(m_bulletPool);
}

void Boss::buildBoss()
{
    MeshCache* meshes = m_context->meshes;

    m_coreMesh = meshes->getSphere(24);
    m_outerringMesh = meshes->getTorus(4.6f, 0.08f, 64);
    m_innerRingMesh = meshes->getTorus(3.2f, 0.18f, 64);

#ifdef _DEBUG
    m_debugSphere = meshes->getSphere(16);         // unit (diameter=1)；submitRender() で 2*radius 倍
#endif
    m_skull.initialize(GetAssetPath(L"Textures/skull.png"));
}

void Boss::activate()
{
    m_transform.position = Vector3(P1_ORBIT_RADIUS, P1_HEIGHT, 0.0f);
    m_health = m_maxHealth;
    m_activated = true;
    m_active = true;

    // 移動状態リセット
    m_moveAngle = 0.0f;
    m_bobPhase  = 0.0f;
    m_dashTimer = 0.0f;

    m_attacks.initialize(m_bulletPool);
    m_attacks.setPosition(m_transform.position);
    m_attacks.setPlayerTarget(m_playerTarget);

    m_phaseFSM.addState(BossPhase::phase1,
        nullptr,
        [this](float dt) {updatePhase1(dt); },
        nullptr);

    m_phaseFSM.addState(BossPhase::phase2,
        [this]()
        {
            if (m_particles)
            {
                m_particles->emit(m_transform.position,
                    Vector4(2.4f, 0.0f, 4.0f, 1.0f),
                    50, 8.0f, 1.5f, 1.0f);
            }
            if (m_camera)
            {
                m_camera->triggerShake(0.5f, 0.5f);
            }
            m_attacks.setPhase(2);
        },
        [this](float dt) {updatePhase2(dt); },
        nullptr);

    m_phaseFSM.addState(BossPhase::phase3,
        [this]()
        {
            if (m_particles)
            {
                m_particles->emit(m_transform.position,
                    Vector4(2.4f, 0.0f, 4.0f, 1.0f),
                    80, 10.0f, 2.0f, 1.0f);
            }
            if (m_camera)
            {
                m_camera->triggerShake(0.7f, 0.8f);
            }
            m_attacks.setPhase(3);
        },
        [this](float dt) {updatePhase3(dt); },
        nullptr);

    m_phaseFSM.changeState(BossPhase::phase1);
}

// === ICombatTarget ===

void Boss::collectHitColliders(std::vector<CombatHitCollider>& out)
{
    if (!m_activated || isDead())
    {
        return;
    }

    // ボディ — 軌道リング + コアを覆う大きなスフィア
    {
        CombatHitCollider c;
        c.target = this;
        c.part = HitPart::Body;
        c.bounds.Center = XMFLOAT3(
            m_transform.position.x,
            m_transform.position.y,
            m_transform.position.z);
        c.bounds.Radius = BODY_RADIUS;
        c.damageMultiplier = 1.0f;
        out.push_back(c);
    }

    // スカル弱点 — ボディの上に浮いている小さなスフィア（2倍ダメージ）
    {
        CombatHitCollider c;
        c.target = this;
        c.part = HitPart::WeakPoint;
        c.bounds.Center = XMFLOAT3(
            m_transform.position.x,
            m_transform.position.y + SKULL_OFFSET_Y,
            m_transform.position.z);
        c.bounds.Radius = SKULL_RADIUS;
        c.damageMultiplier = WEAK_POINT_MULT;
        out.push_back(c);
    }
}

void Boss::onHit(const CombatHit& hit)
{
    if (!m_activated || isDead())
    {
        return;
    }

    m_health -= hit.finalDamage;
    m_hitFlashTimer = HIT_FLASH_DURATION;

    EventBus::publish(BossDamagedEvent{
        m_transform.position,
        hit.finalDamage,
        std::max(m_health, 0.0f),
        m_maxHealth });

    if (m_health <= 0.0f)
    {
        m_health = 0.0f;
        m_active = false;
        EventBus::publish(BossDiedEvent{ m_transform.position });
    }
}

// === 更新 ===

void Boss::update(float deltaTime)
{
    if (!m_activated || isDead())
    {
        return;
    }

    // リング回転
    m_ringOrbitAngle += RING_SPIN_SPEED * deltaTime;

    // ヒットフラッシュ減衰
    if (m_hitFlashTimer > 0.0f)
    {
        m_hitFlashTimer -= deltaTime;
    }

    float hp = m_health / m_maxHealth;
    auto current = m_phaseFSM.getCurrentState();
    if (hp <= PHASE3_HP_THRESHOLD && current != BossPhase::phase3)
    {
        m_phaseFSM.changeState(BossPhase::phase3);
    }
    else if (hp <= PHASE2_HP_THRESHOLD && current == BossPhase::phase1)
    {
        m_phaseFSM.changeState(BossPhase::phase2);
    }

    m_phaseFSM.update(deltaTime);

    m_attacks.setPosition(m_transform.position);
    m_attacks.update(deltaTime);
}

void Boss::updatePhase1(float dt)
{
    // 一定高度で中心周りをゆっくり周回
    m_moveAngle += P1_ORBIT_SPEED * dt;
    m_transform.position.x = cosf(m_moveAngle) * P1_ORBIT_RADIUS;
    m_transform.position.z = sinf(m_moveAngle) * P1_ORBIT_RADIUS;
    m_transform.position.y = P1_HEIGHT;
}

void Boss::updatePhase2(float dt)
{
    // 速い軌道 + 垂直バウンド — プレイヤーは XZ 追跡に加え Y 成分も読む必要がある
    m_moveAngle += P2_ORBIT_SPEED * dt;
    m_bobPhase  += P2_BOB_SPEED   * dt;
    m_transform.position.x = cosf(m_moveAngle) * P2_ORBIT_RADIUS;
    m_transform.position.z = sinf(m_moveAngle) * P2_ORBIT_RADIUS;
    m_transform.position.y = P2_BASE_HEIGHT + sinf(m_bobPhase) * P2_BOB_AMPLITUDE;
}

void Boss::updatePhase3(float dt)
{
    // ホールド → ランダム再配置 → ダッシュ補間 — 予測困難化
    m_dashTimer -= dt;
    if (m_dashTimer <= 0.0f)
    {
        float angle  = (rand() / static_cast<float>(RAND_MAX)) * XM_2PI;
        float radius = P3_RADIUS_MIN
            + (rand() / static_cast<float>(RAND_MAX)) * (P3_RADIUS_MAX - P3_RADIUS_MIN);
        float height = P3_HEIGHT_MIN
            + (rand() / static_cast<float>(RAND_MAX)) * (P3_HEIGHT_MAX - P3_HEIGHT_MIN);

        m_dashTarget.x = cosf(angle) * radius;
        m_dashTarget.z = sinf(angle) * radius;
        m_dashTarget.y = height;

        m_dashTimer = P3_DASH_INTERVAL;
    }

    // 指数減衰補間 — 最初は速く、目標に近づくと減速
    float t = std::min(P3_DASH_LERP * dt, 1.0f);
    m_transform.position = Vector3::Lerp(m_transform.position, m_dashTarget, t);
}

// === 描画 ===

void Boss::submitRender(RenderCommandQueue& queue) const
{
    if (!m_activated)
    {
        return;
    }

    auto submitMesh = [&queue](
        GeometricPrimitive* mesh,
        const Matrix& world,
        const Color& color,
        bool wireframe = false,
        BlendMode blendMode = BlendMode::Opaque,
        float emissiveIntensity = 0.0f)
    {
        if (!mesh)
        {
            return;
        }

        MeshCommand command;
        command.mesh = mesh;
        command.world = world;
        command.color = color;
        command.wireframe = wireframe;
        command.blendMode = blendMode;
        command.emissiveIntensity = emissiveIntensity;
        queue.submit(command);
    };

    Matrix world = Matrix::CreateTranslation(m_transform.position);

    Color coreColor = Color(0.8f, 0.8f, 0.8f);
    Color ringColor = Color(0.4f, 0.0f, 1.0f);
    const Color energyColor(0.0f, 0.85f, 1.0f);
    if (m_hitFlashTimer > 0.0f)
    {
        float t = m_hitFlashTimer / HIT_FLASH_DURATION;
        Color flashColor = Color(1.0f, 0.6f, 0.0f);
        coreColor = Color::Lerp(coreColor, flashColor, t);
        ringColor = Color::Lerp(ringColor, flashColor, t);
    }

    submitMesh(m_coreMesh,
        Matrix::CreateScale(1.8f) * world,
        coreColor,
        false,
        BlendMode::Opaque,
        0.8f);

    submitMesh(m_outerringMesh,
        Matrix::CreateRotationX(0.35f) *
        Matrix::CreateRotationY(m_ringOrbitAngle) *
        Matrix::CreateTranslation(m_transform.position),
        energyColor,
        false,
        BlendMode::Opaque,
        4.0f);

    submitMesh(m_innerRingMesh,
        Matrix::CreateRotationX(-0.55f) *
        Matrix::CreateRotationZ(XM_PIDIV2 * 0.78f) *
        Matrix::CreateRotationY(-m_ringOrbitAngle * 1.3f) *
        Matrix::CreateTranslation(m_transform.position),
        Color(0.8f, 0.0f, 1.0f),
        false,
        BlendMode::Opaque,
        3.5f);

#ifdef _DEBUG
    const Vector3 weakPointPosition = m_transform.position + Vector3(0, SKULL_OFFSET_Y, 0);

    submitMesh(
        m_debugSphere,
        Matrix::CreateScale(BODY_RADIUS * 2.0f)
        * Matrix::CreateTranslation(m_transform.position),
        Color(0.0f, 1.0f, 0.0f, 0.3f),
        true,
        BlendMode::AlphaBlend);

    submitMesh(
        m_debugSphere,
        Matrix::CreateScale(SKULL_RADIUS * 2.0f)
        * Matrix::CreateTranslation(weakPointPosition),
        Color(1.0f, 0.0f, 1.0f, 0.35f),
        true,
        BlendMode::AlphaBlend);
#endif

    BillboardCommand billboard;
    billboard.billboard = &m_skull;
    billboard.position = m_transform.position + Vector3(0, SKULL_OFFSET_Y, 0);
    billboard.size = SKULL_SIZE;
    queue.submit(billboard);
}

// === 終了処理 ===

void Boss::finalize()
{
    m_skull.finalize();
    // 借用ポインタを nullptr 化（実体は MeshCache が所有）
    m_coreMesh = nullptr;
    m_outerringMesh = nullptr;
    m_innerRingMesh = nullptr;
#ifdef _DEBUG
    m_debugSphere = nullptr;
#endif
}
