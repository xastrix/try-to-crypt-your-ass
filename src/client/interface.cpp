#include "interface.h"

INTERFACE_STATUS c_interface::init(int argc, const char** argv)
{
	int i = 0;
	int len = (util::_rand() % 16) + 16;

	DO_IF_DEBUG(
		printf("[CLIENT] Initializing interface...\n");
	);

	for (; i < len; i++) {
		m_window_name[i] = (util::_rand() % 26) + 'a';
		m_class_name[i] = (util::_rand() % 26) + 'a';
	}

	m_window_name[i] = '\0';
	m_class_name[i] = '\0';

	/* Just for testing arguments */
	for (int a = 1; a < argc; a++) {
		if (strcmp(argv[a], __("--test_arg")) == 0) { /*...*/ }
	}

	if (!init_window()) return IFACE_FAILED;
	if (!init_d3d()) return IFACE_FAILED;

	return IFACE_SUCCESS;
}

void c_interface::uninit()
{
	g_ui.uninit();
	g_font.uninit();
	g_renderer.uninit();

	if (m_device) {
		m_device->Release();
		m_device = nullptr;
	}

	if (m_d3d9) {
		m_d3d9->Release();
		m_d3d9 = nullptr;
	}

	if (m_window) {
		DestroyWindow(m_window);
		m_window = nullptr;
	}

	UnregisterClassA(m_class_name, m_wc.hInstance);
}

