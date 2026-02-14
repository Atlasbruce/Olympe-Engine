# Copilot Instructions for Olympe Engine

> **Purpose:** This document ensures consistent, compatible code generation for the Olympe Engine project.  
> **Audience:** GitHub Copilot agents, AI assistants, and human contributors.

---

## ⚠️ CRITICAL: Pre-Implementation Checklist

**Before writing ANY code, you MUST:**

- [ ] **1. Verify ImGui/ImNodes versions** by checking `Source/third_party/` directory
- [ ] **2. Confirm C++14 compatibility** (NO C++17/20 features allowed)
- [ ] **3. Check for custom implementations** (JSON, helpers, utilities)
- [ ] **4. Scan existing code patterns** in similar files (use `grep` or code search)

**Golden Rule:**  
> **"Search existing code FIRST, implement SECOND"**

If you cannot verify compatibility, **STOP and ask for clarification** rather than guessing.

---

## 🔧 1. External Libraries - MANDATORY VERIFICATION

### ImGui & ImNodes

**⚠️ CRITICAL:** API availability varies by version. **NEVER assume an API exists.**

#### Verification Process

**Step 1: Check ImGui version**
```bash
grep "IMGUI_VERSION" Source/third_party/imgui/imgui.h
```

**Step 2: Check ImNodes API availability**
```bash
# Search for specific function
grep -r "EditorContextGetZoom\|EditorContextSetZoom" Source/third_party/imnodes/

# List all available ImNodes functions
grep "IMNODES_API" Source/third_party/imnodes/imnodes.h
```

**Step 3: Search for existing usage in codebase**
```bash
# Find what ImNodes APIs are actually used
grep -r "ImNodes::" Source/ | grep -v "third_party" | cut -d: -f2 | sort | uniq

# Example output might show:
#   ImNodes::BeginNodeEditor()
#   ImNodes::EndNodeEditor()
#   ImNodes::SetNodeScreenSpacePos()
#   ImNodes::GetStyle()
```

#### Common ImNodes API Variants

Different versions have different APIs:

```cpp
// ❌ ImNodes v0.5+ (may NOT be available)
ImNodes::EditorContextGetZoom();
ImNodes::EditorContextSetZoom(float);
ImNodes::EditorContextResetPanning(ImVec2);

// ✅ ImNodes v0.4 (commonly available)
ImNodes::SetNodeScreenSpacePos(id, ImVec2);
ImNodes::SetNodeGridSpacePos(id, ImVec2);
ImNodes::GetStyle().NodePadding = ImVec2;
ImNodes::GetStyle().GridSpacing = float;
```

**Before using ANY ImNodes function:**
1. Search for it in `Source/third_party/imnodes/imnodes.h`
2. Search for existing usage: `grep -r "FunctionName" Source/`
3. If not found → Find alternative that IS used

#### Example: Implementing Zoom Feature

```cpp
// ❌ WRONG: Assume v0.5 API exists
float zoom = ImNodes::EditorContextGetZoom();  // May not exist!

// ✅ CORRECT: Check existing code first
// Step 1: grep -r "Zoom" Source/third_party/imnodes/
// Step 2: Not found → Look for alternatives
// Step 3: grep -r "GetStyle\|GridSpacing" Source/AI/
// Step 4: Found pattern → Use style scaling instead

// ✅ Working implementation
static float currentZoom = 1.0f;
ImNodes::GetStyle().GridSpacing = 32.0f * currentZoom;
ImNodes::GetStyle().NodePadding = ImVec2(8.0f, 8.0f) * currentZoom;
```

### SDL Version

Check which SDL version is used:
```bash
ls Source/third_party/ | grep -i sdl
# Verify if SDL2 or SDL3
```

Use corresponding APIs (SDL2 vs SDL3 have different function signatures).

---

## 📐 2. C++14 Standard - STRICT COMPLIANCE

**Compiler:** MSVC with `/std:c++14` flag  
**Standard:** C++14 (ISO/IEC 14882:2014)

### ✅ Allowed C++14 Features

```cpp
// Generic lambdas
auto lambda = [](auto x) { return x * 2; };

// std::make_unique
auto ptr = std::make_unique<MyClass>();

// Binary literals
int binary = 0b1010;

// Return type deduction
auto GetValue() { return 42; }

// Variable templates
template<typename T>
constexpr T pi = T(3.1415926535897932385);
```

### ❌ FORBIDDEN C++17/20 Features

