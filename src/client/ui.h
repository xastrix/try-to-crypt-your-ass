#pragma once

#include <functional>
#include <xorstr.h>

#include "renderer.h"

#define UI_FORM_WIDTH          535
#define UI_FORM_HEIGHT         350
#define UI_CONNECT_FORM_WIDTH  385
#define UI_CONNECT_FORM_HEIGHT 240

#ifdef _DEBUG
#define DBG_WINDOW_WIDTH       700
#define DBG_WINDOW_HEIGHT      510
#endif

enum UI_INTERFACE_STATE {
	STATE_START_UP,
	STATE_AUTHING,
	STATE_FAILED_CONNECTED,
	STATE_BANNED,
	STATE_HWID_ERR,
	STATE_USER_EXPIRES,
	STATE_INJECTING,
	STATE_SESSION_EXPIRED
};

enum UI_SPRITES {
	BACKTEXTURE_SPRITE,
	CSGO_ICO_SPRITE,
	RUST_ICO_SPRITE,
	maxSprites
};

struct game {
	std::string game_name;
	c_sprite* game_icon;
};

class c_ui_element {
public:
	virtual void think() = 0;
	virtual void draw() = 0;

	virtual c_ui_element* get_parent() {
		return m_parent;
	}

	virtual void set_parent(c_ui_element* parent) {
		m_parent = parent;
	}

	virtual bool is_child(c_ui_element* child) {
		for (auto elem : m_childs) {
			if (elem == child) return true;
		}

		return false;
	}

	virtual void add_child(c_ui_element* child) {
		m_childs.push_back(child);
		child->set_parent(this);
	}

	virtual void set_label(const std::string& label) {
		m_label = label;
	}

	virtual std::string get_label() {
		return m_label;
	}

	virtual void set_size(const Vec2& size) {
		m_size = size;
	}

	virtual Vec2 get_size() {
		return m_size;
	}

	virtual void set_pos(const Vec2& pos) {
		m_pos = pos;
	}

	virtual Vec2& get_pos() {
		return m_pos;
	}

	virtual Vec2 get_child_draw_pos() {
		if (m_parent == nullptr) return m_pos;

		return m_parent->get_child_draw_pos() + m_pos;
	}

	virtual bool is_mouse_over() {
		for (auto* child : m_childs) {
			if (child->is_mouse_over()) return true;
		}

		return false;
	}

protected:
	Vec2 m_pos;
	Vec2 m_size;

	std::string m_label;
	std::vector<c_ui_element*> m_childs;
	
	c_ui_element* m_parent;
};

class c_ui_form : public c_ui_element {
public:
	c_ui_form() {}

	void think() override {
		for (auto child : m_childs) child->think();
	}

	void draw() override {
		for (auto child : m_childs) child->draw();
	}

	void add_child(c_ui_element* child) override {
		c_ui_element::add_child(child);

		m_cursor_pos.y += child->get_size().y + 16;

		if (m_cursor_pos.y - 16 >= (m_parent->get_size().y - 32)) {
			m_cursor_pos.x += child->get_size().x + 16;
			m_cursor_pos.y = child->get_size().y + 16;
		}

		child->set_pos(m_cursor_pos - Vec2(0, child->get_size().y + 16));
	}

	Vec2 get_child_draw_pos() override {
		return c_ui_element::get_child_draw_pos();
	}

	Vec2 get_cursor_pos() {
		return m_cursor_pos;
	}

	void set_cursor_pos(const Vec2& pos) {
		m_cursor_pos = pos;
	}

private:
	Vec2 m_cursor_pos;
};

class c_ui_groupbox : public c_ui_element {
public:
	c_ui_groupbox(const std::string& label, const Vec2& size, bool no_use) {
		m_label = label;
		m_size = size;
		m_no_use = no_use;
	}

	void think() override;
	void draw() override;

	void add_child(c_ui_element* child) override {
		c_ui_element::add_child(child);

		m_cursor_pos.y += child->get_size().y + 15;
		child->set_pos(m_cursor_pos - Vec2(0, child->get_size().y + 15));
	}

	Vec2 get_child_draw_pos() override {
		return m_parent->get_child_draw_pos() + m_pos;
	}

	Vec2 get_cursor_pos() {
		return m_cursor_pos;
	}

	void set_cursor_pos(const Vec2& pos) {
		m_cursor_pos = pos;
	}

	void push_cursor_pos(const Vec2& pos) {
		m_cursor_pos += pos;
	}

	void pop_cursor_pos(const Vec2& pos) {
		m_cursor_pos -= pos;
	}

private:
	Vec2 m_cursor_pos;
	bool m_no_use;
};

class c_ui_button : public c_ui_element {
public:
	c_ui_button(const std::string& label, std::function<void()> callback, const Vec2& size) {
		m_label = label;
		m_size = size;
		m_callback = callback;
		m_hold = false;
	}

	void think() override;
	void draw() override;

	bool is_mouse_over() override;

private:
	std::function<void()> m_callback;
	bool m_hold;
};

class c_ui_listbox : public c_ui_element {
public:
	c_ui_listbox(int* index, std::vector<std::string>* items, const Vec2& size, int height) {
		m_size = size;
		m_index = index;
		m_height = height;
		m_scroll_offset = 0;
		m_items = items;
	}

	void think() override;
	void draw() override;

	bool is_mouse_over() override;

private:
	int* m_index;
	int m_height;
	int m_scroll_offset;
	std::vector<std::string>* m_items;
};

