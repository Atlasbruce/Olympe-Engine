# 🐛 BUG ANALYSIS - Multi-Tab Node Position Reset

## Problem Statement
When opening multiple tabs of **different graph types** (VisualScript, EntityPrefab, BehaviorTree) and switching between tabs, returning to a previously viewed tab causes **all nodes to stack at position (0,0)**, losing the graph layout.

### Reproduction Steps
1. ✅ Open VisualScript graph → nodes at correct positions
2. ✅ Open EntityPrefab graph → nodes at correct positions
3. ✅ Open BehaviorTree graph → nodes at correct positions
4. ❌ Switch back to VisualScript → **ALL nodes superposed at (0,0)**
5. ❌ Same issue when switching between ANY two tab types

## Root Cause Analysis

### Architecture Overview
```
TabManager (Central dispatcher)
    ├── EditorTab (VS, EP, BT tabs)
    │   └── IGraphRenderer* renderer
    │       ├── VisualScriptRenderer
    │       │   └── VisualScriptEditorPanel m_panel
    │       │       └── VisualScriptNodeGraphEditor (imnodes + EditorContext)
    │       │
    │       ├── EntityPrefabRenderer
    │       │   └── PrefabCanvas& m_canvas
    │       │       └── CustomCanvasEditor (handles pan/zoom)
    │       │
    │       └── BehaviorTreeRenderer
    │           └── NodeGraphPanel& m_panel (shared reference)
    │               └── imnodes rendering
```

### The Critical Issue: No Per-Renderer Canvas State Storage

**PROBLEM #1: VisualScriptEditorPanel**
```cpp
// VisualScriptRenderer.cpp:34-36
void VisualScriptRenderer::Render()
{
    m_panel.RenderContent();  // ← m_panel is stateful
}

// VisualScriptEditorPanel has imnodes EditorContext
// When tab is switched AWAY, state is preserved in m_panel
// BUT: When another renderer modifies imnodes state (like EntityPrefab or BT),
// it may affect the shared GLOBAL imnodes context
```

**PROBLEM #2: EntityPrefabRenderer**
```cpp
// EntityPrefabRenderer.cpp:26-99
void EntityPrefabRenderer::Render()
{
    RenderLayoutWithTabs();
    // Initializes CustomCanvasEditor with default state
    // Creates NEW adapter on first frame
    // On tab SWITCH AWAY and BACK, adapter might reinitialize or state lost
}
```

**PROBLEM #3: BehaviorTreeRenderer**
```cpp
// BehaviorTreeRenderer uses shared NodeGraphPanel reference
// NodeGraphPanel is LEGACY and likely doesn't preserve per-graph viewport state
// When another renderer uses it, state is corrupted
```

### The Actual Mechanism of Node Position Loss

#### Scenario 1: VisualScript → EntityPrefab → VisualScript

```
Frame 1: VisualScript Tab Active
├─ VisualScriptRenderer::Render()
├─ VisualScriptEditorPanel::RenderContent()
├─ imnodes::BeginNodeEditor() with EditorContext[VS_context]
├─ Nodes positioned via stored positions from file
└─ Canvas state PRESERVED in m_panel

Frame 2: User clicks EntityPrefab Tab
├─ TabManager calls SetActiveTab()
├─ VisualScriptRenderer::Render() NOT called (tab inactive)
├─ EntityPrefabRenderer::Render() called
├─ PrefabCanvas::Render() called
├─ CustomCanvasEditor created/updated
├─ Nodes positioned correctly in EntityPrefab
└─ Canvas state in EntityPrefabRenderer

Frame 3: User clicks VisualScript Tab Again
├─ TabManager calls SetActiveTab()
├─ VisualScriptRenderer::Render() called AGAIN
├─ VisualScriptEditorPanel::RenderContent() called
├─ imnodes::BeginNodeEditor() starts...
├─ ❌ BUT: EditorContext might have been modified by EntityPrefab rendering
├─ ❌ OR: Node positions loaded from graph file are at WRONG COORDINATES
├─ ❌ All nodes appear at (0,0) because:
│   ├─ EditorContext pan/zoom corrupted
│   ├─ OR nodes' stored positions were (0,0) as default
│   └─ OR viewport wasn't restored from saved state
└─ Layout LOST
```

#### Root Cause Chain

1. **imnodes Global State Issue** (if using ImNodes)
   - imnodes uses a single global EditorContext by default
   - When Tab A uses imnodes, then Tab B uses different imnodes adapter
   - Then Tab A is shown again, EditorContext is corrupted/reset

2. **Canvas Viewport Not Restored** (EntityPrefab)
   - CustomCanvasEditor stores m_canvasOffset
   - On tab switch AWAY, offset is in EntityPrefabRenderer::m_canvasEditor
   - On tab switch BACK, if m_canvasEditor reinitializes, offset is RESET to (0,0)
   - See EntityPrefabRenderer.cpp:70-94 (window resize detection)

