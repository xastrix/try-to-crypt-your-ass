#include "ui.h"

void c_ui_groupbox::think()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	for (auto child : m_childs)
	{
		child->think();
	}
}

void c_ui_groupbox::draw()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	if (!m_no_use)
		g_renderer.rect_fill(drawPos.x, drawPos.y, m_size.x, m_size.y, c_color(23, 23, 23, 220));

	for (auto child : m_childs)
	{
		if (!g_ui.is_block() || (g_ui.is_block() && !g_ui.is_block(child)))
			child->draw();
	}

	if (!m_no_use)
	{
		const auto textWidth = g_font[VerdanaBold12px].get_text_width(m_label);

		g_renderer.line(drawPos.x + 1, drawPos.y + 1, drawPos.x + 7, drawPos.y + 1, c_color(44, 44, 44));
		g_renderer.line(drawPos.x + textWidth + 16, drawPos.y + 1, drawPos.x + m_size.x - 1, drawPos.y + 1, c_color(44, 44, 44));
		g_renderer.line(drawPos.x + 1, drawPos.y + 1, drawPos.x + 1, drawPos.y + m_size.y - 1, c_color(44, 44, 44));
		g_renderer.line(drawPos.x + m_size.x - 1, drawPos.y + 1, drawPos.x + m_size.x - 1, drawPos.y + m_size.y - 1, c_color(44, 44, 44));
		g_renderer.line(drawPos.x + 1, drawPos.y + m_size.y - 1, drawPos.x + m_size.x - 1, drawPos.y + m_size.y - 1, c_color(44, 44, 44));

		g_renderer.line(drawPos.x, drawPos.y, drawPos.x + 7, drawPos.y, c_color(17, 17, 17));
		g_renderer.line(drawPos.x + textWidth + 16, drawPos.y, drawPos.x + m_size.x, drawPos.y, c_color(17, 17, 17));
		g_renderer.line(drawPos.x, drawPos.y, drawPos.x, drawPos.y + m_size.y, c_color(17, 17, 17));
		g_renderer.line(drawPos.x + m_size.x, drawPos.y, drawPos.x + m_size.x, drawPos.y + m_size.y, c_color(17, 17, 17));
		g_renderer.line(drawPos.x, drawPos.y + m_size.y, drawPos.x + m_size.x, drawPos.y + m_size.y, c_color(17, 17, 17));

		g_font[VerdanaBold12px].draw(m_label, drawPos.x + 12, drawPos.y - 6,
			TEXT_OUTLINE, c_color(200, 200, 200));
	}
}