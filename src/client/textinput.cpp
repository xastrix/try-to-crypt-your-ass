#include "ui.h"
#include "util.h"

#include <cctype>

void c_ui_textinput::think()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	Vec2 itemMin(drawPos);
	Vec2 itemMax(itemMin + Vec2(m_size.x, m_size.y));

	if (m_ctx_open)
	{
		for (int i = 0; i < m_deps.size(); i++)
		{
			Vec2 ctxMin(m_ctx_rect_pos + Vec2(1, 1 + (i * 20)));
			Vec2 maxMax(60, 20);

			if (g_ui.is_key_pressed(VK_LBUTTON))
			{
				if (g_ui.is_hovered(ctxMin, ctxMin + maxMax))
					m_deps[i].second(this);

				m_ctx_open = false;
			}
		}
	}
	else
	{
		if (g_ui.is_key_pressed(VK_LBUTTON))
		{
			if (is_mouse_over())
			{
				if (!g_ui.is_block(this)) {
					if (m_cursor_pos >= 0) m_cursor_pos = m_text->length();

					g_ui.set_block(this);
				}
			}
			else
			{
				if (g_ui.is_block(this)) {
					g_ui.set_block(nullptr);

					m_selected_all = false;
				}
			}
		}
		else if (g_ui.is_key_pressed(VK_RBUTTON))
		{
			if (is_mouse_over() && g_ui.is_block(this))
			{
				if (!m_ctx_open) {
					Vec2 mousePos = g_ui.get_mouse_pos();

					m_ctx_rect_pos.x = std::max(itemMin.x, std::min(mousePos.x, itemMax.x));
					m_ctx_rect_pos.y = std::max(itemMin.y, std::min(mousePos.y, itemMax.y));

					m_ctx_open = true;
				}
			}
			else
			{
				if (m_ctx_open) {
					g_ui.set_block(nullptr);
					m_ctx_open = false;
				}
			}
		}
	}

	if (!g_ui.is_block(this) || !m_text || m_ctx_open) return;

	bool isShiftPressed = g_ui.is_key_down(VK_SHIFT);
	bool isControlPressed = g_ui.is_key_down(VK_CONTROL);

	for (int k = 0; k < 256; k++)
	{
		bool isCurrentlyPressed = g_ui.is_key_pressed(k);

		if (isCurrentlyPressed && !g_ui.m_prev_key_state[k])
		{
			if (isControlPressed)
			{
				if (k == 'A')
				{
					m_selected_all = true;
					m_cursor_pos = m_text->length();

					g_ui.m_prev_key_state[k] = isCurrentlyPressed;

					continue;
				}
				else if (k == 'C')
				{
					if (m_selected_all && !m_text->empty())
						util::copy_to_clipboard(*m_text);

					g_ui.m_prev_key_state[k] = isCurrentlyPressed;

					continue;
				}
				else if (k == 'V')
				{
					std::string clipData = util::get_data_from_clipboard();

					if (!clipData.empty())
					{
						if (m_selected_all)
						{
							*m_text = clipData;
							m_cursor_pos = m_text->length();
							m_selected_all = false;
						}
						else
						{
							m_text->insert(m_cursor_pos, clipData);
							m_cursor_pos += clipData.length();
						}
					}

					g_ui.m_prev_key_state[k] = isCurrentlyPressed;

					continue;
				}
			}

			if (k == VK_LEFT || k == VK_RIGHT) m_selected_all = false;

			if (k == VK_LEFT)
			{
				if (m_cursor_pos > 0) m_cursor_pos--;
			}
			else if (k == VK_RIGHT)
			{
				if (m_cursor_pos < m_text->length()) m_cursor_pos++;
			}
			else if (k == VK_BACK)
			{
				if (m_selected_all) {
					m_text->clear();
					m_cursor_pos = 0;

					m_selected_all = false;
				}
				else if (!m_text->empty() && m_cursor_pos > 0) {
					m_text->erase(m_cursor_pos - 1, 1);
					m_cursor_pos--;
				}
			}
			else if (k == VK_SPACE)
			{
				if (m_selected_all)
					m_selected_all = false;

				m_text->insert(m_cursor_pos, 1, ' ');
				m_cursor_pos++;
			}
			else
			{
				char c = 0;

				if (k >= 'A' && k <= 'Z') {
					c = (isShiftPressed || (GetKeyState(VK_CAPITAL) & 0x0001) != 0) ? k : std::tolower(k);
				}
				else if (k >= '0' && k <= '9')
				{
					if (isShiftPressed) {
						const char shiftNumbers[] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };
						c = shiftNumbers[k - '0'];
					}
					else {
						c = k;
					}
				}

				if (c != 0)
				{
					if (m_selected_all) {
						m_text->clear();
						m_cursor_pos = 0;

						m_selected_all = false;
					}

					m_text->insert(m_cursor_pos, 1, c);
					m_cursor_pos++;
				}
			}
		}

		g_ui.m_prev_key_state[k] = isCurrentlyPressed;
	}
}

