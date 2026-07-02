#include "pch.h"
#include "Render/Visuals/WaveSurface.h"
#include "Render/Pipeline/RenderUtil.h"
#include "DeviceResources.h"
#include "Services/SceneContext.h"

#include <algorithm>
#include <utility>

WaveSurface::WaveSurface(SceneContext& context) : m_context(&context) {}

// === 初期化・終了 ===

void WaveSurface::initialize()
{
    auto device = m_context->device->GetD3DDevice();

    // 状態バッファを 0 で確保し、中央に最初の水滴を落とす
    m_prev.assign(ROWS, std::vector<double>(COLS, 0.0));
    m_curr.assign(ROWS, std::vector<double>(COLS, 0.0));
    m_next.assign(ROWS, std::vector<double>(COLS, 0.0));
    splash(COLS / 2, ROWS / 2);

    // インデックスバッファ（静的）
    std::vector<uint16_t> indices;
    indices.reserve((COLS - 1) * (ROWS - 1) * 6);
    for (int z = 0; z < ROWS - 1; ++z)
        for (int x = 0; x < COLS - 1; ++x)
        {
            uint16_t tl = uint16_t(z * COLS + x);
            uint16_t tr = uint16_t(tl + 1);
            uint16_t bl = uint16_t(tl + COLS);
            uint16_t br = uint16_t(bl + 1);
            indices.insert(indices.end(), { tl, bl, tr,  tr, bl, br });
        }
    m_indexCount  = static_cast<UINT>(indices.size());
    m_indexBuffer = RenderUtil::createStaticIndexBuffer(device, indices.data(), m_indexCount);

    // 頂点バッファ（動的・毎フレーム書き換え）
    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth      = static_cast<UINT>(sizeof(Vertex) * COLS * ROWS);
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DX::ThrowIfFailed(device->CreateBuffer(&bd, nullptr, m_vertexBuffer.ReleaseAndGetAddressOf()));

    // 定数バッファ
    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<WaveCB>(device);

    // シェーダー読み込み
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    m_vertexShader = RenderUtil::loadVS(device, L"VS_WaveSurface.cso", &vsBlob);
    m_pixelShader  = RenderUtil::loadPS(device, L"PS_WaveSurface.cso");

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    DX::ThrowIfFailed(device->CreateInputLayout(
        layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        m_inputLayout.ReleaseAndGetAddressOf()));
}

void WaveSurface::finalize()
{
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_constantBuffer.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_inputLayout.Reset();
}

// === 更新 ===

void WaveSurface::update(float dt)
{
    // フレームレート非依存の固定タイムステップ
    m_accumulator += dt * m_simSpeed;

    int steps = 0;
    while (m_accumulator >= STEP_DT && steps < MAX_STEPS_PER_FRAME)
    {
        updateWave(m_curr, m_prev, m_next, m_cConst);
        std::swap(m_prev, m_curr);   // prev ← curr ← next
        std::swap(m_curr, m_next);
        m_accumulator -= STEP_DT;
        ++steps;
    }
    if (steps == MAX_STEPS_PER_FRAME)
        m_accumulator = 0.0f;   // 遅延の蓄積を防ぐ
}

// === 描画 ===

void WaveSurface::render(const Matrix& view, const Matrix& projection)
{
    auto context = m_context->device->GetD3DDeviceContext();

    // 高さ場を頂点バッファへ書き込む（位置・法線）
    D3D11_MAPPED_SUBRESOURCE ms;
    context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    auto* v = reinterpret_cast<Vertex*>(ms.pData);
    for (int z = 0; z < ROWS; ++z)
        for (int x = 0; x < COLS; ++x)
        {
            float dhx = sampleH(x + 1, z) - sampleH(x - 1, z);   // 中心差分で法線
            float dhz = sampleH(x, z + 1) - sampleH(x, z - 1);
            Vector3 n(-dhx * m_heightScale, 2.0f * CELL, -dhz * m_heightScale);
            n.Normalize();
            Vertex& out  = v[z * COLS + x];
            out.position = XMFLOAT3((x - COLS * 0.5f) * CELL,
                                    sampleH(x, z) * m_heightScale,
                                    (z - ROWS * 0.5f) * CELL);
            out.normal   = XMFLOAT3(n.x, n.y, n.z);
        }
    context->Unmap(m_vertexBuffer.Get(), 0);

    // 定数バッファ更新（転置して格納）
    WaveCB cb;
    cb.worldViewProjection = (m_world * view * projection).Transpose();
    RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

    // パイプライン設定
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(Vertex), offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // ステート（不透明・深度書き込み・両面）
    auto* states = m_context->commonStates;
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(states->Opaque(), blendFactor, 0xffffffff);
    context->OMSetDepthStencilState(states->DepthDefault(), 0);
    context->RSSetState(states->CullNone());

    context->DrawIndexed(m_indexCount, 0, 0);

    // ステート復元
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}

// === ヘルパー ===

void WaveSurface::splash(int x, int z, float amount)
{
    if (x <= 0 || x >= COLS - 1 || z <= 0 || z >= ROWS - 1) return;
    m_curr[z][x] = amount;
    m_prev[z][x] = amount;   // 初速 0
}

float WaveSurface::sampleH(int x, int z) const
{
    x = std::clamp(x, 0, COLS - 1);
    z = std::clamp(z, 0, ROWS - 1);
    return static_cast<float>(m_curr[z][x]);
}
