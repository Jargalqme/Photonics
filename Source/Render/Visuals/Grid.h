//---------------------------------------------------------------------------
//! @file   Grid.h
//! @brief  グリッド床 (解析的アンチエイリアスグリッド)
//---------------------------------------------------------------------------
#pragma once

struct SceneContext;

//===========================================================================
//! グリッド床
//! 1枚のクアッドに PS がグリッド線を解析的に描く (Pristine Grid 方式)。
//! ライン色は HDR — 1 超の値でブルームに乗る
//===========================================================================
class Grid
{
public:
    Grid(SceneContext& context);
    ~Grid() = default;

    void initialize();

    void update();

    //! 床面を描画します
    void render(const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& projection);

    //! 任意のワールド行列で1面を描画します
    void renderPlane(const Matrix& world, const Matrix& view, const Matrix& projection);

    void finalize();

    void setLineColor(const DirectX::SimpleMath::Color& color) { m_lineColor = color; }
    void setBaseColor(const DirectX::SimpleMath::Color& color) { m_baseColor = color; }
    void setBeatPulse(float pulse) { m_beatPulse = pulse; }

    // デバッグUI用ポインタ
    float* getLineWidthXPtr() { return &m_lineWidthX; }
    float* getLineWidthYPtr() { return &m_lineWidthY; }
    float* getGridScalePtr() { return &m_gridScale; }
    float* getLineEmissiveIntensityPtr() { return &m_lineEmissiveIntensity; }
    float* getLineColorPtr() { return reinterpret_cast<float*>(&m_lineColor); }
    float* getBaseColorPtr() { return reinterpret_cast<float*>(&m_baseColor); }

private:
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
    };

    struct GridCB
    {
        DirectX::SimpleMath::Matrix  worldViewProjection;
        DirectX::SimpleMath::Vector4 gridParams;   //!< x=lineWidthX, y=lineWidthY, z=scale, w=emissiveIntensity
        DirectX::SimpleMath::Vector4 lineColor;
        DirectX::SimpleMath::Vector4 baseColor;
    };

    SceneContext* m_context;

    // グリッドパラメータ
    static constexpr float GRID_SIZE     = 200.0f;
    static constexpr float FLOOR_Y       = -1.0f;
    static constexpr float WALL_HEIGHT   = 199.5f;    //!< 未使用 (壁描画の名残)
    static constexpr float WALL_DISTANCE = 50.0f;     //!< 未使用 (壁描画の名残)

    float m_gridSize = GRID_SIZE;
    float m_lineWidthX = 0.015f;
    float m_lineWidthY = 0.015f;
    float m_gridScale = 0.1f;
    float m_beatPulse = 0.0f;                    //!< ビート同期パルス (現在 update では未使用)
    float m_lineEmissiveIntensity = 2.0f;        //!< ブルームに乗せる発光倍率

    // 既定色 (シーン側が setLineColor/setBaseColor で上書きする)
    DirectX::SimpleMath::Color m_lineColor{ 1.0f, 0.0f, 0.2f, 1.0f };
    DirectX::SimpleMath::Color m_baseColor{ 0.0f, 0.0f, 0.0f, 1.0f };
    DirectX::SimpleMath::Color m_finalColor{ 0.0f, 0.0f, 0.0f, 1.0f };    //!< update で算出する表示色

    // グリッド面のワールド行列
    DirectX::SimpleMath::Matrix m_worldFloor = Matrix::Identity * Matrix::CreateTranslation(0, FLOOR_Y, 0);

    // GPUリソース
    com_ptr<ID3D11Buffer>            m_vertexBuffer;
    com_ptr<ID3D11Buffer>            m_indexBuffer;
    com_ptr<ID3D11Buffer>            m_constantBuffer;
    com_ptr<ID3D11VertexShader>      m_vertexShader;
    com_ptr<ID3D11PixelShader>       m_pixelShader;
    com_ptr<ID3D11InputLayout>       m_inputLayout;
};
