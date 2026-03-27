# 📦 FILE MANIFEST — All Files to Create/Archive

**Date:** 2026-03-27  
**Purpose:** Quick reference for all file operations  
**For:** Developers & Project Managers

---

## 📋 PHASE 0: CREATE NEW FILES

### P0.1: EventToBlackboardBridge

**CREATE:**
```
✅ Source/TaskSystem/EventToBlackboardBridge.h        (~100 lines)
   • struct EventMapping
   • class EventToBlackboardBridge

✅ Source/TaskSystem/EventToBlackboardBridge.cpp      (~150 lines)
   • EventToBlackboardBridge::Get()
   • EventToBlackboardBridge::RegisterEventMapping()
   • EventToBlackboardBridge::ProcessEvents()

✅ Tests/TaskSystem/EventToBlackboardBridgeTests.cpp  (~200 lines)
   • Test: mapping validation
   • Test: event processing
   • Test: localBB writes
   • Test: target routing
```

**Total New Lines:** ~450

---

### P0.2: SensorSystem

**CREATE:**
```
✅ Source/ECS/Systems/SensorSystem.h                  (~150 lines)
   • class SensorSystem : public ECS_System
   • QueryVisibilityRay()
   • QueryNoiseRay()
   • IsTargetVisible()

✅ Source/ECS/Systems/SensorSystem.cpp                (~250 lines)
   • SensorSystem::Process()
   • UpdateEntityPerception()
   • CastVisionRay()
   • Timeslicing logic

✅ Source/ECS/Components/SensorComponent.h            (~30 lines)
   • struct SensorComponent
   • AUTO_REGISTER_COMPONENT

✅ Tests/ECS/Systems/SensorSystemTests.cpp            (~300 lines)
   • Test: vision range
   • Test: vision angle
   • Test: line-of-sight
   • Test: hearing range
   • Test: timeslicing
   • Test: localBB writes
```

**Total New Lines:** ~730

---

### P0.3: AI_MoveTask

**CREATE:**
```
✅ Source/TaskSystem/AtomicTasks/AI_MoveTask.h        (~120 lines)
   • class AI_MoveTask : public IAtomicTask
   • ExecuteWithContext()
   • Abort()
   • Helper methods

✅ Source/TaskSystem/AtomicTasks/AI_MoveTask.cpp      (~200 lines)
   • Pathfinding integration
   • Movement following
   • Waypoint navigation
   • Status returns

✅ Tests/TaskSystem/AtomicTasks/AI_MoveTaskTests.cpp (~250 lines)
   • Test: pathfinding request
   • Test: waypoint following
   • Test: arrival detection
   • Test: task abort
   • Test: path replanning
   • Test: failure handling
```

**Total New Lines:** ~570

---

### P0: GRAPH FILES (New Content)

**CREATE:**
```
✅ Blueprints/AI/ai_state_transitions.ats
   • VS Graph (v4 schema)
   • 6-8 nodes (Branch, SetBBValue, Return)
   • Phase 24 Condition Presets

✅ Blueprints/AI/ai_move_behavior.ats
   • VS Graph (v4 schema)
   • AtomicTask: AI_Move node
   • Success/Failure handlers

Total: ~200 lines JSON
```

**Phase 0 Total:** ~1950 lines (code + tests)

---

## 📋 PHASE 1-4: ARCHIVE FILES

### P1.2: Archive AIBlackboard_data

**BACKUP:**
```
📦 Archive/Deprecated_AI_Components/
   ✅ AIBlackboard_data_original.h  (original ECS_Components_AI.h)
```

**MODIFY:**
```
🔄 Source/ECS_Components_AI.h
   • Replace with stub (30 lines)
   • Include CompatibilityLayer.h
   • Deprecation warnings
```

---

### P1.1: Create CompatibilityLayer

