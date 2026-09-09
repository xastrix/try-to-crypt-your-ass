#pragma once

#include <vector>
#include <time.h>
#include <d3d9.h>

#include "globals.h"
#include "ui.h"
#include "net.h"
#include "util.h"

enum INTERFACE_STATUS {
	IFACE_FAILED = -1,
	IFACE_SUCCESS
};

class c_interface {
public:
	INTERFACE_STATUS init(int argc, const char** argv);
	void uninit();

	void loop();
	void on_reset();

private:
	bool init_window();
	bool init_d3d();
	void change_state(UI_INTERFACE_STATE state);

private:
	WNDCLASSEX m_wc;
	HWND       m_window;

	LPDIRECT3D9           m_d3d9;
	LPDIRECT3DDEVICE9     m_device;
	D3DPRESENT_PARAMETERS m_present_params;

	char m_window_name[32];
	char m_class_name[32];

	int m_width{};
	int m_height{};
};

inline c_interface g_interface;