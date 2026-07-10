//---------------------------------------------------------------------------
//! @file   PlayerWeapon.cpp
//! @brief  プレイヤー武器 (戦闘・モーション・モデルの統合)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Gameplay/Weapon/PlayerWeapon.h"
#include "Gameplay/Weapon/WeaponShot.h"
#include "Gameplay/Weapon/Weapon.h"
#include "Render/Pipeline/RenderCommandQueue.h"
#include "Services/SceneContext.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

//---------------------------------------------------------------------------
//! コンストラクタ (戦闘状態のみ生成、モデルは initialize でロード)
//---------------------------------------------------------------------------
PlayerWeapon::PlayerWeapon(SceneContext& context)
    : m_context(&context)
    , m_combat(std::make_unique<Weapon>())
{
}

//---------------------------------------------------------------------------
//! デストラクタ (Weapon の完全型が見えるここで定義)
//---------------------------------------------------------------------------
PlayerWeapon::~PlayerWeapon() = default;

//---------------------------------------------------------------------------
//! 武器モデルをロードし弾数を初期化
//---------------------------------------------------------------------------
void PlayerWeapon::initialize()
{
    m_model.loadRifle(*m_context, RIFLE_MODEL_PATH);
    m_combat->initialize();
}

//---------------------------------------------------------------------------
//! 終了処理
//---------------------------------------------------------------------------
void PlayerWeapon::finalize()
{
    m_model.finalize();
}

//---------------------------------------------------------------------------
//! モーションと弾数を開始状態へ戻します
//---------------------------------------------------------------------------
void PlayerWeapon::reset()
{
    m_motion.reset();
    m_combat->initialize();
}

//---------------------------------------------------------------------------
//! 押しっぱなし入力を解除します
//---------------------------------------------------------------------------
void PlayerWeapon::clearInputState()
{
    m_combat->stopFire();
}

//---------------------------------------------------------------------------
//! 入力反映 -> モーション更新 -> 発射判定
//---------------------------------------------------------------------------
void PlayerWeapon::update(const PlayerWeaponFrame& frame, std::vector<WeaponShot>& outShots)
{
    applyInputToCombat(frame);
    m_motion.update(buildMotionInput(frame));

    // トレーサーは実際のマズル位置から、ヒットスキャンは目線レイで飛ばす
    const Vector3 tracerStart = getMuzzlePosition(frame.cameraWorld);
    if (m_combat->update(
        frame.deltaTime,
        frame.hitScanOrigin,
        frame.hitScanDirection,
        tracerStart,
        outShots))
    {
        m_motion.onFire();
    }
}

//---------------------------------------------------------------------------
//! ビュー行列から目線カメラを復元して武器モデルを積む
//---------------------------------------------------------------------------
void PlayerWeapon::render(RenderCommandQueue& queue, const Matrix& view) const
{
    const Matrix cameraWorld = view.Invert();
    m_model.submit(queue, buildModelRootWorld(cameraWorld));
}

//---------------------------------------------------------------------------
//! リロード中かを取得します
//---------------------------------------------------------------------------
bool PlayerWeapon::isReloading() const
{
    return m_combat->isReloading();
}

//---------------------------------------------------------------------------
//! 残弾数を取得します
//---------------------------------------------------------------------------
int PlayerWeapon::getAmmo() const
{
    return m_combat->getAmmoCount();
}

//---------------------------------------------------------------------------
//! マガジン容量を取得します
//---------------------------------------------------------------------------
int PlayerWeapon::getMaxAmmo() const
{
    return m_combat->getClipSize();
}

//---------------------------------------------------------------------------
//! 入力を戦闘状態へ反映 (トリガー・リロード)
//---------------------------------------------------------------------------
void PlayerWeapon::applyInputToCombat(const PlayerWeaponFrame& frame)
{
    if (frame.fireHeld)
    {
        m_combat->startFire();
    }
    else
    {
        m_combat->stopFire();
    }

    if (frame.reloadPressed)
    {
        m_combat->reload();
    }
}

//---------------------------------------------------------------------------
//! フレーム入力からモーション入力を組み立てる
//---------------------------------------------------------------------------
WeaponMotionInput PlayerWeapon::buildMotionInput(const PlayerWeaponFrame& frame) const
{
    WeaponMotionInput input;
    input.deltaTime = frame.deltaTime;
    input.lookDeltaDegrees = frame.viewDeltaDegrees;
    input.moveSpeed = frame.moveSpeed;
    input.grounded = frame.isGrounded;
    input.isAiming = frame.adsHeld;
    return input;
}

//---------------------------------------------------------------------------
//! モーション出力 (カメラローカル姿勢) をカメラワールドへ合成
//---------------------------------------------------------------------------
Matrix PlayerWeapon::buildModelRootWorld(const Matrix& cameraWorld) const
{
    const WeaponMotionOutput animation = m_motion.getMotionOutput();
    const Vector3 rotationRadians(
        XMConvertToRadians(animation.rotation.x),
        XMConvertToRadians(animation.rotation.y),
        XMConvertToRadians(animation.rotation.z));

    const Matrix animationRotation = Matrix::CreateFromYawPitchRoll(
        rotationRadians.y,
        rotationRadians.x,
        rotationRadians.z);

    return animationRotation
        * Matrix::CreateTranslation(animation.position)
        * cameraWorld;
}

//---------------------------------------------------------------------------
//! マズルのワールド位置を取得します (VM_MuzzlePoint をモデルワールドへ変換)
//---------------------------------------------------------------------------
Vector3 PlayerWeapon::getMuzzlePosition(const Matrix& cameraWorld) const
{
    const Matrix rootWorld = buildModelRootWorld(cameraWorld);
    const Matrix modelWorld = m_model.buildModelWorldMatrix(rootWorld);
    return Vector3::Transform(m_model.getMuzzleLocalPosition(), modelWorld);
}
