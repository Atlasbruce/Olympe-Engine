# EntityPrefab V2 Integration - Exhaustive Fit-Gap Analysis

**Date:** March 2026  
**Status:** DIAGNOSTIC PHASE - NO FIXES APPLIED YET  
**Objective:** Complete analysis of EntityPrefabEditorV2 vs Legacy (V1) to identify all missing functionality

---

## PROBLEM STATEMENT

When creating a new EntityPrefab graph in V2:
- **Canvas is EMPTY** (no nodes visible, even after load)
- **Toolbar is MISSING** (no Save/SaveAs/Browse buttons)
- **Right Panel is MISSING** (no component palette, no properties)
- **Layout is BROKEN** (no split panel, no tabs)

Legacy EntityPrefabRenderer (V1) shows the correct layout with all elements.

---

## SYSTEM OVERVIEW

### Three Graph Editor Systems Compared:

| Aspect | VisualScript (V1) | BehaviorTree (V1) | EntityPrefab V1 | EntityPrefab V2 |
|--------|------------------|-------------------|-----------------|-----------------|
| **Renderer Type** | Panel-based (VisualScriptEditorPanel) | Custom Renderer (BehaviorTreeRenderer) | Custom Renderer (EntityPrefabRenderer) | Custom Renderer (EntityPrefabEditorV2) |
| **Canvas System** | imnodes native | BehaviorTreeImNodesAdapter | CustomCanvasEditor (zoom support) | CustomCanvasEditor (should support zoom) |
| **Framework Integration** | Phase 41 ✓ | Phase 41 ✓ | Phase 41 ✓ | Phase 2.1 ✗ |
| **Toolbar Rendering** | CanvasToolbarRenderer ✓ | CanvasToolbarRenderer ✓ | CanvasToolbarRenderer ✓ | CanvasToolbarRenderer ✗ |
| **Right Panel (Tabs)** | Yes ✓ | Yes ✓ | Yes ✓ | No ✗ |
| **Component Palette** | N/A | BTNodePalette ✓ | ComponentPalettePanel ✓ | ComponentPalettePanel ✗ |
| **Property Editor** | Yes ✓ | Yes ✓ | PropertyEditorPanel ✓ | PropertyEditorPanel ✗ |

---

## LAYER-BY-LAYER FIT-GAP ANALYSIS

### LAYER 1: RENDERER INITIALIZATION (Constructor)

**Pattern in Legacy (BehaviorTreeRenderer):**
```cpp
BehaviorTreeRenderer::BehaviorTreeRenderer(NodeGraphPanel& panel)
{
    m_palette = std::make_unique<AI::BTNodePalette>();              // ✓ Node palette
    m_propertyPanel.Initialize();                                     // ✓ Property panel
    m_executionTestPanel = std::make_unique<ExecutionTestPanel>();  // ✓ Test panel
    m_lastTracer = std::make_unique<GraphExecutionTracer>();        // ✓ Tracer
    m_document = std::make_unique<BehaviorTreeGraphDocument>(this);  // ✓ Document
    m_framework = std::make_unique<CanvasFramework>(m_document.get());  // ✓ Framework
}
```

**Current Implementation (EntityPrefabEditorV2):**
```cpp
EntityPrefabEditorV2::EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document)
{
    m_canvas = new PrefabCanvas();
    m_canvas->Initialize(nullptr);  // ✗ CRITICAL: Passes nullptr instead of document!
    // ✗ NO palette initialization
    // ✗ NO property panel initialization  
    // ✗ NO framework initialization
}
```

