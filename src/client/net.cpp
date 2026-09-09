#include "net.h"
#include "cfg.h"

#include <thread>

#include <mbedtls/build_info.h>
#include <mbedtls/base64.h>
#include <mbedtls/x509_crt.h>
#define mbedtls_x509_crt_parse_file(chain, path) mbedtls_x509_crt_parse_file(chain, path, 1)
#define mbedtls_x509_crt_parse(chain, buf, buflen) mbedtls_x509_crt_parse(chain, buf, buflen, 1)

#ifndef CPPHTTPLIB_MBEDTLS_SUPPORT
#define CPPHTTPLIB_MBEDTLS_SUPPORT
#endif
#include <httplib.h>
#include <VMProtectSDK.h>
#include <jsonreader.hpp>

#include "globals.h"
#include "util.h"
#include "injector.h"

#define SSL_HOSTNAME "127.0.0.1"
#define SSL_PORT     443

using T_NtSetInformationThread = NTSTATUS(NTAPI*)(IN HANDLE, IN ULONG, IN PVOID, IN ULONG);

using T_NtResumeThread = NTSTATUS(NTAPI*)(IN HANDLE, OUT PULONG);

using T_NtWaitForSingleObject = NTSTATUS(NTAPI*)(IN HANDLE, IN BOOLEAN, IN PLARGE_INTEGER);

using T_NtCreateThreadEx = NTSTATUS(NTAPI*)(OUT PHANDLE, IN ACCESS_MASK, IN PVOID, IN HANDLE,
	IN PVOID, IN PVOID, IN ULONG, IN ULONG, IN SIZE_T, IN SIZE_T, IN PVOID);

using T_NtClose = NTSTATUS(NTAPI*)(IN HANDLE);

static HANDLE g_NetThread = {};
static std::atomic<bool> g_NetThreadLock{ true };

static DWORD __stdcall net_thread_routine(LPVOID param);

NETWORK_STATUS c_network::init()
{
	VMProtectBeginUltra("NETWORK_INIT");

	if (c_cfg::read_creds(g::fields[FT_USERNAME], g::fields[FT_PASSWORD]) == CFG_SUCCESS)
		g_net.set_conn_state(HCONN_AUTHING);

	NTSTATUS status = util::get_export<T_NtCreateThreadEx>(__("ntdll.dll"), __("NtCreateThreadEx"))(
		&g_NetThread,
		THREAD_ALL_ACCESS,
		nullptr,
		(HANDLE)-1,
		(PVOID)net_thread_routine,
		this,
		0x00000001,
		0, 0, 0,
		nullptr
	);

	if (status != 0) {
		VMProtectEnd();
		return NET_FAILED;
	}

	if (g_NetThread != nullptr) {
		util::get_export<T_NtSetInformationThread>(__("ntdll.dll"), __("NtSetInformationThread"))(
			g_NetThread,
			0x11,
			nullptr,
			0
		);

		ULONG scount = { 0 };
		util::get_export<T_NtResumeThread>(__("ntdll.dll"), __("NtResumeThread"))(g_NetThread, &scount);
	}
	else {
		g_NetThreadLock.store(false);
	}

	VMProtectEnd();

	return NET_SUCCESS;
}

void c_network::uninit()
{
	VMProtectBeginUltra("NETWORK_UNINIT");

	g_NetThreadLock.store(false);

	util::get_export<T_NtWaitForSingleObject>(__("ntdll.dll"), __("NtWaitForSingleObject"))(
		g_NetThread,
		FALSE,
		nullptr
	);

	util::get_export<T_NtClose>(__("ntdll.dll"), __("NtClose"))(g_NetThread);

	g_NetThread = nullptr;

	VMProtectEnd();
}

