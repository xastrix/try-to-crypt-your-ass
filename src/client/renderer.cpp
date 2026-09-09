#include "renderer.h"

void c_renderer::init(IDirect3DDevice9* device)
{
	m_device = device;
}

void c_renderer::uninit()
{
	m_device = nullptr;
}

void c_renderer::begin()
{
	m_device->CreateStateBlock(D3DSBT_PIXELSTATE, &m_state_block);

	m_state_block->Capture();

	m_device->GetVertexDeclaration(&m_vert_decl);
	m_device->GetVertexShader(&m_vert_shader);

	m_device->SetVertexShader(nullptr);
	m_device->SetPixelShader(nullptr);

	m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_device->SetRenderState(D3DRS_FOGENABLE, FALSE);
	m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
	m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	m_device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	m_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	m_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);

	m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
	m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA);
	m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);

	m_device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	m_device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
}

void c_renderer::end()
{
	m_state_block->Apply();
	m_state_block->Release();

	m_device->SetVertexDeclaration(m_vert_decl);
	m_device->SetVertexShader(m_vert_shader);
}

void c_renderer::rect(Vec2 pos, Vec2 size, c_color c)
{
	vertice verts[5] = {
		{ int(pos.x), int(pos.y), 0.01f, 0.01f, c.get_d3d() },
		{ int(pos.x + size.x), int(pos.y), 0.01f, 0.01f, c.get_d3d() },
		{ int(pos.x + size.x), int(pos.y + size.y), 0.01f, 0.01f, c.get_d3d() },
		{ int(pos.x), int(pos.y + size.y), 0.01f, 0.01f, c.get_d3d() },
		{ int(pos.x), int(pos.y), 0.01f, 0.01f, c.get_d3d() }
	};

	m_device->SetTexture(0, nullptr);
	m_device->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &verts, 20);
}

void c_renderer::rect_fill(Vec2 pos, Vec2 size, c_color c)
{
	vertice verts[4] = {
		{ int(pos.x), int(pos.y), 0.01f, 0.01f, c.get_d3d() },
		{ int(pos.x + size.x), int(pos.y), 0.01f, 0.01f, c.get_d3d() },
		{ int(pos.x), int(pos.y + size.y), 0.01f, 0.01f, c.get_d3d() },
		{ int(pos.x + size.x), int(pos.y + size.y), 0.01f, 0.01f, c.get_d3d() }
	};

	m_device->SetTexture(0, nullptr);
	m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &verts, 20);
}

void c_renderer::line(Vec2 a, Vec2 b, c_color c)
{
	vertice verts[2] = {
		{ (int)a.x, (int)a.y, 0.01f, 0.01f, c.get_d3d() },
		{ (int)b.x, (int)b.y, 0.01f, 0.01f, c.get_d3d() }
	};

	m_device->SetTexture(0, nullptr);
	m_device->DrawPrimitiveUP(D3DPT_LINELIST, 1, &verts, 20);
}

void c_renderer::gradient_v(Vec2 pos, Vec2 size, c_color c_a, c_color c_b)
{
	vertice verts[4] = {
		{ int(pos.x), int(pos.y), 0.01f, 0.01f, c_a.get_d3d() },
		{ int(pos.x + size.x), int(pos.y), 0.01f, 0.01f, c_a.get_d3d() },
		{ int(pos.x), int(pos.y + size.y), 0.01f, 0.01f, c_b.get_d3d() },
		{ int(pos.x + size.x), int(pos.y + size.y), 0.01f, 0.01f, c_b.get_d3d() }
	};

	m_device->SetTexture(0, nullptr);
	m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &verts, 20);
}

void c_renderer::gradient_h(Vec2 pos, Vec2 size, c_color c_a, c_color c_b)
{
	vertice verts[4] = {
		{ int(pos.x), int(pos.y), 0.01f, 0.01f, c_a.get_d3d() },
		{ int(pos.x + size.x), int(pos.y), 0.01f, 0.01f, c_b.get_d3d() },
		{ int(pos.x), int(pos.y + size.y), 0.01f, 0.01f, c_a.get_d3d() },
		{ int(pos.x + size.x), int(pos.y + size.y), 0.01f, 0.01f, c_b.get_d3d() }
	};

	m_device->SetTexture(0, nullptr);
	m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &verts, 20);
}