**CREATE:**
```
✅ Source/TaskSystem/CompatibilityLayer.h             (~200 lines)
   • class AIBlackboardCompat
   • 30+ accessor pairs

✅ Source/TaskSystem/CompatibilityLayer.cpp           (~300 lines)
   • All accessors implemented
   • LocalBB forwarding

✅ Tests/TaskSystem/CompatibilityLayerTests.cpp       (~250 lines)
   • Test: all accessors
   • Test: multiple entities
   • Test: defaults
   • Test: persistence
```

**Total:** ~750 lines

---

### P1.3: Archive AIStimuliSystem

**ARCHIVE:**
```
📦 Archive/Deprecated_AI_Systems/
   ✅ ECS_Systems_AI.h (original - save full copy)
   ✅ ECS_Systems_AI.cpp (original - save full copy)

   Plus specific archive:
   ✅ AIStimuliSystem_header.h (extracted declaration)
   ✅ AIStimuliSystem_impl.cpp (extracted implementation)
```

**MODIFY:**
```
🔄 Source/ECS_Systems_AI.h
   • Remove: AIStimuliSystem class declaration

🔄 Source/ECS_Systems_AI.cpp
   • Remove: AIStimuliSystem::Process() implementation

🔄 Source/World.cpp (or init file)
   • Remove: m_aiStimuliSystem registration
   • Add: EventToBlackboardBridge initialization
```

---

### P1.4: Archive BehaviorTreeRuntime_data

**ARCHIVE:**
```
📦 Archive/Deprecated_AI_Components/
   ✅ BehaviorTreeRuntime_data_original.h
```

**MODIFY:**
```
🔄 Source/ECS_Components_AI.h stub
   • Add comment: "BehaviorTreeRuntime_data → TaskRunnerComponent (P1.4)"
```

---

### P2.1: Archive AIPerceptionSystem

**ARCHIVE:**
```
📦 Archive/Deprecated_AI_Systems/
   ✅ AIPerceptionSystem_header.h
   ✅ AIPerceptionSystem_impl.cpp
```

**MODIFY:**
```
🔄 Source/ECS_Systems_AI.h
   • Remove: AIPerceptionSystem class declaration

🔄 Source/ECS_Systems_AI.cpp
   • Remove: AIPerceptionSystem::Process() implementation

🔄 Source/World.cpp
   • Remove: m_aiPerceptionSystem registration
   • Already: SensorSystem registered (P2.1)
```

---

### P2.2: Modify AI State Transitions

**MODIFY:**
```
🔄 Blueprints/AI/ai_state_transitions.ats
   • Update with actual conditions from AIStateTransitionSystem
   • Add Phase 24 Condition Presets

🔄 Source/World.cpp or prefab
   • Load ai_state_transitions.ats for AI entities
```

---

### P3.2: Archive BehaviorTreeSystem

**ARCHIVE:**
```
📦 Archive/Deprecated_AI_Systems/
   ✅ BehaviorTreeSystem_header.h
   ✅ BehaviorTreeSystem_impl.cpp
   ✅ ECS_Systems_AI_FULL_BACKUP.h
   ✅ ECS_Systems_AI_FULL_BACKUP.cpp
```

**MODIFY:**
```
🔄 Source/ECS_Systems_AI.h
   • Remove: BehaviorTreeSystem class declaration
   • Remove: Other AI systems (already archived in P1-P2)
   • Result: File may be empty or minimal

🔄 Source/ECS_Systems_AI.cpp
   • Remove: BehaviorTreeSystem::Process()
   • Remove: Other implementations
   • Result: File may be empty

🔄 Source/World.cpp
   • Remove: m_behaviorTreeSystem registration
   • Verify: TaskSystem runs VSGraphExecutor
```

---

### P4.1: Archive BehaviorTree Editor Tools

**ARCHIVE:**
```
📦 Archive/BehaviorTree_Editor/
   ✅ BehaviorTreeDebugWindow.h
   ✅ BehaviorTreeDebugWindow.cpp
   ✅ BehaviorTreeDebugWindow_NodeGraph.cpp
   ✅ BehaviorTreeEditorPlugin.h
   ✅ BehaviorTreeEditorPlugin.cpp
   ✅ BehaviorTreeRenderer.h
   ✅ BehaviorTreeRenderer.cpp
```