void c_network::auth(const std::string& username, const std::string& password)
{
	VMProtectBeginUltra("NETWORK_AUTH");

	if (username.empty() || password.empty()) {
		VMProtectEnd();
		return;
	}

	send_post_request(__("/api/auth"), util::format_string(__("hwid=%s&username=%s&password=%s"),
		g::fields[FT_HWID].c_str(), username.c_str(), password.c_str()), [this, username, password](const http_result& Res) {
		VMProtectBeginUltra("NETWORK_AUTH_THREAD");

		switch (Res.status) {
		case 200: {
			jsonreader::Value root = jsonreader::Reader::parse(Res.resp);
			int code = root[__("code")].number_val;

			switch (code) {
			case 0x2: {
				root = jsonreader::Reader::parse(Res.resp);
				g::fields[FT_TOKEN] = root[__("token")].string_val;

				g::log::add(__("Successful connected"));

				CFG_STATUS cfg_status = c_cfg::save_creds(username, password);
				if (cfg_status != CFG_ALREADY_EXISTS) g::log::add(__("Credentials saved! No need to re-login next time"));

				g::log::add(util::format_string(__("Signed in as %s"), username.c_str()));

				for (const auto& game : root[__("games")].get_object()) {
					int days_remaining = util::parse_datetime_diff(game.second[__("expires_at")].string_val);

					if (days_remaining > 0) g::log::add(util::format_string(__("Remaining %d days for %s"),
						days_remaining, game.first.c_str()));
				}

				g::log::add(__("Your session expires in 3m"));

				m_conn_state = HCONN_USER_EXPIRES;

				break;
			}
			case 0x77: {
				m_conn_state = HCONN_HWID_ERR;

				break;
			}
			case 0x98: {
				m_conn_state = HCONN_BANNED;

				break;
			}
			case 0x340:
			case 0x278:
			case 0x114: {
				m_conn_state = HCONN_FAILED;

				break;
			}
			}

			break;
		}
		}

		VMProtectEnd();
	});

	VMProtectEnd();
}

bool c_network::ban_request()
{
	VMProtectBeginUltra("NETWORK_BAN_REQUEST");

	int         status = -1;
	std::string token  = g::fields[FT_TOKEN];

	if (token.empty()) {
		VMProtectEnd();
		return false;
	}

	send_get_request(__("/api/ban"), util::format_string(__("&token=%s"),
		token.c_str()), [this, &status](const http_result& Res) {
		VMProtectBeginUltra("NETWORK_BAN_REQUEST_THREAD");

		status = Res.status;
		m_conn_state = HCONN_BANNED;

		VMProtectEnd();
	});

	VMProtectEnd();

	return status != -1 ? true : false;
}

void c_network::send_get_request(const std::string& url, const std::string& data, http_callback_t callback)
{
	VMProtectBeginUltra("NETWORK_GET_REQUEST");

	DO_IF_DEBUG(
		printf("[CLIENT] (NET) Sending get request (%s?%s)\n", url.c_str(), data.c_str());
	);

	std::thread([this, url, data, callback]() {
		VMProtectBeginUltra("NETWORK_GET_REQUEST_THREAD");

		httplib::SSLClient c(__(SSL_HOSTNAME), SSL_PORT);
		
		c.enable_server_certificate_verification(false);
		c.enable_server_hostname_verification(false);

		c.set_connection_timeout(5, 0);
		c.set_default_headers({
			{ __("User-Agent"), __("Valve/Steam HTTP Client 1.0 (480)") }
		});

		auto Res = c.Get(url + __("?") + data);

		if (Res && Res->status == 200) {
			DO_IF_DEBUG(
				printf("[CLIENT] (NET) get request return %d (%s?%s)\n", Res->status, url.c_str(), data.c_str());
			);

			if (callback) callback({ Res->body, Res->status });
		}
		else {
			int status = Res ? Res->status : -1;

			DO_IF_DEBUG(
				printf("[CLIENT] (NET) get request return %d (%s?%s)\n", status, url.c_str(), data.c_str());
			);

			if (callback) callback({ "", status });
		}

		VMProtectEnd();
	}).detach();

	VMProtectEnd();
}

void c_network::send_post_request(const std::string& url, const std::string& data, http_callback_t callback)
{
	VMProtectBeginUltra("NETWORK_POST_REQUEST");

	DO_IF_DEBUG(
		printf("[CLIENT] (NET) Sending post request (%s?%s)\n", url.c_str(), data.c_str());
	);

	std::thread([this, url, data, callback]() {
		VMProtectBeginUltra("NETWORK_POST_REQUEST_THREAD");

		httplib::SSLClient c(__(SSL_HOSTNAME), SSL_PORT);
		
		c.enable_server_certificate_verification(false);
		c.enable_server_hostname_verification(false);

		c.set_connection_timeout(5, 0);
		c.set_default_headers({
			{ __("User-Agent"), __("Valve/Steam HTTP Client 1.0 (480)") }
		});

		auto Res = c.Post(url, data, __("application/x-www-form-urlencoded"));

		if (Res && Res->status == 200) {
			DO_IF_DEBUG(
				printf("[CLIENT] (NET) post request return %d (%s?%s)\n", Res->status, url.c_str(), data.c_str());
			);

			if (callback) callback({ Res->body, Res->status });
		}
		else {
			int status = Res ? Res->status : -1;

			DO_IF_DEBUG(
				printf("[CLIENT] (NET) post request return %d (%s?%s)\n", status, url.c_str(), data.c_str());
			);

			if (callback) callback({ "", status });
		}

		VMProtectEnd();
	}).detach();

	VMProtectEnd();
}

