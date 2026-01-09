// Minimal and custom subset of nlohmann::json functionality for this project
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
#include <initializer_list>

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

        // Initializer list constructor for objects (key-value pairs)
        json(std::initializer_list<std::pair<std::string, json>> init)
            : type_(Type::Object), number_value(0.0)
        {
            object_value = new std::map<std::string, json>();
            for (const auto& pair : init)
            {
                (*object_value)[pair.first] = pair.second;
            }
        }

        // Initializer list constructor for arrays (implicit when all same type)
        json(std::initializer_list<json> init)
            : type_(Type::Array), number_value(0.0)
        {
            array_value = new std::vector<json>();
            for (const auto& item : init)
            {
                array_value->push_back(item);
            }
        }

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
        
        // Assignment from initializer list (object)
        json& operator=(std::initializer_list<std::pair<std::string, json>> init)
        {
            clear();
            type_ = Type::Object;
            object_value = new std::map<std::string, json>();
            for (const auto& pair : init)
            {
                (*object_value)[pair.first] = pair.second;
            }
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

        // is_number_integer() check
        bool is_number_integer() const { return is_number(); }
        
        // is_number_float() check
        bool is_number_float() const { return is_number(); }

        // Comparison operator for strings
        bool operator==(const std::string& str) const
        {
            if (!is_string()) return false;
            return *string_value == str;
        }

        bool operator==(const char* str) const
        {
            if (!is_string()) return false;
            return *string_value == std::string(str);
        }
        
        // Comparison operator for json objects
        bool operator==(const json& other) const
        {
            if (type_ != other.type_) return false;
            
            switch (type_)
            {
                case Type::Null:
                    return true;
                case Type::Boolean:
                    return bool_value == other.bool_value;
                case Type::Number:
                    return number_value == other.number_value;
                case Type::String:
                    return *string_value == *other.string_value;
                case Type::Array:
                    if (array_value->size() != other.array_value->size()) return false;
                    for (size_t i = 0; i < array_value->size(); ++i)
                    {
                        if (!((*array_value)[i] == (*other.array_value)[i])) return false;
                    }
                    return true;
                case Type::Object:
                    if (object_value->size() != other.object_value->size()) return false;
                    for (const auto& kv : *object_value)
                    {
                        auto it = other.object_value->find(kv.first);
                        if (it == other.object_value->end()) return false;
                        if (!(kv.second == it->second)) return false;
                    }
                    return true;
                default:
                    return false;
            }
        }
        
        bool operator!=(const json& other) const
        {
            return !(*this == other);
        }

        // items() method for iterating over object key-value pairs
        // Returns a vector of pairs since we can't create a proper iterator range easily
        std::vector<std::pair<std::string, json*>> items()
        {
            std::vector<std::pair<std::string, json*>> result;
            if (!is_object()) return result;
            for (auto& kv : *object_value)
            {
                result.push_back(std::make_pair(kv.first, &kv.second));
            }
            return result;
        }

        std::vector<std::pair<std::string, const json*>> items() const
        {
            std::vector<std::pair<std::string, const json*>> result;
            if (!is_object()) return result;
            for (const auto& kv : *object_value)
            {
                result.push_back(std::make_pair(kv.first, &kv.second));
            }
            return result;
        }

        // erase method for arrays (by iterator-like index)
        void erase(size_t index)
        {
            if (!is_array()) throw std::runtime_error("not an array");
            if (index >= array_value->size()) throw std::out_of_range("index out of range");
            array_value->erase(array_value->begin() + index);
        }

        // value() with default - returns value if key exists, otherwise returns default
        template<typename T>
        T value(const std::string& key, const T& default_value) const
        {
            if (!is_object()) return default_value;
            auto it = object_value->find(key);
            if (it == object_value->end()) return default_value;
            try {
                return it->second.get<T>();
            } catch (...) {
                return default_value;
            }
        }

        // Specialization for const char* to return std::string
        std::string value(const std::string& key, const char* default_value) const
        {
            return value<std::string>(key, std::string(default_value));
        }

        // getters
        template<typename T>
        T get() const;

        template<typename T>
        void get_to(T& out) const { out = get<T>(); }

        std::string dump(int indent = -1) const
        {
            std::ostringstream ss;
            dump_internal(ss, indent, 0);
            return ss.str();
        }

        // parse from string
        static json parse(const std::string& s)
        {
            size_t idx = 0;
            return parse_internal(s, idx);
        }

        // Iterators for objects
        using object_iterator = std::map<std::string, json>::iterator;
        using const_object_iterator = std::map<std::string, json>::const_iterator;

        object_iterator begin()
        {
            if (!is_object()) throw std::runtime_error("not an object");
            return object_value->begin();
        }

        object_iterator end()
        {
            if (!is_object()) throw std::runtime_error("not an object");
            return object_value->end();
        }

        const_object_iterator begin() const
        {
            if (!is_object()) throw std::runtime_error("not an object");
            return object_value->begin();
        }

        const_object_iterator end() const
        {
            if (!is_object()) throw std::runtime_error("not an object");
            return object_value->end();
        }

        // at() method for safe access with exception if key absent
        const json& at(const std::string& key) const
        {
            if (!is_object()) throw std::runtime_error("not an object");
            auto it = object_value->find(key);
            if (it == object_value->end()) throw std::out_of_range("key not found: " + key);
            return it->second;
        }

        json& at(const std::string& key)
        {
            if (!is_object()) throw std::runtime_error("not an object");
            auto it = object_value->find(key);
            if (it == object_value->end()) throw std::out_of_range("key not found: " + key);
            return it->second;
        }

        // count() method to check if key exists
        size_t count(const std::string& key) const
        {
            if (!is_object()) return 0;
            return object_value->count(key);
        }

        // empty() method to check if array or object is empty
        bool empty() const
        {
            if (is_array()) return array_value->empty();
            if (is_object()) return object_value->empty();
            return true;
        }

        // Array iterator support
        using array_iterator = std::vector<json>::iterator;
        using const_array_iterator = std::vector<json>::const_iterator;

        array_iterator array_begin()
        {
            if (!is_array()) throw std::runtime_error("not an array");
            return array_value->begin();
        }

        array_iterator array_end()
        {
            if (!is_array()) throw std::runtime_error("not an array");
            return array_value->end();
        }

        const_array_iterator array_begin() const
        {
            if (!is_array()) throw std::runtime_error("not an array");
            return array_value->begin();
        }

        const_array_iterator array_end() const
        {
            if (!is_array()) throw std::runtime_error("not an array");
            return array_value->end();
        }

    private:
        Type type_;
        bool bool_value = false;
        double number_value = 0.0;
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
                if (s.compare(i, 4, "null") == 0) { i += 4; return json(); }
            }
            if (c == 't') { if (s.compare(i, 4, "true") == 0) { i += 4; return json(true); } }
            if (c == 'f') { if (s.compare(i, 5, "false") == 0) { i += 5; return json(false); } }
            if (c == '"') { // string
                ++i; std::ostringstream ss;
                while (i < s.size()) {
                    char ch = s[i++];
                    if (ch == '"') break;
                    if (ch == '\\') {
                        if (i >= s.size()) break;
                        char esc = s[i++];
                        if (esc == 'n') ss << '\n'; else if (esc == 't') ss << '\t'; else if (esc == 'r') ss << '\r'; else ss << esc;
                    }
                    else ss << ch;
                }
                return json(ss.str());
            }
            if (c == '{') {
                ++i; json obj = json::object(); skip_ws(s, i);
                if (s[i] == '}') { ++i; return obj; }
                while (i < s.size()) {
                    skip_ws(s, i);
                    json key = parse_internal(s, i);
                    if (!key.is_string()) throw std::runtime_error("object key must be string");
                    skip_ws(s, i);
                    if (s[i] != ':') throw std::runtime_error("expected ':'"); ++i;
                    json value = parse_internal(s, i);
                    (*obj.object_value)[*key.string_value] = value;
                    skip_ws(s, i);
                    if (s[i] == ',') { ++i; continue; }
                    if (s[i] == '}') { ++i; break; }
                    throw std::runtime_error("expected ',' or '}'");
                }
                return obj;
            }
            if (c == '[') {
                ++i; json arr = json::array(); skip_ws(s, i);
                if (s[i] == ']') { ++i; return arr; }
                while (i < s.size()) {
                    json val = parse_internal(s, i);
                    arr.push_back(val);
                    skip_ws(s, i);
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
                double val = std::stod(s.substr(start, i - start));
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
                    if (indent >= 0) ss << '\n';
                    for (size_t i = 0; i < array_value->size(); ++i) {
                        if (indent >= 0) ss << std::string((level + 1) * indent, ' ');
                        (*array_value)[i].dump_internal(ss, indent, level + 1);
                        if (i + 1 < array_value->size()) ss << ',';
                        if (indent >= 0) ss << '\n';
                    }
                    if (indent >= 0) ss << std::string(level * indent, ' ');
                }
                ss << ']';
                return;
            }
            if (is_object()) {
                ss << '{';
                if (!object_value->empty()) {
                    if (indent >= 0) ss << '\n';
                    size_t idx = 0;
                    for (auto& kv : *object_value) {
                        if (indent >= 0) ss << std::string((level + 1) * indent, ' ');
                        ss << '"' << kv.first << '"' << ':';
                        if (indent >= 0) ss << ' ';
                        kv.second.dump_internal(ss, indent, level + 1);
                        if (idx + 1 < object_value->size()) ss << ',';
                        if (indent >= 0) ss << '\n';
                        ++idx;
                    }
                    if (indent >= 0) ss << std::string(level * indent, ' ');
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

    template<>
    inline float json::get<float>() const
    {
        if (!is_number()) throw std::runtime_error("not a number");
        return static_cast<float>(number_value);
    }

    // Template specialization for std::vector<int>
    template<>
    inline std::vector<int> json::get<std::vector<int>>() const
    {
        if (!is_array()) throw std::runtime_error("not an array");
        std::vector<int> result;
        for (size_t i = 0; i < array_value->size(); ++i)
        {
            result.push_back((*array_value)[i].get<int>());
        }
        return result;
    }

    // Template specialization for std::vector<std::string>
    template<>
    inline std::vector<std::string> json::get<std::vector<std::string>>() const
    {
        if (!is_array()) throw std::runtime_error("not an array");
        std::vector<std::string> result;
        for (size_t i = 0; i < array_value->size(); ++i)
        {
            result.push_back((*array_value)[i].get<std::string>());
        }
        return result;
    }

} // namespace nlohmann