**MODIFY:**
```
🔄 Source/BlueprintEditor/*.cpp
   • Remove: BehaviorTreeEditorPlugin registration
   • Keep: Other editor plugins active

🔄 Source/AI/*.h / *.cpp
   • Remove: BehaviorTree debug window references
```

---

### P4.2: Archive BehaviorTree Core

**ARCHIVE:**
```
📦 Archive/BehaviorTree_Core/
   ✅ BehaviorTree.h
   ✅ BehaviorTree.cpp
   ✅ BehaviorTreeDependencyScanner.h
   ✅ BehaviorTreeDependencyScanner.cpp
   ✅ BehaviorTreeAdapter.h  (decision: keep for reference?)
```

**MODIFY:**
```
🔄 Source/AI/*.cpp
   • Remove: #include "BehaviorTree.h"
   • Remove: BehaviorTree runtime logic
```

---

### P4.3: Archive BehaviorTree Manager

**ARCHIVE:**
```
📦 Archive/BehaviorTree_Manager/
   ✅ BehaviorTreeManager.h
   ✅ BehaviorTreeManager.cpp
```

**MODIFY:**
```
🔄 Source/AI/*.cpp
   • Remove: BehaviorTreeManager references
   • Verify: AssetManager handles .ats loading
```

---

## 📦 ARCHIVE STRUCTURE (Final)

```
Archive/
├── Deprecated_AI_Components/
│   ├── AIBlackboard_data_original.h
│   ├── BehaviorTreeRuntime_data_original.h
│   └── [Other legacy components]
│
├── Deprecated_AI_Systems/
│   ├── ECS_Systems_AI_FULL_BACKUP.h
│   ├── ECS_Systems_AI_FULL_BACKUP.cpp
│   ├── AIStimuliSystem_header.h
│   ├── AIStimuliSystem_impl.cpp
│   ├── AIPerceptionSystem_header.h
│   ├── AIPerceptionSystem_impl.cpp
│   ├── AIStateTransitionSystem_*
│   ├── BehaviorTreeSystem_*
│   └── AIMotionSystem_*
│
├── BehaviorTree_Core/
│   ├── BehaviorTree.h
│   ├── BehaviorTree.cpp
│   ├── BehaviorTreeDependencyScanner.h
│   ├── BehaviorTreeDependencyScanner.cpp
│   └── BehaviorTreeAdapter.h  (optional keep)
│
├── BehaviorTree_Editor/
│   ├── BehaviorTreeDebugWindow.h
│   ├── BehaviorTreeDebugWindow.cpp
│   ├── BehaviorTreeDebugWindow_NodeGraph.cpp
│   ├── BehaviorTreeEditorPlugin.h
│   ├── BehaviorTreeEditorPlugin.cpp
│   ├── BehaviorTreeRenderer.h
│   └── BehaviorTreeRenderer.cpp
│
├── BehaviorTree_Manager/
│   ├── BehaviorTreeManager.h
│   └── BehaviorTreeManager.cpp
│
├── BehaviorTree_Migration_Tools/
│   └── BehaviorTreeAdapter.h  (if not in Core)
│
└── README_LEGACY_SYSTEMS.md  ← Explain what happened
```

---

## 📝 FILE COUNT SUMMARY

### Created
```
Phase 0:  6 new files (2 .h, 2 .cpp, 2 tests, + 2 graph files)
Phase 1:  3 new files (1 .h, 1 .cpp, 1 test)
Phase 4:  0 new files (only archives + modifications)

TOTAL NEW FILES CREATED: ~9 files
TOTAL NEW LINES WRITTEN: ~2700 lines (code + tests)
```

