#include "security.h"
#include "globals.h"
#include "net.h"

#include <intrin.h>
#include <atomic>
#include <VMProtectSDK.h>
#include <mbedtls/sha256.h>

#define _MUTEX_STR_ "soXlgraBhTIgFXftBsPfxAGYGTJyjF"

using T_NtSetInformationThread = NTSTATUS(NTAPI*)(IN HANDLE, IN ULONG, IN PVOID, IN ULONG);

using T_NtResumeThread = NTSTATUS(NTAPI*)(IN HANDLE, OUT PULONG);

using T_NtWaitForSingleObject = NTSTATUS(NTAPI*)(IN HANDLE, IN BOOLEAN, IN PLARGE_INTEGER);

using T_NtCreateThreadEx = NTSTATUS(NTAPI*)(OUT PHANDLE, IN ACCESS_MASK, IN PVOID, IN HANDLE,
	IN PVOID, IN PVOID, IN ULONG, IN ULONG, IN SIZE_T, IN SIZE_T, IN PVOID);

using T_NtRemoveProcessDebug = NTSTATUS(NTAPI*)(IN HANDLE, IN HANDLE);

using T_RtlGetVersion = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);

using T_NtClose = NTSTATUS(NTAPI*)(IN HANDLE);

static HANDLE g_SecThread = {};
static std::atomic<bool> g_SecThreadLock{ true };

static NTSTATUS(__stdcall *o_NtCreateThreadEx)(PHANDLE, ACCESS_MASK, PVOID, HANDLE,
	PVOID, PVOID, ULONG, ULONG_PTR, SIZE_T, SIZE_T, PVOID);
static NTSTATUS __stdcall _NtCreateThreadEx(PHANDLE, ACCESS_MASK, PVOID, HANDLE,
	PVOID, PVOID, ULONG, ULONG_PTR, SIZE_T, SIZE_T, PVOID);

static DWORD __stdcall sec_thread_routine(LPVOID);

_SECURITY_STATUS c_security::init()
{
	VMProtectBeginUltra("SECURITY_INIT");

#ifndef _DEBUG
	/*
	* Detect debugger
	*/
	if (VMProtectIsDebuggerPresent(TRUE)) {
		VMProtectEnd();
		return SEC_FAILED;
	}

	/*
	* Detect virtual machine
	*/
	if (VMProtectIsVirtualMachinePresent()) {
		VMProtectEnd();
		return SEC_FAILED;
	}

	/*
	* Check windows version
	*/
	if (!verify_windows_version()) {
		util::call_message_box(__("This program only supports Windows 10 or later"), MB_OK | MB_ICONERROR);
		VMProtectEnd();

		return SEC_FAILED;
	}

	/*
	* Initialize MH for API hooking
	*/
	MH_Initialize();

	/*
	* Check if the application is already running
	*/
	if (!mutex_init()) {
		util::call_message_box(__("The program is already running"), MB_OK | MB_ICONWARNING);
		VMProtectEnd();

		return SEC_FAILED;
	}

	/*
	* A small check for a NOPed or removed TLS callback. If util::_srand() wasn't called inside it,
	* we will get these exact numbers (7719/38), signaling that the program has been tampered with
	*/
	int rand1 = util::_rand(), rand2 = util::_rand();
	if (rand1 == 38 && rand2 == 7719) {
		VMProtectEnd();
		return SEC_FAILED;
	}
#endif

	NTSTATUS status = util::get_export<T_NtCreateThreadEx>(__("ntdll.dll"), __("NtCreateThreadEx"))(
		&g_SecThread,
		THREAD_ALL_ACCESS,
		nullptr,
		(HANDLE)-1,
		(PVOID)sec_thread_routine,
		nullptr,
		0x00000001,
		0, 0, 0,
		nullptr
	);

	if (status != 0) {
		VMProtectEnd();
		return SEC_FAILED;
	}

	if (g_SecThread != nullptr) {
		util::get_export<T_NtSetInformationThread>(__("ntdll.dll"), __("NtSetInformationThread"))(
			g_SecThread,
			0x11,
			nullptr,
			0
		);

		ULONG scount = { 0 };
		util::get_export<T_NtResumeThread>(__("ntdll.dll"), __("NtResumeThread"))(g_SecThread, &scount);
	}
	else {
		g_SecThreadLock.store(false);
	}

#ifndef _DEBUG
	/*
	* Windows native security policy -> blocks unsigned DLL injections
	*/
	if (!proc_mitigation_policy_init()) {
		VMProtectEnd();
		return SEC_FAILED;
	}

	/*
	* Hook NtCreateThreadEx & etc
	*/
	apply_api_hooks();
#endif

	/*
	* Generates a unique hwid using cpu and disk serials hashed with sha256
	*/
	if (!hwid_init(g::fields[FT_HWID])) {
		VMProtectEnd();
		return SEC_FAILED;
	}

	VMProtectEnd();

	return SEC_SUCCESS;
}

void c_security::uninit()
{
	VMProtectBeginUltra("SECURITY_UNINIT");

#ifndef _DEBUG
	for (int i = 0; i < 1; i++) m_hooks[i].unhook();
	MH_Uninitialize();
#endif

	g_SecThreadLock.store(false);

	util::get_export<T_NtWaitForSingleObject>(__("ntdll.dll"), __("NtWaitForSingleObject"))(
		g_SecThread,
		FALSE,
		nullptr
	);

	util::get_export<T_NtClose>(__("ntdll.dll"), __("NtClose"))(g_SecThread);

	g_SecThread = nullptr;

	VMProtectEnd();
}