3. **Node Positions vs Canvas Offset Mismatch** (VisualScript)
   - Nodes have absolute positions stored in JSON
   - Canvas has a viewport offset (pan amount)
   - When switching tabs, absolute positions stay same
   - BUT viewport offset resets to (0,0)
   - Result: All nodes appear at their absolute coordinates (often 0,0 for new graphs)

4. **No Per-Renderer State Persistence**
   - TabManager doesn't preserve EditorTab → Renderer → Canvas state
   - When renderer is activated/deactivated, no checkpoint occurs
   - State is implicit in renderer objects, but not protected on tab switch

---

## Affected Components

| Component | Issue | Severity |
|-----------|-------|----------|
| **VisualScriptEditorPanel** | imnodes EditorContext corruption on tab switch | 🔴 HIGH |
| **EntityPrefabRenderer** | CustomCanvasEditor offset not saved/restored | 🔴 HIGH |
| **BehaviorTreeRenderer** | Shared NodeGraphPanel corrupts state | 🔴 HIGH |
| **TabManager** | No state preservation on SetActiveTab() | 🔴 CRITICAL |
| **imnodes Integration** | Global EditorContext shared across tabs | 🔴 CRITICAL |

---

## Evidence

### EntityPrefabRenderer Canvas State Loss
```cpp
// EntityPrefabRenderer.cpp:68-93
else
{
    // Update canvas position/size (may change on window resize)
    ImVec2 currentSize = ImGui::GetContentRegionAvail();
    if (currentSize.x != m_canvasEditor->GetCanvasSize().x || 
        currentSize.y != m_canvasEditor->GetCanvasSize().y)
    {
        // ❌ PROBLEM: Saves old state then creates NEW adapter
        float oldZoom = m_canvasEditor->GetZoom();
        ImVec2 oldPan = m_canvasEditor->GetPan();

        m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
            "PrefabCanvas",
            canvasScreenPos,
            currentSize,
            oldZoom,  // ← Restored
            0.1f, 3.0f
        );

        m_canvasEditor->SetPan(oldPan);  // ← Restored

        // BUT: If tab switching triggers resize detection,
        // canvas offset might be reset!
    }
}
```

### VisualScriptRenderer No Canvas State Save/Restore
```cpp
// VisualScriptRenderer.cpp:34-36
void VisualScriptRenderer::Render()
{
    m_panel.RenderContent();
    // ❌ NO canvas state checkpoint!
    // ❌ NO viewport save/restore on tab switch!
}

// NO implementation to:
// - Save viewport before tab switch
// - Restore viewport on tab reactivation
```

### BehaviorTreeRenderer Using Shared Panel
```cpp
// BehaviorTreeRenderer uses SHARED NodeGraphPanel reference
// Multiple different graph types use same NodeGraphPanel
// State collision is inevitable
```

---

## Solution Architecture

### Solution 1: Per-Renderer Canvas State Storage (ESSENTIAL)

Each renderer must save/restore its own viewport state:

```cpp
// IGraphRenderer interface
class IGraphRenderer
{
public:
    // NEW: State management methods
    virtual void SaveCanvasState() = 0;      // Called on tab deactivation
    virtual void RestoreCanvasState() = 0;   // Called on tab reactivation
    virtual std::string GetCanvasStateJSON() const = 0;
    virtual void SetCanvasStateJSON(const std::string& json) = 0;
};

// Implementation in each renderer
class VisualScriptRenderer : public IGraphRenderer
{
private:
    struct CanvasState {
        ImVec2 viewport_offset;
        float zoom;
        std::string editorContextState;  // imnodes context snapshot
    } m_savedCanvasState;

    void SaveCanvasState() override
    {
        m_savedCanvasState.viewport_offset = m_panel.GetCanvasOffset();
        m_savedCanvasState.zoom = m_panel.GetCanvasZoom();
        // Save imnodes EditorContext state
    }

    void RestoreCanvasState() override
    {
        m_panel.SetCanvasOffset(m_savedCanvasState.viewport_offset);
        m_panel.SetCanvasZoom(m_savedCanvasState.zoom);
        // Restore imnodes EditorContext state
    }
};
```

### Solution 2: Tab Switch Lifecycle Hooks

Modify TabManager to call state management:

```cpp
// TabManager.cpp
void TabManager::SetActiveTab(const std::string& tabID)
{
    // NEW: Save previous tab's canvas state
    EditorTab* previousTab = GetActiveTab();
    if (previousTab && previousTab->renderer)
    {
        previousTab->renderer->SaveCanvasState();  // ← NEW
    }

    // Switch to new tab
    for (size_t i = 0; i < m_tabs.size(); ++i)
        m_tabs[i].isActive = (m_tabs[i].tabID == tabID);
    m_activeTabID = tabID;
    m_pendingSelectTabID = tabID;

    // NEW: Restore new tab's canvas state
    EditorTab* newTab = GetActiveTab();
    if (newTab && newTab->renderer)
    {
        newTab->renderer->RestoreCanvasState();  // ← NEW
    }
}
```

