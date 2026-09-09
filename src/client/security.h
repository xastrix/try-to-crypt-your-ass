#pragma once

#include "util.h"

enum _SECURITY_STATUS {
	SEC_FAILED = -1,
	SEC_SUCCESS
};

class c_security {
public:
	_SECURITY_STATUS init();
	void uninit();

private:
	bool hwid_init(std::string& out_hwid);
	bool mutex_init();
	bool proc_mitigation_policy_init();
	bool verify_windows_version();
	void apply_api_hooks();

private:
	util::c_minhook m_hooks[1];
};

inline c_security g_security;