bool c_security::hwid_init(std::string& out_hwid)
{
	std::string cpu_raw = util::get_cpu_raw(), phys, hwid;
	if (!util::get_physical_disk_serial(phys)) return false;

	hwid = __("--") + cpu_raw + __("-") + phys + __("-");

	unsigned char output_hash[32];
	if (mbedtls_sha256((const unsigned char*)hwid.c_str(), hwid.length(), output_hash, 0) != 0) return false;

	out_hwid.clear();
	out_hwid.reserve(64);

	std::string fmt = __("%02x");

	char buf[3];
	for (int i = 0; i < 32; i++) {
		snprintf(buf, sizeof(buf), fmt.c_str(), output_hash[i] & 0xFF);
		out_hwid.append(buf);
	}

	return true;
}

bool c_security::mutex_init()
{
	HANDLE mutex = OpenMutexA(MUTEX_ALL_ACCESS, 0, __(_MUTEX_STR_));

	if (mutex) {
		CloseHandle(mutex);
		return false;
	}

	CreateMutexA(nullptr, FALSE, __(_MUTEX_STR_));

	return true;
}

bool c_security::proc_mitigation_policy_init()
{
	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY bin_sig_policy = { 0 };
	bin_sig_policy.MicrosoftSignedOnly = 1;

	PROCESS_MITIGATION_IMAGE_LOAD_POLICY image_load_policy = { 0 };
	image_load_policy.NoRemoteImages = 1;

	if (!SetProcessMitigationPolicy(ProcessSignaturePolicy,
		&bin_sig_policy, sizeof(bin_sig_policy))) return false;

	if (!SetProcessMitigationPolicy(ProcessImageLoadPolicy,
		&image_load_policy, sizeof(image_load_policy))) return false;

	return true;
}

bool c_security::verify_windows_version()
{
	RTL_OSVERSIONINFOW os_info = { 0 };
	os_info.dwOSVersionInfoSize = sizeof(os_info);

	if (util::get_export<T_RtlGetVersion>(__("ntdll.dll"), __("RtlGetVersion"))(&os_info) == 0)
		return (os_info.dwMajorVersion >= 10);

	return false;
}

void c_security::apply_api_hooks()
{
	m_hooks[0].hook(__(L"ntdll.dll"), __("NtCreateThreadEx"),
		&_NtCreateThreadEx, reinterpret_cast<void**>(&o_NtCreateThreadEx));
}

static NTSTATUS __stdcall _NtCreateThreadEx(
	PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, PVOID ObjectAttributes, HANDLE ProcessHandle,
	PVOID StartRoutine, PVOID Argument, ULONG CreateFlags, ULONG_PTR ZeroBits,
	SIZE_T StackSize, SIZE_T MaximumStackSize, PVOID AttributeList)
{
	DWORD targetProcId = GetProcessId(ProcessHandle);

	if (targetProcId == 0 || targetProcId == GetCurrentProcessId())
	{
		void* callerAddress = _ReturnAddress();
		bool  callerValid   = false;

		MEMORY_BASIC_INFORMATION callerMbi;
		if (VirtualQuery(callerAddress, &callerMbi, sizeof(callerMbi))) {
			callerValid = true;

			if (callerMbi.Type == MEM_PRIVATE ||
				callerMbi.Type == MEM_MAPPED) return 0xC0000022;
		}

		void* pLLA = util::get_export<void*>(__("kernel32.dll"), __("LoadLibraryA"));
		void* pLLW = util::get_export<void*>(__("kernel32.dll"), __("LoadLibraryW"));
		void* pLLEX = util::get_export<void*>(__("kernel32.dll"), __("LoadLibraryExW"));

		if (StartRoutine == pLLA ||
			StartRoutine == pLLW ||
			StartRoutine == pLLEX) return 0xC0000022;

		MEMORY_BASIC_INFORMATION targetMbi;
		if (VirtualQuery(StartRoutine, &targetMbi, sizeof(targetMbi))) {
			if (targetMbi.Type == MEM_PRIVATE || targetMbi.Type == MEM_MAPPED) {
				if (callerValid && callerMbi.Type == MEM_IMAGE) {
					return o_NtCreateThreadEx(
						ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle,
						StartRoutine, Argument, CreateFlags, ZeroBits,
						StackSize, MaximumStackSize, AttributeList
					);
				}

				return 0xC0000022;
			}
		}
	}

	return o_NtCreateThreadEx(
		ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle,
		StartRoutine, Argument, CreateFlags, ZeroBits,
		StackSize, MaximumStackSize, AttributeList
	);
}

static DWORD __stdcall sec_thread_routine(LPVOID)
{
	VMProtectBeginUltra("SECURITY_THREAD_ROUTINE");

	while (g_SecThreadLock.load()) {
		Sleep(250);

#ifndef _DEBUG
		if (VMProtectIsDebuggerPresent(TRUE))
		{
			// Setting this flag triggers a countdown to a security BSOD and system reboot
			if (!g::flags[EMULATE_SYSTEM_ERR]) g::flags[EMULATE_SYSTEM_ERR] = g_net.ban_request();

			// Break debug port
			NTSTATUS status = util::get_export<T_NtRemoveProcessDebug>(__("ntdll.dll"), __("NtRemoveProcessDebug"))(
				(HANDLE)-1, NULL
			);
		}
#endif
	}

	VMProtectEnd();

	return 0;
}