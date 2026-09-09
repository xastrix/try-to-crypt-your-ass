#pragma once

#include <MinHook.h>
#include <string>

#ifdef _DEBUG
#define DO_IF_DEBUG(...) do { __VA_ARGS__ } while (false)
#else
#define DO_IF_DEBUG(...) do {} while (false)
#endif

enum SYSTEM_ERR {
	SE_STATUS_RETRY,       // STATUS_RETRY (0xC000022D)
	SE_ASSERTION_FAILURE,  // STATUS_ASSERTION_FAILURE (0xC0000420)
	SE_MEMORY_CORRUPTION,  // STATUS_DATATYPE_MISALIGNMENT (0x80000002)
	SE_PAGE_FAULT,         // STATUS_IN_PAGE_ERROR (0xC0000006)
	SE_SYSTEM_CORRUPTED,   // STATUS_CORRUPT_SYSTEM_FILE (0xC0000242)
	SE_REGISTRY_CORRUPTED, // STATUS_REGISTRY_CORRUPT (0xC000014C)
	SE_STACK_OVERFLOW,     // STATUS_STACK_OVERFLOW (0xC00000FD)
	SE_ACCESS_DENIED,      // STATUS_ACCESS_DENIED (0xC0000022)
	SE_PRIVILEGE_NOT_HELD  // STATUS_PRIVILEGE_NOT_HELD (0xC0000061)
};

namespace util
{
	void _srand();
	int _rand();

	std::string trim(const std::string& string);

	std::string format_string(const char* format, ...);
	int parse_datetime_diff(const std::string& string);

	void copy_to_clipboard(const std::string& data);
	std::string get_data_from_clipboard();

	std::string get_program_full_name();

	std::string get_cpu_raw();
	bool get_physical_disk_serial(std::string& out_serial);

	void call_message_box(const std::string& msg, uint32_t flags);
	void trigger_system_error(SYSTEM_ERR err_type);

	void* get_proc_address(void* mod_base, const char* func_name);
	void* get_module_handle(const char* mod_name);
	void* get_module_handle_w(const wchar_t* mod_name);

	template <typename T>
	inline T get_export(const char* mod_name, const char* func_name) {
		return reinterpret_cast<T>(get_proc_address(get_module_handle(mod_name), func_name));
	}

	template <typename T>
	inline T get_exportW(const wchar_t* mod_name, const char* func_name) {
		return reinterpret_cast<T>(get_proc_address(get_module_handle_w(mod_name), func_name));
	}

	class c_minhook {
	public:
		c_minhook() : m_hooked(false), m_target(nullptr), m_detour(nullptr), m_original(nullptr) {}
		~c_minhook() { unhook(); }

		void hook(const wchar_t* mod_name, const char* func_name, void* detour, void** original) {
			if (m_hooked) return;

			m_target = get_exportW<void*>(mod_name, func_name);
			if (!m_target) return;

			m_detour = detour;
			m_original = original;

			MH_STATUS status = MH_CreateHook(m_target, m_detour, m_original);
			if (status != MH_OK) return;

			status = MH_EnableHook(m_target);
			if (status != MH_OK) {
				MH_RemoveHook(m_target);
				return;
			}

			m_hooked = true;
		}

		void unhook() {
			if (!m_hooked || !m_target) return;

			if (MH_DisableHook(m_target) != MH_OK) return;
			if (MH_RemoveHook(m_target) != MH_OK) return;

			m_hooked = false;
			m_target = nullptr;
			m_detour = nullptr;
			m_original = nullptr;
		}

	private:
		void*  m_target;
		void*  m_detour;
		void** m_original;
		bool   m_hooked;
	};
}