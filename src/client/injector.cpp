#include "injector.h"

#include <winternl.h>
#include <tlhelp32.h>
#include <xorstr.h>

using T_DllMain = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);

using T_NtOpenProcess = NTSTATUS(NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, CLIENT_ID*);

using T_NtCreateThreadEx = NTSTATUS(NTAPI*)(OUT PHANDLE, IN ACCESS_MASK, IN PVOID, IN HANDLE,
	IN PVOID, IN PVOID, IN ULONG, IN ULONG, IN SIZE_T, IN SIZE_T, IN PVOID);

using T_NtAllocateVirtualMemory = NTSTATUS(NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);

using T_NtWriteVirtualMemory = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, ULONG, PSIZE_T);

static DWORD WINAPI __shellcode__(mmap_loader_data* data);
static void         __shellcode_end__() {}

bool c_injector::open_process()
{
	OBJECT_ATTRIBUTES oa = {};
	CLIENT_ID cid = {};

	InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
	cid.UniqueProcess = [this]() -> HANDLE {
		DWORD ret = 0;

		PROCESSENTRY32W proc_info;
		proc_info.dwSize = sizeof(proc_info);

		const auto h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (h == INVALID_HANDLE_VALUE) return 0;

		if (Process32FirstW(h, &proc_info)) {
			do {
				if (wcscmp(proc_info.szExeFile, m_proc_name.c_str()) == 0) {
					ret = proc_info.th32ProcessID;
					break;
				}
			} while (Process32NextW(h, &proc_info));
		}

		CloseHandle(h);

		return reinterpret_cast<HANDLE>(ret);
	}();
	cid.UniqueThread = NULL;

	NTSTATUS status = util::get_export<T_NtOpenProcess>(__("ntdll.dll"), __("NtOpenProcess"))(
		&m_process,
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
		PROCESS_VM_WRITE | PROCESS_VM_READ,
		&oa, &cid
	);

	return (status == 0L);
}

bool c_injector::inject(std::vector<BYTE> module_bytes)
{
	PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_bytes.data());
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) Invalid DOS signature\n");
		);
		return false;
	}

	PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(module_bytes.data() + dos_header->e_lfanew);
	if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) Invalid NT signature\n");
		);
		return false;
	}

	SIZE_T image_size = nt_headers->OptionalHeader.SizeOfImage;
	NTSTATUS status = util::get_export<T_NtAllocateVirtualMemory>(__("ntdll.dll"), __("NtAllocateVirtualMemory"))(
		m_process, &m_target_base, 0, &image_size,
		MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE
	);

	if (status != 0) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) NtAllocateVirtualMemory (Image) failed with status: 0x%X\n", status);
		);
		return false;
	}

	SIZE_T bytes_written = 0;
	status = util::get_export<T_NtWriteVirtualMemory>(__("ntdll.dll"), __("NtWriteVirtualMemory"))(
		m_process, m_target_base, module_bytes.data(),
		nt_headers->OptionalHeader.SizeOfHeaders, &bytes_written
	);

	if (status != 0) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) NtWriteVirtualMemory (Headers) failed with status: 0x%X\n", status);
		);
		return false;
	}

	PIMAGE_SECTION_HEADER section_header = IMAGE_FIRST_SECTION(nt_headers);
	for (UINT i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
	{
		if (section_header[i].SizeOfRawData > 0)
		{
			PVOID section_dest = (PVOID)((ULONG_PTR)m_target_base + section_header[i].VirtualAddress);
			PVOID section_src = (PVOID)((ULONG_PTR)module_bytes.data() + section_header[i].PointerToRawData);

			status = util::get_export<T_NtWriteVirtualMemory>(__("ntdll.dll"), __("NtWriteVirtualMemory"))(
				m_process, section_dest, section_src,
				section_header[i].SizeOfRawData, &bytes_written
			);

			if (status != 0) {
				DO_IF_DEBUG(
					printf("[CLIENT] (Injector) NtWriteVirtualMemory (Section %s) failed with status: 0x%X\n", section_header[i].Name, status);
				);
				return false;
			}
		}
	}

	mmap_loader_data data = {};
	data.base        = m_target_base;
	data.headers     = (PIMAGE_NT_HEADERS)((ULONG_PTR)m_target_base + dos_header->e_lfanew);
	data.base_reloc  = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)m_target_base + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
	data.import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)m_target_base + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	data.ll          = util::get_export<T_LoadLibraryA>(__("kernel32.dll"), __("LoadLibraryA"));
	data.gpa         = util::get_export<T_GetProcAddress>(__("kernel32.dll"), __("GetProcAddress"));

	PVOID remote_data_addr = nullptr;
	SIZE_T data_size = sizeof(mmap_loader_data);
	
	status = util::get_export<T_NtAllocateVirtualMemory>(__("ntdll.dll"), __("NtAllocateVirtualMemory"))(
		m_process, &remote_data_addr, 0, &data_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE
	);

	if (status != 0) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) NtAllocateVirtualMemory failed with status: 0x%X\n", status);
		);
		return false;
	}

	status = util::get_export<T_NtWriteVirtualMemory>(__("ntdll.dll"), __("NtWriteVirtualMemory"))(
		m_process, remote_data_addr, &data, sizeof(mmap_loader_data), &bytes_written
	);

	if (status != 0) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) NtWriteVirtualMemory failed with status: 0x%X\n", status);
		);
		return false;
	}

	PVOID remote_shellcode_addr = nullptr;
	SIZE_T shellcode_size = 4096;
	
	status = util::get_export<T_NtAllocateVirtualMemory>(__("ntdll.dll"), __("NtAllocateVirtualMemory"))(
		m_process, &remote_shellcode_addr, 0, &shellcode_size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE
	);

	if (status != 0) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) NtAllocateVirtualMemory failed with status: 0x%X\n", status);
		);
		return false;
	}

	status = util::get_export<T_NtWriteVirtualMemory>(__("ntdll.dll"), __("NtWriteVirtualMemory"))(
		m_process, remote_shellcode_addr, (PVOID)__shellcode__, shellcode_size, &bytes_written
	);

	if (status != 0) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) NtWriteVirtualMemory failed with status: 0x%X\n", status);
		);
		return false;
	}

	HANDLE thread_handle = nullptr;
	status = util::get_export<T_NtCreateThreadEx>(__("ntdll.dll"), __("NtCreateThreadEx"))(
		&thread_handle, 0x1fffff, 0, m_process,
		remote_shellcode_addr, remote_data_addr, FALSE,
		0, 0, 0, 0
	);

	if (status != 0) {
		DO_IF_DEBUG(
			printf("[CLIENT] (Injector) NtCreateThreadEx failed with status: 0x%X\n", status);
		);
		return false;
	}

	WaitForSingleObject(thread_handle, INFINITE);
	CloseHandle(thread_handle);

	return true;
}

