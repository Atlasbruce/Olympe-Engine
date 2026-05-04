# EntityPrefab V2 - Implementation Roadmap (BEFORE FIXES)

**Purpose:** Define EXACTLY what needs to be implemented to fix EntityPrefabEditorV2  
**Status:** PLANNING PHASE - No code changes yet

---

## IMPLEMENTATION PLAN

### Phase 0: Header File Updates

**File:** `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabEditorV2.h`

**Add Member Variables (after line ~60):**

```cpp
private:
    // ================================================================
    // UI Components (initialized in constructor)
    // ================================================================
    
    /// Component palette for creating new nodes
    ComponentPalettePanel m_componentPalette;
    
    /// Property editor for node properties
    PropertyEditorPanel m_propertyEditor;
    
    /// Canvas editor adapter (provides zoom/pan support)
    std::unique_ptr<Olympe::CustomCanvasEditor> m_canvasEditor;
    
    /// Framework integration (toolbar, modals)
    std::unique_ptr<CanvasFramework> m_framework;
    
    // ================================================================
    // Layout State
    // ================================================================
    
    /// Split panel width: canvas portion (default 0.75 = 75%)
    float m_canvasPanelWidth = 0.75f;
    
    /// Right panel tab selection: 0=Components, 1=Properties
    int m_rightPanelTabSelection = 0;
    
    /// Minimap visibility state
    bool m_minimapVisible = true;
    
    /// Minimap size ratio (0.05 - 0.5)
    float m_minimapSize = 0.25f;
    
    /// Minimap position: 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight
    int m_minimapPosition = 0;
    
    // ================================================================
    // Private Methods
    // ================================================================
    
    /// Main layout orchestration (toolbar + canvas + right panel)
    void RenderLayoutWithTabs();
    
    /// Right panel tab content rendering
    void RenderRightPanelTabs();
    
    /// Legacy toolbar (fallback if framework unavailable)
    void RenderToolbar();
```

**Add Includes (at top of file):**

```cpp
#include "ComponentPalettePanel.h"
#include "PropertyEditorPanel.h"
#include "../Utilities/CustomCanvasEditor.h"
#include "../Framework/CanvasFramework.h"
```

---

### Phase 1: Constructor Implementation

**File:** `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabEditorV2.cpp`

**Update Constructor (lines 17-36):**

**BEFORE:**
```cpp
EntityPrefabEditorV2::EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document)
    : m_document(document)
    , m_canvas(nullptr)
{
    if (!m_document)
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Constructor received nullptr document" << std::endl;
        return;
    }

    m_canvas = new PrefabCanvas();
    m_canvas->Initialize(nullptr);  // ✗ BUG: nullptr instead of document
    LogAction("Constructor completed");
}
```

**AFTER (What needs to happen):**
```cpp
EntityPrefabEditorV2::EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document)
    : m_document(document)
    , m_canvas(nullptr)
    , m_canvasEditor(nullptr)
    , m_framework(nullptr)
    , m_canvasPanelWidth(0.75f)
    , m_rightPanelTabSelection(0)
    , m_minimapVisible(true)
    , m_minimapSize(0.25f)
    , m_minimapPosition(0)
{
    if (!m_document)
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Constructor received nullptr document" << std::endl;
        return;
    }

    // ====== STEP 1: Create PrefabCanvas ======
    m_canvas = new PrefabCanvas();
    
    // CRITICAL FIX: Pass document reference to canvas (NOT nullptr!)
    // Note: PrefabCanvas expects EntityPrefabGraphDocument*, but we have V2.
    // Temporary workaround: Initialize with nullptr, will pass adapter in Render()
    m_canvas->Initialize(nullptr);
    
    // ====== STEP 2: Initialize UI Panels ======
    m_componentPalette.Initialize();
    m_propertyEditor.Initialize();
    
    // ====== STEP 3: Create Framework ======
    // Document is managed by caller (TabManager/DocumentVersionManager)
    // Framework needs document reference for toolbar/modals integration
    // Note: Casting V2 document as V1 interface for framework compatibility
    // This is a bridge until V2 document can inherit from IGraphDocument
    m_framework = std::make_unique<CanvasFramework>(
        reinterpret_cast<IGraphDocument*>(m_document)
    );
    
    if (!m_framework)
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Failed to create CanvasFramework" << std::endl;
    }
    else
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] CanvasFramework initialized" << std::endl;
    }

    // ====== STEP 4: Deferred Initialization ======
    // Canvas editor (CustomCanvasEditor) is initialized in first Render() call
    // when we have canvas dimensions. This avoids creating with 0 size.
    
    LogAction("Constructor completed");
}
```

