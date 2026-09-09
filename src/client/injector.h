#pragma once

#include "util.h"

#include <vector>

using T_LoadLibraryA = HMODULE(WINAPI*)(LPCSTR);
using T_GetProcAddress = FARPROC(WINAPI*)(HMODULE, LPCSTR);

struct mmap_loader_data {
	PVOID base;
	PIMAGE_NT_HEADERS headers;
	PIMAGE_BASE_RELOCATION base_reloc;
	PIMAGE_IMPORT_DESCRIPTOR import_desc;
	T_LoadLibraryA ll;
	T_GetProcAddress gpa;
};

class c_injector {
public:
	c_injector(const std::wstring& process_name) : m_pid(0), m_process(nullptr), m_target_base(nullptr), m_proc_name(process_name) {}
	~c_injector() { m_pid = 0; m_process = nullptr; m_target_base = nullptr; }

	bool open_process();
	bool inject(std::vector<BYTE> module_bytes);

private:
	DWORD m_pid;
	HANDLE m_process;
	PVOID m_target_base;
	std::wstring m_proc_name;
};