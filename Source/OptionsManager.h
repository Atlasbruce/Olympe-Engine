#pragma once
#include "object.h"
#include "system/system_utils.h"

// use plain enum with bit ops (C++14)
enum class OptionFlags : uint64_t
{
    None = 0,
    ShowDebugInfo = 1u << 0,
    ShowBoundingBox = 1u << 1,
    LogToFile = 1u << 2,
    ShowMessages = 1u << 3,
    // ajouter d'autres flags...
};

inline OptionFlags operator|(OptionFlags a, OptionFlags b) {
    return static_cast<OptionFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}
inline OptionFlags operator&(OptionFlags a, OptionFlags b) {
    return static_cast<OptionFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}
inline OptionFlags& operator|=(OptionFlags& a, OptionFlags b) { a = a | b; return a; }
inline OptionFlags& operator&=(OptionFlags& a, OptionFlags b) { a = static_cast<OptionFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b)); return a; }


class OptionsManager 
{
public:
    OptionsManager()
    {
        name = "OptionsManager";
        Set(OptionFlags::ShowDebugInfo); // default option
        SYSTEM_LOG << "OptionsManager created and Initialized\n";
    }
    virtual ~OptionsManager()
    {
        SYSTEM_LOG << "OptionsManager destroyed\n";
	}

    static OptionsManager& GetInstance()
    {
        static OptionsManager instance;
        return instance;
    }
    static OptionsManager& Get() { return GetInstance(); }

    // Flags manipulation
    void Set(OptionFlags f) { flags_ |= f; }
    void Clear(OptionFlags f) { flags_ &= static_cast<OptionFlags>(~static_cast<uint64_t>(f)); }
    void Toggle(OptionFlags f)
    {
        if (IsSet(f)) {
            Clear(f);
        }
        else {
            Set(f);
        }
    }
    bool IsSet(OptionFlags f) const { return (flags_ & f) != OptionFlags::None; }


private:
    string name;
    OptionFlags flags_ = OptionFlags::None;
};