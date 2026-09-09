#include "renderer.h"

#include <xorstr.h>

static const std::vector<font> g_fonts = {
	{ Tahoma11px,      11, __("Tahoma"),  FW_NORMAL, ANTIALIASED_QUALITY },
	{ Tahoma12px,      12, __("Tahoma"),  FW_MEDIUM, ANTIALIASED_QUALITY },
	{ Verdana12px,     12, __("Verdana"), FW_NORMAL, PROOF_QUALITY },
	{ VerdanaBold12px, 12, __("Verdana"), FW_BOLD,   PROOF_QUALITY },
};

void c_font::init(IDirect3DDevice9* device)
{
	for (const auto& font : g_fonts)
	{
		ID3DXFont* tmpFont = nullptr;

		D3DXCreateFontA(
			device,
			font.px, 0,
			font.weight, 1, 0,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			font.quality,
			FF_DONTCARE,
			font.name.c_str(),
			&tmpFont
		);

		m_fonts[font.index].set_font(tmpFont);
	}
}

void c_font::uninit()
{
	for (const auto& font : g_fonts)
	{
		m_fonts[font.index].release();
	}
}

void c_font::c_font_wrapper::draw(const std::string& string, float x, float y, uint8_t flags, c_color color)
{
	RECT r{ x, y, x, y };

	if (flags & TEXT_OUTLINE) {
		RECT o_r{ x + 1, y + 1, x + 1, y + 1 };
		m_font->DrawTextA(NULL, string.c_str(), -1, &o_r, DT_NOCLIP, c_color(0, 0, 0, color._a).get_d3d());
	}

	m_font->DrawTextA(NULL, string.c_str(), -1, &r, DT_NOCLIP, color.get_d3d());
}

float c_font::c_font_wrapper::get_text_width(const std::string& string)
{
	RECT r;
	m_font->DrawTextA(0, string.c_str(), -1, &r, DT_CALCRECT, 0xffffffff);

	return (r.right - r.left);
}

float c_font::c_font_wrapper::get_text_height(const std::string& string)
{
	RECT r;
	m_font->DrawTextA(0, string.c_str(), -1, &r, DT_CALCRECT, 0xffffffff);

	return (r.bottom - r.top);
}