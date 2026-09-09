#include "cfg.h"
#include "util.h"

#include <vector>
#include <mbedtls/cipher.h>
#include <xorstr.h>
#include <VMProtectSDK.h>

#define TAG_LEN                16
#define REGEDIT_ROOT_KEY_PATH  "xbox-gaming-launcher"
#define REGEDIT_USERNAME_FIELD "field1"
#define REGEDIT_PASSWORD_FIELD "field2"

const BYTE g_secretKey[32] = {
	'z', 'f', 'y', 'g', 'b', 'u', 'w', '3', '5', '1', 'j', 'o', 'p', 'w', 'j', 'z',
	'h', '0', 'x', 'a', 's', 't', 'c', 'l', 'i', 'e', 'n', 't', '8', '4', '6', 'k'
};

const BYTE g_Iv[12] = { 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22 };

CFG_STATUS c_cfg::save_creds(const std::string& username, const std::string& password)
{
	VMProtectBeginUltra("CFG_SAVE_CREDENTIALS");

	if (is_creds_exists()) {
		VMProtectEnd();
		return CFG_ALREADY_EXISTS;
	}

	HKEY hKey;
	LSTATUS status = RegCreateKeyExA(
		HKEY_CLASSES_ROOT, __(REGEDIT_ROOT_KEY_PATH),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL
	);

	if (status != ERROR_SUCCESS) {
		VMProtectEnd();
		return CFG_FAIL;
	}

	RegSetValueExA(hKey, __(REGEDIT_USERNAME_FIELD), 0, REG_SZ, reinterpret_cast<const BYTE*>(username.c_str()), static_cast<DWORD>(username.length() + 1));

	auto encryptedPassword = [](const std::string& plainText) -> std::vector<BYTE> {
		mbedtls_cipher_context_t ctx;
		mbedtls_cipher_init(&ctx);

		const mbedtls_cipher_info_t* cipherInfo = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_GCM);
		if (mbedtls_cipher_setup(&ctx, cipherInfo) != 0) {};

		if (mbedtls_cipher_setkey(&ctx, g_secretKey, 256, MBEDTLS_ENCRYPT) != 0) {
			mbedtls_cipher_free(&ctx);
			return {};
		}

		std::vector<BYTE> output(plainText.length() + TAG_LEN);
		
		size_t olen = 0;
		int ret = mbedtls_cipher_auth_encrypt_ext(
			&ctx,
			g_Iv, sizeof(g_Iv),
			NULL, 0,
			reinterpret_cast<const BYTE*>(plainText.c_str()), plainText.length(),
			output.data(), output.size(), &olen,
			TAG_LEN
		);

		mbedtls_cipher_free(&ctx);
		if (ret != 0) return {};

		output.resize(olen);

		return output;
	}(password);

	RegSetValueExA(hKey, __(REGEDIT_PASSWORD_FIELD), 0, REG_BINARY, encryptedPassword.data(), static_cast<DWORD>(encryptedPassword.size()));
	RegCloseKey(hKey);

	VMProtectEnd();

	return CFG_SUCCESS;
}

CFG_STATUS c_cfg::read_creds(std::string& out_username, std::string& out_password)
{
	VMProtectBeginUltra("CFG_READ_CREDENTIALS");

	HKEY hKey;
	LSTATUS status = RegOpenKeyExA(HKEY_CLASSES_ROOT, __(REGEDIT_ROOT_KEY_PATH), 0, KEY_READ, &hKey);
	if (status != ERROR_SUCCESS) {
		VMProtectEnd();
		return CFG_FAIL;
	}

	char userBuffer[256] = { 0 };
	DWORD bufferSize = sizeof(userBuffer);
	RegQueryValueExA(hKey, __(REGEDIT_USERNAME_FIELD), NULL, NULL, reinterpret_cast<LPBYTE>(userBuffer), &bufferSize);

	out_username = std::string(userBuffer);

	DWORD encryptedSize = 0;
	status = RegQueryValueExA(hKey, __(REGEDIT_PASSWORD_FIELD), NULL, NULL, NULL, &encryptedSize);

	if (status == ERROR_SUCCESS && encryptedSize > 0)
	{
		std::vector<BYTE> encryptedPassword(encryptedSize);
		RegQueryValueExA(hKey, __(REGEDIT_PASSWORD_FIELD), NULL, NULL, encryptedPassword.data(), &encryptedSize);

		out_password = [](const std::vector<BYTE>& cipherText) -> std::string {
			if (cipherText.size() <= TAG_LEN) return "";

			mbedtls_cipher_context_t ctx;
			mbedtls_cipher_init(&ctx);

			const mbedtls_cipher_info_t* cipherInfo = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_GCM);
			if (mbedtls_cipher_setup(&ctx, cipherInfo) != 0) return "";

			if (mbedtls_cipher_setkey(&ctx, g_secretKey, 256, MBEDTLS_DECRYPT) != 0) {
				mbedtls_cipher_free(&ctx);
				return "";
			}

			std::vector<BYTE> output((cipherText.size() - TAG_LEN) + 1);
			
			size_t olen = 0;
			int ret = mbedtls_cipher_auth_decrypt_ext(
				&ctx,
				g_Iv, sizeof(g_Iv),
				NULL, 0,
				cipherText.data(), cipherText.size(),
				output.data(), output.size(), &olen,
				TAG_LEN
			);

			mbedtls_cipher_free(&ctx);

			if (ret != 0) return "";

			return std::string(reinterpret_cast<char*>(output.data()), olen);
		}(encryptedPassword);
	}

	RegCloseKey(hKey);

	DO_IF_DEBUG(
		printf("[CLIENT] Reading credentials in regedit (%s:%s)\n", out_username.c_str(), out_password.c_str());
	);

	VMProtectEnd();

	return (!out_username.empty() && !out_password.empty()) ? CFG_SUCCESS : CFG_FAIL;
}

bool c_cfg::is_creds_exists()
{
	VMProtectBeginUltra("CFG_CREDENTIALS_EXISTS");

	HKEY hKey;
	LSTATUS status = RegOpenKeyExA(HKEY_CLASSES_ROOT, __(REGEDIT_ROOT_KEY_PATH), 0, KEY_READ, &hKey);
	if (status != ERROR_SUCCESS) {
		VMProtectEnd();
		return false;
	}

	DWORD   user_size = 0;
	LSTATUS user_status = RegQueryValueExA(hKey, __(REGEDIT_USERNAME_FIELD), NULL, NULL, NULL, &user_size);
	
	DWORD   pass_size = 0;
	LSTATUS pass_status = RegQueryValueExA(hKey, __(REGEDIT_PASSWORD_FIELD), NULL, NULL, NULL, &pass_size);

	RegCloseKey(hKey);

	bool exists = (user_status == ERROR_SUCCESS) && (pass_status == ERROR_SUCCESS) &&
		(user_size > 0) && (pass_size > 0);

	VMProtectEnd();

	return exists;
}