#pragma once

#include <algorithm>
#include <vector>
#include <d3dx9.h>

#include "vec2.h"

enum RENDERER_FONT_LIST {
	Tahoma11px,
	Tahoma12px,
	Verdana12px,
	VerdanaBold12px,
	maxFonts
};

enum RENDERER_TEXT_FLAGS {
	TEXT_NONE,
	TEXT_OUTLINE = (1 << 0)
};

struct font {
	RENDERER_FONT_LIST index;
	int px;
	std::string name;
	uint32_t weight;
	DWORD quality;
};

struct vertice {
	float x, y, z, rhw;
	D3DCOLOR col;
};

class c_color {
public:
	int _r, _g, _b, _a;
	c_color(int r = 255, int g = 255, int b = 255, int a = 255)
		: _r(r), _g(g), _b(b), _a(a) {}

	D3DCOLOR get_d3d() const {
		return D3DCOLOR_RGBA(_r, _g, _b, _a);
	}
};

class c_font {
public:
	void init(IDirect3DDevice9* device);
	void uninit();

	class c_font_wrapper {
	public:
		c_font_wrapper() : m_font(nullptr) {}

		void draw(const std::string& string, float x, float y, uint8_t flags, c_color color);

		float get_text_width(const std::string& string);
		float get_text_height(const std::string& string);

		void set_font(ID3DXFont* font) { m_font = font; }
		ID3DXFont* get_font() const { return m_font; }

		void release() {
			if (m_font) {
				m_font->Release();
				m_font = nullptr;
			}
		}

	private:
		ID3DXFont* m_font;
	};

	c_font_wrapper& operator[](RENDERER_FONT_LIST Index) { return m_fonts[Index]; }

private:
	c_font_wrapper m_fonts[maxFonts];
};

class c_sprite {
public:
	c_sprite() : m_device(nullptr), m_sprite(nullptr), m_texture(nullptr),
		m_img(nullptr), m_img_sz(0), m_width(0), m_height(0), m_res(E_FAIL) {}

	~c_sprite() {}

	void init(IDirect3DDevice9* device, const byte* img, const size_t img_size, int width, int height) {
		HRESULT hr = E_FAIL;

		release();

		m_width = width;
		m_height = height;
		m_device = device;
		m_img = img;
		m_img_sz = img_size;

		D3DXCreateSprite(m_device, &m_sprite);
		hr = D3DXCreateTextureFromFileInMemoryEx(m_device, m_img, m_img_sz, m_width, m_height,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &m_texture);

		if (FAILED(hr))
		{
			if (m_texture) {
				m_texture->Release();
				m_texture = nullptr;
			}
		}

		m_res = hr;
	}

	void release() {
		if (m_texture) {
			m_texture->Release();
			m_texture = nullptr;
		}

		if (m_sprite) {
			m_sprite->Release();
			m_sprite = nullptr;
		}
	}

	void begin(DWORD flags) {
		if (!m_device || !m_sprite)
			return;

		m_sprite->Begin(flags);
	}

	void end() {
		if (!m_device || !m_sprite)
			return;

		m_sprite->End();
	}

	void on_reset() {
		if (!m_device || !m_sprite || !m_texture)
			return;

		m_sprite->OnLostDevice();

		if (m_texture) {
			m_texture->Release();
			m_texture = nullptr;
		}
	}

	void on_reset_end() {
		HRESULT hr = E_FAIL;

		if (!m_device || !m_sprite)
			return;

		m_sprite->OnResetDevice();

		hr = D3DXCreateTextureFromFileInMemoryEx(m_device, m_img, m_img_sz, m_width, m_height,
			D3DX_DEFAULT, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &m_texture);

		if (FAILED(hr))
		{
			if (m_texture) {
				m_texture->Release();
				m_texture = nullptr;
			}
		}

		m_res = hr;
	}

	void draw(const Vec2& pos, c_color color) {
		if (!m_device || !m_texture || !m_sprite)
			return;

		D3DXMATRIX Matrix;
		D3DXMatrixTranslation(&Matrix, pos.x, pos.y, 0.0f);

		m_sprite->SetTransform(&Matrix);
		m_sprite->Draw(m_texture, 0, 0, 0, color.get_d3d());
	}

	void draw(float x, float y, c_color color) {
		return draw({ x, y, }, color);
	}

	int get_width() { return m_width; }
	int get_height() { return m_height; }

	HRESULT get_result() { return m_res; }

private:
	int m_width;
	int m_height;

	IDirect3DDevice9*  m_device;
	ID3DXSprite*       m_sprite;
	IDirect3DTexture9* m_texture;

	const byte* m_img;
	size_t m_img_sz;

	HRESULT m_res;
};

class c_renderer {
public:
	void init(IDirect3DDevice9* device);
	void uninit();

	void begin();
	void end();

	void rect(Vec2 pos, Vec2 size, c_color c);
	void rect(float x, float y, float w, float h, c_color c) {
		return rect({ x, y }, { w, h }, c);
	}

	void rect_fill(Vec2 pos, Vec2 size, c_color c);
	void rect_fill(float x, float y, float w, float h, c_color c) {
		return rect_fill({ x, y }, { w, h }, c);
	}

	void line(Vec2 a, Vec2 b, c_color c);
	void line(float x, float y, float w, float h, c_color c) {
		return line({ x, y }, { w, h }, c);
	}

	void gradient_v(Vec2 pos, Vec2 size, c_color c_a, c_color c_b);
	void gradient_v(float x, float y, float w, float h, c_color c_a, c_color c_b) {
		return gradient_v({ x, y }, { w, h }, c_a, c_b);
	}

	void gradient_h(Vec2 pos, Vec2 size, c_color c_a, c_color c_b);
	void gradient_h(float x, float y, float w, float h, c_color c_a, c_color c_b) {
		return gradient_h({ x, y }, { w, h }, c_a, c_b);
	}

	IDirect3DDevice9* get_device() { return m_device; }

private:
	IDirect3DDevice9*            m_device;
	IDirect3DStateBlock9*        m_state_block;
	IDirect3DVertexDeclaration9* m_vert_decl;
	IDirect3DVertexShader9*      m_vert_shader;
};

inline c_font g_font;
inline c_renderer g_renderer;