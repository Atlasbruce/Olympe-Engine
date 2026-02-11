#pragma once

// "system_utils.h" -> simplified logging implementation
// Centralized logger that duplicates output to std::cout, std::clog, to a file (olympe.log)
// and to the UI log window. Use:
// Logging::InitLogger();
// SYSTEM_LOG << "Hello" << std::endl;

#include <streambuf>
#include <ostream>
#include <vector>
#include <memory>
#include <fstream>
#include <mutex>
#include <iostream>
#include <string>
#include <sstream>
#include "system_consts.h"
#include "log_sink.h"

//-------------------------------------------------------------
// Convenience macro (keeps existing code using SYSTEM_LOG)
#define SYSTEM_LOG ::Logging::Logger()

namespace Logging
{
    // Output destinations flags
    enum Outputs : unsigned
    {
        Out_Cout  = 1u << 0,
        Out_Clog  = 1u << 1,
        Out_File  = 1u << 2,
        Out_Panel = 1u << 3
    };

    // Simple line-buffering logger with operator<< overloads
    class Log
    {
    public:
        Log() : outputs_(Out_Cout | Out_Clog | Out_Panel), file_open_(false) {}

        // Initialize file logging (optional). Safe to call multiple times.
        bool Init(const std::string& filename = "olympe.log")
        {
            std::lock_guard<std::mutex> lock(m_);
            if (file_open_) return true;
            file_.open(filename.c_str(), std::ios::app);
            if (file_.is_open()) file_open_ = true;
            // ensure panel sink exists (log_sink handles buffering until UI attached)
            return true;
        }

        void Shutdown()
        {
            std::lock_guard<std::mutex> lock(m_);
            if (file_.is_open()) { file_.flush(); file_.close(); }
            file_open_ = false;
        }

        void SetOutputs(unsigned flags)
        {
            std::lock_guard<std::mutex> lock(m_);
            outputs_ = flags;
        }

        unsigned GetOutputs() const { return outputs_; }

        // Generic stream insertion
        template<typename T>
        auto operator<<(const T& v) -> decltype(std::declval<std::ostringstream&>() << v, std::declval<Log&>())
        {
            {
                std::lock_guard<std::mutex> lock(m_);
                buffer_ << v;
            }
            flushCompleteLinesIfAny();
            return *this;
        }

        // Manipulators like std::endl
        using Manip = std::ostream& (*)(std::ostream&);
        Log& operator<<(Manip m)
        {
            if (m == static_cast<Manip>(&std::endl))
            {
                // endl inserts newline and flushes
                {
                    std::lock_guard<std::mutex> lock(m_);
                    buffer_ << m;
                }
                flushCompleteLinesIfAny();
                flushRemaining();
            }
            else if (m == static_cast<Manip>(&std::flush))
            {
                flushRemaining();
            }
            else
            {
                // other manipulators forward into buffer
                std::lock_guard<std::mutex> lock(m_);
                buffer_ << m;
            }
            return *this;
        }

    private:
        void writeToOutputs(const std::string& s)
        {
            // write to each enabled output (no duplication)
            if (outputs_ & Out_Cout) {
                std::cout << s;
                std::cout.flush();
            }
            /*if (outputs_ & Out_Clog) {
                std::clog << s;
                std::clog.flush();
            }/**/
            if ((outputs_ & Out_File) && file_open_) {
                file_ << s;
                file_.flush();
            }
            if (outputs_ & Out_Panel) {
                // send to UI log sink (thread-safe buffered)
                SystemLogSink::AppendToLogWindow(s);
            }
        }

        void flushCompleteLinesIfAny()
        {
            std::lock_guard<std::mutex> lock(m_);
            std::string s = buffer_.str();
            size_t pos = 0;
            while ((pos = s.find('\n')) != std::string::npos)
            {
                std::string line = s.substr(0, pos + 1);
                writeToOutputs(line);
                s.erase(0, pos + 1);
            }
            buffer_.str(""); buffer_.clear();
            buffer_ << s;
        }

        void flushRemaining()
        {
            std::lock_guard<std::mutex> lock(m_);
            std::string s = buffer_.str();
            if (!s.empty())
            {
                writeToOutputs(s);
                buffer_.str(""); buffer_.clear();
            }
        }

    private:
        mutable std::mutex m_;
        std::ostringstream buffer_;
        unsigned outputs_;
        std::ofstream file_;
        bool file_open_;
    };

    // Access global logger instance
    inline Log& Logger()
    {
        static Log s_logger;
        return s_logger;
    }

    // Convenience init/shutdown
    inline bool InitLogger(const std::string& filename = "olympe.log")
    {
        bool res = Logger().Init(filename);
        // enable file output only if file opened
        if (!Logger().GetOutputs()) {}
        // by default, keep cout, clog and panel enabled; file enabled if opened
        return res;
    }

    inline void ShutdownLogger()
    {
        Logger().Shutdown();
    }
}


//-------------------------------------------------------------
// JSON helpers (kept here for compatibility with existing code)
//
static std::string escape_json_string(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}
//-------------------------------------------------------------
static bool extract_json_string(const std::string& json, const std::string& key, std::string& out)
{
    size_t pos = json.find('"' + key + '"');
    if (pos == std::string::npos) pos = json.find(key);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < json.size() && isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    if (pos < json.size() && json[pos] == '"') ++pos;
    size_t start = pos;
    while (pos < json.size() && json[pos] != '"' && json[pos] != '\n' && json[pos] != '\r') ++pos;
    if (pos <= start) return false;
    out = json.substr(start, pos - start);
    return true;
}
//-------------------------------------------------------------
static bool extract_json_double(const std::string& json, const std::string& key, double& out)
{
    size_t pos = json.find('"' + key + '"');
    if (pos == std::string::npos) pos = json.find(key);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < json.size() && isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    size_t start = pos;
    while (pos < json.size() && (isdigit(static_cast<unsigned char>(json[pos])) || json[pos] == '+' || json[pos] == '-' || json[pos] == '.' || json[pos] == 'e' || json[pos] == 'E')) ++pos;
    if (pos <= start) return false;
    try { out = std::stod(json.substr(start, pos - start)); return true; }
    catch (...) { return false; }
}
//-------------------------------------------------------------
static bool extract_json_int(const std::string& json, const std::string& key, int& out)
{
    double d;
    if (!extract_json_double(json, key, d)) return false;
    out = static_cast<int>(d);
    return true;
}
//-------------------------------------------------------------
static bool extract_json_bool(const std::string& json, const std::string& key, bool& out)
{
    size_t pos = json.find('"' + key + '"');
    if (pos == std::string::npos) pos = json.find(key);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < json.size() && isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    if (json.compare(pos, 4, "true") == 0) { out = true; return true; }
    if (json.compare(pos, 5, "false") == 0) { out = false; return true; }
    return false;
}

//-------------------------------------------------------------
// Load configuration from JSON file (keeps previous behavior)
void LoadOlympeConfig(const char* filename);
//-------------------------------------------------------------