static DWORD __stdcall net_thread_routine(LPVOID param)
{
	auto* _this = reinterpret_cast<c_network*>(param);
	if (!_this) return 1;

	VMProtectBeginUltra("NET_THREAD_ROUTINE");

	static int max_get_requests = 0;

	while (g_NetThreadLock.load()) {
		Sleep(250);

		switch (_this->get_conn_state()) {
		case HCONN_INJECTING:
		case HCONN_INJECTING_END: {
			Sleep(1000);

			if (g::fields[FT_TOKEN].empty()) break;

			if (max_get_requests >= 5) {
				_this->set_conn_state(HCONN_EXPIRE_SESSION);
				break;
			}

			_this->send_get_request(__("/api/dll"), util::format_string(__("game=%d&token=%s"),
				g::injector::game_index, g::fields[FT_TOKEN].c_str()), [_this](const http_result& Res) {
				VMProtectBeginUltra("DLL_GET_REQUEST_THREAD");

				max_get_requests++;

				switch (Res.status) {
				case 200: {
					jsonreader::Value root = jsonreader::Reader::parse(Res.resp);
					int code = root[__("code")].number_val;

					switch (code) {
					case 0x70: {
						std::string data = root[__("base64-payload")].string_val;
						size_t out_len = 0;

						int ret = mbedtls_base64_decode(
							nullptr, 0, &out_len,
							reinterpret_cast<const unsigned char*>(data.c_str()),
							data.length()
						);

						std::vector<BYTE> out_bytes(out_len);

						ret = mbedtls_base64_decode(
							reinterpret_cast<unsigned char*>(out_bytes.data()),
							out_bytes.size(),
							&out_len,
							reinterpret_cast<const unsigned char*>(data.c_str()),
							data.length()
						);

						out_bytes.resize(out_len);

						std::thread([_this, out_bytes]() {
							VMProtectBeginUltra("INJECT_FUNCTION");

							std::wstring p = g::injector::proc_list[g::injector::game_index];
							c_injector mmap(p);

							if (!mmap.open_process()) {
								g::log::add(util::format_string(__("Attention: %s isn't running. Please launch the game and try again"),
									std::string(p.begin(), p.end()).c_str()
								));

								_this->set_conn_state(HCONN_USER_EXPIRES);
								return;
							}

							DO_IF_DEBUG(
								wprintf(L"[CLIENT] (Injector) Process %ls opened. Injecting...\n", p.c_str());
							);

							if (!mmap.inject(out_bytes)) {
								g::log::add(__("An unknown error occurred during injection. Please restart your PC and try again"));

								_this->set_conn_state(HCONN_USER_EXPIRES);
								return;
							}

#ifndef _DEBUG
							g_NetThreadLock.store(false);
							_this->set_conn_state(HCONN_EXIT_SERVICE);
#else
							_this->set_conn_state(HCONN_USER_EXPIRES);
#endif
							
							VMProtectEnd();
						}).detach();

						break;
					}
					case 0x76: {
						g::log::add(util::format_string(__("You have no days remaining to use %s"), g::injector::game_full_list[g::injector::game_index].c_str()));
						_this->set_conn_state(HCONN_USER_EXPIRES);
						
						break;
					}
					case 0x67:
					case 0x69: {
						_this->set_conn_state(HCONN_EXPIRE_SESSION);

						break;
					}
					}

					break;
				}
				case 404: {
					_this->set_conn_state(HCONN_EXPIRE_SESSION);

					break;
				}
				}

				VMProtectEnd();
			});
		}
		}
	}

	VMProtectEnd();

	return 0;
}