#include "ui.h"
#include "globals.h"
#include "net.h"

#include <cctype>

void c_ui::init(UI_INTERFACE_STATE state, const Vec2 form_size)
{
	c_ui_form* form = create_form(form_size);

	switch (state) {
	case STATE_START_UP: {
		set_group(form, __("?"), Vec2(355, 205), [](c_ui_groupbox* ctx) {
			ctx->set_cursor_pos(Vec2(92, 50));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_textinput>(&g::fields[FT_USERNAME], __("Username"), false, 170));

			ctx->set_cursor_pos(Vec2(92, 80));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_textinput>(&g::fields[FT_PASSWORD], __("Password"), true, 170));

			ctx->set_cursor_pos(Vec2(102, 120));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Login"), []() {
				if (g::fields[FT_USERNAME].empty() || g::fields[FT_PASSWORD].empty()) return;
				
				if (std::any_of(g::fields[FT_USERNAME].begin(), g::fields[FT_USERNAME].end(), [](unsigned char ch) {
					return std::isspace(ch);
				})) goto invalid;

				if (std::any_of(g::fields[FT_PASSWORD].begin(), g::fields[FT_PASSWORD].end(), [](unsigned char ch) {
					return std::isspace(ch);
				})) goto invalid;

				g_net.set_conn_state(HCONN_AUTHING);
				return;

			invalid:
				g_net.set_conn_state(HCONN_FAILED);
			}, Vec2(150, 24)));

			ctx->set_cursor_pos(Vec2(102, 155));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Exit"), []() {
#ifndef _DEBUG
				PostQuitMessage(0);
#else
				printf("[USER] (Interface) Exit buttons are disabled while running in debug mode\n");
#endif
			}, Vec2(150, 24)));
		}, true);

		break;
	}
	case STATE_AUTHING: {
		set_group(form, __("?"), Vec2(355, 205), [](c_ui_groupbox* ctx) {
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_label>(std::string(
				__("Signing in as ") + g::fields[FT_USERNAME]).substr(0, 42) + __("..."), Tahoma12px, c_color(200, 200, 200),
				c_ui_label::UILABEL_FLAGS::CENTER_X | c_ui_label::UILABEL_FLAGS::CENTER_Y | c_ui_label::UILABEL_FLAGS::PULSE));
		}, true);

		break;
	}
	case STATE_FAILED_CONNECTED: {
		set_group(form, __("?"), Vec2(355, 205), [](c_ui_groupbox* ctx) {
			ctx->set_cursor_pos(Vec2(0, 50));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_label>(__("Invalid username or password"), Tahoma12px,
				c_color(130, 130, 130), c_ui_label::UILABEL_FLAGS::CENTER_X));

			ctx->set_cursor_pos(Vec2(102, 155));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Exit"), []() {
#ifndef _DEBUG
				PostQuitMessage(0);
#else
				printf("[USER] (Interface) Exit buttons are disabled while running in debug mode\n");
#endif
			}, Vec2(150, 24)));
		}, true);

		break;
	}
	case STATE_BANNED: {
		set_group(form, __("?"), Vec2(355, 205), [](c_ui_groupbox* ctx) {
			ctx->set_cursor_pos(Vec2(0, 63));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_label>(__("Your account has been locked"), Tahoma12px,
				c_color(160, 25, 25), c_ui_label::UILABEL_FLAGS::CENTER_X | c_ui_label::UILABEL_FLAGS::PULSE));

			ctx->set_cursor_pos(Vec2(102, 155));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Exit"), []() {
#ifndef _DEBUG
				PostQuitMessage(0);
#else
				printf("[USER] (Interface) Exit buttons are disabled while running in debug mode\n");
#endif
			}, Vec2(150, 24)));
		}, true);

		break;
	}
	case STATE_HWID_ERR: {
		set_group(form, __("?"), Vec2(355, 205), [](c_ui_groupbox* ctx) {
			ctx->set_cursor_pos(Vec2(0, 63));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_label>(__("HWID mismatch"), Tahoma12px,
				c_color(160, 25, 25), c_ui_label::UILABEL_FLAGS::CENTER_X));

			ctx->set_cursor_pos(Vec2(102, 155));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Exit"), []() {
#ifndef _DEBUG
				PostQuitMessage(0);
#else
				printf("[USER] (Interface) Exit buttons are disabled while running in debug mode\n");
#endif
			}, Vec2(150, 24)));
		}, true);

		break;
	}
	case STATE_USER_EXPIRES: {
		form->set_cursor_pos(Vec2(10, 10));
		set_group(form, __("?"), Vec2(280, 116), [this](c_ui_groupbox* ctx) {
			ctx->push_cursor_pos(Vec2(1, 1));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_gameselector>(&g::injector::game_index, std::vector<game> {
				{ g::injector::game_full_list[0], &m_sprites[1] },
				{ g::injector::game_full_list[1], &m_sprites[2] },
			}, 280, 115));
		}, true);

		form->set_cursor_pos(Vec2(305, 10));
		set_group(form, __("Options"), Vec2(190, 115), [](c_ui_groupbox* ctx) {
			ctx->push_cursor_pos(Vec2(25, 20));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Load"), []() {
				g_net.set_conn_state(HCONN_INJECTING);
			}, Vec2(140, 30)));

			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Exit"), []() {