### Solution 3: Independent Canvas State per Graph Type

```cpp
// EntityPrefabRenderer must track canvas offset separately
class EntityPrefabRenderer : public IGraphRenderer
{
private:
    ImVec2 m_savedCanvasOffset = ImVec2(0, 0);
    float m_savedCanvasZoom = 1.0f;

    void SaveCanvasState() override
    {
        if (m_canvasEditor)
        {
            m_savedCanvasOffset = m_canvasEditor->GetPan();
            m_savedCanvasZoom = m_canvasEditor->GetZoom();
        }
    }

    void RestoreCanvasState() override
    {
        if (m_canvasEditor)
        {
            m_canvasEditor->SetPan(m_savedCanvasOffset);
            m_canvasEditor->SetZoom(m_savedCanvasZoom);
        }
    }
};
```

### Solution 4: imnodes EditorContext Isolation

Ensure each VisualScript renderer has its own EditorContext:

```cpp
// VisualScriptEditorPanel.h
class VisualScriptEditorPanel
{
private:
    ImNodesEditorContext* m_editorContext = nullptr;

    void Initialize()
    {
        // Create UNIQUE context for this panel instance
        m_editorContext = ImNodes::EditorContextCreate();
        ImNodes::EditorContextSet(m_editorContext);
    }

    void Shutdown()
    {
        if (m_editorContext)
        {
            ImNodes::EditorContextDestroy(m_editorContext);
            m_editorContext = nullptr;
        }
    }

    void RenderContent()
    {
        // CRITICAL: Activate correct context before rendering
        ImNodes::EditorContextSet(m_editorContext);  // ← Set context
        // ... render nodes ...
        ImNodes::EditorContextSet(nullptr);          // ← Unset context
    }
};
```

---

## Implementation Priority

### Phase 1: CRITICAL (Immediate)
- [ ] Add SaveCanvasState() / RestoreCanvasState() to IGraphRenderer
- [ ] Implement in all 3 renderers (VS, EP, BT)
- [ ] Wire TabManager::SetActiveTab() to call save/restore

### Phase 2: HIGH
- [ ] Test multi-tab switching with all 3 types
- [ ] Verify node positions preserved
- [ ] Verify pan/zoom preserved

### Phase 3: MEDIUM
- [ ] Add state persistence to JSON (save canvas offset in file)
- [ ] Load canvas offset on file load
- [ ] Test with loaded files

### Phase 4: LOW
- [ ] Performance optimization if needed
- [ ] Memory profiling for state storage

---

## Files to Modify

### CRITICAL
1. **IGraphRenderer.h** - Add state management interface
2. **TabManager.cpp** - Wire save/restore on tab switch
3. **VisualScriptRenderer.h/cpp** - Implement save/restore
4. **EntityPrefabRenderer.h/cpp** - Implement save/restore
5. **BehaviorTreeRenderer.h/cpp** - Implement save/restore

### IMPORTANT
6. **VisualScriptEditorPanel.h/cpp** - Isolate imnodes EditorContext
7. **CustomCanvasEditor.h/cpp** - Ensure pan/zoom preservation

---

## Testing Strategy

```
Test Case 1: VS → EP → VS
├─ Open VisualScript, arrange nodes, pan/zoom
├─ Switch to EntityPrefab
├─ Switch back to VisualScript
└─ ✅ Verify nodes at same positions, pan/zoom restored

Test Case 2: VS → BT → EP → VS
├─ Open all 3 types in sequence
├─ Modify each graph (move nodes, pan, zoom)
├─ Switch between them in random order
└─ ✅ Verify each state preserved independently

Test Case 3: Load File → Switch Tabs → Load Another
├─ Load VS file, pan/zoom
├─ Open EP file in new tab
├─ Switch back to VS
└─ ✅ Verify VS state still at pan/zoom from Step 1

Test Case 4: Close Tab → Reopen
├─ Open VS file, modify, close tab
├─ Reopen same VS file
└─ ✅ Verify positions NOT from previous session (file state, not tab memory)
```

---

## Estimated Impact

### Before Fix
- ❌ Multi-tab workflow broken for different graph types
- ❌ Users must re-pan/zoom when switching tabs
- ❌ Layout arrangement lost on every tab switch

### After Fix
- ✅ Canvas state fully preserved per tab
- ✅ Seamless tab switching experience
- ✅ Each tab maintains its viewport independently
- ✅ imnodes EditorContext isolated

---

## Next Steps

1. ✅ Present analysis to user
2. ⏳ Create IGraphRenderer interface additions
3. ⏳ Implement state save/restore in all renderers
4. ⏳ Wire TabManager lifecycle hooks
5. ⏳ Test all scenarios
6. ⏳ Deploy as Phase 35.1 enhancement