static DWORD WINAPI __shellcode__(mmap_loader_data* data)
{
	ULONG_PTR delta = (ULONG_PTR)data->base - data->headers->OptionalHeader.ImageBase;

	if (delta)
	{
		if (data->headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
		{
			auto* reloc = data->base_reloc;

			while (reloc->VirtualAddress) {
				UINT entries_count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
				WORD* relative_info = (WORD*)(reloc + 1);

				for (UINT i = 0; i < entries_count; i++) {
					if ((relative_info[i] >> 12) == IMAGE_REL_BASED_DIR64 || (relative_info[i] >> 12) == IMAGE_REL_BASED_HIGHLOW) {
						ULONG_PTR* patch = (ULONG_PTR*)((ULONG_PTR)data->base + reloc->VirtualAddress + (relative_info[i] & 0xFFF));
						*patch += delta;
					}
				}

				reloc = (IMAGE_BASE_RELOCATION*)((ULONG_PTR)reloc + reloc->SizeOfBlock);
			}
		}
	}

	if (data->headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
	{
		IMAGE_IMPORT_DESCRIPTOR* import_desc = data->import_desc;

		while (import_desc->Name)
		{
			char* lib_name = (char*)((ULONG_PTR)data->base + import_desc->Name);
			
			HMODULE ll_mod = data->ll(lib_name);
			if (!ll_mod) return 0;

			auto* thunk_ref = (IMAGE_THUNK_DATA*)((ULONG_PTR)data->base + import_desc->OriginalFirstThunk);
			auto* func_ref = (IMAGE_THUNK_DATA*)((ULONG_PTR)data->base + import_desc->FirstThunk);

			if (!thunk_ref) thunk_ref = func_ref;

			while (thunk_ref->u1.AddressOfData)
			{
				if (IMAGE_SNAP_BY_ORDINAL(thunk_ref->u1.Ordinal))
				{
					func_ref->u1.Function = (ULONG_PTR)data->gpa(ll_mod, (LPCSTR)(thunk_ref->u1.Ordinal & 0xFFFF));
				}
				else {
					auto* import_by_name = (IMAGE_IMPORT_BY_NAME*)((ULONG_PTR)data->base + thunk_ref->u1.AddressOfData);
					func_ref->u1.Function = (ULONG_PTR)data->gpa(ll_mod, import_by_name->Name);
				}

				thunk_ref++;
				func_ref++;
			}

			import_desc++;
		}
	}

	if (data->headers->OptionalHeader.AddressOfEntryPoint)
	{
		auto DllMain = (T_DllMain)((ULONG_PTR)data->base + data->headers->OptionalHeader.AddressOfEntryPoint);

		DllMain((HINSTANCE)data->base, DLL_PROCESS_ATTACH, nullptr);
	}

	return 1;
}