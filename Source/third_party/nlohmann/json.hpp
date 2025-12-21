// Minimal subset of nlohmann::json functionality for this project
// This is a tiny JSON implementation (parse + dump) sufficient for serialization
// of the project's data structures. Not a full replacement of nlohmann::json.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace nlohmann {

class json
{
public:
 enum class Type { Null, Boolean, Number, String, Array, Object };

 // Constructors
 json() : type_(Type::Null), number_value(0.0) {}
 json(std::nullptr_t) : type_(Type::Null), number_value(0.0) {}
 json(bool b) : type_(Type::Boolean), bool_value(b), number_value(0.0) {}
 json(double d) : type_(Type::Number), number_value(d) {}
 json(int i) : type_(Type::Number), number_value(static_cast<double>(i)) {}
 json(const std::string& s) : type_(Type::String), string_value(new std::string(s)), number_value(0.0) {}
 json(const char* s) : type_(Type::String), string_value(new std::string(s)), number_value(0.0) {}

 // object/array creators
 static json object()
 {
 json j; j.type_ = Type::Object; j.object_value = new std::map<std::string, json>(); return j;
 }
 static json array()
 {
 json j; j.type_ = Type::Array; j.array_value = new std::vector<json>(); return j;
 }

 // copy
 json(const json& other)
 {
 copy_from(other);
 }
 json& operator=(const json& other)
 {
 if (this != &other) { clear(); copy_from(other); }
 return *this;
 }
 ~json() { clear(); }

 // accessors
 bool is_null() const { return type_ == Type::Null; }
 bool is_boolean() const { return type_ == Type::Boolean; }
 bool is_number() const { return type_ == Type::Number; }
 bool is_string() const { return type_ == Type::String; }
 bool is_array() const { return type_ == Type::Array; }
 bool is_object() const { return type_ == Type::Object; }

 // object access
 json& operator[](const std::string& key)
 {
 ensure_object();
 return (*object_value)[key];
 }
 const json& operator[](const std::string& key) const
 {
 if (!is_object()) throw std::runtime_error("not an object");
 auto it = object_value->find(key);
 if (it == object_value->end()) throw std::out_of_range("key not found");
 return it->second;
 }

 // array access
 json& operator[](size_t idx)
 {
 ensure_array();
 if (idx >= array_value->size()) throw std::out_of_range("index out of range");
 return (*array_value)[idx];
 }
 const json& operator[](size_t idx) const
 {
 if (!is_array()) throw std::runtime_error("not an array");
 return (*array_value)[idx];
 }

 void push_back(const json& j)
 {
 ensure_array();
 array_value->push_back(j);
 }

 size_t size() const
 {
 if (is_array()) return array_value->size();
 if (is_object()) return object_value->size();
 return 0;
 }

 bool contains(const std::string& key) const
 {
 if (!is_object()) return false;
 return object_value->find(key) != object_value->end();
 }

 // getters
 template<typename T>
 T get() const;

 template<typename T>
 void get_to(T& out) const { out = get<T>(); }

 std::string dump(int indent = -1) const
 {
 std::ostringstream ss;
 dump_internal(ss, indent,0);
 return ss.str();
 }

 // parse from string
 static json parse(const std::string& s)
 {
 size_t idx =0;
 return parse_internal(s, idx);
 }

private:
 Type type_;
 bool bool_value = false;
 double number_value =0.0;
 std::string* string_value = nullptr;
 std::vector<json>* array_value = nullptr;
 std::map<std::string, json>* object_value = nullptr;

 void clear()
 {
 if (string_value) { delete string_value; string_value = nullptr; }
 if (array_value) { delete array_value; array_value = nullptr; }
 if (object_value) { delete object_value; object_value = nullptr; }
 type_ = Type::Null;
 }
 void copy_from(const json& other)
 {
 type_ = other.type_;
 bool_value = other.bool_value;
 number_value = other.number_value;
 if (other.string_value) string_value = new std::string(*other.string_value); else string_value = nullptr;
 if (other.array_value) array_value = new std::vector<json>(*other.array_value); else array_value = nullptr;
 if (other.object_value) object_value = new std::map<std::string, json>(*other.object_value); else object_value = nullptr;
 }

 void ensure_object()
 {
 if (!is_object()) { clear(); type_ = Type::Object; object_value = new std::map<std::string, json>(); }
 }
 void ensure_array()
 {
 if (!is_array()) { clear(); type_ = Type::Array; array_value = new std::vector<json>(); }
 }

 static void skip_ws(const std::string& s, size_t& i)
 {
 while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
 }

