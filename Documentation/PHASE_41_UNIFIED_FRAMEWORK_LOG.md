# PHASE 41: Unified Canvas/Tab/Document Framework - Complete Chronological Log

**Project Objective**: Implement unified framework where all graph types (VisualScript, EntityPrefab, BehaviorTree) use same Document interface, toolbar, modals, and canvas controls.

**Status**: IN PROGRESS

---

## TIMELINE & PROGRESSION

### 2024 - SESSION START - Framework Design & Planning Phase

#### [SESSION_START] Framework Design Initiated
**Time**: Current Session  
**Task**: Plan unified canvas/tab/document framework architecture

**Key Discovery - Root Cause Analysis**:
```
Problem Description:
- Visual Script: Save/SaveAs don't open modals (Browse works)
- Entity Prefab: Missing Save/SaveAs buttons (Browse works)  
- BehaviorTree: Browse broken, Save/SaveAs missing (MOST BROKEN)
- SaveAs modal: Missing left folder selection panel

Root Cause:
No unified framework → three graph types manage modals/toolbars independently
Inconsistent implementations across editors cause inconsistent behavior
```

**User Requirements Identified**:
1. SaveAs modal MUST have folder panel on LEFT (mentioned 2x)
2. All editors must show identical toolbar
3. Browse must work for ALL graph types
4. Same canvas features: Minimap, Pan, Zoom, Snap
5. Same side panels: Palette + Properties
6. Unified document lifecycle: Load → Edit → SaveAs → Close

**Framework Design Approach**:
- Polymorphism via IGraphDocument interface
- Centralization: CanvasFramework orchestrator
- Adapters for VisualScript and BehaviorTree

---

## IMPLEMENTATION PLAN - 16 STEPS

**Created**: Current Session  
**Total Steps**: 16 (Design → Implement → Test → Document)

### Phase 1: Core Framework Design (Steps 1-4)
- [STEP-1] Design IGraphDocument interface ← **CURRENT**
- [STEP-2] Refactor EntityPrefabGraphDocument → implement IGraphDocument
- [STEP-3] Create VisualScript adapter (IGraphDocument wrapper)
- [STEP-4] Create BehaviorTree adapter (IGraphDocument wrapper)

### Phase 2: Modal & File System (Steps 5-6)
- [STEP-5] Fix SaveFilePickerModal: add folder panel (LEFT SIDE - CRITICAL)
- [STEP-6] Fix FilePickerModal: support all file types, fix BT Browse

### Phase 3: Unified UI Components (Steps 7-10)
- [STEP-7] Create CanvasToolbarRenderer (Save/SaveAs/Browse buttons)
- [STEP-8] Refactor TabManager (centralize modal operations)
- [STEP-9] Create CanvasControlsRenderer (Minimap, Pan, Zoom, Snap)
- [STEP-10] Create unified SidePanelRenderer (Palette + Properties)

### Phase 4: Integration (Steps 11-14)
- [STEP-11] Integrate EntityPrefabRenderer with framework
- [STEP-12] Integrate VisualScriptEditorPanel with framework
- [STEP-13] Integrate BehaviorTreeRenderer with framework
- [STEP-14] Create CanvasFramework orchestrator

### Phase 5: Validation (Steps 15-16)
- [STEP-15] Test unified workflow (Load → Edit → SaveAs)
- [STEP-16] Document framework & architecture

---

## STEP 1: Design IGraphDocument Interface

**Status**: ✅ COMPLETED  
**Started**: Current Session  
**Completed**: Current Session  
**Duration**: 15 minutes

### Requirements Analysis

**What is IGraphDocument?**
- Abstract base class defining contract for all graph document types
- Enables polymorphic handling of VisualScript, EntityPrefab, BehaviorTree
- Centralizes lifecycle: Load, Edit, Save, Dirty tracking

