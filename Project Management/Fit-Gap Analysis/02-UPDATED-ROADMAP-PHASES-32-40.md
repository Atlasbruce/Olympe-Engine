# UPDATED ROADMAP: BlueprintEditor Path to Production
**Document Date**: 2026-04-04  
**Version**: 2.0 (Updated with Fit-Gap Analysis)  
**Status**: In Progress  

---

## ROADMAP OVERVIEW

### Phase Breakdown

```
COMPLETED (✅ 31 phases)
├─ Phases 1-26: Foundation
├─ Phases 27-31: Entity Prefab Editor
└─ Total: 95% features implemented

IN PROGRESS (⚠️ Gaps identified)
├─ Phase 32: Code Generation Pipeline (BLOCKER)
├─ Phase 33: Entity Factory Runtime
└─ Phase 34-40: Performance & Polish

FUTURE (📋 Planned)
└─ Phases 35-40: Advanced features & optimization
```

---

## EXECUTIVE SUMMARY: CRITICAL PATH

### To Reach 95% (Production Ready)

```
Week 1-2: Resolve Blockers
├─ Generate GenerateComponents.py
├─ Add CMake pre-build hook  
├─ Implement Entity Factory
└─ → Result: 95% complete, 100x perf gain

Week 3: Validation & Performance
├─ Create performance baselines
├─ Add runtime entity factory
├─ Test end-to-end workflow
└─ → Result: Fully validated architecture

Week 4+: Polish & Features
├─ Undo/Redo for prefabs
├─ Graph debugger
├─ Minimap for large graphs
└─ → Result: Production-ready
```

### Critical Dependencies

```
GenerateComponents.py (Phase 32)
    ↓
CMake Hook
    ↓
Code Generation Pipeline
    ↓
Entity Factory (Phase 33)
    ↓
Runtime Prefab Instantiation (Phase 33)
    ↓
Performance Baselines (Phase 34)
    ↓
95% Complete
```

---

## DETAILED ROADMAP: PHASES 32-40

### PHASE 32: CODE GENERATION PIPELINE (BLOCKER)

**Status**: ❌ NOT STARTED (CRITICAL BLOCKER)  
**Target Completion**: Week 1-2  
**Time Estimate**: 12-16 hours  
**Team**: 1 Developer (Backend/Python)  
**Priority**: 🔴 CRITICAL  

#### Deliverables

1. **GenerateComponents.py** (300-400 LOC Python)
   - Input: `Gamedata/EntityPrefab/ComponentsParameters.json`
   - Output: 
     - `Source/Generated/ECS_Components_Generated.h` (struct definitions)
     - `Source/Generated/ComponentIDs_Generated.h` (enum + metadata)
     - `Source/Generated/ComponentQueries_Generated.h` (query specializations)
   - Features:
     - ✅ Component struct generation from JSON schema
     - ✅ ComponentID enum with unique IDs
     - ✅ Bitset configuration (TOTAL_COMPONENTS = N)
     - ✅ Constructor parameter lists
     - ✅ Copy semantics generation (default/delete)
     - ✅ Component pool declarations

2. **CMakeLists.txt Enhancement** (20 LOC CMake)
   - Add pre-build custom_command for code generation
   - Dependency tracking (schema changes trigger rebuild)
   - Error reporting (Python script failures block build)

3. **Build Integration**
   - ✅ CMake hook: GenerateComponents.py → Source/Generated/
   - ✅ Dependency: Source/Generated/ must exist before compilation
   - ✅ Clean target: Remove Source/Generated/ on clean

#### Implementation Details

**GenerateComponents.py Structure:**

```python
def main(input_json, output_dir):
    1. Load ComponentsParameters.json
       ├─ Parse JSON schema
       ├─ Validate component definitions
       └─ Extract 29+ component types

    2. Generate ECS_Components_Generated.h
       ├─ FOR EACH component:
       │  ├─ struct ComponentName {
       │  │  ├─ member types from parameters
       │  │  ├─ default constructors
       │  │  ├─ parameterized constructors
       │  │  ├─ copy/move semantics
       │  │  └─ member functions (get/set)
       │  │ }
       │  └─ ComponentPool<ComponentName>
       └─ #endif

    3. Generate ComponentIDs_Generated.h
       ├─ enum class ComponentID { ... }
       │  ├─ Identity_data = 0
       │  ├─ Position_data = 1
       │  └─ ... (29 entries)
       ├─ static constexpr TOTAL_COMPONENTS = 29
       └─ COMPONENT_NAMES[] array

    4. Generate ComponentQueries_Generated.h
       ├─ Query<Position, Movement>
       ├─ Query<Position, Health>
       └─ Pre-instantiated common queries
```