#ifndef _DEBUG
				PostQuitMessage(0);
#else
				printf("[USER] (Interface) Exit buttons are disabled while running in debug mode\n");
#endif
			}, Vec2(140, 30)));
		}, false);

		form->set_cursor_pos(Vec2(10, 140));
		set_group(form, __("Status"), Vec2(485, 165), [](c_ui_groupbox* ctx) {
			ctx->push_cursor_pos(Vec2(20, 20));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_listbox>(&g::log::index, &g::log::list, Vec2(445, 20), 130));
		}, false);

		break;
	}
	case STATE_INJECTING: {
		set_group(form, __("?"), Vec2(355, 205), [](c_ui_groupbox* ctx) {
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_label>(__("Injecting..."), Tahoma12px, c_color(200, 200, 200),
				c_ui_label::UILABEL_FLAGS::CENTER_X | c_ui_label::UILABEL_FLAGS::CENTER_Y | c_ui_label::UILABEL_FLAGS::PULSE));
		}, true);

		break;
	}
	case STATE_SESSION_EXPIRED: {
		set_group(form, __("?"), Vec2(355, 205), [](c_ui_groupbox* ctx) {
			ctx->set_cursor_pos(Vec2(0, 50));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_label>(__("Your session has been expired"), Tahoma12px,
				c_color(130, 130, 130), c_ui_label::UILABEL_FLAGS::CENTER_X));

			ctx->set_cursor_pos(Vec2(102, 155));
			ctx->add_child(c_ui_mem_pool::alloc<c_ui_button>(__("Exit"), []() {
#ifndef _DEBUG
				PostQuitMessage(0);
#else
				printf("[USER] (Interface) Exit buttons are disabled while running in debug mode\n");
#endif
			}, Vec2(150, 24)));
		}, true);

		break;
	}
	}

	m_block = nullptr;
}

void c_ui::uninit()
{
	for (int i = 0; i < maxSprites; i++) m_sprites[i].release();
	c_ui_mem_pool::release();
}

void c_ui::think()
{
	if (!m_active_window) return;

	m_form->think();
}

void c_ui::poll_input(HWND window)
{
	m_prev_mouse_pos = m_mouse_pos;

	for (int i = 0; i < 256; i++) {
		m_prev_key_state[i] = m_key_state[i];
		m_key_state[i] = GetAsyncKeyState(i);
	}

	POINT p;

	GetCursorPos(&p);
	ScreenToClient(window, &p);

	m_mouse_pos = Vec2(p.x, p.y);

#ifdef _DEBUG
	if (is_key_down(VK_LBUTTON) && is_hovered(m_pos, m_pos + m_size) && m_dragging)
		m_pos -= (m_prev_mouse_pos - m_mouse_pos);

	else if (is_key_pressed(VK_LBUTTON) && !m_dragging)
		m_dragging = true;

	m_pos.x = std::max(0.0f, std::min(m_pos.x, DBG_WINDOW_WIDTH - m_size.x));
	m_pos.y = std::max(0.0f, std::min(m_pos.y, DBG_WINDOW_HEIGHT - m_size.y));
#endif

	m_active_window = window == GetForegroundWindow();
}

