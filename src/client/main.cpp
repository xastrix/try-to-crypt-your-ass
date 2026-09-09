#include "security.h"
#include "interface.h"

#include <chrono>
#include <VMProtectSDK.h>

#ifndef _DEBUG
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")
#endif

static void NTAPI _TLS_CALLBACK_0(void*, unsigned long reason, void*)
{
	if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
	{
		util::_srand();
	}
}

#pragma section(".CRT$XLB", read)
__declspec(allocate(".CRT$XLB")) PIMAGE_TLS_CALLBACK __TLS__ = _TLS_CALLBACK_0;

int main(int argc, const char** argv)
{
	VMProtectBeginUltra("MAIN_FUNCTION");

	_SECURITY_STATUS sec_status = g_security.init();
	if (sec_status != SEC_SUCCESS) {
		VMProtectEnd();
		return EXIT_FAILURE;
	}

	INTERFACE_STATUS iface_status = g_interface.init(argc, argv);
	if (iface_status != IFACE_SUCCESS) {
		VMProtectEnd();
		return EXIT_FAILURE;
	}

	NETWORK_STATUS net_status = g_net.init();
	if (net_status != NET_SUCCESS) {
		VMProtectEnd();
		return EXIT_FAILURE;
	}

	bool timer_running = false;
	std::chrono::steady_clock::time_point timer_start;

	do {
		g_interface.loop();
		
		if (g::flags[EMULATE_SYSTEM_ERR]) {
			auto now = std::chrono::steady_clock::now();

			if (!timer_running) {
				timer_start = now;
				timer_running = true;
			}
			else {
				auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - timer_start);

				if (elapsed >= std::chrono::seconds{ 5 })
				{
					util::trigger_system_error(SE_SYSTEM_CORRUPTED);
				}
			}
		}

		Sleep(10);
	} while (g_net.get_conn_state() != HCONN_EXIT_SERVICE);

	g_net.uninit();
	g_interface.uninit();
	g_security.uninit();

	VMProtectEnd();

	return EXIT_SUCCESS;
}