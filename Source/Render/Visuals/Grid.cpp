//---------------------------------------------------------------------------
//! @file   Grid.cpp
//! @brief  グリッド床 (解析的アンチエイリアスグリッド)
//---------------------------------------------------------------------------
#include "pch.h"
#include "Render/Visuals/Grid.h"
#include "Render/Pipeline/RenderUtil.h"
#include "DeviceResources.h"
#include "Services/SceneContext.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Grid::Grid(SceneContext& context) : m_context(&context) {}

//===========================================================================
// 初期化・終了
//===========================================================================

//---------------------------------------------------------------------------
//! ジオメトリ・シェーダー・入力レイアウトを構築します
//---------------------------------------------------------------------------
void Grid::initialize()
{
    auto device = m_context->device->GetD3DDevice();

    // ジオメトリ作成（頂点・インデックスバッファ）
    float half = m_gridSize;
    Vertex vertices[4] = {
        { XMFLOAT3(-half, 0, -half) },
        { XMFLOAT3(-half, 0,  half) },
        { XMFLOAT3( half, 0, -half) },
        { XMFLOAT3( half, 0,  half) }
    };
    uint16_t indices[6] = {
        0, 1, 2,
        2, 1, 3
    };

    m_vertexBuffer   = RenderUtil::createStaticVertexBuffer(device, vertices, static_cast<UINT>(std::size(vertices)));
    m_indexBuffer    = RenderUtil::createStaticIndexBuffer (device, indices,  static_cast<UINT>(std::size(indices)));
    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<GridCB>(device);

    // シェーダー読み込み
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    m_vertexShader = RenderUtil::loadVS(device, L"VS_PristineGrid.cso", &vsBlob);
    m_pixelShader  = RenderUtil::loadPS(device, L"PS_PristineGrid.cso");

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    DX::ThrowIfFailed(device->CreateInputLayout(
        layout, 1,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        m_inputLayout.ReleaseAndGetAddressOf()));
}

void Grid::finalize()
{
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_constantBuffer.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_inputLayout.Reset();
}

//===========================================================================
// 更新
//===========================================================================

//---------------------------------------------------------------------------
//! 表示色を算出します
//---------------------------------------------------------------------------
void Grid::update()
{
    // ビートパルス効果（現在は無効 — ライン色をそのまま使用）
    m_finalColor = Color(
        m_lineColor.R(),
        m_lineColor.G(),
        m_lineColor.B(),
        m_lineColor.A()
    );
}

//===========================================================================
// 描画
//===========================================================================

//---------------------------------------------------------------------------
//! 床面を描画します
//---------------------------------------------------------------------------
void Grid::render(const Matrix& view, const Matrix& projection)
{
    renderPlane(m_worldFloor, view, projection);
}

//---------------------------------------------------------------------------
//! 任意のワールド行列で1面を描画します
//---------------------------------------------------------------------------
void Grid::renderPlane(const Matrix& world, const Matrix& view, const Matrix& projection)
{
    auto context = m_context->device->GetD3DDeviceContext();

    // 定数バッファ更新
    GridCB cb;
    cb.worldViewProjection = (world * view * projection).Transpose();
    cb.gridParams = Vector4(m_lineWidthX, m_lineWidthY, m_gridScale, m_lineEmissiveIntensity);
    cb.lineColor = Vector4(m_finalColor.R(), m_finalColor.G(), m_finalColor.B(), m_finalColor.A());
    cb.baseColor = Vector4(m_baseColor.R(), m_baseColor.G(), m_baseColor.B(), m_baseColor.A());
    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

    // パイプライン設定
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    auto* states = m_context->commonStates;
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(states->NonPremultiplied(), blendFactor, 0xffffffff);
    context->OMSetDepthStencilState(states->DepthRead(), 0);
    context->RSSetState(states->CullNone());

    context->DrawIndexed(6, 0, 0);

    // ステート復元
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}