**CMakeLists.txt Addition:**

```cmake
# Pre-build code generation
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/Source/Generated/ECS_Components_Generated.h
           ${CMAKE_SOURCE_DIR}/Source/Generated/ComponentIDs_Generated.h
           ${CMAKE_SOURCE_DIR}/Source/Generated/ComponentQueries_Generated.h
    COMMAND python3 
        ${CMAKE_SOURCE_DIR}/Tools/GenerateComponents.py
        --input ${CMAKE_SOURCE_DIR}/Gamedata/EntityPrefab/ComponentsParameters.json
        --output ${CMAKE_SOURCE_DIR}/Source/Generated/
    DEPENDS ${CMAKE_SOURCE_DIR}/Gamedata/EntityPrefab/ComponentsParameters.json
    COMMENT "Generating ECS component types from schema..."
)

# Make compilation depend on generated files
add_dependencies(OlympeBlueprintEditor
    CodeGeneration  # depends on custom_command output
)
```

#### Testing

- ✅ Unit test: JSON parsing (valid/invalid schemas)
- ✅ Integration test: Generated C++ compiles
- ✅ Regression test: Bitset operations work
- ✅ Performance test: Struct alignment (SoA optimization)

#### Success Criteria

- [ ] GenerateComponents.py generates all 29 components
- [ ] ComponentID enum has unique IDs
- [ ] Generated code compiles without errors
- [ ] Bitset queries execute (100x faster than runtime)
- [ ] Build system triggers regeneration on schema changes
- [ ] CMake hook integrates with existing build pipeline

#### Blockers Resolved

- ✅ No more runtime-only component lookup
- ✅ Type-safe component access
- ✅ ECS pool management automated
- ✅ Performance: 100x improvement on queries

---

### PHASE 33: ENTITY FACTORY RUNTIME

**Status**: ⚠️ PARTIALLY STARTED  
**Target Completion**: Week 2-3  
**Time Estimate**: 8-10 hours  
**Team**: 1 Developer (Runtime/C++)  
**Dependency**: Phase 32 (GenerateComponents.py)  
**Priority**: 🔴 CRITICAL  

#### Deliverables

1. **EntityFactory.h/cpp** (200-300 LOC C++)
   - Input: Prefab name, position, override properties
   - Output: EntityID of spawned entity
   - Features:
     - Load prefab JSON
     - Create components from template
     - Apply default values + overrides
     - Initialize LocalBlackboard
     - Attach TaskRunnerComponent
     - Register in World

2. **EntityFactory Integration** (50 LOC)
   - Register with World as factory service
   - Hot-reload support (dynamic prefab loading)
   - Error handling & validation

#### Implementation Details

**EntityFactory::CreateEntity Pseudo-code:**

```cpp
EntityID EntityFactory::CreateEntity(
    const std::string& prefabName,
    const Vector& position,
    const PropertyOverrides& overrides = {}) {

    // 1. Load prefab template
    auto prefabData = LoadPrefab(prefabName);
    if (!prefabData) throw std::runtime_error("Prefab not found");

    // 2. Create entity
    EntityID entity = world->CreateEntity();

    // 3. Create and attach components
    for (const auto& componentDef : prefabData.components) {
        auto component = CreateComponent(componentDef);

        // Apply overrides
        if (overrides.has(componentDef.name)) {
            ApplyOverrides(component, overrides.get(componentDef.name));
        }

        // Attach to entity
        world->AddComponent(entity, component);
    }

    // 4. Setup Position component (from factory argument)
    auto posComponent = world->GetComponent<Position_data>(entity);
    if (posComponent) {
        posComponent->x = position.x;
        posComponent->y = position.y;
        posComponent->z = position.z;
    }

    // 5. Setup TaskRunnerComponent
    if (prefabData.hasVisualScript) {
        TaskRunnerComponent runner;
        runner.GraphTemplateID = LoadGraphTemplate(prefabData.graphPath);
        runner.CurrentNodeID = 0;  // EntryPoint
        runner.LocalBlackboard = new EntityBlackboard(entity);
        runner.LocalBlackboard->Initialize(
            GetGraphTemplate(runner.GraphTemplateID),
            prefabData.globalOverrides
        );
        world->AddComponent(entity, runner);
    }

    return entity;
}
```

