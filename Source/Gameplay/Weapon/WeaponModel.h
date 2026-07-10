//---------------------------------------------------------------------------
//! @file   WeaponModel.h
//! @brief  一人称武器モデル (ビューモデル)
//---------------------------------------------------------------------------
#pragma once

#include <SimpleMath.h>
#include <string>

class ImportedModel;
class RenderCommandQueue;
struct SceneContext;

//! ビューモデルの配置設定 (DebugUI から調整可能)
struct RifleModelSettings
{
    float targetLength = 1.0f;    //!< 正規化後の全長 (最長辺をこの長さへスケール)
    DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3(-0.015f, -0.195f, 0.180f);     //!< カメラローカル位置
    DirectX::SimpleMath::Vector3 rotationDegrees = DirectX::SimpleMath::Vector3(0.0f, 180.0f, 0.0f);    //!< 前方補正 (度)。銃口の作成時の向きをカメラ +Z へ合わせる
};

//===========================================================================
//! 一人称武器モデル
//! モデル側の配置は使わず、AABB中心への再センタリング + 最長辺の正規化を
//! 毎回計算して構える (計算フィックスアップのため Transform は不使用)
//===========================================================================
class WeaponModel
{
public:
    //----------------------------------------------------------
    //! @name   ロード・描画
    //----------------------------------------------------------
    //!@{

    //! キャッシュから借用し、頂点群から AABB中心と最長辺を算出します
    bool loadRifle(SceneContext& context, const std::string& path);

    //! モーション適用済みルートに載せて ImportedModelCommand を積む
    void submit(RenderCommandQueue& queue, const DirectX::SimpleMath::Matrix& rootWorld) const;

    //! 再センタリング -> 正規化 -> 前方補正 -> 配置 -> ルート合成のワールド行列
    DirectX::SimpleMath::Matrix buildModelWorldMatrix(const DirectX::SimpleMath::Matrix& rootWorld) const;

    //! マズル位置 (モデル空間) を取得します。VM_MuzzlePoint ノード、無ければ +Z 固定値
    DirectX::SimpleMath::Vector3 getMuzzleLocalPosition() const;

    void finalize();

    //!@}
    //----------------------------------------------------------
    //! @name   取得・設定
    //----------------------------------------------------------
    //!@{

    bool hasRifle() const { return m_importedRifle != nullptr; }
    RifleModelSettings& rifleSettings() { return m_rifleSettings; }
    const RifleModelSettings& rifleSettings() const { return m_rifleSettings; }
    void resetRifleSettings();

    //!@}

private:
    const ImportedModel* m_importedRifle = nullptr;    //!< 取込モデル (ImportedModelCache から借用・非所有)
    RifleModelSettings m_rifleSettings;                //!< 配置設定 (DebugUI 直結)
    DirectX::SimpleMath::Vector3 m_importedRifleCenter = DirectX::SimpleMath::Vector3::Zero;    //!< AABB中心 (モデル空間・再センタリング用)
    float m_importedRifleLongestSide = 1.0f;           //!< 正規化用の最長辺

    static constexpr float kFallbackMuzzleZ = 0.3f;    //!< マズルノードが無い場合の前方オフセット
};
