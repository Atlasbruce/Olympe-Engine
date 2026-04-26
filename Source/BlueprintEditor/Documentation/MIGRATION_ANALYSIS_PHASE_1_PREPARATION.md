# Migration Analysis: Legacy Graph Types to Framework
**Status**: ✅ ANALYSIS COMPLETE - READY FOR IMPLEMENTATION
**Date**: 2026-04-09
**Analyst**: GitHub Copilot
**Scope**: BehaviorTree, EntityPrefab, VisualScript → Modern Framework

---

## 1. FRAMEWORK FOUNDATION (Already Exists ✅)

### Core Components Available
```
✅ IGraphRenderer (interface contract)
   - Load(path), Save(path), Render(), GetGraphType(), IsDirty()
   - RenderFrameworkModals() for Save/SaveAs dialogs

✅ GraphEditorBase (80% functionality provided)
   - Selection management, dirty flag tracking
   - Pan/zoom utilities, grid management
   - Keyboard shortcuts (Ctrl+S, Delete, etc.)
   - Template methods for customization

✅ CanvasFramework (toolbar + modals)
   - Unified [Save] [SaveAs] [Browse] buttons
   - Grid toggle, Reset View, Minimap toggle
   - File dialogs and error handling

✅ Canvas Utilities
   - CustomCanvasEditor: Pan/zoom/coordinate transformation
   - CanvasGridRenderer: Standardized grid appearance
   - CanvasMinimapRenderer: Bird's-eye view for all canvas types

✅ PlaceholderEditor (reference implementation)
   - Complete working example using all framework features
   - Can be studied as "correct pattern" for any new type
```

---

## 2. LEGACY ANALYSIS BY TYPE

### BehaviorTree Legacy Structure
```
Current Implementation:
├─ BTNodeGraphManager (backend)
│  ├─ Manages graph state (nodes, connections)
│  ├─ Handles serialization to JSON
│  └─ CRUD operations on nodes/links
├─ NodeGraphPanel (shared rendering)
│  ├─ ImGui + imnodes rendering
│  ├─ Mouse interactions (pan/zoom/drag)
│  └─ Selection management
└─ BehaviorTreeRenderer (adapter)
   ├─ IGraphRenderer implementation
   ├─ Wraps BTNodeGraphManager
   └─ Delegates to NodeGraphPanel for rendering

Data Model:
- NodeType enum (16+ types)
- Node: ID, Type, Position, Parameters, Children (tree structure)
- Link: SourceID, TargetID, implicit connections (parent-child)
- JSON Schema: v3/v4 with nodes[], links[] arrays

Key Features:
✓ Copy/Paste nodes (ClipboardNode structure)
✓ Drag-drop from palette
✓ Property editing (node parameters)
✓ Execution simulation (ExecutionTestPanel)
✓ Canvas state save/restore

Legacy Issues:
✗ Mixes concerns (BTNodeGraphManager + NodeGraphPanel)
✗ Canvas interaction code scattered across multiple files
✗ No unified coordinate transformation
✗ Copy/Paste logic hardcoded
✗ Can't reuse in other contexts easily
```

### EntityPrefab Legacy Structure
```
Current Implementation:
├─ EntityPrefabGraphDocument (data model)
│  ├─ Holds ComponentNode structures
│  ├─ JSON serialization
│  └─ Node CRUD operations
├─ PrefabCanvas (custom rendering)
│  ├─ ImGui-based canvas
│  ├─ Pan/zoom/drag interactions
│  └─ Connection visualization
├─ ComponentPalettePanel (drag-drop source)
└─ EntityPrefabRenderer (adapter)
   └─ Connects all pieces

Data Model:
- ComponentNode: ID, ComponentType, Position, Properties
- Connection: SourceID, TargetID, PortIndices
- JSON Schema: nodes[], connections[], metadata

Key Features:
✓ Custom canvas rendering (not imnodes)
✓ Drag-drop component instantiation
✓ Port-based connections
✓ Rectangle selection
✓ Property editing via PropertyEditorPanel

Legacy Issues:
✗ Canvas interactions hardcoded
✗ Coordinate transformation formulas scattered
✗ Minimap partially implemented
✗ No unified grid system
```

