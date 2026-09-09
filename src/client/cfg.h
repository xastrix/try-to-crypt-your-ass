#pragma once

#include <string>

enum CFG_STATUS {
	CFG_ALREADY_EXISTS = -2,
	CFG_FAIL = -1,
	CFG_SUCCESS
};

class c_cfg {
public:
	static CFG_STATUS save_creds(const std::string& username, const std::string& password);
	static CFG_STATUS read_creds(std::string& out_username, std::string& out_password);

private:
	static bool is_creds_exists();
};