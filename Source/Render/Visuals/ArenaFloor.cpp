#include "pch.h"
#include "Render/Visuals/ArenaFloor.h"
#include "Render/Pipeline/RenderUtil.h"
#include "DeviceResources.h"
#include "Services/SceneContext.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

ArenaFloor::ArenaFloor(SceneContext& context) : m_context(&context) {}

// === 初期化 ===

void ArenaFloor::initialize()
{
    auto device = m_context->device->GetD3DDevice();

    float halfWidth = m_size * 0.5f;
    float bottomY = -1.0f;
    float topY = 80.0f;
    float z = 80.0f;

    Vertex vertices[4] = {
        { XMFLOAT3(-halfWidth, bottomY, z)},
        { XMFLOAT3(-halfWidth, topY, z)},
        { XMFLOAT3(halfWidth, bottomY,z)},
        { XMFLOAT3(halfWidth, topY, z)}
    };

    uint16_t indices[6] = { 0, 1, 2, 2, 1, 3 };

    m_vertexBuffer   = RenderUtil::createStaticVertexBuffer(device, vertices, static_cast<UINT>(std::size(vertices)));
    m_indexBuffer    = RenderUtil::createStaticIndexBuffer (device, indices,  static_cast<UINT>(std::size(indices)));
    m_constantBuffer = RenderUtil::createDynamicConstantBuffer<ArenaFloorCB>(device);

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    m_vertexShader = RenderUtil::loadVS(device, L"VS_WaveWorld.cso", &vsBlob);
    m_pixelShader = RenderUtil::loadPS(device, L"PS_WaveWorld.cso");

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    DX::ThrowIfFailed(device->CreateInputLayout(
        layout, 1,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        m_inputLayout.ReleaseAndGetAddressOf()));
}

// === 終了処理 ===

void ArenaFloor::finalize()
{
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_constantBuffer.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_inputLayout.Reset();
}

// === 更新 ===

void ArenaFloor::update(float deltaTime)
{
    m_time += deltaTime;
}

void ArenaFloor::setTransform(
    const Vector3& position,
    const Vector3& rotationDegrees,
    const Vector3& scale)
{
    m_position = position;
    m_rotationDegrees = rotationDegrees;
    m_scale = scale;
}

// === 描画 ===

void ArenaFloor::render(const Matrix& view, const Matrix& projection)
{
    auto context = m_context->device->GetD3DDeviceContext();

    // 定数バッファ更新
    const Matrix world =
        Matrix::CreateScale(m_scale) *
        Matrix::CreateFromYawPitchRoll(
            XMConvertToRadians(m_rotationDegrees.y),
            XMConvertToRadians(m_rotationDegrees.x),
            XMConvertToRadians(m_rotationDegrees.z)) *
        Matrix::CreateTranslation(m_position);
    ArenaFloorCB cb;
    XMStoreFloat4x4(&cb.worldViewProjection,
        (world * view * projection).Transpose());
    cb.time = m_time;
    cb.speed = m_speed;
    cb.brightness = m_brightness;
    cb.alpha = m_alpha;
    RenderUtil::updateDynamicConstantBuffer(context,m_constantBuffer, cb);

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
