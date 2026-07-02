#pragma once

#include "Render/Visuals/WaveSim.h"

// 前方宣言
struct SceneContext;

/// 波動方程式（有限差分）の高さ場を 3D サーフェスとして描画する。
class WaveSurface
{
public:
    WaveSurface(SceneContext& context);
    ~WaveSurface() = default;

    void initialize();
    void update(float dt);
    void render(const DirectX::SimpleMath::Matrix& view,
                const DirectX::SimpleMath::Matrix& projection);
    void finalize();

    // 指定セルに水滴を落とす
    void splash(int x, int z, float amount = 100.0f);

    // デバッグUI用ポインタ
    float* getHeightPtr() { return &m_heightScale; }
    float* getcConstPtr() { return &m_cConst; }
    float* getSimSpeedPtr() { return &m_simSpeed; }

private:
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
    };

    struct WaveCB
    {
        DirectX::SimpleMath::Matrix worldViewProjection;
    };

    // === グリッド／シミュレーション設定 ===
    static constexpr int   COLS                = 128;          // 横の頂点数
    static constexpr int   ROWS                = 128;          // 縦の頂点数
    static constexpr float CELL                = 0.5f;         // 頂点間隔（ワールド単位）
    static constexpr float STEP_DT             = 1.0f / 60.0f; // 固定タイムステップ（秒）
    static constexpr int   MAX_STEPS_PER_FRAME = 8;            // 1フレームの最大ステップ数

    float m_heightScale = 0.3f;    // 高さの描画スケール
    float m_cConst      = 0.05f;   // 伝播係数（< 0.5）
    float m_simSpeed    = 0.5f;    // 再生速度
    float m_accumulator = 0.0f;    // 固定タイムステップの時間蓄積

    float sampleH(int x, int z) const;

    SceneContext* m_context;

    // === シミュレーション状態 ===
    WaveGrid m_prev;
    WaveGrid m_curr;
    WaveGrid m_next;

    // ワールド変換（回転なし＝法線は不変）
    DirectX::SimpleMath::Matrix m_world = DirectX::SimpleMath::Matrix::Identity;

    // === GPU リソース ===
    com_ptr<ID3D11Buffer>       m_vertexBuffer;   // 動的（毎フレーム書き換え）
    com_ptr<ID3D11Buffer>       m_indexBuffer;
    com_ptr<ID3D11Buffer>       m_constantBuffer;
    com_ptr<ID3D11VertexShader> m_vertexShader;
    com_ptr<ID3D11PixelShader>  m_pixelShader;
    com_ptr<ID3D11InputLayout>  m_inputLayout;
    UINT                        m_indexCount = 0;
};