class c_ui_gameselector : public c_ui_element {
public:
	c_ui_gameselector(int* index, const std::vector<game>& games, int width, int height) {
		m_size = Vec2(width, 32);
		m_index = index;
		m_height = height;
		m_scroll_offset = 0;
		m_games = games;
	}

	void think() override;
	void draw() override;

	bool is_mouse_over() override;

private:
	int* m_index;
	int m_height;
	int m_scroll_offset;
	std::vector<game> m_games;
};

class c_ui_textinput : public c_ui_element {
public:
	c_ui_textinput(std::string* text, const std::string& help_mark_text, bool password, int width) {
		m_size = Vec2(width, 24);
		m_text = text;
		m_help_mark = help_mark_text;
		m_password = password;
		m_cursor_pos = m_text->length();
		m_selected_all = false;
		m_ctx_open = false;

		m_deps.push_back({ __("Copy"), &c_ui_textinput::do_copy });
		m_deps.push_back({ __("Clear"), &c_ui_textinput::do_clear });
	}

	void think() override;
	void draw() override;

	bool is_mouse_over() override;

private:
	static void do_copy(c_ui_textinput* self);
	static void do_clear(c_ui_textinput* self);

private:
	std::string* m_text;
	std::string m_help_mark;
	bool m_password;
	size_t m_cursor_pos;
	bool m_selected_all;
	bool m_ctx_open;
	Vec2 m_ctx_rect_pos;
	std::vector<std::pair<std::string, std::function<void(c_ui_textinput*)>>> m_deps;
};

class c_ui_label : public c_ui_element {
public:
	enum UILABEL_FLAGS : uint8_t {
		NONE     = 0,
		PULSE    = (1 << 0),
		CENTER_X = (1 << 1),
		CENTER_Y = (1 << 2)
	};

	c_ui_label(const std::string& label, RENDERER_FONT_LIST font_index, c_color color, uint8_t flags) {
		m_label = label;
		m_font_index = font_index;
		m_color = color;
		m_flags = flags;
	}

	void think() override {}
	void draw() override {
		Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;
		c_color revColor = m_color;

		if (m_flags & CENTER_X) {
			float textWidth = g_font[m_font_index].get_text_width(m_label);
			drawPos.x = m_parent->get_child_draw_pos().x + (m_parent->get_size().x * 0.5f) - (textWidth * 0.5f);
		}

		if (m_flags & CENTER_Y) {
			float textHeight = g_font[m_font_index].get_text_height(m_label);
			drawPos.y = m_parent->get_child_draw_pos().y + (m_parent->get_size().y * 0.5f) - (textHeight * 0.5f);
		}

		if (m_flags & PULSE) {
			float pulse = std::sin(GetTickCount64() * 0.003f) * 0.4f + 0.6f; // 0.5f + 0.5f
			revColor._a = pulse * 255.0f;
		}

		g_font[m_font_index].draw(m_label, drawPos.x, drawPos.y, TEXT_OUTLINE, revColor);
	}

private:
	RENDERER_FONT_LIST m_font_index;
	c_color m_color;
	uint8_t m_flags;
};

class c_ui_mem_pool {
private:
	static constexpr size_t MAX_ELEMENTS = 512;
	static constexpr size_t MAX_ELEMENT_SIZE = 256;

public:
	template <typename T, typename... Args>
	static T* alloc(Args&&... args) {
		if (m_index >= MAX_ELEMENTS)
			return nullptr;

		void* memory_address = m_pool[m_index];
		T* object = ::new (memory_address) T(std::forward<Args>(args)...);

		m_elements[m_index] = object;
		m_index++;

		return object;
	}

	static void release() {
		for (int i = 0; i < m_index; i++) {
			if (m_elements[i]) {
				m_elements[i]->~c_ui_element();
				m_elements[i] = nullptr;
			}
		}

		m_index = 0;
	}

private:
	inline static uint8_t       m_pool[MAX_ELEMENTS][MAX_ELEMENT_SIZE];
	inline static c_ui_element* m_elements[MAX_ELEMENTS];
	inline static int           m_index = 0;
};

class c_ui : public c_ui_element {
public:
	void init(UI_INTERFACE_STATE state, const Vec2 form_size);
	void uninit();

	void think() override;
	void draw() override;

	Vec2 get_child_draw_pos() override;

	void poll_input(HWND window);
	LRESULT handle_dragging(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	bool is_key_down(int key);
	bool is_key_pressed(int key);
	bool is_key_released(int key);

	bool is_hovered(const Vec2 min, const Vec2 max);

	void on_reset();
	void on_reset_end();

	c_ui_form* create_form(const Vec2 form_size);
	c_ui_groupbox* set_group(c_ui_form* form, const std::string& label,
		const Vec2& size, std::function<void(c_ui_groupbox*)> items, bool no_use);

	bool is_block(c_ui_element* elem = nullptr);

	void set_block(c_ui_element* elem) { m_block = elem; }
	c_ui_element* get_block() { return m_block; }

	Vec2 get_mouse_pos() { return m_mouse_pos; }
	Vec2 get_prev_mouse_pos() { return m_prev_mouse_pos; }

	void set_mouse_wheel(int value) { m_mouse_wheel = value; }

	int get_mouse_wheel() { return m_mouse_wheel; }

	bool m_key_state[256];
	bool m_prev_key_state[256];

private:
	bool m_active_window = false;
	int m_mouse_wheel = 0;

#ifdef _DEBUG
	bool m_dragging = false;
#endif

	Vec2 m_prev_mouse_pos;
	Vec2 m_mouse_pos;

	c_ui_element* m_block = nullptr;
	c_ui_form*    m_form;
	
	c_sprite m_sprites[4];
};

inline c_ui g_ui;