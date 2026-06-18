#include "pch.h"
#include "Render/Visuals/Tracers.h"

#include "DeviceResources.h"
#include "Render/Pipeline/RenderUtil.h"
#include "Render/Assets/ShaderCache.h"
#include "Services/SceneContext.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
	constexpr float  TRACER_LIFETIME = 0.120f;
	constexpr float  TRACER_WIDTH = 0.075f;
	constexpr float  TRACER_NUDGE_FORWARD = 0.35f;
	constexpr float  TRACER_MIN_SEGMENT_LENGTH = 14.0f;
	constexpr float  TRACER_MAX_SEGMENT_LENGTH = 58.0f;
	constexpr float  TRACER_SEGMENT_FRACTION = 0.38f;
	constexpr size_t TRACER_RESERVE = 16;
}

Tracers::Tracers(SceneContext& context)
	: m_context(&context)
{
}

void Tracers::initialize()
{
	auto* device = m_context->device->GetD3DDevice();

	m_vertexShader = m_context->shaders->getVS(L"VS_Tracer.cso");
	m_pixelShader = m_context->shaders->getPS(L"PS_Tracer.cso");
	m_constantBuffer = RenderUtil::createDynamicConstantBuffer<TracerCB>(device);

	m_tracers.reserve(TRACER_RESERVE);
}

void Tracers::finalize()
{
	m_vertexShader = nullptr;
	m_pixelShader = nullptr;
	m_constantBuffer.Reset();
	m_tracers.clear();
}

void Tracers::spawn(const Vector3& start, const Vector3& end, const Vector4& color)
{
	Vector3 toEnd = end - start;
	const float distance = toEnd.Length();
	if (distance <= TRACER_NUDGE_FORWARD)
	{
		return;
	}

	toEnd /= distance;
	const Vector3 nudgedStart = start + toEnd * TRACER_NUDGE_FORWARD;
	m_tracers.push_back(Tracer{ nudgedStart, end, color, 1.0f, true });
}

void Tracers::update(float deltaTime)
{
	const float decay = deltaTime / TRACER_LIFETIME;
	for (auto& tracer : m_tracers)
	{
		if (tracer.fresh)
		{
			tracer.fresh = false;
			continue;
		}

		tracer.life -= decay;
	}

	std::erase_if(m_tracers, [](const Tracer& tracer)
	{
		return tracer.life <= 0.0f;
	});
}

void Tracers::render(const Matrix& view, const Matrix& projection, const Vector3& cameraPos)
{
	if (m_tracers.empty())
	{
		return;
	}

	auto* context = m_context->device->GetD3DDeviceContext();
	auto* commonStates = m_context->commonStates;

	const Matrix viewProjection = (view * projection).Transpose();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(nullptr);

	context->VSSetShader(m_vertexShader, nullptr, 0);
	context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	context->PSSetShader(m_pixelShader, nullptr, 0);
	context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	float blendFactor[4] = { 0, 0, 0, 0 };
	context->OMSetBlendState(commonStates->Additive(), blendFactor, 0xFFFFFFFF);
	context->OMSetDepthStencilState(commonStates->DepthRead(), 0);
	context->RSSetState(commonStates->CullNone());

	for (const auto& tracer : m_tracers)
	{
		const Vector3 path = tracer.end - tracer.start;
		const float pathLength = path.Length();
		if (pathLength <= 0.001f)
		{
			continue;
		}

		const float progress = 1.0f - tracer.life;
		const float segmentLength = std::clamp(
			pathLength * TRACER_SEGMENT_FRACTION,
			TRACER_MIN_SEGMENT_LENGTH,
			TRACER_MAX_SEGMENT_LENGTH);
		const float segmentT = std::min(segmentLength / pathLength, 1.0f);
		const float headT = std::clamp(progress + segmentT, 0.0f, 1.0f);
		const float tailT = std::clamp(headT - segmentT, 0.0f, 1.0f);
		const Vector3 segmentStart = Vector3::Lerp(tracer.start, tracer.end, tailT);
		const Vector3 segmentEnd = Vector3::Lerp(tracer.start, tracer.end, headT);

		TracerCB cb = {};
		cb.viewProjection = viewProjection;
		cb.beamStart = segmentStart;
		cb.beamWidth = TRACER_WIDTH;
		cb.beamEnd = segmentEnd;
		cb.beamLife = tracer.life;
		cb.beamColor = tracer.color;
		cb.cameraPosition = cameraPos;
		RenderUtil::updateDynamicConstantBuffer(context, m_constantBuffer, cb);

		context->Draw(6, 0);
	}

	context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(nullptr, 0);
	context->RSSetState(nullptr);
}