void c_ui_textinput::draw()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	Vec2 itemMin(drawPos);
	Vec2 itemMax(itemMin + Vec2(m_size.x, m_size.y));

	g_renderer.rect(drawPos.x, drawPos.y - 1, m_size.x, m_size.y - 1, c_color(10, 10, 10));
	g_renderer.rect_fill(drawPos.x + 1, drawPos.y, m_size.x - 1, m_size.y - 2, c_color(35, 35, 35));

	bool showHelpMark = (!m_text || m_text->empty()) && !m_help_mark.empty();

	std::string renderText = m_text ? *m_text : "";
	if (m_password && !showHelpMark) renderText = std::string(renderText.length(), '*');

	float textWithMarkHeight = g_font[Tahoma12px].get_text_height(showHelpMark ? m_help_mark : renderText);

	float paddingX = 5.0f;
	float maxTextWidth = m_size.x - (paddingX * 2.0f);

	size_t startIndex = 0;
	for (int i = static_cast<int>(m_cursor_pos) - 1; i >= 0; i--) {
		std::string testStr = renderText.substr(i, m_cursor_pos - i);
		if (g_font[Tahoma12px].get_text_width(testStr) > maxTextWidth) {
			startIndex = i + 1;
			break;
		}
	}

	std::string visibleText = renderText.substr(startIndex);
	float textWidth = g_font[Tahoma12px].get_text_width(visibleText);
	while (!visibleText.empty() && textWidth > maxTextWidth) visibleText.pop_back();

	Vec2 textPos(
		drawPos.x + paddingX,
		drawPos.y + (m_size.y / 2 - textWithMarkHeight / 2) - 2
	);

	if (showHelpMark) {
		g_font[Tahoma12px].draw(m_help_mark, textPos.x, textPos.y, TEXT_OUTLINE, c_color(100, 100, 100));
	}
	else {
		g_font[Tahoma12px].draw(visibleText, textPos.x, textPos.y, TEXT_OUTLINE, c_color(190, 190, 190));
	}

	if (g_ui.is_block(this) && (GetTickCount64() / 500) % 2 == 0)
	{
		size_t relativeCursorPos = m_cursor_pos - startIndex;
		std::string textBeforeCursor = visibleText.substr(0, relativeCursorPos);

		if (!textBeforeCursor.empty() && textBeforeCursor.back() == ' ')
			textBeforeCursor.back() = '.';

		g_renderer.rect(textPos.x + g_font[Tahoma12px].get_text_width(textBeforeCursor),
			textPos.y, 0.5f, textWithMarkHeight, c_color(153, 195, 255, 230));
	}

	if (m_selected_all)
	{
		g_renderer.rect_fill(textPos.x - 1, drawPos.y + 4,
			textWidth + 1, m_size.y - textWithMarkHeight + 1, c_color(153, 195, 255, 50));
	}

	if (m_ctx_open)
	{
		g_renderer.rect(m_ctx_rect_pos, Vec2(61, (m_deps.size() * 20) + 1), c_color(10, 10, 10));

		for (int i = 0; i < m_deps.size(); i++)
		{
			Vec2 ctxMin(m_ctx_rect_pos + Vec2(1, 1 + (i * 20)));
			Vec2 ctxMax(60, 20);

			bool bIsHovered = g_ui.is_hovered(ctxMin, ctxMin + ctxMax);

			g_renderer.rect_fill(ctxMin, ctxMax, bIsHovered ? c_color(60, 60, 60) : c_color(44, 44, 44));

			g_font[Tahoma12px].draw(m_deps[i].first, ctxMin.x + 10, ctxMin.y + 3,
				TEXT_OUTLINE, bIsHovered ? c_color(153, 195, 255) : c_color(200, 200, 200));
		}
	}
}

bool c_ui_textinput::is_mouse_over()
{
	Vec2 drawPos = m_parent->get_child_draw_pos() + m_pos;

	Vec2 itemMin(drawPos);
	Vec2 itemMax(itemMin + Vec2(m_size.x, m_size.y));

	return g_ui.is_hovered(itemMin, itemMax);
}

void c_ui_textinput::do_copy(c_ui_textinput* self)
{
	if (!self->m_text->empty()) util::copy_to_clipboard(*self->m_text);
}

void c_ui_textinput::do_clear(c_ui_textinput* self)
{
	self->m_selected_all = false;

	self->m_text->clear();
	self->m_cursor_pos = 0;
}