### Modified
```
Source/ECS_Components_AI.h          (reduce to stub)
Source/ECS_Systems_AI.h             (remove classes)
Source/ECS_Systems_AI.cpp           (remove implementations)
Source/World.cpp                    (update system registrations)
Source/TaskSystem/CompatibilityLayer (new)
Project file (.vcxproj)             (remove files, add files)

TOTAL FILES MODIFIED: ~6 files
```

### Archived
```
AI Components:        2-4 files
AI Systems:           5-8 files
BT Editor:            7 files
BT Core:              4-5 files
BT Manager:           2 files

TOTAL ARCHIVED: ~20-25 files
```

---

## ✅ PROJECT FILE UPDATES

### OlympeEngine.vcxproj Changes

**REMOVE from project:**
```xml
<!-- Legacy AI Systems -->
<ClCompile Include="Source\ECS_Systems_AI.cpp" />
<ClInclude Include="Source\ECS_Systems_AI.h" />

<!-- Legacy AI Components -->
<ClInclude Include="Source\ECS_Components_AI.h" />

<!-- BehaviorTree Engine -->
<ClCompile Include="Source\AI\BehaviorTree.cpp" />
<ClInclude Include="Source\AI\BehaviorTree.h" />
<ClCompile Include="Source\AI\BehaviorTreeDependencyScanner.cpp" />
<ClInclude Include="Source\AI\BehaviorTreeDependencyScanner.h" />
<ClCompile Include="Source\AI\BehaviorTreeManager.cpp" />
<ClInclude Include="Source\AI\BehaviorTreeManager.h" />

<!-- BT Editor -->
<ClCompile Include="Source\AI\BehaviorTreeDebugWindow.cpp" />
<ClInclude Include="Source\AI\BehaviorTreeDebugWindow.h" />
<ClCompile Include="Source\AI\BehaviorTreeDebugWindow_NodeGraph.cpp" />
<ClCompile Include="Source\BlueprintEditor\BehaviorTreeEditorPlugin.cpp" />
<ClInclude Include="Source\BlueprintEditor\BehaviorTreeEditorPlugin.h" />
<ClCompile Include="Source\BlueprintEditor\BehaviorTreeRenderer.cpp" />
<ClInclude Include="Source\BlueprintEditor\BehaviorTreeRenderer.h" />
```

**ADD to project:**
```xml
<!-- Phase 0: EventToBlackboardBridge -->
<ClInclude Include="Source\TaskSystem\EventToBlackboardBridge.h" />
<ClCompile Include="Source\TaskSystem\EventToBlackboardBridge.cpp" />

<!-- Phase 0: SensorSystem -->
<ClInclude Include="Source\ECS\Systems\SensorSystem.h" />
<ClCompile Include="Source\ECS\Systems\SensorSystem.cpp" />
<ClInclude Include="Source\ECS\Components\SensorComponent.h" />

<!-- Phase 0: AI_MoveTask -->
<ClInclude Include="Source\TaskSystem\AtomicTasks\AI_MoveTask.h" />
<ClCompile Include="Source\TaskSystem\AtomicTasks\AI_MoveTask.cpp" />

<!-- Phase 1: CompatibilityLayer -->
<ClInclude Include="Source\TaskSystem\CompatibilityLayer.h" />
<ClCompile Include="Source\TaskSystem\CompatibilityLayer.cpp" />

<!-- Tests -->
<ClCompile Include="Tests\TaskSystem\EventToBlackboardBridgeTests.cpp" />
<ClCompile Include="Tests\ECS\Systems\SensorSystemTests.cpp" />
<ClCompile Include="Tests\TaskSystem\AtomicTasks\AI_MoveTaskTests.cpp" />
<ClCompile Include="Tests\TaskSystem\CompatibilityLayerTests.cpp" />
```

---

## 🔄 FILE DEPENDENCY MATRIX

