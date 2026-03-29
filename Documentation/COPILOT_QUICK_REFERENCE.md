# ⚡ C++14 Quick Reference - Olympe Engine

## 🚫 INTERDICTIONS ABSOLUES
- ❌ `std::optional`, `std::variant`, `std::any`
- ❌ Structured bindings: `auto& [key, value]`
- ❌ `if constexpr`, `std::string_view`
- ❌ JSON: `json["key"].get<T>()` ou `json::array({"a", "b"})`
- ❌ Code hors de `namespace Olympe { }`
- ❌ `localtime()` sans `#ifdef _WIN32`

## ✅ PATTERNS OBLIGATOIRES

### JSON Iteration
```cpp
for (auto it = obj.begin(); it != obj.end(); ++it) {
    std::string key = it.key();
    auto& val = it.value();
}
```

### JSON Access
```cpp
int val = json_get_int(obj, "key", defaultValue);
```

### JSON Array Creation
```cpp
json arr = json::array();
arr.push_back(item);
```

### Platform-Safe Time
```cpp
std::tm info;
#ifdef _WIN32
    localtime_s(&info, &time);
#else
    localtime_r(&time, &info);
#endif
```

### Struct Initialization
```cpp
struct MyStruct {
    int id = 0;
    float ratio = 1.0f;
    bool active = false;
};
```

## 🧪 PRE-COMMIT CHECKLIST
- [ ] Tout dans `namespace Olympe`
- [ ] JSON helpers utilisés
- [ ] Structs initialisées
- [ ] API signatures vérifiées
- [ ] Compilation mentale OK

## 🔧 PROMPT TEMPLATE
"CRITICAL: C++14 strict. Use json_get_*() helpers. All code in namespace Olympe. Declare before use. Initialize all members. Use localtime_s with #ifdef. Verify API signatures. Mental compilation mandatory."