**Core Responsibilities**:
```
1. Document State Management
   - Load(filePath): Load document from file
   - Save(filePath): Save document to file
   - IsDirty(): Check if unsaved changes exist

2. Document Metadata
   - GetName(): Return document display name (e.g., "SimpleGraph.bt.json")
   - GetType(): Return document type (VISUAL_SCRIPT, BEHAVIOR_TREE, ENTITY_PREFAB)
   - GetFilePath(): Return current file path
   - SetFilePath(path): Update file path

3. Renderer Integration
   - GetRenderer(): Return IGraphRenderer for canvas rendering
   - OnDocumentModified(): Notify framework when changes occur

4. Serialization Format Support
   - GetSchemaVersion(): Return JSON schema version (v4, etc)
   - CanExportAs(format): Check if export to other format possible
```

**Design Constraints**:
- Must support three existing graph types without breaking changes
- Must work with existing Tab system (TabManager)
- Must integrate with existing Modal system (DataManager)
- C++14 compatible (no C++17 features)
- Minimal dependencies (avoid circular includes)

**File Structure**:
```cpp
namespace Olympe {

// Document type enumeration
enum class DocumentType {
    VISUAL_SCRIPT,
    BEHAVIOR_TREE,
    ENTITY_PREFAB,
    UNKNOWN
};

// Abstract interface
class IGraphDocument {
public:
    virtual ~IGraphDocument() = default;
    
    // Lifecycle
    virtual bool Load(const std::string& filePath) = 0;
    virtual bool Save(const std::string& filePath) = 0;
    virtual bool IsDirty() const = 0;
    
    // Metadata
    virtual std::string GetName() const = 0;
    virtual DocumentType GetType() const = 0;
    virtual std::string GetFilePath() const = 0;
    virtual void SetFilePath(const std::string& path) = 0;
    
    // Rendering
    virtual IGraphRenderer* GetRenderer() = 0;
    virtual void OnDocumentModified() = 0;
};

} // namespace Olympe
```

**Implementation Targets** (existing classes to adapt):
1. EntityPrefabGraphDocument - Already has Load/Save, just needs interface
2. VisualScriptGraphDocument (NEW) - Wrap existing VisualScript tab data
3. BehaviorTreeGraphDocument (NEW) - Wrap existing BehaviorTree tab data

**Integration Points**:
- TabManager: Will use IGraphDocument* instead of type-specific handling
- DataManager: Modals will receive IGraphDocument* for file operations
- CanvasFramework: Will orchestrate document + rendering + toolbar
- Toolbar: Will conditionally enable buttons based on GetType()

### Design Decisions

**Decision 1: Adapter vs Inheritance**
- EntityPrefabGraphDocument: Direct inheritance (already owns data)
- VisualScript/BehaviorTree: Adapter wrappers (encapsulate existing tab data)
- Rationale: Minimize changes to working code, use composition where possible

**Decision 2: Renderer Access**
- IGraphDocument::GetRenderer() returns IGraphRenderer*
- Allows CanvasFramework to render any document type uniformly
- Keeps rendering logic in renderer, not document

**Decision 3: Dirty Flag Semantics**
- IsDirty() reflects unsaved changes (for "Save" button activation)
- Load() and Save() automatically manage dirty flag
- OnDocumentModified() called by editor whenever changes occur

**Decision 4: Error Handling**
- Load/Save return bool (false = error, details logged)
- Caller responsible for error display via modals
- No exceptions (consistent with existing codebase)

### Implementation Details

**File Created**: `Source/BlueprintEditor/Framework/IGraphDocument.h`

**Interface Methods**:
```cpp
// Lifecycle
virtual bool Load(const std::string& filePath) = 0;
virtual bool Save(const std::string& filePath) = 0;
virtual bool IsDirty() const = 0;

// Metadata
virtual std::string GetName() const = 0;
virtual DocumentType GetType() const = 0;
virtual std::string GetFilePath() const = 0;
virtual void SetFilePath(const std::string& path) = 0;

// Rendering
virtual IGraphRenderer* GetRenderer() = 0;
virtual const IGraphRenderer* GetRenderer() const = 0;

// Notifications
virtual void OnDocumentModified() = 0;
```

**Key Design Decisions**:
1. ✅ Adapter pattern for VisualScript/BehaviorTree (minimal changes to existing code)
2. ✅ Direct inheritance for EntityPrefabGraphDocument (already owns data)
3. ✅ Error handling via return bool (no exceptions, consistent with codebase)
4. ✅ Dirty flag managed by lifecycle methods
5. ✅ Renderer access via GetRenderer() (polymorphic rendering)
6. ✅ Static helper for type name display