### VisualScript Legacy Structure
```
Current Implementation:
├─ VisualScriptEditorPanel (monolithic)
│  ├─ ImNodes integration
│  ├─ Node rendering
│  ├─ Interaction handling
│  ├─ Multiple file operations
│  └─ ~1500+ lines in main class
├─ TaskGraphTemplate (data model)
│  ├─ TaskNodeDefinition structures
│  └─ Connection definitions
└─ VisualScriptGraphDocument (adapter)
   └─ Framework integration

Data Model:
- VSEditorNode: ID, Position, TaskDefinition
- VSEditorLink: ID, SourceAttr, TargetAttr, isData flag
- JSON Schema: v4 with nodes[], connections[], presets[]

Key Features:
✓ ImNodes-based rendering (native appearance)
✓ Execution/data flow distinction
✓ Condition presets (embedded in graph)
✓ Blackboard integration
✓ Undo/Redo system
✓ Complex type validation

Legacy Issues:
✗ ~1500 lines in single file
✗ Multiple rendering passes (frame cycle issues)
✗ ImGui state management scattered
✗ Canvas utilities not centralized
```

---

## 3. MAPPING: Legacy → Framework

### Concept Mapping
```
Legacy Concept          → Framework Equivalent
─────────────────────────────────────────────
BTNodeGraphManager      → IGraphDocument adapter
NodeGraphPanel          → CustomCanvasEditor (imnodes variant)
BehaviorTreeRenderer    → GraphEditorBase subclass
Node CRUD operations    → Implement in IGraphDocument
Interaction handling    → RenderGraphContent() override
Copy/Paste             → Standard framework patterns
Pan/Zoom               → CustomCanvasEditor methods
Selection              → GraphEditorBase::SelectNode()
Dirty flag             → GraphEditorBase::MarkDirty()
Canvas state save      → CustomCanvasEditor state
```

### File Routing Discovery
```
TabManager::DetectGraphType()      ← Returns "BehaviorTree"
TabManager::CreateRenderer()       ← New: Create framework version
                                      Old: Create legacy version
                                      
Result: File opens in NEW renderer (legacy untouched)
```

---

## 4. ARCHITECTURE FOR MIGRATION

### New Structure (For Each Type)

**BehaviorTree Example:**
```
Source/BlueprintEditor/BehaviorTreeEditor/
├─ BehaviorTreeGraphDocument.h/cpp
│  └─ Holds node/link data, serialization
├─ BehaviorTreeCanvas.h/cpp
│  └─ Canvas rendering + interactions
│  └─ Inherits: CustomCanvasEditor or ImNodesCanvasEditor
├─ BehaviorTreePropertyPanel.h/cpp
│  └─ Node property editing UI
├─ BehaviorTreeNodePalette.h/cpp
│  └─ Drag-drop node creation
└─ BehaviorTreeEditorV2.h/cpp
   └─ Main renderer (inherits: GraphEditorBase)
   └─ Orchestrates above components

→ PlaceholderEditor shows this exact pattern (3 types already working!)
```

### Code Reuse Strategy
```
Phase 1: Analysis (CURRENT)
└─ Study legacy, document patterns
└─ Create migration architecture

Phase 2: Create New Implementations (NEXT)
├─ Copy relevant legacy code → new locations
├─ Refactor for framework compatibility
├─ Use legacy as reference (parallel development)
└─ NO modifications to legacy code

Phase 3: Integration Testing
├─ Load same files in new + legacy renderers
├─ Compare outputs
├─ Fix discrepancies
└─ Keep legacy for troubleshooting

Phase 4: Gradual Cutover
├─ Route new files to new renderer
├─ Old files still load in legacy (if needed)
├─ Verify all features work
└─ Eventually remove legacy code

Result: Zero risk regression, easy debugging
```

---

## 5. FEATURE COMPLETENESS CHECK

### Framework Capabilities Available
```
Toolbar
├─ [Save] button              ✅ CanvasToolbarRenderer
├─ [SaveAs] file dialog      ✅ CanvasToolbarRenderer::RenderModals()
├─ [Browse] open file        ✅ CanvasToolbarRenderer
├─ [Grid ☑] toggle           ✅ CustomCanvasEditor::RenderGrid()
├─ [Reset View]              ✅ CustomCanvasEditor::ResetViewport()
└─ [Minimap ☑] toggle        ✅ CanvasMinimapRenderer

Canvas Interactions
├─ Pan (middle-mouse)         ✅ CustomCanvasEditor::Pan()
├─ Zoom (scroll wheel)        ✅ CustomCanvasEditor::SetZoom()
├─ Left-click select          ✅ GraphEditorBase::SelectNode()
├─ Ctrl+Click multi-select    ✅ GraphEditorBase::SelectMultipleNodes()
├─ Right-click context menu   ✅ GraphEditorBase::RenderContextMenu()
└─ Delete key                 ✅ Handled by framework

Coordinate System
├─ ScreenToCanvas()           ✅ CustomCanvasEditor
├─ CanvasToScreen()           ✅ CustomCanvasEditor
├─ Grid scaling               ✅ CanvasGridRenderer
└─ Minimap normalization      ✅ CanvasMinimapRenderer

File Management
├─ Load/Save with path        ✅ IGraphRenderer contract
├─ Dirty flag tracking        ✅ GraphEditorBase
├─ Tab management             ✅ TabManager
└─ File caching               ✅ TabManager (prevent reload)

State Management
├─ Selection state            ✅ GraphEditorBase::m_selectedNodeIds
├─ Hover state                ✅ GraphEditorBase::m_hoveredNodeId
├─ Context menu state         ✅ GraphEditorBase::m_contextNodeId
└─ Canvas pan/zoom state      ✅ CustomCanvasEditor (saveable)
```

