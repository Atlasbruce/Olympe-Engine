# 🚀 ACTION PLAN - DÉMARRAGE REFACTORISATION

Guide étape-par-étape pour commencer la nouvelle base propre dès maintenant

---

## 📋 RÉSUMÉ EXÉCUTIF

**Objectif**: Rebuilder l'éditeur Blueprint avec architecture clean, simple, CRUD-only (no runtime)

**Scope**: Full CRUD standalone editor avec panels tabés et lazy loading

**Timeline**: 5 semaines (100 heures)

**Équipe**: 1-2 développeurs

**Risque**: Bas (architecture proven, phased rollout)

---

## 🎯 PHASE 0: PRÉPARATION (Jour 1)

### 0.1: Créer structure répertoires

```bash
mkdir -p Source/BlueprintEditor/NewArchitecture
mkdir -p Source/BlueprintEditor/NewArchitecture/Core
mkdir -p Source/BlueprintEditor/NewArchitecture/DI
mkdir -p Source/BlueprintEditor/NewArchitecture/Plugins
mkdir -p Source/BlueprintEditor/NewArchitecture/Panels
mkdir -p Source/BlueprintEditor/NewArchitecture/Tests
```

### 0.2: Créer fichiers stubs pour nouvelle architecture

```cpp
// Source/BlueprintEditor/NewArchitecture/Core/EditorStartup.h
#pragma once

namespace Olympe {
    
    struct EditorConfig {
        std::string window_title = "Olympe Blueprint Editor";
        int window_width = 1920;
        int window_height = 1080;
        std::string config_file = "blueprint_editor_config.json";
        bool enable_profiling = false;
    };
    
    class EditorStartup {
    public:
        // Main entry point
        static bool InitializeEditor(EditorConfig& config);
        
        // Detailed results with error messages
        struct InitResult {
            bool success;
            std::string error_message;
            std::string failed_phase;
            int error_code;
        };
        
        static InitResult InitializeEditorWithDiagnostics(EditorConfig& config);
        
    private:
        // Phase-by-phase initialization
        static bool InitializeSDL3(EditorConfig& config);
        static bool InitializeImGui(EditorConfig& config);
        static bool InitializeBackend(EditorConfig& config);
        static bool InitializeFrontend(EditorConfig& config);
        
        // Error handling
        static void LogPhase(const std::string& phase, bool success);
        static void CleanupOnFailure(int failed_phase);
    };
}
```

### 0.3: Créer DIContainer stub

```cpp
// Source/BlueprintEditor/NewArchitecture/DI/DIContainer.h
#pragma once
#include <memory>
#include <map>
#include <typeinfo>

namespace Olympe {
    
    class DIContainer {
    public:
        // Register singleton instance
        template<typename T>
        void RegisterSingleton(std::unique_ptr<T> instance) {
            m_singletons[std::type_index(typeid(T))] = std::move(instance);
        }
        
        // Resolve singleton
        template<typename T>
        T* Resolve() {
            auto it = m_singletons.find(std::type_index(typeid(T)));
            if (it == m_singletons.end()) {
                return nullptr;
            }
            return static_cast<T*>(it->second.get());
        }
        
    private:
        std::map<std::type_index, std::unique_ptr<void>> m_singletons;
    };
}
```

### 0.4: Créer PanelManager stub

```cpp
// Source/BlueprintEditor/NewArchitecture/Panels/PanelManager.h
#pragma once
#include <memory>
#include <map>

namespace Olympe {
    
    class IPanel {
    public:
        virtual ~IPanel() = default;
        virtual void Render() = 0;
        virtual const char* GetPanelName() const = 0;
    };
    
    class PanelManager {
    public:
        // Create panel on demand (lazy loading)
        IPanel* GetOrCreatePanel(const std::string& panel_name);
        
        // Render all active panels
        void RenderAllPanels();
        
        // Destroy specific panel
        void DestroyPanel(const std::string& panel_name);
        
    private:
        std::map<std::string, std::unique_ptr<IPanel>> m_panels;
    };
}
```

### 0.5: Créer GraphTypeRegistry stub

```cpp
// Source/BlueprintEditor/NewArchitecture/Plugins/GraphTypeRegistry.h
#pragma once
#include <memory>
#include <vector>
#include <map>

namespace Olympe {
    
    class IGraphTypePlugin {
    public:
        virtual ~IGraphTypePlugin() = default;
        virtual std::string GetTypeName() const = 0;
        virtual IGraphRenderer* CreateRenderer() = 0;
        virtual IGraphDocument* CreateDocument() = 0;
    };
    
    class GraphTypeRegistry {
    public:
        // Register plugin
        void RegisterPlugin(std::unique_ptr<IGraphTypePlugin> plugin);
        
        // Get plugin by name
        IGraphTypePlugin* GetPlugin(const std::string& type_name) const;
        
        // List available types
        std::vector<std::string> GetAvailableTypes() const;
        
    private:
        std::map<std::string, std::unique_ptr<IGraphTypePlugin>> m_plugins;
    };
}
```