```cpp
// ❌ Structured bindings (C++17)
auto [x, y] = std::make_pair(1, 2);

// ❌ if-init statements (C++17)
if (auto result = GetValue(); result > 0) { }

// ❌ std::optional (C++17)
std::optional<int> value;

// ❌ std::filesystem (C++17)
#include <filesystem>
namespace fs = std::filesystem;

// ❌ Fold expressions (C++17)
template<typename... Args>
auto sum(Args... args) { return (args + ...); }

// ❌ constexpr if (C++17)
if constexpr (sizeof(T) > 4) { }

// ❌ std::string_view (C++17)
std::string_view text;

// ❌ Concepts (C++20)
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

// ❌ Ranges (C++20)
std::ranges::sort(vec);

// ❌ Coroutines (C++20)
co_await something;

// ❌ Modules (C++20)
import std.core;

// ❌ Three-way comparison (C++20)
auto result = a <=> b;
```

### Verification

**Before using ANY standard library feature:**

```bash
# Check C++ standard version
grep "CMAKE_CXX_STANDARD" CMakeLists.txt

# Should show: set(CMAKE_CXX_STANDARD 14)
```

**Online reference:** https://en.cppreference.com/w/cpp/14

---

## 📦 3. JSON Library - Custom Implementation

**Library:** nlohmann/json (header-only)  
**Location:** `Source/third_party/nlohmann/json.hpp`

### Usage Pattern

```cpp
// ✅ CORRECT: Use nlohmann json
#include "third_party/nlohmann/json.hpp"
using json = nlohmann::json;

// Parse JSON
json data = json::parse(fileContent);
std::string name = data["name"];

// Create JSON
json obj;
obj["name"] = "Player";
obj["health"] = 100;
std::string output = obj.dump(4); // Pretty print with 4 spaces
```

### ❌ DON'T Invent Your Own

```cpp
// ❌ WRONG: Custom JSON parser
class MyJSONParser { /* ... */ };

// ❌ WRONG: Assume rapidjson
#include <rapidjson/document.h>

// ❌ WRONG: Assume jsoncpp
#include <json/json.h>
```

### Verification

```bash
# Check JSON library
ls Source/third_party/nlohmann/

# Find usage examples
grep -r "nlohmann::json\|using json =" Source/ | head -10
```

---

## 🎯 4. Existing Code Patterns - ALWAYS COPY FIRST

**Rule:** Find similar code, copy structure, modify data.

### Pattern Discovery Process

**Step 1: Identify similar functionality**
```bash
# Example: Adding a new component
grep -r "class.*Component" Source/ | grep "\.h:"

# Example: Adding a new system
grep -r "class.*System" Source/ | grep "\.h:"
```

**Step 2: Study existing implementation**
```bash
# View similar file
cat Source/Components/SimilarComponent.h

# Search for patterns
grep -r "RegisterComponent\|AddComponent" Source/
```

**Step 3: Copy and adapt**
- Copy file structure
- Copy naming conventions
- Copy registration patterns
- Modify only data/logic specific to new feature

### Common Patterns

#### Component Registration

```cpp
// ✅ Found in existing code
REGISTER_COMPONENT(ComponentName, "ComponentName_data")

// Copy this pattern for new components
REGISTER_COMPONENT(MyNewComponent, "MyNewComponent_data")
```

#### System Initialization

```cpp
// ✅ Found in OlympeEngine.cpp
m_existingSystem = std::make_unique<ExistingSystem>();

// Copy pattern
m_newSystem = std::make_unique<NewSystem>();
```

#### Logging

```cpp
// ✅ Found in existing code
#include "system/system_utils.h"
SYSTEM_LOG("[SystemName] Message\n");

// ❌ DON'T use std::cout
std::cout << "Message" << std::endl;  // Inconsistent!
```

---

## 🏷️ 5. Naming Conventions

### Variables

```cpp
// Class members: m_ prefix
class MyClass {
    int m_health;           // ✅ Member variable
    float m_speed;          // ✅ Member variable
    std::string m_name;     // ✅ Member variable
};

// Local variables: camelCase (NO m_ prefix)
void MyFunction() {
    int playerHealth = 100;     // ✅ Local variable
    float movementSpeed = 5.0f; // ✅ Local variable
    auto entityId = 42;         // ✅ Local variable
}

// Constants: UPPER_SNAKE_CASE or constexpr
const int MAX_ENTITIES = 1000;          // ✅ Constant
constexpr float PI = 3.14159f;          // ✅ Constexpr
static const std::string CONFIG_PATH;   // ✅ Static constant
```

