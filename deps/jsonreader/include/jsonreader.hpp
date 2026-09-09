#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace jsonreader
{
	enum class Type { Null, Object, Array, String, Number };

	struct Value {
		Type type = Type::Null;
		std::string string_val;
		std::map<std::string, Value> object_val;
		std::vector<Value> array_val;
		double number_val = 0.0;

		const Value& operator[](const std::string& key) const {
			auto it = object_val.find(key);
			if (it == object_val.end()) {
				static const Value null_value;
				return null_value;
			}
			return it->second;
		}

		const Value& operator[](size_t index) const {
			if (index >= array_val.size()) {
				static const Value null_value;
				return null_value;
			}
			return array_val[index];
		}

		const std::map<std::string, Value>& get_object() const {
			return object_val;
		}
	};

	class Reader {
	public:
		static Value parse(const std::string& src) {
			size_t offset = 0;
			skip_whitespace(src, offset);
			return parse_value(src, offset);
		}

	private:
		static void skip_whitespace(const std::string& src, size_t& offset) {
			while (offset < src.size() && std::isspace(static_cast<unsigned char>(src[offset]))) offset++;
		}

		static Value parse_value(const std::string& src, size_t& offset) {
			if (offset >= src.size()) return {};

			char ch = src[offset];
			if (ch == '{') return parse_object(src, offset);
			if (ch == '[') return parse_array(src, offset);
			if (ch == '"') return parse_string(src, offset);

			if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '-') {
				return parse_number(src, offset);
			}

			Value val;
			val.type = Type::Null;
			return val;
		}

		static Value parse_string(const std::string& src, size_t& offset) {
			Value val;
			val.type = Type::String;
			offset++;

			while (offset < src.size() && src[offset] != '"') {
				val.string_val += src[offset];
				offset++;
			}

			if (offset < src.size()) offset++;
			return val;
		}

		static Value parse_number(const std::string& src, size_t& offset) {
			Value val;
			val.type = Type::Number;

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
					val.number_val = std::stod(num_str);
				}
				catch (...) {
					val.type = Type::Null;
				}
			}
			else {
				val.type = Type::Null;
			}

			return val;
		}

		static Value parse_object(const std::string& src, size_t& offset) {
			Value val;
			val.type = Type::Object;
			offset++;

			while (offset < src.size()) {
				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == '}') {
					offset++;
					return val;
				}

				if (offset >= src.size() || src[offset] != '"') {
					Value error_val;
					error_val.type = Type::Null;
					return error_val;
				}
				Value key_val = parse_string(src, offset);

				skip_whitespace(src, offset);
				if (offset >= src.size() || src[offset] != ':') {
					Value error_val;
					error_val.type = Type::Null;
					return error_val;
				}
				offset++;

				skip_whitespace(src, offset);
				val.object_val[key_val.string_val] = parse_value(src, offset);

				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == ',') {
					offset++;
				}
				else if (offset < src.size() && src[offset] == '}') {
					offset++;
					return val;
				}
				else {
					Value error_val;
					error_val.type = Type::Null;
					return error_val;
				}
			}

			return val;
		}

		static Value parse_array(const std::string& src, size_t& offset) {
			Value val;
			val.type = Type::Array;
			offset++;

			while (offset < src.size()) {
				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == ']') {
					offset++;
					return val;
				}

				val.array_val.push_back(parse_value(src, offset));

				skip_whitespace(src, offset);
				if (offset < src.size() && src[offset] == ',') {
					offset++;
				}
				else if (offset < src.size() && src[offset] == ']') {
					offset++;
					return val;
				}
				else {
					Value error_val;
					error_val.type = Type::Null;
					return error_val;
				}
			}

			return val;
		}
	};
}