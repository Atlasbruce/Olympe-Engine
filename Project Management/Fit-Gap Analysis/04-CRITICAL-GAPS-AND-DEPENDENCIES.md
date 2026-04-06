# CRITICAL GAPS & DEPENDENCY MAP

**Document Date**: 2026-04-04  
**Severity Levels**: 🔴 BLOCKER | 🟠 HIGH | 🟡 MEDIUM | 🟢 LOW

---

## CRITICAL BLOCKERS (must resolve before 95% completion)

### BLOCKER #1: GenerateComponents.py MISSING

**Status**: ❌ NOT IMPLEMENTED  
**Severity**: 🔴 CRITICAL  
**Impact**: 100x performance degradation, no type safety  
**Location**: Should be `Tools/GenerateComponents.py` (MISSING)  
**Size**: 300-400 lines Python  
**Time to Fix**: 6-8 hours  

#### Why It's Critical

- Without it, all component access is runtime-only (std::any reflection)
- Bitset queries fail (no ComponentID enum)
- ECS pool management not automated
- Cannot validate 100x performance claim in document

#### Dependency Chain

```
GenerateComponents.py (BLOCKER)
    ↓
Source/Generated/ECS_Components_Generated.h
    ↓
Source/Generated/ComponentIDs_Generated.h
    ↓
CMake can compile with bitset queries
    ↓
Phase 33 (Entity Factory) can use generated components
    ↓
Phase 34 (Performance validation)
```

#### Solution

**Create file**: `Tools/GenerateComponents.py` (see Phase 32 implementation guide)

---

### BLOCKER #2: CMake Pre-build Hook MISSING

**Status**: ⚠️ PARTIALLY CONFIGURED  
**Severity**: 🔴 CRITICAL  
**Impact**: Code generation never runs, generated files not created  
**Location**: `Source/BlueprintEditor/CMakeLists.txt`  
**Change Required**: Add custom_command + add_dependencies  
**Time to Fix**: 1-2 hours  

#### Why It's Critical

- Even with GenerateComponents.py written, build system won't run it
- Generated files won't exist for compilation
- Build system won't track schema file changes
- Regeneration won't happen automatically on schema edits

#### Current State (CMakeLists.txt)

```cmake
# MISSING: No pre-build rule for code generation
# MISSING: No dependency on GenerateComponents.py
# MISSING: No add_custom_command for code generation
```

#### Solution

**Add to CMakeLists.txt**:
```cmake
# Pre-build code generation
set(COMPONENTS_SCHEMA "${CMAKE_SOURCE_DIR}/Gamedata/EntityPrefab/ComponentsParameters.json")
set(GENERATED_DIR "${CMAKE_SOURCE_DIR}/Source/Generated")

add_custom_command(
    OUTPUT ${GENERATED_DIR}/ECS_Components_Generated.h
           ${GENERATED_DIR}/ComponentIDs_Generated.h
           ${GENERATED_DIR}/ComponentQueries_Generated.h
    COMMAND python3 ${CMAKE_SOURCE_DIR}/Tools/GenerateComponents.py
            --input ${COMPONENTS_SCHEMA}
            --output ${GENERATED_DIR}
    DEPENDS ${COMPONENTS_SCHEMA}
    COMMENT "Generating ECS component types..."
)

# Make compilation depend on generated files
target_include_directories(OlympeBlueprintEditor PRIVATE ${GENERATED_DIR})
add_dependencies(OlympeBlueprintEditor GenerateComponents)
```

---

### BLOCKER #3: Entity Factory MISSING

**Status**: ❌ NOT IMPLEMENTED  
**Severity**: 🔴 CRITICAL  
**Impact**: Cannot spawn entities from prefabs at runtime  
**Location**: Should be `Source/Core/EntityFactory.h/cpp` (NOT FOUND)  
**Size**: 200-300 lines C++  
**Time to Fix**: 8-10 hours  
**Dependency**: Blocker #1 (GenerateComponents.py must work first)

#### Why It's Critical

- End-to-end workflow requires entity spawning
- Currently: no way to instantiate prefabs in game
- Prefab editor works but output can't be used at runtime
- Phase 33 cannot proceed without this

#### What It Needs to Do