bool c_interface::init_window()
{
	memset(&m_wc, 0, sizeof(m_wc));
	m_wc.cbSize = sizeof(WNDCLASSEX);
	m_wc.lpfnWndProc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT __stdcall {
		DO_IF_DEBUG(
			if (m == WM_KEYDOWN || m == WM_SYSKEYDOWN)
				printf("[CLIENT] TRACE KEY PRESSING: VK_CODE = 0x%X (%lld)\n", (UINT)w, (long long)w);

			else if (m == WM_LBUTTONDOWN || m == WM_RBUTTONDOWN || m == WM_MBUTTONDOWN) {
				int x = (int)(short)LOWORD(l);
				int y = (int)(short)HIWORD(l);

				printf("[CLIENT] TRACE MOUSE PRESSING: %s BUTTON at X: %d, Y: %d\n",
					(m == WM_LBUTTONDOWN) ? "LEFT" : (m == WM_RBUTTONDOWN) ? "RIGHT" : "MIDDLE", x, y);
			}
		);

		switch (m) {
		case WM_SYSCOMMAND: {
			if ((w & 0xfff0) == SC_KEYMENU) return 0;
			break;
		}
#ifndef _DEBUG
		case WM_NCHITTEST: {
			return g_ui.handle_dragging(h, m, w, l);
		}
#endif
		case WM_MOUSEWHEEL: {
			int delta = GET_WHEEL_DELTA_WPARAM(w);

			if (delta > 0) g_ui.set_mouse_wheel(1337);
			else if (delta < 0) g_ui.set_mouse_wheel(-1337);

			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
		}

		return DefWindowProcA(h, m, w, l);
	};
	m_wc.cbClsExtra = 0;
	m_wc.cbWndExtra = 0;
	m_wc.hCursor = LoadCursorA(0, IDC_ARROW);
	m_wc.hInstance = reinterpret_cast<HINSTANCE>(util::get_module_handle(nullptr));
	m_wc.lpszMenuName = 0;
	m_wc.lpszClassName = m_class_name;

	RegisterClassExA(&m_wc);

	m_width = UI_CONNECT_FORM_WIDTH;
	m_height = UI_CONNECT_FORM_HEIGHT;

#ifndef _DEBUG
	m_window = CreateWindowExA(0, m_class_name, m_window_name, WS_POPUP | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height, nullptr, nullptr, nullptr, 0);
#else
	RECT wr = { 0, 0, DBG_WINDOW_WIDTH, DBG_WINDOW_HEIGHT };
	DWORD window_style = { WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX };

	AdjustWindowRect(&wr, window_style, FALSE);

	m_window = CreateWindowExA(0, m_class_name, m_window_name, window_style,
		CW_USEDEFAULT, CW_USEDEFAULT, DBG_WINDOW_WIDTH, DBG_WINDOW_HEIGHT, nullptr, nullptr, nullptr, 0);
#endif

	if (!m_window) {
		UnregisterClassA(m_class_name, m_wc.hInstance);
		return false;
	}

#ifndef _DEBUG
	MoveWindow(m_window, (GetSystemMetrics(SM_CXSCREEN) - m_width) / 2,
		                 (GetSystemMetrics(SM_CYSCREEN) - m_height) / 2, m_width, m_height, TRUE);
#else
	MoveWindow(m_window, (GetSystemMetrics(SM_CXSCREEN) - DBG_WINDOW_WIDTH) / 2,
		                 (GetSystemMetrics(SM_CYSCREEN) - DBG_WINDOW_HEIGHT) / 2, wr.right - wr.left, wr.bottom - wr.top, TRUE);
#endif

	ShowWindow(m_window, SW_SHOWDEFAULT);
	UpdateWindow(m_window);

	return true;
}

bool c_interface::init_d3d()
{
	if (!(m_d3d9 = Direct3DCreate9(D3D_SDK_VERSION))) return false;

	memset(&m_present_params, 0, sizeof(m_present_params));
	m_present_params.Windowed = TRUE;
	m_present_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_present_params.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_present_params.hDeviceWindow = m_window;
	m_present_params.EnableAutoDepthStencil = TRUE;
	m_present_params.AutoDepthStencilFormat = D3DFMT_D16;
	m_present_params.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_present_params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	m_present_params.BackBufferCount = 1;
#ifndef _DEBUG
	m_present_params.BackBufferWidth = m_width;
	m_present_params.BackBufferHeight = m_height;
#else
	m_present_params.BackBufferWidth = DBG_WINDOW_WIDTH;
	m_present_params.BackBufferHeight = DBG_WINDOW_HEIGHT;
#endif

	if (FAILED(m_d3d9->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		m_window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&m_present_params, &m_device))) goto relD3D;

	g_font.init(m_device);
	g_renderer.init(m_device);

#ifdef _DEBUG
	int centX = (DBG_WINDOW_WIDTH - m_width) / 2;
	int centY = (DBG_WINDOW_HEIGHT - m_height) / 2;

	g_ui.set_pos(Vec2(centX, centY));
#endif

	g_ui.init(STATE_START_UP, Vec2(m_width, m_height));

	return true;

relD3D:
	if (m_d3d9) {
		m_d3d9->Release();
		m_d3d9 = nullptr;
	}

	return false;
}