 static json parse_internal(const std::string& s, size_t& i)
 {
 skip_ws(s, i);
 if (i >= s.size()) throw std::runtime_error("unexpected end of input");
 char c = s[i];
 if (c == 'n') { // null
 if (s.compare(i,4,"null") ==0) { i +=4; return json(); }
 }
 if (c == 't') { if (s.compare(i,4,"true") ==0) { i +=4; return json(true); } }
 if (c == 'f') { if (s.compare(i,5,"false") ==0) { i +=5; return json(false); } }
 if (c == '"') { // string
 ++i; std::ostringstream ss;
 while (i < s.size()) {
 char ch = s[i++];
 if (ch == '"') break;
 if (ch == '\\') {
 if (i >= s.size()) break;
 char esc = s[i++];
 if (esc == 'n') ss << '\n'; else if (esc == 't') ss << '\t'; else if (esc == 'r') ss << '\r'; else ss << esc;
 } else ss << ch;
 }
 return json(ss.str());
 }
 if (c == '{') {
 ++i; json obj = json::object(); skip_ws(s,i);
 if (s[i] == '}') { ++i; return obj; }
 while (i < s.size()) {
 skip_ws(s,i);
 json key = parse_internal(s,i);
 if (!key.is_string()) throw std::runtime_error("object key must be string");
 skip_ws(s,i);
 if (s[i] != ':') throw std::runtime_error("expected ':'"); ++i;
 json value = parse_internal(s,i);
 (*obj.object_value)[*key.string_value] = value;
 skip_ws(s,i);
 if (s[i] == ',') { ++i; continue; }
 if (s[i] == '}') { ++i; break; }
 throw std::runtime_error("expected ',' or '}'");
 }
 return obj;
 }
 if (c == '[') {
 ++i; json arr = json::array(); skip_ws(s,i);
 if (s[i] == ']') { ++i; return arr; }
 while (i < s.size()) {
 json val = parse_internal(s,i);
 arr.push_back(val);
 skip_ws(s,i);
 if (s[i] == ',') { ++i; continue; }
 if (s[i] == ']') { ++i; break; }
 throw std::runtime_error("expected ',' or ']'");
 }
 return arr;
 }
 // number
 if (c == '-' || std::isdigit((unsigned char)c)) {
 size_t start = i;
 if (s[i] == '-') ++i;
 while (i < s.size() && std::isdigit((unsigned char)s[i])) ++i;
 if (i < s.size() && s[i] == '.') { ++i; while (i < s.size() && std::isdigit((unsigned char)s[i])) ++i; }
 double val = std::stod(s.substr(start, i-start));
 return json(val);
 }
 throw std::runtime_error(std::string("unexpected token: ") + c);
 }

 void dump_internal(std::ostringstream& ss, int indent, int level) const
 {
 if (is_null()) { ss << "null"; return; }
 if (is_boolean()) { ss << (bool_value ? "true" : "false"); return; }
 if (is_number()) { ss << number_value; return; }
 if (is_string()) {
 ss << '"';
 for (char c : *string_value) {
 if (c == '\n') ss << "\\n";
 else if (c == '\\') ss << "\\\\";
 else if (c == '"') ss << "\\\"";
 else ss << c;
 }
 ss << '"';
 return;
 }
 if (is_array()) {
 ss << '[';
 if (!array_value->empty()) {
 if (indent >=0) ss << '\n';
 for (size_t i =0; i < array_value->size(); ++i) {
 if (indent >=0) ss << std::string((level+1)*indent, ' ');
 (*array_value)[i].dump_internal(ss, indent, level+1);
 if (i +1 < array_value->size()) ss << ',';
 if (indent >=0) ss << '\n';
 }
 if (indent >=0) ss << std::string(level*indent, ' ');
 }
 ss << ']';
 return;
 }
 if (is_object()) {
 ss << '{';
 if (!object_value->empty()) {
 if (indent >=0) ss << '\n';
 size_t idx =0;
 for (auto& kv : *object_value) {
 if (indent >=0) ss << std::string((level+1)*indent, ' ');
 ss << '"' << kv.first << '"' << ':';
 if (indent >=0) ss << ' ';
 kv.second.dump_internal(ss, indent, level+1);
 if (idx +1 < object_value->size()) ss << ',';
 if (indent >=0) ss << '\n';
 ++idx;
 }
 if (indent >=0) ss << std::string(level*indent, ' ');
 }
 ss << '}';
 return;
 }
 }
};

// basic get specializations
template<>
inline std::string json::get<std::string>() const
{
 if (!is_string()) throw std::runtime_error("not a string");
 return *string_value;
}

template<>
inline bool json::get<bool>() const
{
 if (!is_boolean()) throw std::runtime_error("not a bool");
 return bool_value;
}

template<>
inline int json::get<int>() const
{
 if (!is_number()) throw std::runtime_error("not a number");
 return static_cast<int>(number_value);
}

template<>
inline unsigned int json::get<unsigned int>() const
{
    if (!is_number()) throw std::runtime_error("not a number");
    return static_cast<unsigned int>(number_value);
}

template<>
inline double json::get<double>() const
{
 if (!is_number()) throw std::runtime_error("not a number");
 return number_value;
}

} // namespace nlohmann

