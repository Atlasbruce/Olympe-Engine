---
id: coding-standards
title: "C++14 Coding Standards"
sidebar_label: "Coding Standards"
sidebar_position: 2
---

# C++14 Coding Standards

This project strictly adheres to **C++14** for maximum compatibility across platforms (Windows, Linux, macOS).

## Quick Links

- [Full Standards (Source)](https://github.com/Atlasbruce/Olympe-Engine/blob/master/Source/COPILOT_CODING_RULES.md) - Complete comprehensive reference
- [Quick Reference](https://github.com/Atlasbruce/Olympe-Engine/blob/master/Source/COPILOT_QUICK_REFERENCE.md) - One-page quick lookup

:::warning Required Reading
**All contributors must follow these standards** to ensure code compiles correctly and avoids common errors.
:::

## Core Principles

### 1. C++14 Strict Compliance

Olympe Engine uses **C++14 as the maximum standard**. This ensures compatibility across all supported platforms and compilers.

#### ❌ Prohibited (C++17/20 Features)

- `std::optional`, `std::variant`, `std::any`
- Structured bindings: `auto& [key, value]`
- `if constexpr`
- `std::string_view`
- Fold expressions: `(... + args)`
- Inline variables
- C++17 attributes: `[[nodiscard]]`, `[[maybe_unused]]`
- Init-statements in if/switch: `if (auto x = foo(); x > 0)`
- Class template argument deduction (CTAD)

#### ✅ Allowed (C++14 Features)

- `auto` for type inference
- Lambda expressions with captures: `[&]`, `[=]`, `[this]`
- Range-based for loops: `for (const auto& item : container)`
- `constexpr` functions/variables
- `nullptr`
- `enum class`
- `override`, `final`
- Uniform initialization: `MyStruct{1, 2, 3}`
- Move semantics: `std::move()`, `&&`

### 2. Custom JSON Helpers

The project uses custom JSON helper functions to safely access JSON data with default values.

#### ❌ Don't Use Direct Access
```cpp
// WRONG - Direct access can throw exceptions
int value = json["key"].get<int>();

// WRONG - json::array with initializers (C++17 feature)
json arr = json::array({"a", "b"});
```

#### ✅ Use Helper Functions
```cpp
// CORRECT - Safe access with default value
int value = json_get_int(json, "key", 0);
float ratio = json_get_float(json, "ratio", 1.0f);
std::string name = json_get_string(json, "name", "default");
bool active = json_get_bool(json, "active", false);

// CORRECT - Create empty array first
json arr = json::array();
arr.push_back("a");
arr.push_back("b");
```

#### Iterating JSON Objects (C++14 Way)
```cpp
// ✅ CORRECT - C++14 compatible iteration
for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it) {
    std::string key = it.key();
    const auto& value = it.value();
    // Process key and value
}

// ❌ WRONG - C++17 structured binding
for (auto& [key, value] : jsonObj.items()) {
    // This won't compile in C++14!
}
```

**Rationale**: These helpers prevent exceptions from missing keys and provide consistent default value handling across the codebase.

### 3. Namespace Requirements

**ALL** code must be inside the `Olympe` namespace. No exceptions.

#### ✅ Correct Structure
```cpp
#include "Header.h"

using json = nlohmann::json;  // OK before namespace

namespace Olympe
{
    constexpr float MAX_VALUE = 10.0f;
    
    void MyClass::MyMethod() {
        // Implementation
    }
    
} // namespace Olympe
```

#### ❌ Common Mistake
```cpp
} // namespace Olympe

// WRONG - Orphaned code outside namespace!
void HelperFunction() {
    // This will cause linking issues
}
```

### 4. Platform Compatibility

#### Cross-Platform Time Functions

Windows and Linux have different time functions. Always use platform-specific versions:

```cpp
// ❌ WRONG - Causes warnings on Windows
std::tm* timeInfo = std::localtime(&time);

// ✅ CORRECT - Platform-specific
std::tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &time);
#else
    localtime_r(&time, &timeInfo);
#endif
```

#### Safe String Functions

```cpp
// ❌ Avoid unsafe C functions
strcpy(dest, src);
sprintf(buffer, "%s", text);

// ✅ Use safe alternatives
strcpy_s(dest, size, src);
snprintf(buffer, size, "%s", text);

// ✅ Or prefer C++ strings
std::string dest = src;
```

### 5. Struct Initialization

Always provide default values for all struct members to prevent undefined behavior.

#### ✅ Correct
```cpp
struct PlayerData {
    int health = 100;
    float speed = 5.0f;
    bool isAlive = true;
    std::string name;          // OK - std::string has default constructor
    std::vector<int> inventory; // OK - std::vector has default constructor
    Weapon* weapon = nullptr;
};
```

#### ❌ Wrong
```cpp
struct PlayerData {
    int health;        // Uninitialized!
    float speed;       // Uninitialized!
    bool isAlive;      // Uninitialized!
};
```

### 6. Third-Party API Usage

Always verify API signatures before using third-party libraries (ImGui, ImNodes, SDL3).

#### Common Pitfalls
```cpp
// ✅ CORRECT - ImGui::GetWindowDrawList() returns pointer
ImDrawList* drawList = ImGui::GetWindowDrawList();
if (drawList != nullptr) {
    drawList->AddLine(...);
}

// ✅ CORRECT - ImNodes::Link() takes 3 arguments
ImNodes::Link(linkId, startPinId, endPinId);

// ✅ CORRECT - json::array() with NO arguments
json arr = json::array();
```

## Pre-Commit Checklist

Before submitting code, verify:

- [ ] No C++17/20 features used
- [ ] All code inside `namespace Olympe { }`
- [ ] All JSON access uses `json_get_*()` helpers
- [ ] `json::array()` and `json::object()` called without arguments
- [ ] JSON iteration uses `.begin()/.end()` with `it.key()/it.value()`
- [ ] Platform-specific functions wrapped with `#ifdef _WIN32`
- [ ] All struct members have default values
- [ ] All member variables declared in header
- [ ] All declared functions are implemented
- [ ] Third-party API signatures verified
- [ ] All pointers checked before use
- [ ] Explicit casts used for type conversions
- [ ] Namespace closed with comment
- [ ] Includes in correct order
- [ ] Forward declarations used where possible

## Common Error Patterns

### Error 1: Orphaned Code Outside Namespace
```cpp
// ❌ WRONG
} // namespace Olympe

void MyFunction() { }  // Linker error!
```

### Error 2: C++17 Structured Bindings
```cpp
// ❌ WRONG
for (auto& [k, v] : map) { }

// ✅ CORRECT
for (auto it = map.begin(); it != map.end(); ++it) {
    auto& k = it->first;
    auto& v = it->second;
}
```

### Error 3: JSON Array with Initializers
```cpp
// ❌ WRONG
json arr = json::array({"a", "b"});

// ✅ CORRECT
json arr = json::array();
arr.push_back("a");
arr.push_back("b");
```

### Error 4: Undeclared Member Variables
```cpp
// ❌ WRONG in .cpp
m_myVar = 5;  // Not declared in header!

// ✅ CORRECT: Add to header
class MyClass {
    int m_myVar = 0;
};
```

## Code Validation Template

Use this mental checklist when writing code:

```
✅ C++14 STRICT
- No std::optional, std::variant, structured binding
- Classic iterators with .begin()/.end()
- No if constexpr, std::string_view

✅ JSON HELPERS
- All access via json_get_*()
- json::array() without arguments
- Iteration with it.key()/it.value()

✅ NAMESPACE
- 100% of code in namespace Olympe
- Closed with comment
- No orphaned code

✅ DECLARATIONS
- All member variables in header
- All functions implemented
- Structs with default values

✅ PLATFORM
- localtime_s() with #ifdef _WIN32
- Explicit casts
- Pointers verified

✅ THIRD-PARTY APIs
- Signatures verified
- Argument count correct
- Return types respected

✅ MENTAL COMPILATION
- Code reviewed line by line
- 0 predictable errors
- 0 predictable warnings
```

## Quick Prompt Template

When working with AI coding assistants, use this prompt:

> "CRITICAL: C++14 strict. Use json_get_*() helpers. All code in namespace Olympe. Declare before use. Initialize all members. Use localtime_s with #ifdef. Verify API signatures. Mental compilation mandatory."

## Additional Resources

- [General Code Style Guide](./code-style.md) - Naming conventions, formatting, documentation
- [Testing Guide](./testing-guide.md) - How to test your contributions
- [Adding Components](./adding-components.md) - Guide to adding new ECS components

## Why These Rules?

These standards exist because:

1. **Compilation Errors**: Past PRs had C++17 features that failed to compile on C++14 compilers
2. **Cross-Platform Issues**: Platform-specific code needs proper guards
3. **JSON Crashes**: Direct JSON access caused runtime exceptions from missing keys
4. **Linker Errors**: Code outside namespace caused undefined reference errors
5. **Warnings**: Uninitialized members and unsafe functions generate compiler warnings

Following these rules ensures your code compiles cleanly on all platforms without warnings or errors.

---

**Remember**: If in doubt, check the [full standards document](https://github.com/Atlasbruce/Olympe-Engine/blob/master/Source/COPILOT_CODING_RULES.md) or ask in your pull request!
