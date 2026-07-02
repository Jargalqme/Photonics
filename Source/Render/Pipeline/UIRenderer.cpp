#include "pch.h"
#include "Render/Pipeline/UIRenderer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void UIRenderer::createDeviceDependentResources(ID3D11Device* device,
	ID3D11DeviceContext* context)
{
	m_spriteBatch = std::make_unique<SpriteBatch>(context);

	try
	{
		m_fontTitle = std::make_unique<SpriteFont>(device, GetAssetPath(L"Fonts/Michroma_64_Regular.spritefont").c_str());
		m_fontQuote = std::make_unique<SpriteFont>(device, GetAssetPath(L"Fonts/RobotoCondensed_42_Regular.spritefont").c_str());
		m_fontMenu  = std::make_unique<SpriteFont>(device, GetAssetPath(L"Fonts/RobotoCondensed_32_Regular.spritefont").c_str());
		m_fontHud   = std::make_unique<SpriteFont>(device, GetAssetPath(L"Fonts/RobotoCondensed_24_Regular.spritefont").c_str());
	}
	catch (const std::exception& e)
	{
		OutputDebugStringA("Font loading failed: ");
		OutputDebugStringA(e.what());
		OutputDebugStringA("\n");
	}

	// Create 1x1 white texture for drawRect
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = 1;
	texDesc.Height = 1;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	uint32_t whitePixel = 0xFFFFFFFF;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = &whitePixel;
	initData.SysMemPitch = sizeof(uint32_t);

	ComPtr<ID3D11Texture2D> tex;
	DX::ThrowIfFailed(device->CreateTexture2D(&texDesc, &initData, tex.GetAddressOf()));
	DX::ThrowIfFailed(device->CreateShaderResourceView(tex.Get(), nullptr,
		m_whiteTexture.ReleaseAndGetAddressOf()));
}

void UIRenderer::setScreenSize(float width, float height)
{
	m_screenWidth = width;
	m_screenHeight = height;
	updateScaling();
}

void UIRenderer::onDeviceLost()
{
	m_spriteBatch.reset();
	m_fontTitle.reset();
	m_fontQuote.reset();
	m_fontMenu.reset();
	m_fontHud.reset();
	m_whiteTexture.Reset();
}

SpriteFont* UIRenderer::getFont(FontType type) const
{
	switch (type)
	{
	case FontType::Title: return m_fontTitle.get();
	case FontType::Quote: return m_fontQuote.get();
	case FontType::Menu:  return m_fontMenu.get();
	case FontType::Hud:   return m_fontHud.get();
	default:              return m_fontMenu.get();
	}
}

void UIRenderer::updateScaling()
{
	m_scaleX = m_screenWidth / REF_WIDTH;
	m_scaleY = m_screenHeight / REF_HEIGHT;
}

void UIRenderer::begin()
{
	m_spriteBatch->Begin(SpriteSortMode_Deferred, nullptr);  // Default blend state (alpha)
}

void UIRenderer::end()
{
	m_spriteBatch->End();
}

RECT UIRenderer::fullscreenRect() const
{
	return RECT{ 0, 0, static_cast<LONG>(REF_WIDTH), static_cast<LONG>(REF_HEIGHT) };
}

void UIRenderer::drawRect(const RECT& refRect, FXMVECTOR color, float alpha)
{
	if (!m_whiteTexture) return;

	// Scale from reference coords to screen coords
	RECT screenRect;
	screenRect.left = static_cast<LONG>(refRect.left * m_scaleX);
	screenRect.top = static_cast<LONG>(refRect.top * m_scaleY);
	screenRect.right = static_cast<LONG>(refRect.right * m_scaleX);
	screenRect.bottom = static_cast<LONG>(refRect.bottom * m_scaleY);

	// SpriteBatch's default blend state expects premultiplied alpha
	XMVECTOR finalColor = XMVectorSetW(XMVectorScale(color, alpha), alpha);

	m_spriteBatch->Draw(m_whiteTexture.Get(), screenRect, finalColor);
}

void UIRenderer::drawText(const std::wstring& text, XMFLOAT2 refPosition,
	FXMVECTOR color, float alpha, FontType font, float scale)
{
	SpriteFont* spriteFont = getFont(font);
	if (!spriteFont) return;

	XMFLOAT2 screenPos;
	screenPos.x = refPosition.x * m_scaleX;
	screenPos.y = refPosition.y * m_scaleY;

	float finalScale = scale * std::min(m_scaleX, m_scaleY);

	// SpriteBatch's default blend state expects premultiplied alpha
	XMVECTOR finalColor = XMVectorSetW(XMVectorScale(color, alpha), alpha);

	spriteFont->DrawString(
		m_spriteBatch.get(),
		text.c_str(),
		screenPos,
		finalColor,
		0.0f,
		XMFLOAT2(0.f, 0.f),
		finalScale
	);
}

void UIRenderer::drawTextCentered(const std::wstring& text, XMFLOAT2 refPosition,
	FXMVECTOR color, float alpha, FontType font, float scale)
{
	SpriteFont* spriteFont = getFont(font);
	if (!spriteFont) return;

	XMVECTOR textSize = spriteFont->MeasureString(text.c_str());
	XMFLOAT2 size;
	XMStoreFloat2(&size, textSize);

	float finalScale = scale * std::min(m_scaleX, m_scaleY);

	XMFLOAT2 origin(size.x * 0.5f, size.y * 0.5f);

	XMFLOAT2 screenPos;
	screenPos.x = refPosition.x * m_scaleX;
	screenPos.y = refPosition.y * m_scaleY;

	// SpriteBatch's default blend state expects premultiplied alpha
	XMVECTOR finalColor = XMVectorSetW(XMVectorScale(color, alpha), alpha);

	spriteFont->DrawString(
		m_spriteBatch.get(),
		text.c_str(),
		screenPos,
		finalColor,
		0.0f,
		origin,
		finalScale
	);
}

XMFLOAT2 UIRenderer::measureText(const std::wstring& text, FontType font) const
{
	SpriteFont* spriteFont = getFont(font);
	if (!spriteFont) return XMFLOAT2(0.0f, 0.0f);

	XMFLOAT2 size;
	XMStoreFloat2(&size, spriteFont->MeasureString(text.c_str()));
	return size;
}