**GAP #1: Incomplete Constructor Initialization**
- [ ] NO ComponentPalettePanel (for node creation)
- [ ] NO PropertyEditorPanel (for property editing)
- [ ] NO CanvasFramework (for toolbar/modals)
- [ ] PrefabCanvas initialized with nullptr document (can't access data)

---

### LAYER 2: RENDER METHOD (Main Loop)

**Pattern in Legacy (BehaviorTreeRenderer::Render):**
```cpp
void BehaviorTreeRenderer::Render()
{
    RenderLayoutWithTabs();           // ← Renders entire layout
    if (m_executionTestPanel)
        m_executionTestPanel->Render();
    if (m_framework)
        m_framework->RenderModals();  // ← Framework modals
}
```

**Current Implementation (EntityPrefabEditorV2::Render):**
```cpp
void EntityPrefabEditorV2::Render()
{
    if (!m_canvas)
        return;
    m_canvas->Render();  // ← ONLY renders canvas, nothing else!
    // ✗ NO layout rendering
    // ✗ NO toolbar rendering
    // ✗ NO right panel rendering
    // ✗ NO framework modal rendering
}
```

**GAP #2: Render Method Missing Entire Layout Pipeline**
- [ ] NO RenderLayoutWithTabs() method
- [ ] NO toolbar rendering (CanvasToolbarRenderer)
- [ ] NO right panel tabs rendering
- [ ] NO palette panel rendering
- [ ] NO property panel rendering
- [ ] NO modal framework rendering

---

### LAYER 3: CANVAS + ADAPTER INITIALIZATION

**Pattern in Legacy (EntityPrefabRenderer::RenderLayoutWithTabs):**
```cpp
// First Frame: Initialize CustomCanvasEditor adapter with zoom support
if (!m_canvasEditor)
{
    m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
        "PrefabCanvas",
        canvasScreenPos,
        canvasSize,
        1.0f,           // initial zoom
        0.1f,           // min zoom
        3.0f            // max zoom
    );
    m_canvas.SetCanvasEditor(m_canvasEditor.get());  // ← CRITICAL: Pass adapter to canvas
}
```

**Current Implementation (EntityPrefabEditorV2):**
```cpp
void EntityPrefabEditorV2::Render()
{
    // ✗ NO CustomCanvasEditor initialization
    // ✗ NO canvas editor adapter setup
    // ✗ NO zoom support implementation
    // ✗ NO input handling (pan/zoom from adapter)
    m_canvas->Render();  // Canvas renders without adapter!
}
```

**GAP #3: Canvas Adapter NOT Initialized**
- [ ] CustomCanvasEditor adapter NOT created
- [ ] Zoom support NOT implemented (fixed 1.0x zoom only)
- [ ] Canvas editor reference NOT passed to PrefabCanvas
- [ ] Pan/zoom input NOT handled
- [ ] Minimap support NOT connected

---

### LAYER 4: UI ELEMENTS (Toolbar, Palette, Properties)

**Pattern in Legacy (EntityPrefabRenderer::RenderLayoutWithTabs):**
```cpp
// 1. Toolbar
if (m_framework)
    m_framework->GetToolbar()->Render();
else
    RenderToolbar();

// 2. Canvas with resize handle

// 3. Right panel with tabs
ImGui::BeginChild("EntityPrefabRightPanel", ...);
    ImGui::BeginTabBar("EntityPrefabTabs");
    
    if (ImGui::BeginTabItem("Components"))
    {
        m_componentPalette.Render();  // ← Component palette
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Properties"))
    {
        m_propertyEditor.Render();    // ← Property editor
        ImGui::EndTabItem();
    }
    
    ImGui::EndTabBar();
ImGui::EndChild();
```

**Current Implementation (EntityPrefabEditorV2):**
```cpp
void EntityPrefabEditorV2::Render()
{
    m_canvas->Render();  // ✗ ONLY this - no tabs, no palette, no properties
}
```

**GAP #4: Layout, Toolbar, and Panels Missing**
- [ ] Framework toolbar NOT rendered
- [ ] Right panel tabs NOT created
- [ ] Component palette NOT rendered in tab
- [ ] Property editor NOT rendered in tab
- [ ] Resize handle between canvas/panels NOT implemented
- [ ] Tab switching logic NOT implemented

---

### LAYER 5: FRAMEWORK INTEGRATION

**Pattern in Legacy (BehaviorTreeRenderer):**
```cpp
BehaviorTreeRenderer::BehaviorTreeRenderer()
{
    m_document = std::make_unique<BehaviorTreeGraphDocument>(this);
    m_framework = std::make_unique<CanvasFramework>(m_document.get());  // ← Document passed!
}

void BehaviorTreeRenderer::Render()
{
    if (m_framework)
        m_framework->RenderModals();  // ← Framework handles all modals
}
```

**Current Implementation (EntityPrefabEditorV2):**
```cpp
// ✗ NO CanvasFramework created
// ✗ NO framework initialization in constructor
// ✗ NO modal rendering in Render()

void EntityPrefabEditorV2::RenderFrameworkModals()
{
    // Currently no framework modals - canvas handles all modals internally
    // This is a placeholder for potential future framework modal integration
}
```

**GAP #5: Framework NOT Integrated**
- [ ] CanvasFramework NOT created in constructor
- [ ] Framework toolbar NOT available
- [ ] Framework save/saveas modals NOT available
- [ ] File picker modals NOT available via framework

---

### LAYER 6: DOCUMENT-CANVAS LINKAGE

**Pattern in Legacy (EntityPrefabRenderer):**
```cpp
// Line 113: CRITICAL linkage
m_canvas.SetCanvasEditor(m_canvasEditor.get());  // ← Canvas receives adapter

// Line 27: Document reference passed to framework
EntityPrefabGraphDocument* document = m_canvas.GetDocument();
if (document)
    m_framework = std::make_unique<CanvasFramework>(document);
```

**Current Implementation (EntityPrefabEditorV2):**
```cpp
EntityPrefabEditorV2::EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document)
    : m_document(document), m_canvas(nullptr)
{
    m_canvas = new PrefabCanvas();
    m_canvas->Initialize(nullptr);  // ✗ Document NOT passed!
    // ✗ NO canvas editor passed to canvas
    // ✗ NO framework created with document
}
```

**GAP #6: Critical Linkage Missing**
- [ ] PrefabCanvas initialized with nullptr instead of document
- [ ] Canvas editor adapter NOT passed to canvas
- [ ] Document NOT available to canvas for rendering
- [ ] Framework NOT connected to document
- [ ] No way for canvas to access graph data

---

## STRUCTURAL COMPARISON: RENDER PIPELINE

### BehaviorTreeRenderer (Working) - Call Stack:

```
BehaviorTreeRenderer::Render() [Called once per frame]
  ├─ RenderLayoutWithTabs()
  │   ├─ m_framework->GetToolbar()->Render()  [Toolbar + Save buttons]
  │   │
  │   ├─ ImGui::BeginChild("BTNodeCanvas")    [Left panel]
  │   │   ├─ m_imNodesAdapter->Render()       [Actual graph rendering]
  │   │   └─ ImGui::EndChild()
  │   │
  │   ├─ Resize handle
  │   │
  │   └─ ImGui::BeginChild("BTRightPanel")    [Right panel]
  │       └─ RenderRightPanelTabs()
  │           ├─ Tab "Palette" → m_palette->Render()
  │           ├─ Tab "Properties" → m_propertyPanel->Render()
  │           └─ Tab "Execution" → m_executionTestPanel->Render()
  │       └─ ImGui::EndChild()
  │
  ├─ m_executionTestPanel->Render()           [Overlay execution test]
  │
  └─ m_framework->RenderModals()              [Save/SaveAs modals]
```

### EntityPrefabEditorV2 (Broken) - Call Stack:

```
EntityPrefabEditorV2::Render() [Called once per frame]
  └─ m_canvas->Render()  ← ONLY THIS! Everything else missing!
      ├─ Grid rendering
      ├─ Node rendering
      ├─ Connection rendering
      └─ Input handling
      
  [MISSING]
  ✗ Toolbar not rendered
  ✗ Right panel not created
  ✗ Palette not rendered
  ✗ Properties not rendered
  ✗ Modals not rendered
```

---

## MISSING METHODS IN EntityPrefabEditorV2

### Must Implement:

1. **RenderLayoutWithTabs()** - Main layout orchestration
   - Create/manage left canvas panel
   - Create/manage right panel with tabs
   - Render resize handle
   - Coordinate all sub-renderers

2. **RenderToolbar()** - Legacy toolbar (fallback)
   - Minimap visibility toggle
   - Zoom controls (if not using framework)
   - Graph info display

3. **RenderRightPanelTabs()** - Tab content rendering
   - Tab bar creation
   - Component palette rendering (Tab 0)
   - Property editor rendering (Tab 1)
   - Tab selection state management

4. **Initialize Canvas Adapter** (in Render or constructor)
   - Create CustomCanvasEditor with zoom support
   - Pass adapter reference to PrefabCanvas
   - Set up minimap on adapter

5. **Initialize Framework** (in constructor)
   - Create CanvasFramework with document
   - Toolbar will auto-render in RenderLayoutWithTabs

6. **Initialize UI Panels** (in constructor)
   - Create ComponentPalettePanel
   - Create PropertyEditorPanel
   - Initialize both

---

## MISSING MEMBERS IN EntityPrefabEditorV2

Currently has:
```cpp
EntityPrefabGraphDocumentV2* m_document;
PrefabCanvas* m_canvas;
```

Should have:
```cpp
// Framework integration
std::unique_ptr<CanvasFramework> m_framework;

// UI Panels
ComponentPalettePanel m_componentPalette;
PropertyEditorPanel m_propertyEditor;

// Canvas adapter
std::unique_ptr<CustomCanvasEditor> m_canvasEditor;

// Layout state
float m_canvasPanelWidth = 0.75f;
int m_rightPanelTabSelection = 0;
bool m_minimapVisible = true;
float m_minimapSize = 0.25f;
int m_minimapPosition = 0;  // TopLeft
```

---

## DATA FLOW: What SHOULD Happen

### Scenario: "Create New EntityPrefab Graph"

1. **TabManager calls DocumentVersionManager::CreateNewDocument("EntityPrefab")**
   - Creates EntityPrefabGraphDocumentV2
   - Creates EntityPrefabEditorV2(document)
   - ✗ Currently: Constructor incomplete, no panels initialized

2. **TabManager calls Render() each frame**
   - ✗ Currently: Only calls m_canvas->Render()
   - Should: Call RenderLayoutWithTabs() which orchestrates everything

3. **User interacts with toolbar**
   - ✓ Clicks "New" button
   - Should: Framework Save/SaveAs modals appear
   - ✗ Currently: No toolbar rendered, no modals possible

4. **User drags component from palette**
   - ✓ Should create new node
   - ✗ Currently: NO PALETTE VISIBLE, can't drag

5. **User selects node**
   - ✓ Should show properties in right panel
   - ✗ Currently: NO RIGHT PANEL, properties not visible

---

## INTEGRATION CHECKLIST

### Constructor Phase (EntityPrefabEditorV2::EntityPrefabEditorV2)

- [ ] Initialize ComponentPalettePanel: `m_componentPalette.Initialize()`
- [ ] Initialize PropertyEditorPanel: `m_propertyEditor.Initialize()`
- [ ] Create CanvasFramework: `m_framework = std::make_unique<CanvasFramework>(m_document)`
- [ ] Pass document to canvas: `m_canvas->Initialize(GetDocument())`
- [ ] Initialize canvas editor reference (deferred to first Render)

### Render Phase (EntityPrefabEditorV2::Render)

- [ ] Call RenderLayoutWithTabs() instead of just m_canvas->Render()
- [ ] Render framework modals if framework exists

### RenderLayoutWithTabs Implementation

- [ ] Render framework toolbar (via m_framework->GetToolbar())
- [ ] Create left canvas child with size control
- [ ] Initialize CustomCanvasEditor on first frame
- [ ] Render canvas using adapter
- [ ] Create resize handle between panels
- [ ] Create right panel with tab bar
- [ ] Implement tab content rendering (palette, properties)

### Frame-by-Frame Interaction

- [ ] Input handling via CustomCanvasEditor adapter
- [ ] Multi-select/selection state in PropertyEditorPanel
- [ ] Drag-drop component creation from palette to canvas
- [ ] Node property editing in PropertyEditorPanel

---

## CRITICAL DEPENDENCIES

Files that EntityPrefabEditorV2 needs to properly initialize:

| Component | Header File | Status |
|-----------|------------|--------|
| ComponentPalettePanel | ComponentPalettePanel.h | ✓ Exists |
| PropertyEditorPanel | PropertyEditorPanel.h | ✓ Exists |
| CustomCanvasEditor | CustomCanvasEditor.h | ✓ Exists |
| CanvasFramework | CanvasFramework.h | ✓ Exists |
| PrefabCanvas | PrefabCanvas.h | ✓ Exists |
| EntityPrefabGraphDocumentV2 | EntityPrefabGraphDocumentV2.h | ✓ Exists |

All dependencies are available - it's pure implementation work.

---

## SUMMARY OF GAPS

| Gap # | Component | Status | Impact | Severity |
|-------|-----------|--------|--------|----------|
| 1 | Constructor Initialization | Missing panels | UI elements not created | **CRITICAL** |
| 2 | RenderLayoutWithTabs() | Not implemented | No layout rendered | **CRITICAL** |
| 3 | Canvas Adapter Init | Not done | No zoom/pan support | **HIGH** |
| 4 | Toolbar + Right Panel | Not rendered | No UI elements visible | **CRITICAL** |
| 5 | Framework Integration | Not done | No modals/toolbar | **HIGH** |
| 6 | Document-Canvas Linkage | Broken (nullptr) | Canvas can't access data | **CRITICAL** |

---

## IMPLEMENTATION ROADMAP

**Phase A: Constructor Initialization**
- Add member variables for panels, framework, adapter state
- Initialize ComponentPalettePanel
- Initialize PropertyEditorPanel
- Create CanvasFramework
- Pass document to canvas

**Phase B: Render Pipeline**
- Implement RenderLayoutWithTabs()
- Create canvas + right panel layout
- Add resize handle
- Implement tab bar

**Phase C: Canvas Adapter**
- Initialize CustomCanvasEditor on first frame
- Pass adapter to PrefabCanvas
- Set up minimap

**Phase D: Tab Content**
- Implement component palette rendering in Tab 0
- Implement property panel rendering in Tab 1
- Implement tab switching

**Phase E: Framework Integration**
- Verify toolbar renders via framework
- Verify modals render via framework
- Verify save/load functionality

---

**END OF FIT-GAP ANALYSIS**

This analysis identifies EXACTLY what is missing between V1 (working) and V2 (broken).  
NO implementation changes until this analysis is validated.
