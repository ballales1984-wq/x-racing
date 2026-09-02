#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace p0::json {

enum class Type {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

struct Value;

using Array = std::vector<Value>;
using Object = std::map<std::string, Value>;

struct Value {
    Type type = Type::Null;
    double number = 0.0;
    bool boolean = false;
    std::string string;
    Array array;
    Object object;

    Value() : type(Type::Null) {}
    explicit Value(double n) : type(Type::Number), number(n) {}
    explicit Value(bool b) : type(Type::Bool), boolean(b) {}
    explicit Value(const std::string& s) : type(Type::String), string(s) {}
    explicit Value(const char* s) : type(Type::String), string(s) {}
    explicit Value(const Array& a) : type(Type::Array), array(a) {}
    explicit Value(const Object& o) : type(Type::Object), object(o) {}

    bool is_null() const { return type == Type::Null; }
    bool is_number() const { return type == Type::Number; }
    bool is_bool() const { return type == Type::Bool; }
    bool is_string() const { return type == Type::String; }
    bool is_array() const { return type == Type::Array; }
    bool is_object() const { return type == Type::Object; }

    double as_double() const { return number; }
    int as_int() const { return static_cast<int>(number); }
    bool as_bool() const { return boolean; }
    const std::string& as_string() const { return string; }

    bool has(const std::string& key) const {
        return type == Type::Object && object.count(key) > 0;
    }

    const Value& operator[](const std::string& key) const {
        if (type != Type::Object) throw std::runtime_error("not an object");
        auto it = object.find(key);
        if (it == object.end()) throw std::runtime_error("key not found: " + key);
        return it->second;
    }

    const Value& operator[](size_t index) const {
        if (type != Type::Array) throw std::runtime_error("not an array");
        if (index >= array.size()) throw std::runtime_error("array index out of range");
        return array[index];
    }

    size_t size() const {
        if (type == Type::Array) return array.size();
        if (type == Type::Object) return object.size();
        return 0;
    }
};

class Parser {
public:
    explicit Parser(const std::string& input) : input_(input), pos_(0) {}

    Value parse() {
        skip_whitespace();
        Value result = parse_value();
        skip_whitespace();
        return result;
    }

private:
    std::string input_;
    size_t pos_;

    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    char peek() const {
        if (pos_ >= input_.size()) return '\0';
        return input_[pos_];
    }

    char advance() {
        if (pos_ >= input_.size()) return '\0';
        return input_[pos_++];
    }

    bool consume(char expected) {
        if (peek() == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    Value parse_value() {
        skip_whitespace();
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
        throw std::runtime_error("unexpected character in JSON");
    }

    Value parse_object() {
        Object obj;
        consume('{');
        skip_whitespace();
        if (consume('}')) return Value(obj);
        while (true) {
            skip_whitespace();
            Value key = parse_string();
            skip_whitespace();
            if (!consume(':')) throw std::runtime_error("expected ':'");
            skip_whitespace();
            Value val = parse_value();
            obj[key.as_string()] = val;
            skip_whitespace();
            if (consume('}')) break;
            if (!consume(',')) throw std::runtime_error("expected ',' or '}'");
        }
        return Value(obj);
    }

    Value parse_array() {
        Array arr;
        consume('[');
        skip_whitespace();
        if (consume(']')) return Value(arr);
        while (true) {
            skip_whitespace();
            arr.push_back(parse_value());
            skip_whitespace();
            if (consume(']')) break;
            if (!consume(',')) throw std::runtime_error("expected ',' or ']'");
        }
        return Value(arr);
    }

    Value parse_string() {
        consume('"');
        std::string result;
        while (pos_ < input_.size()) {
            char c = advance();
            if (c == '"') return Value(result);
            if (c == '\\') {
                if (pos_ >= input_.size()) throw std::runtime_error("unexpected end of string");
                char esc = advance();
                switch (esc) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: result += esc; break;
                }
            } else {
                result += c;
            }
        }
        throw std::runtime_error("unterminated string");
    }

    Value parse_number() {
        size_t start = pos_;
        if (consume('-')) {}
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
        if (consume('.')) {
            while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
        }
        if (peek() == 'e' || peek() == 'E') {
            advance();
            if (peek() == '+' || peek() == '-') advance();
            while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
        }
        std::string num_str = input_.substr(start, pos_ - start);
        return Value(std::stod(num_str));
    }

    Value parse_bool() {
        if (peek() == 't') {
            if (input_.substr(pos_, 4) == "true") {
                pos_ += 4;
                return Value(true);
            }
        } else {
            if (input_.substr(pos_, 5) == "false") {
                pos_ += 5;
                return Value(false);
            }
        }
        throw std::runtime_error("invalid boolean");
    }

    Value parse_null() {
        if (input_.substr(pos_, 4) == "null") {
            pos_ += 4;
            return Value();
        }
        throw std::runtime_error("invalid null");
    }
};

inline Value parse(const std::string& input) {
    Parser parser(input);
    return parser.parse();
}

inline std::string serialize(const Value& value, int indent = 0) {
    std::ostringstream oss;
    switch (value.type) {
        case Type::Null:
            oss << "null";
            break;
        case Type::Bool:
            oss << (value.boolean ? "true" : "false");
            break;
        case Type::Number: {
            double v = value.number;
            if (v == std::floor(v) && std::abs(v) < 1e15) {
                oss << static_cast<long long>(v);
            } else {
                oss << std::setprecision(15) << v;
            }
            break;
        }
        case Type::String:
            oss << '"' << value.string << '"';
            break;
        case Type::Array:
            oss << "[";
            for (size_t i = 0; i < value.array.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << serialize(value.array[i], indent);
            }
            oss << "]";
            break;
        case Type::Object:
            oss << "{\n";
            {
                std::string indent_str(indent + 2, ' ');
                size_t count = 0;
                for (const auto& [k, v] : value.object) {
                    if (count > 0) oss << ",\n";
                    oss << indent_str << '"' << k << "\": " << serialize(v, indent + 2);
                    ++count;
                }
            }
            oss << "\n" << std::string(indent, ' ') << "}";
            break;
    }
    return oss.str();
}

} // namespace p0::json