void c_ui::draw()
{
	RECT oldScissorRect;
	DWORD scissorStatus = 0;

	c_color outlColors[5] = {
		c_color(50, 50, 50), c_color(35, 35, 35),
		c_color(35, 35, 35), c_color(35, 35, 35),
		c_color(42, 42, 42)
	};

	g_renderer.rect_fill(m_pos.x, m_pos.y, m_size.x, m_size.y, c_color(20, 20, 20));

	g_renderer.get_device()->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorStatus);
	g_renderer.get_device()->GetScissorRect(&oldScissorRect);

	RECT scissorRect{ m_pos.x, m_pos.y, m_pos.x + m_size.x, m_pos.y + m_size.y };
	g_renderer.get_device()->SetScissorRect(&scissorRect);

	m_sprites[BACKTEXTURE_SPRITE].begin(D3DXSPRITE_ALPHABLEND);
	m_sprites[BACKTEXTURE_SPRITE].draw(scissorRect.left, scissorRect.top, c_color(255, 255, 255, 180));
	m_sprites[BACKTEXTURE_SPRITE].end();

	g_renderer.get_device()->SetScissorRect(&scissorRect);
	g_renderer.get_device()->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	for (int i = 0; i < 5; i++) {
		g_renderer.rect(
			m_pos.x + i,
			m_pos.y + i,
			m_size.x - (i * 2) - 1,
			m_size.y - (i * 2) - 1,
			outlColors[i]
		);
	}

	g_renderer.get_device()->SetScissorRect(&oldScissorRect);
	g_renderer.get_device()->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorStatus);

	m_form->draw();
	if (is_block()) m_block->draw();
}

Vec2 c_ui::get_child_draw_pos()
{
	return m_pos + Vec2(15, 15);
}

LRESULT c_ui::handle_dragging(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT hit = DefWindowProcA(wnd, msg, wp, lp);

	if (hit == HTCLIENT) {
		if (m_form->is_mouse_over()) return HTCLIENT;
		return HTCAPTION;
	}

	return hit;
}

bool c_ui::is_key_down(int key)
{
	return m_key_state[key];
}

bool c_ui::is_key_pressed(int key)
{
	return !m_prev_key_state[key] && m_key_state[key];
}

bool c_ui::is_key_released(int key)
{
	return m_prev_key_state[key] && !m_key_state[key];
}

bool c_ui::is_hovered(const Vec2 min, const Vec2 max)
{
	return (m_mouse_pos.x >= min.x && m_mouse_pos.y >= min.y &&
			m_mouse_pos.x <= max.x && m_mouse_pos.y <= max.y);
}

void c_ui::on_reset()
{
	for (int i = 0; i < maxSprites; i++) m_sprites[i].on_reset();
}

void c_ui::on_reset_end()
{
	for (int i = 0; i < maxSprites; i++) m_sprites[i].on_reset_end();
}

#include "sprites.hpp"
c_ui_form* c_ui::create_form(const Vec2 form_size)
{
	m_form = nullptr;
	m_size = form_size;
	
	m_sprites[BACKTEXTURE_SPRITE].init(g_renderer.get_device(), menu_texture_data, sizeof(menu_texture_data), 900, 900);
	m_sprites[CSGO_ICO_SPRITE].init(g_renderer.get_device(), csgo_ico, sizeof(csgo_ico), 24, 24);
	m_sprites[RUST_ICO_SPRITE].init(g_renderer.get_device(), rust_ico, sizeof(rust_ico), 24, 24);

	c_ui_form* form = c_ui_mem_pool::alloc<c_ui_form>();
	form->set_parent(this);

	m_form = form;

	return form;
}

c_ui_groupbox* c_ui::set_group(c_ui_form* form, const std::string& label,
	const Vec2& size, std::function<void(c_ui_groupbox*)> items, bool no_use)
{
	c_ui_groupbox* group = c_ui_mem_pool::alloc<c_ui_groupbox>(label, size, no_use);

	items(group);
	form->add_child(group);

	return group;
}

bool c_ui::is_block(c_ui_element* elem)
{
	if (elem == nullptr) return m_block != nullptr;

	return m_block == elem;
}