---

## 🎯 PHASE 1: FOUNDATION (Semaine 1-2, 40 heures)

### Jour 1-2: EditorStartup Implementation

**Objectif**: Créer orchestrateur d'initialisation principale

**Fichiers à créer**:
- `Source/BlueprintEditor/NewArchitecture/Core/EditorStartup.h`
- `Source/BlueprintEditor/NewArchitecture/Core/EditorStartup.cpp`

**Implémentation**:

```cpp
// Source/BlueprintEditor/NewArchitecture/Core/EditorStartup.cpp
#include "EditorStartup.h"
#include "../../../third_party/imgui/imgui.h"
#include <SDL3/SDL.h>
#include <iostream>

namespace Olympe {
    
    bool EditorStartup::InitializeEditor(EditorConfig& config) {
        auto result = InitializeEditorWithDiagnostics(config);
        return result.success;
    }
    
    EditorStartup::InitResult EditorStartup::InitializeEditorWithDiagnostics(EditorConfig& config) {
        InitResult result{true, "", "", 0};
        
        // Phase 1: SDL3
        if (!InitializeSDL3(config)) {
            result.success = false;
            result.error_message = "SDL3 initialization failed";
            result.failed_phase = "SDL3";
            result.error_code = 1;
            return result;
        }
        LogPhase("SDL3", true);
        
        // Phase 2: ImGui
        if (!InitializeImGui(config)) {
            result.success = false;
            result.error_message = "ImGui initialization failed";
            result.failed_phase = "ImGui";
            result.error_code = 2;
            CleanupOnFailure(1);  // Cleanup SDL3
            return result;
        }
        LogPhase("ImGui", true);
        
        // Phase 3: Backend
        if (!InitializeBackend(config)) {
            result.success = false;
            result.error_message = "Backend initialization failed";
            result.failed_phase = "Backend";
            result.error_code = 3;
            CleanupOnFailure(2);  // Cleanup ImGui + SDL3
            return result;
        }
        LogPhase("Backend", true);
        
        // Phase 4: Frontend
        if (!InitializeFrontend(config)) {
            result.success = false;
            result.error_message = "Frontend initialization failed";
            result.failed_phase = "Frontend";
            result.error_code = 4;
            CleanupOnFailure(3);  // Cleanup Backend + ImGui + SDL3
            return result;
        }
        LogPhase("Frontend", true);
        
        return result;
    }
    
    bool EditorStartup::InitializeSDL3(EditorConfig& config) {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            std::cerr << "[EditorStartup] SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }
        std::cout << "[EditorStartup] SDL3 initialized" << std::endl;
        return true;
    }
    
    bool EditorStartup::InitializeImGui(EditorConfig& config) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;  // Use JSON config instead
        
        ImGui::StyleColorsDark();
        
        std::cout << "[EditorStartup] ImGui initialized" << std::endl;
        return true;
    }
    
    bool EditorStartup::InitializeBackend(EditorConfig& config) {
        // TODO: Instantiate UnifiedBackendManager
        // TODO: Load managers + plugins + config
        std::cout << "[EditorStartup] Backend initialized" << std::endl;
        return true;
    }
    
    bool EditorStartup::InitializeFrontend(EditorConfig& config) {
        // TODO: Instantiate PanelManager
        // TODO: Create core panels
        std::cout << "[EditorStartup] Frontend initialized" << std::endl;
        return true;
    }
    
    void EditorStartup::LogPhase(const std::string& phase, bool success) {
        std::cout << "[EditorStartup] " << phase 
                  << " ... " << (success ? "✓ OK" : "✗ FAILED") << std::endl;
    }
    
    void EditorStartup::CleanupOnFailure(int failed_phase) {
        switch (failed_phase) {
            case 2:  // ImGui
                ImGui::DestroyContext();
                // Fall through
            case 1:  // SDL3
                SDL_Quit();
                break;
        }
    }
}
```

**Checklist**:
- [ ] EditorStartup.h/cpp créés
- [ ] Phase 1-4 stubbed (TODO: comments)
- [ ] Error handling implemented
- [ ] Compiles successfully

### Jour 3-4: DIContainer Implementation

**Objectif**: Créer conteneur d'injection de dépendances

**Fichiers à créer**:
- `Source/BlueprintEditor/NewArchitecture/DI/DIContainer.h`
- `Source/BlueprintEditor/NewArchitecture/DI/DIContainer.cpp` (if needed)