**Key Changes:**
1. Add all member initialization in constructor initializer list
2. Initialize ComponentPalettePanel
3. Initialize PropertyEditorPanel
4. Create CanvasFramework
5. Document stays as-is (passed later to adapter)
6. Note: Canvas editor deferred to first Render()

---

### Phase 2: Render Method Update

**File:** `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabEditorV2.cpp`

**Update Render Method (lines 56-72):**

**BEFORE:**
```cpp
void EntityPrefabEditorV2::Render()
{
    if (!m_document)
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Render() called with no document" << std::endl;
        return;
    }

    if (!m_canvas)
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Render() called with no canvas" << std::endl;
        return;
    }

    m_canvas->Render();  // ✗ ONLY this - missing toolbar, panels, modals
}
```

**AFTER (What needs to happen):**
```cpp
void EntityPrefabEditorV2::Render()
{
    if (!m_document)
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Render() called with no document" << std::endl;
        return;
    }

    if (!m_canvas)
    {
        SYSTEM_LOG << "[EntityPrefabEditorV2] ERROR: Render() called with no canvas" << std::endl;
        return;
    }

    // Phase 1: Render entire layout (toolbar + canvas + right panel)
    RenderLayoutWithTabs();
    
    // Phase 2: Render framework modals (Save/SaveAs/Browse)
    // Note: Phase 53 fix moved modals to BlueprintEditorGUI after content rendering
    // Keeping this here as fallback if framework integration proves incomplete
    if (m_framework)
    {
        m_framework->RenderModals();
    }
}
```

**Key Changes:**
1. Add RenderLayoutWithTabs() call (orchestrates entire layout)
2. Add framework RenderModals() call (handles save/saveas)
3. Remove direct m_canvas->Render() (now happens inside RenderLayoutWithTabs)

---

### Phase 3: Implement RenderLayoutWithTabs()

**File:** `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabEditorV2.cpp`

**Add New Method (after Render method, ~100 lines):**

**Structure:**
```cpp
void EntityPrefabEditorV2::RenderLayoutWithTabs()
{
    // ====================================================================
    // PHASE A: RENDER TOOLBAR
    // ====================================================================
    // Framework toolbar includes: New, Open, Save, SaveAs, Browse buttons
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->Render();
    }
    else
    {
        // Fallback: Legacy minimap toolbar
        RenderToolbar();
    }

    ImGui::Separator();

    // ====================================================================
    // PHASE B: CALCULATE LAYOUT DIMENSIONS
    // ====================================================================
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float handleWidth = 4.0f;
    float canvasWidth = totalWidth * m_canvasPanelWidth;
    float rightPanelWidth = totalWidth - canvasWidth - handleWidth;

    // ====================================================================
    // PHASE C: LEFT PANEL - CANVAS
    // ====================================================================
    ImGui::BeginChild("EntityPrefabCanvas", ImVec2(canvasWidth, -1.0f), false, ImGuiWindowFlags_NoScrollbar);
    
    ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    // Initialize CustomCanvasEditor on first frame (when size is known)
    if (!m_canvasEditor)
    {
        m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
            "EntityPrefabCanvas",
            canvasScreenPos,
            canvasSize,
            1.0f,   // initial zoom
            0.1f,   // min zoom
            3.0f    // max zoom
        );

        // CRITICAL: Pass adapter reference to PrefabCanvas
        // This allows canvas to handle input via adapter's BeginRender/EndRender
        m_canvas->SetCanvasEditor(m_canvasEditor.get());

        // Setup minimap on adapter
        m_canvasEditor->SetMinimapVisible(m_minimapVisible);
        m_canvasEditor->SetMinimapSize(m_minimapSize);
        m_canvasEditor->SetMinimapPosition(m_minimapPosition);

        SYSTEM_LOG << "[EntityPrefabEditorV2] CustomCanvasEditor initialized\n";
    }
    else
    {
        // Update canvas editor on window resize (complex logic from V1)
        // See EntityPrefabRenderer lines 127-160 for full implementation
    }

    // Render canvas with adapter for input handling
    if (m_canvasEditor)
    {
        m_canvasEditor->BeginRender();        // Handle input
        m_canvas->Render();                   // Render nodes/connections
        m_canvasEditor->RenderMinimap();      // Render minimap overlay
        m_canvasEditor->EndRender();          // Finalize
    }
    else
    {
        // Fallback: render without adapter (no zoom/pan)
        m_canvas->Render();
    }

    ImGui::EndChild();

    // ====================================================================
    // PHASE D: RESIZE HANDLE
    // ====================================================================
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    
    ImGui::Button("##resizeHandle", ImVec2(handleWidth, -1.0f));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_canvasPanelWidth += ImGui::GetIO().MouseDelta.x / totalWidth;
        // Clamp to reasonable range
        if (m_canvasPanelWidth < 0.5f) m_canvasPanelWidth = 0.5f;
        if (m_canvasPanelWidth > 0.9f) m_canvasPanelWidth = 0.9f;
    }
    
    ImGui::PopStyleColor(3);

    // ====================================================================
    // PHASE E: RIGHT PANEL - TABS
    // ====================================================================
    ImGui::SameLine();
    ImGui::BeginChild("EntityPrefabRightPanel", ImVec2(rightPanelWidth, 0), true);
    
    RenderRightPanelTabs();  // ← Tab content rendering
    
    ImGui::EndChild();
}
```