```cpp
EntityID EntityFactory::CreateEntity(prefab_name, position, overrides)
{
    1. Load Gamedata/EntityPrefab/{prefab_name}.json
    2. FOR EACH component in prefab:
       - Create component instance using generated class
       - Apply default values from JSON
       - Apply override values from parameters
       - Attach to entity via World::AddComponent()
    3. Create TaskRunnerComponent if graph exists
    4. Initialize LocalBlackboard with global vars
    5. Register entity in World
    6. Return EntityID
}
```

#### Current State

- ❌ No EntityFactory class
- ❌ No factory service registration
- ❌ No prefab-to-entity instantiation code
- ⚠️ PrefabLoader exists but only loads JSON (doesn't instantiate)

#### Solution

Create `Source/Core/EntityFactory.h/cpp` with:
- EntityFactory singleton class
- CreateEntity() method (main factory function)
- Component instantiation logic
- LocalBlackboard initialization
- TaskRunnerComponent attachment

---

## HIGH-PRIORITY GAPS (blocks 80-95% completion)

### GAP #1: Blackboard Configuration UI

**Status**: ⚠️ NON-FUNCTIONAL  
**Severity**: 🟠 HIGH  
**Impact**: Cannot add/edit/delete variables without JSON editing  
**Location**: Would be in `BlueprintEditor/BlackboardVariableEditorPanel`  
**Size**: 150-200 lines C++  
**Time to Fix**: 4-6 hours  

**Currently**: Manual JSON editing required  
**Needed**: UI panel for variable management

---

### GAP #2: Component Registration Automation

**Status**: ⚠️ MANUAL  
**Severity**: 🟠 HIGH  
**Impact**: Only 3 components auto-registered (manual hardcoding)  
**Location**: `Source/ECS/ComponentRegistry.cpp`  
**Size**: 50-100 lines C++  
**Time to Fix**: 2-4 hours  

**Currently**:
```cpp
// MANUAL: Only TaskRunner, Position, Movement registered
ComponentRegistry::Get().Register<TaskRunnerComponent>();
ComponentRegistry::Get().Register<Position_data>();
ComponentRegistry::Get().Register<Movement_data>();
// 27 OTHER COMPONENTS NOT REGISTERED
```

**Needed**: Auto-register all generated components

---

### GAP #3: Graph Execution Validation

**Status**: ⚠️ PARTIAL  
**Severity**: 🟠 HIGH  
**Impact**: Missing error checking before runtime execution  
**Location**: `VSGraphVerifier.cpp` (exists but incomplete)  
**Size**: 100-150 lines C++  
**Time to Fix**: 3-4 hours  

**Currently Missing**:
- Cross-graph reference validation
- Circular dependency detection
- Component availability checking
- Parameter type matching

---

## MEDIUM-PRIORITY GAPS (blocks 90-95% completion)

### GAP #1: Graph Execution Debugger

**Status**: ❌ NOT IMPLEMENTED  
**Severity**: 🟡 MEDIUM  
**Impact**: Cannot debug graphs without running game  
**Location**: Would be `BlueprintEditor/GraphExecutionSimulator`  
**Size**: 300-400 lines C++  
**Time to Fix**: 8-10 hours  

**Needed**: Step-through debugger for graphs in editor

---

### GAP #2: Copy/Paste Nodes (Prefab Editor)

**Status**: ❌ NOT IMPLEMENTED  
**Severity**: 🟡 MEDIUM  
**Impact**: Cannot reuse node configurations  
**Location**: `PrefabCanvas.cpp`  
**Size**: 100-150 lines C++  
**Time to Fix**: 4-6 hours  

---

### GAP #3: Undo/Redo System (Prefab Editor)

**Status**: ⚠️ PARTIAL (20%)  
**Severity**: 🟡 MEDIUM  
**Impact**: Limited edit history, cannot undo complex operations  
**Location**: `BlueprintEditor/UndoRedoStack.cpp`  
**Size**: 80-100 lines C++  
**Time to Fix**: 4-6 hours  

**Currently Partial**:
- Node create/delete tracked
- Missing: property changes, connection changes

---

## LOW-PRIORITY GAPS (polish, 95%+)

### GAP #1: Performance Profiling Panel
- **Time to Fix**: 4-6 hours

### GAP #2: Minimap for Large Graphs
- **Time to Fix**: 3-4 hours

### GAP #3: Subgraph Support
- **Time to Fix**: 8-10 hours

### GAP #4: Node Search/Filter
- **Time to Fix**: 2-3 hours

---

## DEPENDENCY GRAPH: CRITICAL PATH

```
┌────────────────────────────────────────────┐
│ BLOCKER #1: GenerateComponents.py          │ (6-8h)
└────────────────┬─────────────────────────┘
                 │
         ┌───────▼────────┐
         │ CMake Hook     │ (1-2h)
         │ (Blocker #2)   │
         └───────┬────────┘
                 │
    ┌────────────▼────────────┐
    │ Generated Files Created │
    │ • ComponentIDs enum     │ → 95% Complete
    │ • ECS Structs          │
    │ • Bitset config        │
    └────────────┬────────────┘
                 │
         ┌───────▼─────────┐
         │ Entity Factory  │ (8-10h)
         │ (Blocker #3)    │
         └───────┬─────────┘
                 │
    ┌────────────▼──────────┐
    │ Performance Baseline  │
    │ (Phase 34)            │ → 95-98% Complete
    │ • Validate 100x claim │
    │ • Memory measurements │
    └───────────────────────┘

Total Critical Path: ~16-20 hours (2-3 days continuous)
```

---

## DEPENDENCY TABLE

| Phase | Dependency | Status | Unblocks |
|-------|-----------|--------|----------|
| 32 | None | ❌ NOT STARTED | 33, 34, 35 |
| 32 (CMake) | Phase 32 Python | ⚠️ PARTIAL | 33 |
| 33 | Phase 32, Phase 32 CMake | ❌ BLOCKED | 34, 35 |
| 34 | Phase 33 | ❌ BLOCKED | 38, 40 |
| 35 | Phase 33 | ❌ BLOCKED | 38 |
| 36 | Phase 31 (Prefab Editor) | ✅ READY | Polish only |
| 37 | Phase 36 (Undo/Redo) | ✅ READY | Polish only |
| 38 | Phase 34 (Perf data) | ❌ BLOCKED | 40 |
| 39 | Phase 31 (Canvas) | ✅ READY | Polish only |
| 40 | Phase 38 (Simulator) | ⚠️ PARTIAL | Final polish |

---

## RISK MATRIX

### High Risk (must solve)
| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|-----------|
| GenerateComponents.py bugs | Build fails | **HIGH** | Early testing, unit tests |
| CMake hook conflicts | Circular deps | **MEDIUM** | Simple hook structure |
| Entity Factory incomplete | Runtime crash | **HIGH** | Comprehensive validation |

### Medium Risk (should solve)
| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|-----------|
| Performance target miss | 100x claim invalid | **MEDIUM** | Profile during Phase 32/33 |
| Graph cycles | Infinite loops | **LOW** | Add cycle detection |

---

## REMEDIATION PLAN

### IMMEDIATE (Today)
1. ✅ Identify gaps (THIS DOCUMENT)
2. ⏳ Create GenerateComponents.py (Phase 32 guide exists)
3. ⏳ Start CMake hook work

### WEEK 1 (Days 1-3)
1. ✅ Write GenerateComponents.py (8h)
2. ✅ Add CMake hook (2h)
3. ✅ Test build pipeline (2h)
4. ✅ Fix compilation errors (4h)

### WEEK 1-2 (Days 4-8)
1. ✅ Build Entity Factory (8h)
2. ✅ Test prefab spawning (4h)
3. ✅ Performance profiling (6h)
4. ✅ Validation & polish (4h)

### WEEK 2-3 (Days 9-15)
1. ✅ Remaining gaps (polish features)
2. ✅ Documentation updates
3. ✅ Production readiness

---

## SUCCESS CRITERIA FOR 95% COMPLETION

- [ ] GenerateComponents.py generates 29 components
- [ ] CMake hook triggers on schema changes
- [ ] Generated code compiles without errors
- [ ] EntityFactory::CreateEntity works end-to-end
- [ ] Performance: bitset query < 5ms (100x improvement)
- [ ] All blockers resolved
- [ ] Build system fully automated
- [ ] End-to-end workflow functional (editor → runtime)

---

## CONCLUSION

**3 CRITICAL BLOCKERS** must be resolved to reach 95% completion:

1. **GenerateComponents.py** (6-8h) - Missing entirely, no substitute
2. **CMake Hook** (1-2h) - Incomplete, simple fix
3. **Entity Factory** (8-10h) - Missing entirely, complex implementation

**Total Blocking Time**: 15-20 hours (~2-3 days)

**Parallel Work**: High/Medium gaps can proceed in parallel after blockers resolved

**Timeline**: Week 1-2 for blockers + validation = **95% completion ready**

---

**END OF CRITICAL GAPS DOCUMENT**