#### Testing

- ✅ Unit test: Component creation from JSON
- ✅ Unit test: Property override application
- ✅ Integration test: Full prefab spawn
- ✅ Regression test: Existing prefabs still load
- ✅ Stress test: Spawn 1000 entities/second

#### Success Criteria

- [ ] EntityFactory::CreateEntity spawns valid entities
- [ ] All 20+ prefab types load correctly
- [ ] Property overrides apply without errors
- [ ] LocalBlackboard initializes with correct values
- [ ] TaskRunnerComponent attaches when graph exists
- [ ] No memory leaks on repeated spawning
- [ ] Performance: spawn 1000 entities in < 100ms

#### Blockers Resolved

- ✅ Prefabs can be instantiated at runtime
- ✅ Entities execute graphs on spawn
- ✅ End-to-end workflow functional

---

### PHASE 34: PERFORMANCE BASELINES & PROFILING

**Status**: ❌ NOT STARTED  
**Target Completion**: Week 3  
**Time Estimate**: 6-8 hours  
**Team**: 1 Developer (Performance/Profiling)  
**Dependency**: Phase 32 (code gen), Phase 33 (factory)  
**Priority**: 🟠 HIGH  

#### Deliverables

1. **Performance Baseline Tests**
   - Query performance (bitset vs reflection)
   - Entity spawn performance
   - Graph execution performance
   - Component synchronization overhead

2. **Benchmark Suite**
   - Baseline measurements (100x claim validation)
   - Regression detection
   - Memory profiling
   - Comparative analysis (generated vs manual)

3. **Profiling Dashboard** (ImGui panel)
   - Frame-time breakdown
   - Component query timing
   - Memory usage statistics
   - Performance trends

#### Measurements

| Operation | Current | Target | Gain |
|-----------|---------|--------|------|
| Bitset query (1000 entities) | ~520ms | ~5ms | 104x |
| Component lookup | ~100ms | ~1ms | 100x |
| Entity spawn (100 entities) | TBD | <100ms | ? |
| Graph execution (100 nodes) | TBD | <1ms | ? |

#### Success Criteria

- [ ] Baseline measurements established
- [ ] 100x performance claim validated or adjusted
- [ ] Regression tests prevent performance regressions
- [ ] Profiling dashboard functional
- [ ] Memory usage < 10MB for 1000 entities

---

### PHASE 35: RUNTIME VALIDATION & ERROR HANDLING

**Status**: ⚠️ PARTIAL  
**Target Completion**: Week 3-4  
**Time Estimate**: 4-6 hours  
**Team**: 1 Developer (Runtime/QA)  
**Dependency**: Phase 33 (factory)  
**Priority**: 🟠 HIGH  

#### Deliverables

1. **Entity Validator**
   - Verify all components attached
   - Check component dependencies
   - Validate graph template
   - Report missing components

2. **Error Recovery**
   - Graceful component missing handling
   - Graph loading error fallback
   - Property override validation
   - Type mismatch detection

3. **Logging & Diagnostics**
   - Entity spawn logging
   - Component attachment logging
   - Graph execution tracing
   - Error stack traces

#### Success Criteria

- [ ] All entity spawn failures caught and logged
- [ ] No silent failures (errors reported clearly)
- [ ] Recovery paths exist for common errors
- [ ] Diagnostics panel shows error history
- [ ] No crashes on invalid prefabs

---

### PHASE 36: UNDO/REDO FOR PREFAB EDITOR

**Status**: ⚠️ 20% (Basic undo exists)  
**Target Completion**: Week 4  
**Time Estimate**: 6-8 hours  
**Team**: 1 Developer (Editor/UX)  
**Priority**: 🟠 MEDIUM  

#### Deliverables

1. **Full Undo/Redo Stack**
   - Node creation/deletion
   - Property changes
   - Connection creation/deletion
   - Selection changes

2. **Command History UI**
   - Visual undo/redo stack
   - Jump to specific command
   - Edit history sidebar

3. **Undo/Redo Shortcuts**
   - Ctrl+Z: Undo
   - Ctrl+Y: Redo
   - Ctrl+Shift+Z: Redo (alternative)

#### Success Criteria

- [ ] All prefab editor actions are undoable
- [ ] Undo/Redo works across save/reload
- [ ] No command loss on editor crash
- [ ] Performance: undo 1000 commands in < 100ms

---