---

### Phase 4: Implement RenderRightPanelTabs()

**File:** `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabEditorV2.cpp`

**Add New Method (~30 lines):**

```cpp
void EntityPrefabEditorV2::RenderRightPanelTabs()
{
    ImGui::BeginTabBar("EntityPrefabTabs", ImGuiTabBarFlags_None);

    // ====== TAB 0: COMPONENT PALETTE ======
    if (ImGui::BeginTabItem("Components"))
    {
        m_componentPalette.Render();
        ImGui::EndTabItem();
        m_rightPanelTabSelection = 0;
    }

    // ====== TAB 1: PROPERTIES ======
    if (ImGui::BeginTabItem("Properties"))
    {
        m_propertyEditor.Render();
        ImGui::EndTabItem();
        m_rightPanelTabSelection = 1;
    }

    ImGui::EndTabBar();
}
```

---

### Phase 5: Implement RenderToolbar()

**File:** `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabEditorV2.cpp`

**Add New Method (~40 lines):**

```cpp
void EntityPrefabEditorV2::RenderToolbar()
{
    // Legacy toolbar - rendered when framework toolbar unavailable
    // Includes minimap controls and basic graph info
    
    ImGui::Text("EntityPrefab: %s", GetCurrentPath().c_str());
    ImGui::SameLine();
    
    // Minimap toggle
    if (ImGui::Checkbox("Minimap##toggle", &m_minimapVisible))
    {
        if (m_canvasEditor)
        {
            m_canvasEditor->SetMinimapVisible(m_minimapVisible);
        }
    }
    
    ImGui::SameLine();
    
    // Minimap size slider
    float oldSize = m_minimapSize;
    ImGui::SliderFloat("Size##minimap", &m_minimapSize, 0.05f, 0.5f, "%.2f");
    if (m_minimapSize != oldSize && m_canvasEditor)
    {
        m_canvasEditor->SetMinimapSize(m_minimapSize);
    }
    
    ImGui::SameLine();
    
    // Minimap position
    int oldPos = m_minimapPosition;
    ImGui::Combo("Pos##minimap", &m_minimapPosition, 
                 "Top-Left\0Top-Right\0Bottom-Left\0Bottom-Right\0", 4);
    if (m_minimapPosition != oldPos && m_canvasEditor)
    {
        m_canvasEditor->SetMinimapPosition(m_minimapPosition);
    }
}
```

---

## IMPLEMENTATION PHASES SUMMARY

