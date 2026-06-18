#include "pch.h"
#include "Gameplay/Dummy.h"
#include "Gameplay/EventBus.h"
#include "Gameplay/EventTypes.h"
#include "GeometricPrimitive.h"
#include "Render/Assets/MeshCache.h"
#include "Render/Pipeline/RenderCommandQueue.h"
#include "Services/SceneContext.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Dummy::Dummy(SceneContext& context)
    : m_context(&context)
{
    m_active = false;
}

void Dummy::initialize()
{
}

void Dummy::spawn(const Vector3& startPos, DirectX::GeometricPrimitive* mesh)
{
    m_mesh = mesh ? mesh : m_context->meshes->getCube();
    m_spawnPosition = startPos;
    m_transform.position = startPos;

    m_health = m_maxHealth;
    m_respawnTimer = 0.0f;
    m_hitFlashTimer = 0.0f;
    m_color = m_originalColor;
    m_active = true;
}

void Dummy::update(float deltaTime)
{
    if (!m_active)
    {
        if (m_respawnTimer > 0.0f)
        {
            m_respawnTimer -= deltaTime;
            if (m_respawnTimer <= 0.0f)
            {
                respawn();
            }
        }
        return;
    }

    if (m_hitFlashTimer > 0.0f)
    {
        m_hitFlashTimer -= deltaTime;
        const float t = std::clamp(m_hitFlashTimer / HIT_FLASH_DURATION, 0.0f, 1.0f);
        m_color = Color::Lerp(m_originalColor, m_hitColor, t);
    }
    else
    {
        m_color = m_originalColor;
    }
}

void Dummy::submitRender(RenderCommandQueue& queue) const
{
    if (!m_active || !m_mesh)
    {
        return;
    }

    MeshCommand command;
    command.mesh = m_mesh;
    command.world =
        Matrix::CreateScale(1.5f, 2.2f, 1.0f) *
        Matrix::CreateTranslation(m_transform.position + Vector3(0.0f, 1.1f, 0.0f));
    command.color = m_color;
    command.wireframe = false;
    queue.submit(command);
}

// === ICombatTarget ===

void Dummy::collectHitColliders(std::vector<CombatHitCollider>& out)
{
    if (!m_active)
    {
        return;
    }

    // ボディ — 被弾領域
    CombatHitCollider c;
    c.target = this;
    c.part = HitPart::Body;
    c.bounds.Center = XMFLOAT3(
        m_transform.position.x,
        m_transform.position.y + BODY_OFFSET_Y,
        m_transform.position.z);
    c.bounds.Radius = BODY_RADIUS;
    c.damageMultiplier = 1.0f;
    out.push_back(c);
}

void Dummy::onHit(const CombatHit& hit)
{
    if (!m_active)
    {
        return;
    }

    m_health -= hit.finalDamage;
    m_hitFlashTimer = HIT_FLASH_DURATION;

    EventBus::publish(DummyHitEvent{ m_transform.position });

    if (m_health <= 0.0f)
    {
        m_health = 0.0f;
        m_active = false;
        m_respawnTimer = DEFAULT_RESPAWN_DELAY;
        EventBus::publish(DummyDiedEvent{ m_transform.position });
    }
}

void Dummy::respawn()
{
    m_transform.position = m_spawnPosition;
    m_health = m_maxHealth;
    m_respawnTimer = 0.0f;
    m_hitFlashTimer = 0.0f;
    m_color = m_originalColor;
    m_active = true;
}

void Dummy::finalize()
{
    m_mesh = nullptr;
}
