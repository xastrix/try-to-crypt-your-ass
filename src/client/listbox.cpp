#include "ui.h"

void c_ui_listbox::think()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;
	Vec2 contentMax(m_size.x, m_size.y + ((m_items->size() - 1) * m_size.y + 2));

	if (is_mouse_over() && !g_ui.is_block(this))
	{
		if (g_ui.is_key_pressed(VK_LBUTTON))
			g_ui.set_block(this);
	}
	else if (g_ui.is_block(this))
	{
		if (g_ui.is_key_released(VK_LBUTTON))
			g_ui.set_block(nullptr);
	}

	if (is_mouse_over())
	{
		int wheelDelta = g_ui.get_mouse_wheel();

		if (wheelDelta != 0)
		{
			m_scroll_offset -= wheelDelta * m_size.y;

			if (m_scroll_offset < 0) m_scroll_offset = 0;

			int max_scroll = (m_items->size() * m_size.y + 2) - m_height;
			if (max_scroll < 0) max_scroll = 0;

			if (m_scroll_offset > max_scroll) m_scroll_offset = max_scroll;
		}

		if (g_ui.is_key_pressed(VK_LBUTTON))
		{
			for (int i = 0; i < m_items->size(); i++) {
				Vec2 itemSelectedMin(drawPos.x + 1, drawPos.y + (i * m_size.y) - m_scroll_offset);
				Vec2 itemSelectedMax(m_size.x - 1, m_size.y);

				if (itemSelectedMin.y + itemSelectedMax.y < drawPos.y || itemSelectedMin.y > drawPos.y + m_height)
					continue;

				if (g_ui.is_hovered(itemSelectedMin, itemSelectedMin + itemSelectedMax)) {
					*m_index = i;
					break;
				}
			}
		}
	}
}

void c_ui_listbox::draw()
{
	RECT oldScissorRect;
	DWORD scissorStatus = 0;

	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;
	Vec2 contentMax(m_size.x, m_size.y + ((m_items->size() - 1) * m_size.y + 2));

	g_renderer.rect(drawPos.x, drawPos.y - 1, contentMax.x, m_height - 1, c_color(10, 10, 10));
	g_renderer.rect_fill(drawPos.x + 1, drawPos.y + 1, contentMax.x - 1, m_height - 3, c_color(44, 44, 44));

	g_renderer.get_device()->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorStatus);
	g_renderer.get_device()->GetScissorRect(&oldScissorRect);

	RECT scissorRect{ drawPos.x, drawPos.y, drawPos.x + contentMax.x, drawPos.y + m_height - 2 };

	g_renderer.get_device()->SetScissorRect(&scissorRect);
	g_renderer.get_device()->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	for (int i = 0; i < m_items->size(); i++) {
		Vec2 itemSelectedMin(drawPos.x + 1, drawPos.y + (i * m_size.y) - m_scroll_offset);
		Vec2 itemSelectedMax(m_size.x - 1, m_size.y);

		bool isHovered = g_ui.is_hovered(itemSelectedMin, itemSelectedMin + itemSelectedMax);
		bool isSelected = *m_index == i;

		g_renderer.rect_fill(itemSelectedMin, itemSelectedMax,
			(isHovered || isSelected) ? c_color(60, 60, 60) : c_color(44, 44, 44));

		g_font[Tahoma12px].draw((*m_items)[i], itemSelectedMin.x + 9, itemSelectedMin.y + 3,
			TEXT_OUTLINE, isSelected ? c_color(153, 195, 255) : c_color(200, 200, 200));
	}

	g_renderer.get_device()->SetScissorRect(&oldScissorRect);
	g_renderer.get_device()->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorStatus);
}

bool c_ui_listbox::is_mouse_over()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;
	Vec2 contentMax(m_size.x, m_size.y + ((m_items->size() - 1) * m_size.y + 2));

	return g_ui.is_hovered(drawPos, drawPos + contentMax);
}