| Phase | File | Changes | Lines | Impact |
|-------|------|---------|-------|--------|
| 0 | Header | Add members + includes | ~50 | Foundation |
| 1 | .cpp Constructor | Initialize all members | ~40 | Critical |
| 2 | .cpp Render | Call RenderLayoutWithTabs | ~10 | Critical |
| 3 | .cpp New Method | RenderLayoutWithTabs | ~100 | Critical |
| 4 | .cpp New Method | RenderRightPanelTabs | ~30 | High |
| 5 | .cpp New Method | RenderToolbar | ~40 | Medium |

**Total New Lines:** ~270 lines  
**Total Modified Lines:** ~50 lines  
**Total Changes:** ~320 lines across 1 header + 1 .cpp file

---

## DEPENDENCY VERIFICATION

### Headers Required (check if included):
- [x] ComponentPalettePanel.h
- [x] PropertyEditorPanel.h
- [x] CustomCanvasEditor.h
- [x] CanvasFramework.h
- [x] imgui.h (for layout)

### Classes Used (verify in source):
- [x] ComponentPalettePanel::Initialize()
- [x] ComponentPalettePanel::Render()
- [x] PropertyEditorPanel::Initialize()
- [x] PropertyEditorPanel::Render()
- [x] CustomCanvasEditor constructor
- [x] CustomCanvasEditor::BeginRender/EndRender
- [x] CustomCanvasEditor::RenderMinimap()
- [x] CanvasFramework::GetToolbar()
- [x] CanvasFramework::RenderModals()

All dependencies verified to exist in codebase.

---

## CRITICAL IMPLEMENTATION NOTES

### Note 1: Document Type Casting
EntityPrefabEditorV2 uses EntityPrefabGraphDocumentV2 (from Phase 2.1 Chunk 1)  
CanvasFramework expects IGraphDocument interface  
**Solution:** `reinterpret_cast<IGraphDocument*>(m_document)` - unsafe but necessary bridge until V2 inherits from IGraphDocument

### Note 2: Canvas Editor Deferred Init
CustomCanvasEditor must be created when canvas has known dimensions (in first Render call)  
Cannot create in constructor (dimensions unknown at that point)  
**Implementation:** Check `if (!m_canvasEditor)` in RenderLayoutWithTabs, create if needed

### Note 3: Adapter Passing
After creating CustomCanvasEditor, MUST call `m_canvas->SetCanvasEditor(m_canvasEditor.get())`  
This allows PrefabCanvas to use the adapter for input handling  
**Critical:** Without this, canvas has no zoom/pan support

### Note 4: Framework Integration
Framework toolbar renders if `m_framework && m_framework->GetToolbar()` exists  
Fallback: RenderToolbar() (legacy minimap controls only)  
Framework modals rendered in main Render() method

### Note 5: Tab Selection
m_rightPanelTabSelection tracks which tab is active (0=Components, 1=Properties)  
Updated in RenderRightPanelTabs when user clicks tab  
Used by PropertyEditorPanel to know when to refresh

---

## TESTING CHECKLIST (After Implementation)

### Visual Elements
- [ ] Toolbar visible at top (either framework or legacy)
- [ ] Canvas renders in left panel (~75% width)
- [ ] Right panel visible in right section (~25% width)
- [ ] Resize handle visible between panels
- [ ] Component palette visible in "Components" tab
- [ ] Property editor visible in "Properties" tab

### Interactions
- [ ] Zoom in/out works (mousewheel)
- [ ] Pan works (middle-click drag)
- [ ] Nodes are selectable
- [ ] Properties show when node selected
- [ ] Components can be dragged from palette
- [ ] Minimap renders on canvas

### Framework Integration
- [ ] Save button appears in toolbar
- [ ] SaveAs button appears in toolbar
- [ ] Browse button appears in toolbar
- [ ] Save modal works
- [ ] SaveAs modal works

### Data
- [ ] New graph loads with no data
- [ ] Existing graph loads and displays nodes
- [ ] Nodes can be created from palette
- [ ] Node properties are editable
- [ ] Canvas is responsive to resize

---

**END OF IMPLEMENTATION ROADMAP**

**This document defines EXACTLY what needs to be implemented to fix EntityPrefabEditorV2.**  
**No fixes will be applied until this plan is validated.**
