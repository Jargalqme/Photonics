//---------------------------------------------------------------------------
//! @file   LayoutLoader.h
//! @brief  レイアウト JSON 読み込み (DCC ツールから書き出した配置データ)
//---------------------------------------------------------------------------
#pragma once

#include "Common/Transform.h"
#include "GeometricPrimitive.h"
#include <SimpleMath.h>
#include <string>
#include <vector>

class ImportedModel;
struct SceneContext;

//! レイアウトの1パーツ (プリミティブ + 配置 + 衝突フラグ)
struct PrimitiveLayoutPart
{
    DirectX::DX11::GeometricPrimitive* primitive = nullptr;
    const ImportedModel* importedModel = nullptr;    //!< pbr_material 指定時のみ (PBR ボックス差し替え)
    Transform localTransform;
    DirectX::SimpleMath::Color color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
    std::string name;
    std::string type;
    std::string primitiveType;
    bool collidable = true;
};

//! 位置マーカー (スポーン地点など、描画しない)
struct PrimitiveLayoutMarker
{
    Transform localTransform;
    std::string name;
    std::string type;
};

//! レイアウト全体 (パーツ + マーカー)
struct PrimitiveLayout
{
    std::string name;
    std::string root;
    float worldScale = 1.0f;    //!< 全体スケール (JSON: world_scale / layout_scale)
    std::vector<PrimitiveLayoutPart> parts;
    std::vector<PrimitiveLayoutMarker> markers;
};

//===========================================================================
//! レイアウトローダー
//! JSON (objects/parts 配列) を MeshCache のプリミティブ参照付きレイアウトへ変換する
//===========================================================================
class LayoutLoader
{
public:
    //! レイアウト JSON を読み込みます (失敗時 false)
    static bool loadLayout(
        SceneContext& context,
        const std::string& jsonPath,
        PrimitiveLayout& outLayout);

    //! パーツ配列だけを取り出す簡易版
    static bool loadPrimitiveLayout(
        SceneContext& context,
        const std::string& jsonPath,
        std::vector<PrimitiveLayoutPart>& outParts);
};
