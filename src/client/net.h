#pragma once

#include <string>
#include <functional>

struct http_result {
	std::string resp;
	int status;
};

using http_callback_t = std::function<void(const http_result&)>;

enum NETWORK_STATUS {
	NET_FAILED = -1,
	NET_SUCCESS
};

enum HTTPCONNECTION_STATE {
	HCONN_NONE,

	HCONN_AUTHING,
	HCONN_AUTHING_END,

	HCONN_FAILED,
	HCONN_FAILED_END,

	HCONN_BANNED,
	HCONN_BANNED_END,

	HCONN_HWID_ERR,
	HCONN_HWID_ERR_END,

	HCONN_USER_EXPIRES,
	HCONN_USER_EXPIRES_END,

	HCONN_INJECTING,
	HCONN_INJECTING_END,

	HCONN_EXPIRE_SESSION,
	HCONN_EXPIRE_SESSION_END,

	HCONN_EXIT_SERVICE
};

class c_network {
public:
	NETWORK_STATUS init();
	void uninit();

	void auth(const std::string& username, const std::string& password);
	bool ban_request();

	void send_get_request(const std::string& url, const std::string& data, http_callback_t callback);
	void send_post_request(const std::string& url, const std::string& data, http_callback_t callback);

	void set_conn_state(HTTPCONNECTION_STATE state) { m_conn_state = state; }
	HTTPCONNECTION_STATE get_conn_state() { return m_conn_state; }

private:
	HTTPCONNECTION_STATE m_conn_state = HCONN_NONE;
};

inline c_network g_net;