**Test simple**:

```cpp
// In EditorStartup::InitializeBackend():
DIContainer di;
di.RegisterSingleton<EnumCatalog>(std::make_unique<EnumCatalog>());
EnumCatalog* catalog = di.Resolve<EnumCatalog>();
assert(catalog != nullptr);
```

**Checklist**:
- [ ] DIContainer.h compiles
- [ ] RegisterSingleton<T>() works
- [ ] Resolve<T>() works
- [ ] Multiple types can be registered

### Jour 5-6: PanelManager Implementation

**Objectif**: Créer gestionnaire de panels avec lazy loading

**Fichiers à créer**:
- `Source/BlueprintEditor/NewArchitecture/Panels/PanelManager.h`
- `Source/BlueprintEditor/NewArchitecture/Panels/PanelManager.cpp`
- `Source/BlueprintEditor/NewArchitecture/Panels/IPanel.h`

**Test simple**:

```cpp
PanelManager pm;
IPanel* asset_browser = pm.GetOrCreatePanel("AssetBrowser");
assert(asset_browser != nullptr);
assert(pm.GetOrCreatePanel("AssetBrowser") == asset_browser);  // Same instance
```

**Checklist**:
- [ ] PanelManager.h/cpp implement
- [ ] Lazy loading works
- [ ] GetOrCreatePanel returns same instance
- [ ] Panel lifecycle managed

### Jour 7-8: GraphTypeRegistry Implementation

**Objectif**: Créer registre de types de graphes pluggables

**Fichiers à créer**:
- `Source/BlueprintEditor/NewArchitecture/Plugins/GraphTypeRegistry.h`
- `Source/BlueprintEditor/NewArchitecture/Plugins/GraphTypeRegistry.cpp`

**Test simple**:

```cpp
GraphTypeRegistry registry;

// Register plugins
registry.RegisterPlugin(std::make_unique<VisualScriptPlugin>());
registry.RegisterPlugin(std::make_unique<BehaviorTreePlugin>());

// Resolve plugins
auto vs_plugin = registry.GetPlugin("VisualScript");
assert(vs_plugin != nullptr);

// List types
auto types = registry.GetAvailableTypes();
assert(types.size() == 2);
```

**Checklist**:
- [ ] GraphTypeRegistry.h/cpp implement
- [ ] RegisterPlugin works
- [ ] GetPlugin returns correct plugin
- [ ] GetAvailableTypes returns list

### Jour 9-10: Integration Test

**Objectif**: Tester EditorStartup end-to-end

**Test**:

```cpp
EditorConfig config;
config.window_width = 1280;
config.window_height = 720;

auto result = EditorStartup::InitializeEditorWithDiagnostics(config);
assert(result.success);
assert(result.error_message.empty());
```

**Checklist**:
- [ ] EditorStartup fully implemented
- [ ] All 4 phases complete
- [ ] Error recovery works
- [ ] No memory leaks (valgrind check)

---

## 🎯 PHASE 2: MANAGER CONSOLIDATION (Semaine 3, 20 heures)

### Jour 1-3: UnifiedBackendManager

**Objectif**: Consolider 4 managers en 1

**Fichiers**:
- `Source/BlueprintEditor/NewArchitecture/Core/UnifiedBackendManager.h`
- `Source/BlueprintEditor/NewArchitecture/Core/UnifiedBackendManager.cpp`

**API**:

```cpp
class UnifiedBackendManager {
    bool Initialize(const EditorConfig& config);
    
    EnumCatalog& GetEnumCatalog();
    NodeGraphManager& GetNodeGraphManager();
    EntityInspector& GetEntityInspector();
    TemplateLibrary& GetTemplates();
    GraphTypeRegistry& GetGraphTypeRegistry();
};
```

### Jour 4-5: Integration with EditorStartup

**Modification**:
```cpp
// In EditorStartup::InitializeBackend():
auto backend = std::make_unique<UnifiedBackendManager>();
if (!backend->Initialize(config)) {
    return false;
}
```

---

## 🎯 PHASE 3: PLUGIN SYSTEM (Semaine 4, 20 heures)

### Jour 1-5: Implement Plugins

**Fichiers**:
- `Source/BlueprintEditor/NewArchitecture/Plugins/VisualScriptPlugin.h/cpp`
- `Source/BlueprintEditor/NewArchitecture/Plugins/BehaviorTreePlugin.h/cpp`
- `Source/BlueprintEditor/NewArchitecture/Plugins/EntityPrefabPlugin.h/cpp`

**Template**:

```cpp
class VisualScriptPlugin : public IGraphTypePlugin {
public:
    std::string GetTypeName() const override {
        return "VisualScript";
    }
    
    IGraphRenderer* CreateRenderer() override {
        return new VisualScriptRenderer();
    }
    
    IGraphDocument* CreateDocument() override {
        return new VisualScriptGraphDocument();
    }
};
```

### Jour 6-7: TabManager Update

**Modification**: Utiliser GraphTypeRegistry au lieu de hard-coded switching

**Before**:
```cpp
if (type == "VisualScript") {
    renderer = new VisualScriptRenderer();
    document = new VisualScriptGraphDocument();
}
else if (type == "BehaviorTree") {
    // ...
}
```

**After**:
```cpp
auto plugin = registry.GetPlugin(type);
if (!plugin) return "";
renderer = plugin->CreateRenderer();
document = plugin->CreateDocument();
```

---

## 🎯 PHASE 4: CLEANUP & OPTIMIZATION (Semaine 5, 20 heures)

### Jour 1-2: Remove Old Code

- Delete old SingletonManagers
- Delete old BlueprintEditor direct calls
- Update all includes

### Jour 3-5: Profiling & Optimization

- Measure startup time
- Async asset scanning if > 1 sec
- Profile panel loading

---

## 📊 WEEKLY MILESTONE TRACKING

### Week 1-2: Foundation
- [ ] EditorStartup created
- [ ] DIContainer functional
- [ ] PanelManager with lazy loading
- [ ] GraphTypeRegistry operational
- [ ] Phase 1 tests passing
- **Target**: 0 compile errors, init working

### Week 3: Consolidation
- [ ] UnifiedBackendManager replaces 4 managers
- [ ] All existing features still work
- [ ] Memory usage same or lower
- **Target**: No regression

### Week 4: Plugins
- [ ] 3 plugins implemented
- [ ] TabManager uses registry
- [ ] New types can be added without recompile
- **Target**: Extensible

### Week 5: Cleanup
- [ ] Old code removed
- [ ] Startup < 2 seconds
- [ ] All tests passing
- [ ] Documentation updated
- **Target**: Production ready

---

## ✅ SUCCESS CRITERIA

### Code Quality
- ✅ Zero compiler errors
- ✅ Zero compiler warnings
- ✅ No memory leaks (valgrind)
- ✅ No dangling pointers
- ✅ 100% unique_ptr usage

### Performance
- ✅ Startup time < 2 seconds
- ✅ Panel lazy loading < 100ms
- ✅ No frame drops (60 FPS)
- ✅ Memory usage < 200MB

### Architecture
- ✅ < 10 singleton patterns
- ✅ Dependency injection working
- ✅ Plugin system extensible
- ✅ All managers consolidated

### Testing
- ✅ All unit tests passing
- ✅ Integration tests passing
- ✅ End-to-end workflows working
- ✅ Error recovery tested

---

## 🚀 DAY 1 ACTIONS

**Right now (TODAY)**:

1. [ ] Create directory structure (5 min)
2. [ ] Create all .h/.cpp stubs (15 min)
3. [ ] Add #pragma once + namespaces (5 min)
4. [ ] Verify compiles (10 min)
5. [ ] Commit to git (5 min)
6. [ ] Start Phase 1 Day 1 (EditorStartup implementation)

**Total**: < 1 hour to start

---

## 📞 QUESTIONS & DECISIONS

Before starting, answer:

1. **Keep old code?**
   - Option A: Delete immediately (faster, risky)
   - Option B: Keep parallel (slower, safer)
   - **Recommendation**: Option B (parallel during 5 weeks)

2. **Test coverage?**
   - Option A: TDD (tests first)
   - Option B: Tests after (faster shipping)
   - **Recommendation**: Option B (pragmatic approach)

3. **C++ standard?**
   - Keep C++14 (current)
   - Upgrade to C++17 (cleaner code)
   - **Recommendation**: Keep C++14 (no breaking changes)

4. **Smart pointers?**
   - Use unique_ptr everywhere (recommended)
   - Allow raw pointers where needed
   - **Recommendation**: 95% unique_ptr, 5% raw for performance

---

## 📖 REFERENCES

- INITIALIZATION_CALL_GRAPH_BEFORE_MAIN_LOOP.md
- COMPLETE_CALL_GRAPH_SUMMARY.md
- ARCHITECTURE_RECOMMENDATIONS_FOR_CLEAN_REBUILD.md

---

## 🎯 FINAL GOAL

At the end of 5 weeks:

```
✅ New architecture fully implemented
✅ Old code removed
✅ Startup < 2 seconds
✅ Full CRUD working
✅ Panels lazy-loaded
✅ Plugins extensible
✅ Zero technical debt
✅ Team ready for next features
```

**Status**: Ready to start Phase 1 today! 🚀

