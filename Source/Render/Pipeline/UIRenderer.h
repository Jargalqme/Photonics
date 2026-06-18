#pragma once
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <memory>
#include <string>
#include "Common/typedef.h"

enum class FontType{ Title, Quote, Menu, Hud};

class UIRenderer
{
public:
	static constexpr float REF_WIDTH  = 1920.0f;
	static constexpr float REF_HEIGHT = 1080.0f;

	void createDeviceDependentResources(ID3D11Device* device,
		ID3D11DeviceContext* context);
		
	void setScreenSize(float width, float height);
	void onDeviceLost();

	void begin();
	void end();

	void drawRect(const RECT& refRect, DirectX::FXMVECTOR color, float alpha = 1.0f);
	void drawText(const std::wstring& text, DirectX::XMFLOAT2 refPosition,
		DirectX::FXMVECTOR color, float alpha = 1.0f,
		FontType font = FontType::Menu, float scale = 1.0f);

	void drawTextCentered(const std::wstring& text, DirectX::XMFLOAT2 refPosition,
		DirectX::FXMVECTOR color, float alpha = 1.0f,
		FontType font = FontType::Menu, float scale = 1.0f);

	// Returns size in reference (1920x1080) units, i.e. at scale = 1.0
	[[nodiscard]] DirectX::XMFLOAT2 measureText(const std::wstring& text,
		FontType font = FontType::Menu) const;

	[[nodiscard]] RECT fullscreenRect() const;
	[[nodiscard]] float scaleX() const { return m_scaleX; }
	[[nodiscard]] float scaleY() const { return m_scaleY; }

private:
	DirectX::SpriteFont* getFont(FontType type) const;
	void updateScaling();

	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
	std::unique_ptr<DirectX::SpriteFont>  m_fontTitle;  // 64pt Bold
	std::unique_ptr<DirectX::SpriteFont>  m_fontQuote;  // 42pt Regular
	std::unique_ptr<DirectX::SpriteFont>  m_fontMenu;   // 32pt Regular
	std::unique_ptr<DirectX::SpriteFont>  m_fontHud;    // 24pt Regular
	com_ptr<ID3D11ShaderResourceView>     m_whiteTexture;

	float m_screenWidth = REF_WIDTH;
	float m_screenHeight = REF_HEIGHT;
	float m_scaleX = 1.0f;
	float m_scaleY = 1.0f;
};