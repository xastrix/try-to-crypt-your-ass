#pragma once

#include <string>
#include <vector>
#include <xorstr.h>

enum FIELD_TYPE {
	FT_USERNAME,
	FT_PASSWORD,
	FT_TOKEN,
	FT_HWID,
	maxFields
};

enum BOOL_FLAG_TYPE {
	EMULATE_SYSTEM_ERR,
	maxBoolFlags
};

namespace g
{
	inline std::string fields[maxFields] = {};
	inline bool flags[maxBoolFlags] = {};

	namespace injector
	{
		inline int game_index = {};

		inline std::string game_full_list[] = {
			__("Counter-Strike: Global Offensive"),
			__("Rust")
		};

		inline std::wstring proc_list[] = {
#ifndef _DEBUG
			__(L"csgo.exe"),
			__(L"RustClient.exe")
#else
			__(L"Client.exe"),
			__(L"Client.exe")
#endif
		};
	}

	namespace log
	{
		inline int index = {};
		inline std::vector<std::string> list = {};

		inline void add(const char* format, ...) {
			va_list args;
			va_start(args, format);

			int size = vsnprintf(nullptr, 0, format, args);
			va_end(args);

			if (size <= 0) return;

			std::string msg(size, '\0');

			va_start(args, format);
			vsnprintf(&msg[0], size + 1, format, args);
			va_end(args);

			auto it = std::find(list.begin(), list.end(), msg);

			if (it == list.end()) {
				list.push_back(msg);
				index = list.size() - 1;
			}
			else {
				index = std::distance(list.begin(), it);
			}
		}
	}
}