---

## STEP 2: Refactor EntityPrefabGraphDocument to implement IGraphDocument

**Status**: ✅ COMPLETED  
**Started**: Current Session  
**Completed**: Current Session  
**Duration**: 20 minutes

### Implementation Summary

**File Modified**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.h`  
**File Modified**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.cpp`

**Changes Made**:
1. ✅ Added inheritance: `class EntityPrefabGraphDocument : public IGraphDocument`
2. ✅ Added include: `#include "../Framework/IGraphDocument.h"`
3. ✅ Added forward declaration: `class EntityPrefabRenderer`
4. ✅ Implemented all interface methods:
   - `Load(filePath)` → delegates to `LoadFromFile()`, updates `m_filePath`
   - `Save(filePath)` → delegates to `SaveToFile()`, updates `m_filePath`
   - `GetName()` → returns filename or document name
   - `GetType()` → returns `DocumentType::ENTITY_PREFAB`
   - `GetFilePath()` → returns current `m_filePath`
   - `SetFilePath(path)` → updates `m_filePath`
   - `GetRenderer()` → returns nullptr (TODO: integrate with EntityPrefabRenderer)
   - `OnDocumentModified()` → sets `m_isDirty = true`
5. ✅ Added member: `std::string m_filePath` for tracking current file path
6. ✅ Modified `FromJson()` return type from `EntityPrefabGraphDocument` to `GraphDocumentPtr` (avoids deleted copy constructor issue)

**Build Result**: ✅ **0 EntityPrefabGraphDocument compilation errors**

### Architecture Notes

- All interface method implementations are straightforward wrappers
- `GetRenderer()` returns nullptr temporarily (will be implemented when EntityPrefabRenderer is integrated)
- `FromJson()` now returns `std::shared_ptr<IGraphDocument>` for consistency with document lifetime management
- Minimal changes to existing code, maintains backward compatibility

### Next Step  
Step 3: Create VisualScript adapter (IGraphDocument wrapper)

---

## CRITICAL REQUIREMENTS TRACKING

| Requirement | Status | Notes |
|-------------|--------|-------|
| SaveAs modal folder panel (LEFT) | ⏳ PENDING (Step 5) | Mentioned 2x, critical feature |
| Identical toolbar all editors | ⏳ PENDING (Step 7) | Save/SaveAs/Browse buttons |
| Browse working for all types | ⏳ PENDING (Step 6) | BT Browse currently broken |
| Same canvas controls | ⏳ PENDING (Step 9) | Minimap, Pan, Zoom, Snap |
| Unified side panels | ⏳ PENDING (Step 10) | Palette + Properties |

---

## ARCHITECTURE OVERVIEW

```
┌─────────────────────────────────────────────────────────┐
│                    TabManager                            │
│              (Manages all document tabs)                │
└──────────────┬──────────────────────────────────────────┘
               │
       ┌───────▼────────┐
       │  IGraphDocument│  ◄─── Polymorphic interface
       │   (Abstract)   │       - Load/Save
       │                │       - GetRenderer
       │                │       - IsDirty, Metadata
       └───────▲────────┘
               │
    ┌──────────┼──────────┬──────────────┐
    │          │          │              │
┌───▼──────┐┌──▼────────┐┌▼─────────┐┌──▼──────────┐
│ EntityPf │││VisualScript││BehaviorTree││ (Future)   │
│ Document │││ Adapter    ││ Adapter   ││ Graph Type │
└─────────┬┘└┬──────────┘└┬────────┘└───────────┘
          │   │           │
          │   └───────┬───┘
          │           │
    ┌─────▼──────┬────▼──────┐
    │ Renderer 1 │ Renderer 2 │ ... (IGraphRenderer)
    │ (EntityPf) │ (VisScript)│
    └────────────┴───────────┘
           │
    ┌──────▼──────────────┐
    │ CanvasFramework     │
    │ - Toolbar render    │
    │ - Modal handling    │
    │ - Control render    │
    └────────────────────┘
```