### Functions

```cpp
// PascalCase for public functions
class MyClass {
public:
    void UpdatePosition();      // ✅ Public function
    int GetHealth() const;      // ✅ Public getter
    void SetSpeed(float speed); // ✅ Public setter
    
private:
    void updateInternal();      // ⚠️ Check existing style for private
};
```

### Classes

```cpp
// PascalCase
class PlayerController { };     // ✅ Class name
class BehaviorTree { };         // ✅ Class name
struct TransformData { };       // ✅ Struct name
```

### Files

```cpp
// Match class name
// PlayerController.h / PlayerController.cpp
// BehaviorTree.h / BehaviorTree.cpp
```

---

## 🔍 6. Verification Script Template

Before submitting code, run this checklist:

```bash
#!/bin/bash
# Pre-submission verification

echo "=== 1. Check ImGui/ImNodes APIs ==="
grep -r "ImNodes::\|ImGui::" YourFile.cpp | while read line; do
    func=$(echo "$line" | grep -o "Im[^(]*")
    grep -q "$func" Source/third_party/imgui/imgui.h Source/third_party/imnodes/imnodes.h || \
        echo "⚠️  API not found: $func"
done

echo "=== 2. Check C++17/20 features ==="
grep -E "std::optional|std::filesystem|auto \[.*\]|if constexpr|co_await" YourFile.cpp && \
    echo "❌ C++17/20 feature detected!" || echo "✅ C++14 compliant"

echo "=== 3. Check JSON usage ==="
grep -q "nlohmann::json" YourFile.cpp && \
    echo "✅ Using nlohmann json" || \
    (grep -q "json" YourFile.cpp && echo "⚠️  Check JSON implementation")

echo "=== 4. Check naming conventions ==="
grep -E "^[[:space:]]*[a-z][a-zA-Z0-9]* m_" YourFile.cpp && \
    echo "❌ Local variable with m_ prefix!" || echo "✅ Naming conventions OK"

echo "=== 5. Compilation test ==="
# Add your build command here
# cmake --build build/ --target YourTarget
```

---

## 📋 7. Quick Reference Checklist

### Before Starting

```
[ ] 1. Check ImGui version
    → grep "IMGUI_VERSION" Source/third_party/imgui/imgui.h

[ ] 2. Check C++14 standard
    → grep "CMAKE_CXX_STANDARD" CMakeLists.txt
    → No std::optional, std::filesystem, structured bindings, fold expressions, if-init

[ ] 3. Search for custom implementations
    → ls Source/JSON/
    → ls Source/Helpers/
    → grep -r "class.*Helper\|namespace.*Util" Source/

[ ] 4. Find existing patterns for similar features
    → grep -r "similar_pattern" Source/
    → Copy pattern, adapt data

[ ] 5. Add debug logs
    → std::cout << "[SystemName] Message" << std::endl;

[ ] 6. Test compilation (C++14)

[ ] 7. Test runtime behavior

[ ] 8. Verify logs match expected output
```

### During Implementation

```
[ ] Use m_ prefix for class members
[ ] Use PascalCase for functions
[ ] Use camelCase for local variables (NO m_ prefix)
[ ] Add [SystemName] prefix to logs
[ ] Include relevant headers
[ ] Follow existing file structure
[ ] Document assumptions with comments
```

### Before Submitting PR

```
[ ] Code compiles with 0 errors, 0 warnings
[ ] All APIs verified in source code
[ ] C++14 compliant (no modern features)
[ ] Follows existing patterns
[ ] Debug logs included
[ ] Runtime tested
[ ] Expected logs verified
[ ] No regressions
```

---

## 🎯 Summary

### The Golden Rules

1. **VERIFY FIRST, CODE SECOND**
   - Check versions, APIs, patterns BEFORE writing

2. **NEVER ASSUME**
   - If it's not in the source code, it doesn't exist

3. **COPY, DON'T INVENT**
   - Find existing patterns, copy structure, adapt data

4. **C++14 ONLY**
   - No modern features without explicit approval

5. **LOG EVERYTHING**
   - Debug logs save hours of debugging time

### When in Doubt

1. Search the codebase: `grep -r "pattern" Source/`
2. Check third_party APIs: `grep "function" Source/third_party/`
3. Ask for clarification (don't guess)

### Remember

> **"The best code is the code that already works"**

Follow existing patterns. Verify APIs. Stay compatible. Log generously.

---

*Last updated: 2025-02-14*
*For questions or updates, consult project maintainers.*