### What Each Type Must Implement
```
BehaviorTree Specifics:
├─ Load() → Parse BT JSON into m_nodes/m_links
├─ Save() → Serialize m_nodes/m_links to JSON
├─ RenderGraphContent() → Render nodes + connections
├─ InitializeCanvasEditor() → Create imnodes or custom canvas
└─ Property panel for BT node parameters

EntityPrefab Specifics:
├─ Load() → Parse component JSON
├─ Save() → Serialize components + connections
├─ RenderGraphContent() → Render component nodes
├─ InitializeCanvasEditor() → Custom canvas (already done!)
└─ Component palette + property panel (already exists!)

VisualScript Specifics:
├─ Load() → Parse VisualScript JSON
├─ Save() → Serialize nodes + connections
├─ RenderGraphContent() → Render with exec/data distinction
├─ InitializeCanvasEditor() → imnodes with task rendering
└─ Blackboard + condition presets (already complex!)
```

---

## 6. VALIDATION CRITERIA

### Successful Migration = All Features Working

```
Load/Save
  ✓ Load file → nodes appear on canvas
  ✓ Modify graph (drag, delete, etc.)
  ✓ File shows dirty indicator (asterisk)
  ✓ Click [Save] → file saved, asterisk gone
  ✓ Load same file again → data preserved

Rendering
  ✓ Grid renders behind nodes
  ✓ Nodes render with correct positions
  ✓ Connections draw between nodes
  ✓ Minimap shows all nodes + viewport rect

Interaction
  ✓ Pan canvas (middle-mouse drag)
  ✓ Zoom canvas (scroll wheel)
  ✓ Select node (left-click)
  ✓ Multi-select (Ctrl+Click)
  ✓ Right-click context menu appears
  ✓ Delete key removes selected node
  ✓ Drag node to move (visual feedback)

Toolbar
  ✓ [Save] button works
  ✓ [SaveAs] shows file dialog
  ✓ [Browse] opens file browser
  ✓ [Grid] toggle works
  ✓ [Reset View] resets pan/zoom
  ✓ [Minimap] toggle works

Type-Specific Features
  ✓ BehaviorTree: All node types render, params editable
  ✓ EntityPrefab: Components drag-droppable, ports work
  ✓ VisualScript: Tasks render, blackboard works
```

---

## 7. READINESS CHECKLIST

### Analysis Complete ✅
- [x] Framework architecture understood
- [x] PlaceholderEditor pattern verified
- [x] Legacy code locations identified
- [x] Data models documented
- [x] Feature gaps identified
- [x] Migration strategy defined

### Ready for Phase 2: Implementation
```
Next Steps (When User Ready):
1. Select ONE graph type (e.g., EntityPrefab - simplest)
2. Create new directory structure
3. Copy relevant legacy code
4. Refactor for framework compatibility
5. Test with reference files
6. Iterate through remaining types

Small Incremental Chunks:
  Chunk 1: Core document + basic rendering
  Chunk 2: File I/O (Load/Save)
  Chunk 3: Interactions (select, drag, delete)
  Chunk 4: Type-specific features
  Chunk 5: Integration testing

Build Verification After Each Chunk:
  - Must compile with 0 errors, 0 warnings
  - Legacy code untouched and still compilable
  - New renderer loads same file as legacy
  - Output visually identical
```

---

## 8. RECOMMENDED MIGRATION ORDER