```
Before (Legacy):
├─ ECS_Systems_AI.h/cpp (5 systems, 1660 LOC)
│  ├─ ECS_Components_AI.h (AIBlackboard, AISenses, AIState, BT Runtime)
│  ├─ EventQueue
│  ├─ BehaviorTree.h/cpp
│  └─ PhysicsBody_data, Position_data
│
├─ BehaviorTree.h/cpp (600 LOC)
│  ├─ BehaviorTreeManager.h/cpp
│  ├─ BehaviorTreeAdapter.h
│  └─ ECS_Components_AI.h
│
└─ BehaviorTreeDebugWindow*.h/cpp (300+ LOC)
   ├─ BehaviorTreeEditorPlugin.h/cpp
   ├─ BehaviorTreeRenderer.h/cpp
   └─ BehaviorTree.h/cpp

After (Unified):
├─ EventToBlackboardBridge.h/cpp (NEW)
│  ├─ EventQueue
│  └─ LocalBlackboard
│
├─ SensorSystem.h/cpp (NEW)
│  ├─ LocalBlackboard
│  ├─ SpatialPartitioning
│  └─ CollisionSystem (raycast)
│
├─ AI_MoveTask.h/cpp (NEW)
│  ├─ IAtomicTask
│  ├─ LocalBlackboard
│  └─ PathfindingSystem
│
├─ CompatibilityLayer.h/cpp (NEW)
│  └─ LocalBlackboard
│
└─ TaskRunnerComponent.h (EXISTING)
   ├─ TaskGraphTemplate
   ├─ VSGraphExecutor
   └─ LocalBlackboard
```

---

## 🎯 FILE OPERATION CHECKLIST

### P0 (Create + Test)
- [ ] Create: EventToBlackboardBridge.h/cpp + tests
- [ ] Create: SensorSystem.h/cpp + SensorComponent.h + tests
- [ ] Create: AI_MoveTask.h/cpp + tests
- [ ] Create: ai_state_transitions.ats + ai_move_behavior.ats
- [ ] Update: .vcxproj (add all new files)
- [ ] Verify: All tests pass

### P1 (Create + Archive)
- [ ] Create: CompatibilityLayer.h/cpp + tests
- [ ] Update: ECS_Components_AI.h → stub
- [ ] Archive: ECS_Systems_AI.h/cpp (backup)
- [ ] Archive: BehaviorTreeRuntime_data
- [ ] Update: World.cpp (remove system registrations)
- [ ] Verify: Clean build

### P2 (Archive + Modify)
- [ ] Archive: AIPerceptionSystem files
- [ ] Modify: ai_state_transitions.ats (actual conditions)
- [ ] Modify: World.cpp (load state graph)
- [ ] Verify: AI behavior unchanged

### P3 (Archive + Convert)
- [ ] Convert: All BT files → .ats files (BT→ATS converter)
- [ ] Archive: BehaviorTreeSystem files
- [ ] Archive: AIMotionSystem files
- [ ] Verify: All NPC behaviors working

### P4 (Archive + Final)
- [ ] Archive: BehaviorTree editor tools
- [ ] Archive: BehaviorTree core files
- [ ] Archive: BehaviorTreeManager files
- [ ] Verify: Release build clean

---

## 📊 FINAL FILE COUNT

```
BEFORE:
├─ Legacy AI Systems:       5 files
├─ Legacy Components:       1 file (with 4 structs)
├─ BehaviorTree Engine:     3 files
├─ BehaviorTree Editor:     7 files
├─ BehaviorTree Manager:    2 files
└─ TOTAL:                   18+ files (~7710 LOC)

AFTER:
├─ New Components:          3 files (EventBridge, SensorSys, AI_Move)
├─ New Compat Layer:        1 file (CompatibilityLayer)
├─ Legacy (Archived):       18+ files (in Archive/)
├─ Tests:                   4 files
└─ TOTAL ACTIVE:            8 files (~2700 LOC active)

ARCHIVED: 18+ files (~4000+ LOC legacy)
```

---

**Document:** `Project Management/Features/FILE_MANIFEST_27-03-2026.md`  
**Status:** ✅ COMPLETE  
**Use:** Reference when performing file operations
