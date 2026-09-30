#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace jsonreader
{
	enum class json_type { null, object, arr, string, num };

	struct json_value {
		const json_value& operator[](const std::string& key) const {
			auto it = m_object.find(key);

			if (it == m_object.end()) {
				static const json_value null_value;
				return null_value;
			}

			return it->second;
		}

		const json_value& operator[](size_t index) const {
			if (index >= m_array.size()) {
				static const json_value null_value;
				return null_value;
			}

			return m_array[index];
		}

		json_type get_type() const { return m_type; }
		void set_type(json_type type) { m_type = type; }

		const std::string& get_string() const { return m_string; }
		void set_string(const std::string& str) { m_string = str; }

		const std::map<std::string, json_value>& get_object() const { return m_object; }
		void set_object(const std::map<std::string, json_value>& obj) { m_object = obj; }

		const std::vector<json_value>& get_array() const { return m_array; }
		void set_array(const std::vector<json_value>& arr) { m_array = arr; }

		double get_num() const { return m_num; }
		void set_num(double num) { m_num = num; }

	private:
		json_type                         m_type = json_type::null;
		std::string                       m_string;
		std::map<std::string, json_value> m_object;
		std::vector<json_value>           m_array;
		double                            m_num;

		friend class json_reader;
	};

	class json_reader {
	public:
		static json_value parse(const std::string& src) {
			size_t offset = 0;
			skip_whitespace(src, offset);

			return parse_value(src, offset);
		}

	private:
		static json_value parse_object(const std::string& src, size_t& offset) {
			json_value v;
			v.m_type = json_type::object;
			
			offset++;

			while (offset < src.size()) {
				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == '}') {
					offset++;
					return v;
				}

				if (offset >= src.size() || src[offset] != '"') {
					json_value err_v;
					err_v.m_type = json_type::null;
					return err_v;
				}

				json_value key_v = parse_string(src, offset);

				skip_whitespace(src, offset);
				if (offset >= src.size() || src[offset] != ':') {
					json_value err_v;
					err_v.m_type = json_type::null;
					return err_v;
				}
				offset++;

				skip_whitespace(src, offset);
				v.m_object[key_v.m_string] = parse_value(src, offset);

				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == ',') {
					offset++;
				}
				else if (offset < src.size() && src[offset] == '}') {
					offset++;
					return v;
				}
				else {
					json_value err_v;
					err_v.m_type = json_type::null;
					return err_v;
				}
			}

			return v;
		}

		static json_value parse_array(const std::string& src, size_t& offset) {
			json_value v;
			v.m_type = json_type::arr;

			offset++;

			while (offset < src.size()) {
				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == ']') {
					offset++;
					return v;
				}

				v.m_array.push_back(parse_value(src, offset));

				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == ',') {
					offset++;
				}
				else if (offset < src.size() && src[offset] == ']') {
					offset++;
					return v;
				}
				else {
					json_value err_v;
					err_v.m_type = json_type::null;
					return err_v;
				}
			}

			return v;
		}

		static json_value parse_number(const std::string& src, size_t& offset) {
			json_value v;
			v.m_type = json_type::num;

			std::string num_str;
			if (src[offset] == '-') {
				num_str += src[offset];
				offset++;
			}

			while (offset < src.size() && (std::isdigit(static_cast<unsigned char>(src[offset])) || src[offset] == '.')) {
				num_str += src[offset];
				offset++;
			}

			if (!num_str.empty()) {
				try {
					v.m_num = std::stod(num_str);
				}
				catch (...) {
					v.m_type = json_type::null;
				}
			}
			else {
				v.m_type = json_type::null;
			}

			return v;
		}

		static json_value parse_string(const std::string& src, size_t& offset) {
			json_value v;
			v.m_type = json_type::string;

			offset++;

			while (offset < src.size() && src[offset] != '"') {
				v.m_string += src[offset];
				offset++;
			}

			if (offset < src.size()) offset++;

			return v;
		}

		static json_value parse_value(const std::string& src, size_t& offset) {
			if (offset >= src.size()) return {};

			char ch = src[offset];
			if (ch == '{') return parse_object(src, offset);
			if (ch == '[') return parse_array(src, offset);
			if (ch == '"') return parse_string(src, offset);

			if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '-') {
				return parse_number(src, offset);
			}

			return {};
		}

		static void skip_whitespace(const std::string& src, size_t& offset) {
			while (offset < src.size() && std::isspace(static_cast<unsigned char>(src[offset]))) offset++;
		}
	};
}