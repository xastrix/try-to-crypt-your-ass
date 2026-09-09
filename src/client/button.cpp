#include "ui.h"

void c_ui_button::think()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	Vec2 itemMin(drawPos);
	Vec2 itemMax(itemMin + Vec2(m_size.x, m_size.y));

	if (is_mouse_over() && g_ui.is_key_pressed(VK_LBUTTON) && !m_hold) {
		g_ui.set_block(this);
		m_hold = true;
	}

	else if (is_mouse_over() && g_ui.is_key_released(VK_LBUTTON) && m_hold) {
		m_callback();

		g_ui.set_block(nullptr);
		m_hold = false;
	}

	else if (!is_mouse_over() && g_ui.is_key_released(VK_LBUTTON) && m_hold) {
		g_ui.set_block(nullptr);
		m_hold = false;
	}
}

void c_ui_button::draw()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	Vec2 itemMin(drawPos);
	Vec2 itemMax(itemMin + Vec2(m_size.x, m_size.y));

	g_renderer.rect(itemMin.x - 1, itemMin.y - 1, m_size.x + 2, m_size.y + 2, c_color(10, 10, 10));
	g_renderer.rect(itemMin.x, itemMin.y, m_size.x, m_size.y, c_color(50, 50, 50));

	if (m_hold)
	{
		g_renderer.gradient_v(itemMin.x + 1, itemMin.y + 1,
			m_size.x - 1, m_size.y - 1, c_color(30, 30, 30), c_color(20, 20, 20));

		g_renderer.rect(itemMin.x + 1, itemMin.y + 1,
			m_size.x - 2, m_size.y - 2, c_color(10, 10, 10));
	}
	else {
		g_renderer.gradient_v(itemMin.x + 1, itemMin.y + 1,
			m_size.x - 1, m_size.y - 1, c_color(35, 35, 35), c_color(25, 25, 25));

		if (is_mouse_over()) g_renderer.rect(itemMin.x + 1, itemMin.y + 1,
			m_size.x - 2, m_size.y - 2, c_color(10, 10, 10));
	}

	Vec2 textSize = Vec2(g_font[VerdanaBold12px].get_text_width(m_label),
		g_font[VerdanaBold12px].get_text_height(m_label));

	Vec2 labelMin(
		drawPos.x + m_size.x / 2 - textSize.x / 2,
		drawPos.y + m_size.y / 2 - textSize.y / 2
	);

	g_font[VerdanaBold12px].draw(m_label, labelMin.x, labelMin.y, TEXT_OUTLINE, c_color(200, 200, 200));
}

bool c_ui_button::is_mouse_over()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	Vec2 itemMin(drawPos);
	Vec2 itemMax(itemMin + Vec2(m_size.x, m_size.y));

	return g_ui.is_hovered(itemMin, itemMax);
}