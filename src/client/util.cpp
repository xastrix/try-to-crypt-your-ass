#include "util.h"

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <intrin.h>
#include <xorstr.h>
#include <VMProtectSDK.h>

using T_NtRaiseHardError = NTSTATUS(NTAPI*)(IN NTSTATUS, IN ULONG, IN ULONG,
	IN PULONG_PTR, IN ULONG, OUT PULONG);

using T_RtlAdjustPrivilege = NTSTATUS(NTAPI*)(IN ULONG, IN BOOLEAN, IN BOOLEAN, OUT PBOOLEAN);

using T_GetModuleFileNameA = DWORD(WINAPI*)(HMODULE, LPSTR, DWORD);

using T_MessageBoxA = int(WINAPI*)(HWND, LPCSTR, LPCSTR, UINT);

typedef struct _UNICODE_STRING_CUSTOM {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING_CUSTOM;

typedef struct _PEB_LDR_DATA_CUSTOM {
	ULONG Length;
	BOOLEAN Initialized;
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
	BOOLEAN ShutdownInProgress;
	HANDLE ShutdownThreadId;
} PEB_LDR_DATA_CUSTOM, *PPEB_LDR_DATA_CUSTOM;

typedef struct _PEB_CUSTOM {
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	union {
		BOOLEAN BitFields;
		struct {
			BOOLEAN ImageUsesLargePages : 1;
			BOOLEAN IsProtectedProcess : 1;
			BOOLEAN IsImageDynamicallyRelocated : 1;
			BOOLEAN SkipPatchingUser32Forwarders : 1;
			BOOLEAN IsPackagedProcess : 1;
			BOOLEAN IsAppContainer : 1;
			BOOLEAN IsProtectedProcessLight : 1;
			BOOLEAN IsLongPathAwareProcess : 1;
		};
	};
	PVOID Mutant;
	PVOID ImageBaseAddress;
	PPEB_LDR_DATA_CUSTOM Ldr;
} PEB_CUSTOM, *PPEB_CUSTOM;

typedef struct _LDR_DATA_TABLE_ENTRY_CUSTOM {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING_CUSTOM FullDllName;
	UNICODE_STRING_CUSTOM BaseDllName;
} LDR_DATA_TABLE_ENTRY_CUSTOM, *PLDR_DATA_TABLE_ENTRY_CUSTOM;

static __declspec(thread) ULONG_PTR g_TlsSeed = 0;

void util::_srand()
{
	VMProtectBeginUltra("SRAND_FUNCTION");

	g_TlsSeed = GetTickCount();

	VMProtectEnd();
}

int util::_rand()
{
	g_TlsSeed = g_TlsSeed * 214013 + 2531011;
	return static_cast<int>((g_TlsSeed >> 16) & 0x7FFF);
}

std::string util::trim(const std::string& string)
{
	std::string ws = __(" \t\r\n");

	size_t first = string.find_first_not_of(ws);
	if (first == std::string::npos) return "";

	size_t last = string.find_last_not_of(ws);
	return string.substr(first, (last - first + 1));
}

std::string util::format_string(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	int size = vsnprintf(nullptr, 0, format, args);

	va_end(args);

	if (size <= 0) return "";

	std::string res(size, '\0');

	va_start(args, format);
	vsnprintf(&res[0], size + 1, format, args);
	va_end(args);

	return res;
}

int util::parse_datetime_diff(const std::string& string)
{
	std::tm expiry_tm = {};

	if (string.empty()) return 0;

	std::istringstream ss(string);
	ss >> std::get_time(&expiry_tm, __("%Y-%m-%d %H:%M:%S"));

	if (ss.fail()) return 0;

	std::time_t expiry_time = std::mktime(&expiry_tm);

	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);

	double diff_seconds = std::difftime(expiry_time, now_time);
	if (diff_seconds <= 0) return 0;

	return static_cast<int>(diff_seconds / 86400);
}

void util::copy_to_clipboard(const std::string& data)
{
	if (!OpenClipboard(nullptr)) return;

	EmptyClipboard();

	HGLOBAL h = GlobalAlloc(GMEM_FIXED, data.length() + 1);

	if (h) {
		memcpy(h, data.c_str(), data.length() + 1);
		SetClipboardData(CF_TEXT, h);
	}

	CloseClipboard();
}

std::string util::get_data_from_clipboard()
{
	if (!OpenClipboard(nullptr)) return "";

	HANDLE h = GetClipboardData(CF_TEXT);
	if (!h) {
		CloseClipboard();
		return "";
	}

	char* text = static_cast<char*>(GlobalLock(h));
	if (!text) {
		CloseClipboard();
		return "";
	}

	std::string ret(text);

	GlobalUnlock(h);
	CloseClipboard();

	return text;
}