### PHASE 37: COPY/PASTE NODES

**Status**: ❌ NOT STARTED  
**Target Completion**: Week 4  
**Time Estimate**: 4-6 hours  
**Team**: 1 Developer (Editor/UX)  
**Priority**: 🟡 MEDIUM  

#### Deliverables

1. **Clipboard Management**
   - Copy selected nodes
   - Paste with connection relinking
   - Duplicate nodes (Ctrl+D)
   - Move to clipboard content

2. **Smart Connection Relinking**
   - Preserve internal connections
   - Break external connections
   - Optional auto-reconnect

3. **Copy/Paste Shortcuts**
   - Ctrl+C: Copy
   - Ctrl+X: Cut
   - Ctrl+V: Paste

#### Success Criteria

- [ ] Copy preserves node properties
- [ ] Paste creates independent copies
- [ ] Internal connections preserved
- [ ] Performance: paste 100 nodes in < 500ms

---

### PHASE 38: GRAPH EXECUTION SIMULATOR

**Status**: ❌ NOT STARTED  
**Target Completion**: Week 4-5  
**Time Estimate**: 8-10 hours  
**Team**: 1 Developer (Editor/Debugger)  
**Priority**: 🟡 MEDIUM  

#### Deliverables

1. **In-Editor Graph Simulator**
   - Step through execution (node by node)
   - Watch variable values
   - Set breakpoints
   - Inspect data pins

2. **Trace Viewer**
   - Highlight execution path
   - Show data flow
   - Animate transitions

3. **Variable Inspector**
   - Watch local blackboard
   - Watch global blackboard
   - Modify values during stepping

#### Success Criteria

- [ ] Can step through graph execution
- [ ] Variable values visible during stepping
- [ ] Breakpoints pause execution
- [ ] Data flow visualization works
- [ ] No performance impact on normal editing

---

### PHASE 39: MINIMAP PANEL

**Status**: ❌ NOT STARTED  
**Target Completion**: Week 5  
**Time Estimate**: 3-4 hours  
**Team**: 1 Developer (Editor/UI)  
**Priority**: 🟡 LOW  

#### Deliverables

1. **Minimap Rendering**
   - Small overview of entire graph
   - Current viewport highlight
   - Node clusters visualization

2. **Interactive Navigation**
   - Click to pan to location
   - Drag viewport rectangle
   - Zoom to fit/selection

3. **Performance Optimization**
   - Minimap cache updates only on changes
   - LOD (level of detail) for large graphs
   - Async rendering

#### Success Criteria

- [ ] Minimap renders all nodes
- [ ] Viewport rectangle shows current view
- [ ] Click-to-pan works smoothly
- [ ] Performance: minimap for 1000-node graph < 5ms

---

### PHASE 40: SUBGRAPH SUPPORT

**Status**: ⚠️ 30% (Loader exists)  
**Target Completion**: Week 5-6  
**Time Estimate**: 8-10 hours  
**Team**: 1 Developer (Editor/Runtime)  
**Dependency**: Phase 38 (simulator)  
**Priority**: 🟡 LOW  

#### Deliverables

1. **Subgraph Nodes**
   - Reference external graphs
   - Parameter passing
   - Return value handling

2. **Subgraph Editor Integration**
   - Inline subgraph preview
   - Navigate into subgraph
   - Automatic parameter UI

3. **Subgraph Library**
   - Search/browse subgraphs
   - Template subgraphs
   - Reusable components

#### Success Criteria

- [ ] Subgraph nodes load and execute
- [ ] Parameters pass correctly
- [ ] Circular references detected
- [ ] Performance: nested graphs to depth 5+
- [ ] UI: easy navigation between graphs

---

## TIMELINE SUMMARY

### Immediate (Week 1-2): BLOCKERS
```
Phase 32: GenerateComponents.py .............. 12-16h (Mon-Thu)
Phase 32: CMake Integration .................. 2-4h   (Fri)
BUILD: Test code generation pipeline ......... 4-6h   (Fri-Mon)
```

### Short-term (Week 2-3): CRITICAL PATH
```
Phase 33: Entity Factory ..................... 8-10h  (Tue-Wed)
Phase 34: Performance Baselines .............. 6-8h   (Thu-Fri)
TEST: End-to-end prefab spawn ................ 4-6h   (Fri-Mon)
```

### Medium-term (Week 3-4): POLISH
```
Phase 36: Undo/Redo .......................... 6-8h   (Mon-Tue)
Phase 37: Copy/Paste ......................... 4-6h   (Wed)
Phase 35: Validation & Error Handling ........ 4-6h   (Thu-Fri)
```