### Why This Order?
```
1. EntityPrefab (SIMPLEST)
   ✓ Smallest codebase (PrefabCanvas, ComponentNode)
   ✓ Custom rendering (no imnodes complexity)
   ✓ Already partially using framework
   ✓ Clear data model (nodes + components + connections)
   ✓ Property panel already exists
   → Best learning case for framework patterns

2. BehaviorTree (MEDIUM)
   ✓ Well-structured (BTNodeGraphManager clean)
   ✓ Rich node types (many to test)
   ✓ Copy/paste feature (additional complexity)
   ✓ Execution simulation (bonus feature to adapt)
   ✗ Shared NodeGraphPanel (might require refactoring)

3. VisualScript (MOST COMPLEX)
   ✓ Most mature implementation
   ✓ Rich feature set (presets, blackboard, undo/redo)
   ✓ ImNodes complexity (but framework handles this)
   ✗ 1500+ line monolithic class
   ✗ Many interdependencies
   → Wait until patterns proven with other types
```

---

## 9. KNOWN CHALLENGES & MITIGATIONS

```
Challenge 1: Shared NodeGraphPanel (BehaviorTree legacy)
  Issue: Multiple renderers might share same panel
  Mitigation: New implementation uses CustomCanvasEditor
            (doesn't depend on shared NodeGraphPanel)
  Result: Complete independence

Challenge 2: ImNodes State Management (VisualScript)
  Issue: ImNodes context switching between editor instances
  Mitigation: ImNodesCanvasEditor already handles this (Phase 37)
             Framework provides tested pattern
  Result: No additional complexity

Challenge 3: Copy/Paste Implementation (BehaviorTree)
  Issue: Clipboard data structures are complex
  Mitigation: Implement as part of "batch operations"
             Framework provides SelectAll() patterns
  Result: Systematic approach, not ad-hoc

Challenge 4: Coordinate Transformation (All Types)
  Issue: Different canvas types use different coordinate spaces
  Mitigation: CustomCanvasEditor provides unified ScreenToCanvas()
             Fixed formula: (screen - origin - offset) / zoom
  Result: Consistent behavior across all types

Challenge 5: Keeping Legacy Untouched
  Issue: Risk of accidentally modifying legacy code
  Mitigation: Create separate file directory structure
             New: Source/BlueprintEditor/BehaviorTreeEditor/
             Old: Source/BlueprintEditor/ (legacy only)
             Different naming to avoid conflicts
  Result: Clean separation, easy rollback if needed
```

---

## 10. SUCCESS METRICS

### Phase Completion Criteria
```
✅ Phase 1 (ANALYSIS): Current Status
   - Framework components identified
   - Legacy code documented
   - Migration path planned
   - This document created

✅ Phase 2 (FIRST TYPE - EntityPrefab):
   - New directory structure created
   - Document + Canvas + Renderer implemented
   - File I/O working
   - Framework integration verified
   - All 10 validation criteria passing
   - 0 errors, 0 warnings at build

✅ Phase 3 (SECOND TYPE - BehaviorTree):
   - Same criteria as Phase 2
   - Copy/Paste feature adapted
   - Execution simulation adapted (optional)

✅ Phase 4 (THIRD TYPE - VisualScript):
   - Same criteria as Phase 2
   - Blackboard integration adapted
   - Presets/Undo-Redo adapted

✅ Phase 5 (CUTOVER):
   - New renderers routed in TabManager
   - Legacy files still load in new renderer
   - All features verified working
   - Documentation updated
   - Legacy code marked for removal
```

---

## CONCLUSION

**Status**: ✅ **ANALYSIS COMPLETE AND READY FOR IMPLEMENTATION**

The framework is production-ready and comprehensive. All three legacy graph types can be successfully migrated by:

1. **Understanding** the framework patterns (GraphEditorBase, CustomCanvasEditor, CanvasFramework)
2. **Studying** PlaceholderEditor as reference implementation
3. **Migrating incrementally** one type at a time
4. **Preserving** legacy code for comparison and troubleshooting
5. **Testing** rigorously after each chunk

The migration is **low-risk** because:
- Framework is tested and proven (Phase 76 complete)
- PlaceholderEditor shows exact correct pattern
- Legacy code stays untouched (can rollback anytime)
- Incremental approach allows verification at each step
- Build system can verify success immediately

**Next Action**: User provides instructions for Phase 2 (implementation) when ready. Recommend starting with **EntityPrefab** as simplest learning case.

---

**Estimated Implementation Timeline**:
- EntityPrefab: 2-3 hours (learning + implementation)
- BehaviorTree: 2-3 hours (pattern proven, complexity similar)
- VisualScript: 3-4 hours (larger codebase, but framework handles complexity)
- Total: 7-10 hours for full migration

**Quality Assurance**:
- Build verification after each chunk (automated)
- Visual comparison: legacy vs new renderer (manual)
- Feature checklist (all 10 items per type)
- Runtime testing with reference files