void c_interface::loop()
{
	MSG msg;

	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT) {
			g_net.set_conn_state(HCONN_EXIT_SERVICE);
			return;
		}

		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	g_ui.poll_input(m_window);

	switch (g_net.get_conn_state()) {
	case HCONN_AUTHING: {
		change_state(STATE_AUTHING);

		g_net.auth(g::fields[FT_USERNAME], g::fields[FT_PASSWORD]);

		DO_IF_DEBUG(
			printf("[USER] Interface State -> AUTHING\n");
		);

		g_net.set_conn_state(HCONN_AUTHING_END);

		break;
	}
	case HCONN_FAILED: {
		change_state(STATE_FAILED_CONNECTED);

		DO_IF_DEBUG(
			printf("[USER] Interface State -> FAILED_CONNECTED\n");
		);

		g_net.set_conn_state(HCONN_FAILED_END);

		break;
	}
	case HCONN_BANNED: {
		change_state(STATE_BANNED);

		DO_IF_DEBUG(
			printf("[USER] Interface State -> BANNED\n");
		);

		g_net.set_conn_state(HCONN_BANNED_END);

		break;
	}
	case HCONN_HWID_ERR: {
		change_state(STATE_HWID_ERR);

		DO_IF_DEBUG(
			printf("[USER] Interface State -> HWID_ERR\n");
		);

		g_net.set_conn_state(HCONN_HWID_ERR_END);

		break;
	}
	case HCONN_USER_EXPIRES: {
		change_state(STATE_USER_EXPIRES);

		DO_IF_DEBUG(
			printf("[USER] Interface State -> USER_EXPIRES\n");
		);

		g_net.set_conn_state(HCONN_USER_EXPIRES_END);

		break;
	}
	case HCONN_INJECTING: {
		change_state(STATE_INJECTING);

		DO_IF_DEBUG(
			printf("[USER] Interface State -> INJECTING\n");
		);

		g_net.set_conn_state(HCONN_INJECTING_END);

		break;
	}
	case HCONN_EXPIRE_SESSION: {
		change_state(STATE_SESSION_EXPIRED);

		DO_IF_DEBUG(
			printf("[USER] Interface State -> SESSION_EXPIRED\n");
		);

		g_net.set_conn_state(HCONN_EXPIRE_SESSION_END);

		break;
	}
	}

	m_device->Clear(0, 0, D3DCLEAR_TARGET, c_color(0, 0, 0).get_d3d(), 1.0f, 0);

	if (SUCCEEDED(m_device->BeginScene()))
	{
		g_renderer.begin();
		{
			g_ui.think();
			g_ui.draw();
		}
		g_renderer.end();

		m_device->EndScene();
	}

	HRESULT hr = m_device->Present(0, 0, 0, 0);

	if (hr == D3DERR_DEVICELOST && m_device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		on_reset();
}

void c_interface::change_state(UI_INTERFACE_STATE state)
{
	switch (state) {
	case STATE_START_UP:
	case STATE_AUTHING:
	case STATE_FAILED_CONNECTED:
	case STATE_BANNED:
	case STATE_INJECTING:
	case STATE_SESSION_EXPIRED: {
		m_width = UI_CONNECT_FORM_WIDTH;
		m_height = UI_CONNECT_FORM_HEIGHT;

		break;
	}
	case STATE_USER_EXPIRES: {
		m_width = UI_FORM_WIDTH;
		m_height = UI_FORM_HEIGHT;

		break;
	}
	}

#ifndef _DEBUG
	m_present_params.BackBufferWidth = m_width;
	m_present_params.BackBufferHeight = m_height;
#else
	m_present_params.BackBufferWidth = DBG_WINDOW_WIDTH;
	m_present_params.BackBufferHeight = DBG_WINDOW_HEIGHT;
#endif

	on_reset();

	c_ui_mem_pool::release();

#ifndef _DEBUG
	RECT rcOld;
	GetWindowRect(m_window, &rcOld);

	int currentCenterX = rcOld.left + (rcOld.right - rcOld.left) / 2;
	int currentCenterY = rcOld.top + (rcOld.bottom - rcOld.top) / 2;

	int newX = std::clamp(currentCenterX - (m_width / 2), 0, GetSystemMetrics(SM_CXSCREEN) - m_width);
	int newY = std::clamp(currentCenterY - (m_height / 2), 0, GetSystemMetrics(SM_CYSCREEN) - m_height);

	g_ui.init(state, Vec2(m_width, m_height));
	MoveWindow(m_window, newX, newY, m_width, m_height, TRUE);
#else
	int centX = (DBG_WINDOW_WIDTH - m_width) / 2;
	int centY = (DBG_WINDOW_HEIGHT - m_height) / 2;

	g_ui.set_pos(Vec2(centX, centY));
	g_ui.init(state, Vec2(m_width, m_height));
#endif
}

void c_interface::on_reset()
{
	g_ui.on_reset();

	g_font.uninit();
	g_renderer.uninit();

	HRESULT hr = m_device->Reset(&m_present_params);

	if (hr == D3D_OK)
	{
		g_renderer.init(m_device);
		g_font.init(m_device);

		g_ui.on_reset_end();
	}
}