std::string util::get_program_full_name()
{
	char name[MAX_PATH] = {};
	
	DWORD name_sz = get_export<T_GetModuleFileNameA>(__("kernel32.dll"), __("GetModuleFileNameA"))(nullptr, name, MAX_PATH);
	if (name_sz == 0 || name_sz >= MAX_PATH) return {};
	
	std::string full_path(name);

	size_t slash = full_path.find_last_of("\\/");
	return (slash != std::string::npos) ? full_path.substr(slash + 1) : full_path;
}

std::string util::get_cpu_raw()
{
	int reg_s[4] = {};

	__cpuid(reg_s, 0x00000001);

	std::stringstream ss;
	ss << std::hex << reg_s[0] << reg_s[2] << reg_s[3];

	return ss.str();
}

bool util::get_physical_disk_serial(std::string& out_serial)
{
	HANDLE device = CreateFileA(__("\\\\.\\PhysicalDrive0"),
		0,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if (device == INVALID_HANDLE_VALUE) return false;

	STORAGE_PROPERTY_QUERY storage_prop_query;
	ZeroMemory(&storage_prop_query, sizeof(STORAGE_PROPERTY_QUERY));
	storage_prop_query.PropertyId = StorageDeviceProperty;
	storage_prop_query.QueryType = PropertyStandardQuery;

	DWORD bytes_returned = 0;
	STORAGE_DEVICE_DESCRIPTOR dev_descrp_header = { 0 };

	BOOL r = DeviceIoControl(
		device,
		IOCTL_STORAGE_QUERY_PROPERTY,
		&storage_prop_query, sizeof(STORAGE_PROPERTY_QUERY),
		&dev_descrp_header, sizeof(STORAGE_DEVICE_DESCRIPTOR),
		&bytes_returned, NULL
	);

	if (!r) {
		CloseHandle(device);
		return false;
	}

	const DWORD out_buffer_sz = dev_descrp_header.Size;
	std::vector<BYTE> out_buffer(out_buffer_sz, 0);

	r = DeviceIoControl(
		device,
		IOCTL_STORAGE_QUERY_PROPERTY,
		&storage_prop_query, sizeof(STORAGE_PROPERTY_QUERY),
		out_buffer.data(), out_buffer_sz,
		&bytes_returned, NULL
	);

	CloseHandle(device);

	if (!r) return false;

	STORAGE_DEVICE_DESCRIPTOR* dev_descrp = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(out_buffer.data());

	if (dev_descrp->SerialNumberOffset == 0 ||
		dev_descrp->SerialNumberOffset == -1) return false;

	const char* serial_address = reinterpret_cast<const char*>(out_buffer.data() + dev_descrp->SerialNumberOffset);
	const size_t max_len = out_buffer_sz - dev_descrp->SerialNumberOffset;

	std::string raw_serial(serial_address, strnlen(serial_address, max_len));
	out_serial = trim(raw_serial);

	return true;
}

void util::call_message_box(const std::string& msg, uint32_t flags)
{
	VMProtectBeginUltra("MSGBOX_FUNCTION");

	std::string program_name = get_program_full_name();

	size_t dot = program_name.find_last_of('.');
	if (dot != std::string::npos) program_name = program_name.substr(0, dot);

	get_export<T_MessageBoxA>(__("user32.dll"), __("MessageBoxA"))(
		nullptr,
		msg.c_str(),
		program_name.c_str(),
		flags
	);

	VMProtectEnd();
}

void util::trigger_system_error(SYSTEM_ERR err_type)
{
	VMProtectBeginUltra("CALL_SYSTEM_ERROR");

	BOOLEAN enabled;
	ULONG   err_code = 0, resp;

	switch (err_type) {
	case SE_STATUS_RETRY: {
		err_code = 0xC000022D;
		break;
	}
	case SE_ASSERTION_FAILURE: {
		err_code = 0xC0000420;
		break;
	}
	case SE_MEMORY_CORRUPTION: {
		err_code = 0x80000002;
		break;
	}
	case SE_PAGE_FAULT: {
		err_code = 0xC0000006;
		break;
	}
	case SE_SYSTEM_CORRUPTED: {
		err_code = 0xC0000242;
		break;
	}
	case SE_REGISTRY_CORRUPTED: {
		err_code = 0xC000014C;
		break;
	}
	case SE_STACK_OVERFLOW: {
		err_code = 0xC00000FD;
		break;
	}
	case SE_ACCESS_DENIED: {
		err_code = 0xC0000022;
		break;
	}
	case SE_PRIVILEGE_NOT_HELD: {
		err_code = 0xC0000061;
		break;
	}
	default: {
		err_code = 0xC0000001;
		break;
	}
	}

	NTSTATUS status = get_export<T_RtlAdjustPrivilege>(__("ntdll.dll"), __("RtlAdjustPrivilege"))(
		19,
		TRUE,
		FALSE,
		&enabled
	);

	DO_IF_DEBUG(
		printf("[CLIENT] CALL RtlAdjustPrivilege, STATUS: 0x%X\n", status);
	);

	status = get_export<T_NtRaiseHardError>(__("ntdll.dll"), __("NtRaiseHardError"))(
		err_code,
		0, 0,
		NULL, 6,
		&resp
	);

	DO_IF_DEBUG(
		printf("[CLIENT] CALL NtRaiseHardError, STATUS: 0x%X\n", status);
	);

	VMProtectEnd();
}

void* util::get_proc_address(void* mod_base, const char* func_name)
{
	if (!mod_base || !func_name) return nullptr;

	auto* byte_module_base = reinterpret_cast<BYTE*>(mod_base);

	auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(mod_base);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;

	auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(byte_module_base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;

	auto export_dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (export_dir.VirtualAddress == 0 || export_dir.Size == 0) return nullptr;

	auto* exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(byte_module_base + export_dir.VirtualAddress);

	auto* name_array = reinterpret_cast<DWORD*>(byte_module_base + exports->AddressOfNames);
	auto* func_frray = reinterpret_cast<DWORD*>(byte_module_base + exports->AddressOfFunctions);
	auto* ordinal_array = reinterpret_cast<WORD*>(byte_module_base + exports->AddressOfNameOrdinals);

	for (DWORD i = 0; i < exports->NumberOfNames; i++)
	{
		const char* curr_name = reinterpret_cast<const char*>(byte_module_base + name_array[i]);

		size_t j = 0;
		while (curr_name[j] && curr_name[j] == func_name[j]) j++;

		if (curr_name[j] == '\0' && func_name[j] == '\0')
		{
			WORD ordinal = ordinal_array[i];
			if (ordinal >= exports->NumberOfFunctions) return nullptr;

			DWORD funcRVA = func_frray[ordinal];
			if (funcRVA >= export_dir.VirtualAddress && funcRVA < (export_dir.VirtualAddress + export_dir.Size)) return nullptr;

			return reinterpret_cast<void*>(byte_module_base + funcRVA);
		}
	}

	return nullptr;
}

void* util::get_module_handle(const char* mod_name)
{
	auto* peb = reinterpret_cast<PEB_CUSTOM*>(__readfsdword(0x30));

	if (!peb || !peb->Ldr) return nullptr;
	if (!mod_name) return peb->ImageBaseAddress;

	PLIST_ENTRY head = &peb->Ldr->InLoadOrderModuleList;
	PLIST_ENTRY curr = head->Flink;

	while (curr != head)
	{
		auto* entry = reinterpret_cast<PLDR_DATA_TABLE_ENTRY_CUSTOM>(curr);

		if (entry->BaseDllName.Buffer != nullptr)
		{
			const wchar_t* name1 = entry->BaseDllName.Buffer;
			const char* name2 = mod_name;

			size_t i = 0;
			while (name1[i] && name2[i])
			{
				wchar_t c1 = (name1[i] >= L'A' && name1[i] <= L'Z') ? (name1[i] + 32) : name1[i];
				char c2 = (name2[i] >= 'A' && name2[i] <= 'Z') ? (name2[i] + 32) : name2[i];

				if (c1 != static_cast<wchar_t>(c2)) break;

				i++;
			}

			if (name1[i] == L'\0' && name2[i] == '\0') return entry->DllBase;
		}

		curr = curr->Flink;
	}

	return nullptr;
}

void* util::get_module_handle_w(const wchar_t* mod_name)
{
	auto* peb = reinterpret_cast<PEB_CUSTOM*>(__readfsdword(0x30));

	if (!peb || !peb->Ldr) return nullptr;
	if (!mod_name) return peb->ImageBaseAddress;

	PLIST_ENTRY head = &peb->Ldr->InLoadOrderModuleList;
	PLIST_ENTRY curr = head->Flink;

	while (curr != head)
	{
		auto* entry = reinterpret_cast<PLDR_DATA_TABLE_ENTRY_CUSTOM>(curr);

		if (entry->BaseDllName.Buffer != nullptr)
		{
			const wchar_t* name1 = entry->BaseDllName.Buffer;
			const wchar_t* name2 = mod_name;

			size_t i = 0;
			while (name1[i] && name2[i])
			{
				wchar_t c1 = (name1[i] >= L'A' && name1[i] <= L'Z') ? (name1[i] + 32) : name1[i];
				wchar_t c2 = (name2[i] >= L'A' && name2[i] <= L'Z') ? (name2[i] + 32) : name2[i];
				
				if (c1 != c2) break; i++;
			}

			if (name1[i] == L'\0' && name2[i] == L'\0') return entry->DllBase;
		}

		curr = curr->Flink;
	}

	return nullptr;
}