### Long-term (Week 4-6): ADVANCED
```
Phase 38: Graph Simulator .................... 8-10h  (Mon-Tue)
Phase 39: Minimap Panel ...................... 3-4h   (Wed)
Phase 40: Subgraph Support ................... 8-10h  (Thu-Fri)
```

---

## RESOURCE ALLOCATION

### Team Composition

| Role | Phase | Effort | Timeline |
|------|-------|--------|----------|
| Backend (Code Gen) | 32 | 12-16h | Week 1-2 |
| Runtime (Factory) | 33 | 8-10h | Week 2-3 |
| Performance | 34 | 6-8h | Week 3 |
| Editor (UI/UX) | 36, 37, 39 | 12-14h | Week 3-5 |
| QA | 35, 38, 40 | 12-16h | Week 4-6 |
| **Total** | **All** | **57-67h** | **6 weeks** |

### Skills Required

- C++ (14) - Phases 33, 35, 38, 40
- Python - Phase 32
- CMake - Phase 32
- ImGui - Phases 36, 37, 39, 40
- Performance Analysis - Phase 34
- JSON - Phases 33, 35

---

## SUCCESS METRICS

### Phase 32 Success
- ✅ GenerateComponents.py generates 29 components
- ✅ ComponentID enum has unique IDs  
- ✅ Build system auto-triggers on schema changes
- ✅ Generated code compiles without errors

### Phase 33 Success
- ✅ EntityFactory::CreateEntity returns valid EntityID
- ✅ All 20 prefab types spawn successfully
- ✅ LocalBlackboard initializes correctly
- ✅ TaskRunnerComponent executes graphs

### Phase 34 Success
- ✅ Baseline query performance measured
- ✅ 100x claim validated (or adjusted)
- ✅ Memory usage < 10MB for 1000 entities
- ✅ Performance regression tests in CI/CD

### Overall Success (95% Complete)
- ✅ All critical blockers resolved
- ✅ End-to-end workflow functional
- ✅ Performance targets met
- ✅ Production-ready state achieved

---

## DEPENDENCIES & BLOCKING

### Dependency Graph

```
Phase 32 (Code Gen)
    ↓ (blocks)
Phase 33 (Entity Factory)
    ↓ (blocks)
Phase 34 (Perf Baselines)
    ├─ (blocks) Phase 35 (Validation)
    └─ (blocks) Phase 38 (Simulator)
                    ↓
                Phase 40 (Subgraph)

Parallel Tracks:
Phase 36 (Undo/Redo) ← Phase 31 (Prefab Editor)
Phase 37 (Copy/Paste) ← Phase 36
Phase 39 (Minimap) ← Phase 31 (Canvas)
```

### Critical Path

```
Phase 32 → Phase 33 → Phase 34 → Phase 38 → Phase 40
(12h)      (8h)      (6h)      (8h)      (8h)
= 42 hours critical path (1 developer, 6 weeks)
```

---

## RISK ASSESSMENT

### High Risk

| Risk | Impact | Mitigation |
|------|--------|-----------|
| GenerateComponents.py bugs | 100x perf not achieved | Unit test code gen |
| CMake hook conflicts | Build fails | Early integration testing |
| Entity factory incomplete | Runtime crashes | Comprehensive validation |

### Medium Risk

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Performance target miss | Phase 34 extends | Profiling during Phase 32/33 |
| Subgraph cycles | Infinite loops | Cycle detection in Phase 40 |
| Undo/Redo state corruption | Data loss | Atomic transaction logging |

### Low Risk

| Risk | Impact | Mitigation |
|------|--------|-----------|
| UI polish delays | Timeline slip | Prioritize core functionality |
| Minimap performance | Editor sluggish | Implement LOD rendering |

---

## CONCLUSION

The BlueprintEditor is on track for **95% completion in 6 weeks** with focused effort on resolving 3 critical blockers:

1. **Phase 32**: Code Generation Pipeline (2 weeks)
2. **Phase 33**: Entity Factory Runtime (1 week)
3. **Phase 34**: Performance Validation (1 week)

Remaining phases (36-40) are polish and advanced features that can proceed in parallel with core development.

**Go-Live Timeline**: Week 6 (production-ready with core features)  
**Fully Polished**: Week 8 (with all advanced features)

---

**END OF ROADMAP**
