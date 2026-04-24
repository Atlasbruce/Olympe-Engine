# Blueprint Editor Framework: Best Practices & Pitfalls Guide

**Author:** Copilot  
**Date:** 2026-04-08  
**Status:** ✅ Phase 75 - Complete  
**Knowledge Base:** Phases 44-74 lessons learned

---

## Executive Summary

This guide captures **20+ critical lessons** from implementing the Blueprint Editor Framework (Phases 44-74). These are the mistakes that cost time and the patterns that saved time.

**Key Insight**: "Always search for existing working implementations BEFORE recreating features. Look in VisualScript, EntityPrefab, BehaviorTree. Pattern matching reveals the correct approach faster than trial-and-error."

---

## Part 1: Architecture Principles

### Principle 1: Simplicity Over Complexity (KISS)

**When**: Choosing between two design approaches

**Rule**: "Simpler implementations catch fewer bugs"

**Good Example** (Phase 44.4):
```cpp
// Simple: Execute existing backend method
void OnSaveClicked()
{
    if (!ValidateDocument()) return;
    std::string path = m_document->GetFilePath();
    if (path.empty()) { OnSaveAsClicked(); return; }
    ExecuteSave(path);  // 4 lines, proven to work
}
```

**Bad Example** (Phase 44.4 - what we initially did):
```cpp
// Complex: Duplicate all serialization logic
void OnSaveClicked()
{
    // ... 80 lines of serialization logic ...
    // Bugs: Misses edge cases, not tested, duplicates existing code
}
```

**Lesson**: Phase 44.4 Build went from 142 errors → 0 errors by recognizing overcomplexity and using ExecuteSave() that already works.

**Action**: When implementing features:
1. Search for existing implementation first
2. Use it via delegaton (like ExecuteSave pattern)
3. Only implement from scratch if truly new

---

### Principle 2: Proven > New

**When**: Faced with "legacy code" vs "new framework"

**Rule**: "Use working legacy patterns even if newer code exists. Incomplete new code is worse than proven old code."

**Example** (Phase 44.4):
```cpp
// ✗ NEW but INCOMPLETE:
m_canvas->SaveGraph();  // Method exists but never implemented

// ✓ OLD but PROVEN:
m_document->Save(path);  // Tested, works across all graph types
```

**Discovery**: The "incomplete new framework" and "proven legacy system" were false dichotomy. The CORRECT pattern was:
```cpp
// Use framework toolbar (NEW) to call backend save (PROVEN)
Framework UI → ExecuteSave() → m_document->Save()
         ↓           ↓              ↓
    (handles     (delegates)   (actually saves)
     buttons)
```

**Key Lesson**: If 10 years of code exists for a feature, it probably works. Use it.

---

### Principle 3: Two-Layer Architecture

**When**: Building UI that needs backend support

**Pattern**:
```
┌─────────────────────────┐
│  Framework UI Layer     │ (buttons, modals, toolbar)
│  (CanvasToolbarRenderer)│
└────────────┬────────────┘
             │ delegates to
             ▼
┌─────────────────────────┐
│  Backend Layer          │ (file I/O, serialization)
│  (ExecuteSave, modals)  │
└─────────────────────────┘
```

**Rule**: 
- Framework layer: Handles UI presentation only
- Backend layer: Does actual work (file I/O, validation, serialization)
- Framework NEVER reimplements backend logic

**Anti-Pattern** (Don't Do This):
```cpp
// WRONG: UI layer duplicates backend logic
void OnSaveClicked()
{
    // Serialization code duplicated from backend
    nlohmann::json j;
    for (auto& node : m_nodes) {  // ← DUPLICATE CODE!
        j["nodes"].push_back(...);
    }
    // Problem: Bugs fixed in backend won't apply here
}

// RIGHT: UI layer delegates to backend
void OnSaveClicked()
{
    ExecuteSave(path);  // Backend handles all serialization
}
```

**Benefit**: Single source of truth for save logic. Bug fixes apply everywhere.

---

## Part 2: Critical Pitfalls & Solutions

### Pitfall 1: Logging in Render Loops (SPAM)

**Problem**: Console flooded with 60 FPS logs, can't see actual errors

**Anti-Pattern**:
```cpp
void RenderButtons()
{
    SYSTEM_LOG << "[Render] Drawing buttons\n";  // ← 60x per second!
    ImGui::Button("Save");
}
```

**Solution** (Phase 46 Pattern):
```cpp
void OnSaveClicked()  // Event-driven (once per click)
{
    SYSTEM_LOG << "[Save] Button clicked\n";  // ← 1x when user acts
    ExecuteSave(path);
}

void RenderButtons()  // Render loop (called 60x/sec)
{
    // NO LOGS HERE
    ImGui::Button("Save");
}
```

**Rule**: ONLY log on:
- Button clicks (OnSaveClicked, OnDeleteClicked)
- State changes (OnDirectoryChanged)
- User actions (OnNodeSelected)
- Errors (catch blocks)

**Do NOT log**:
- RenderFrame / Render() methods
- OnMouseMove handlers
- Property getters
- Anything in 60+ FPS loops

**Benefit**: Console stays clean, bugs surface immediately.

---

### Pitfall 2: ImGui Frame Ordering (Modals Never Appear)

**Problem**: Right-click → context menu should appear but doesn't

**Root Cause** (Phase 45):
```cpp
// WRONG: Modal rendered at wrong frame point
void RenderTabBar()  // Called early in frame
{
    if (m_showSaveModal) ImGui::OpenPopup("SaveModal");  // ← TOO EARLY
    ImGui::BeginPopupModal("SaveModal");
    ImGui::EndPopupModal();
}

// Result: ImGui flag consumed before user sees anything
```

**Solution** (Phase 45 Pattern):
```cpp
// CORRECT: Content first, modals last
void BlueprintEditorGUI::RenderFrame()
{
    // 1. Content rendering phase
    RenderTabBar();       // Render tabs and buttons
    RenderActiveCanvas(); // Render graph

    // 2. Modal rendering phase (AFTER all content)
    EditorTab* tab = GetActiveTab();
    if (tab && tab->renderer)
        tab->renderer->RenderFrameworkModals();  // Modals render here
}
```

**Rule**: ImGui modal lifecycle requires:
1. Content rendered first (allows window focus detection)
2. OpenPopup() called during content rendering
3. BeginPopupModal() called AFTER all content
4. Order: NewFrame → Content → Modals → EndFrame → Present

**Code Pattern**:
```cpp
// Pseudo-code of correct ImGui flow
ImGui::NewFrame();

// Phase 1: Content
RenderButtons();      // May call OpenPopup("MyModal")
RenderCanvas();

// Phase 2: Modals (MUST be after content)
if (ImGui::BeginPopupModal("MyModal"))
{
    ImGui::Text("Modal content");
    ImGui::EndPopupModal();
}

ImGui::EndFrame();
```

**Benefit**: Modals appear reliably, keyboard input works.

---

### Pitfall 3: Uninitialized Pointers (Silent Failures)

**Problem**: Code compiles, runs, but nothing happens (no crash, no error)

**Anti-Pattern** (Phase 52):
```cpp
class PrefabCanvas
{
private:
    std::unique_ptr<ComponentNodeRenderer> m_renderer;  // Declared

    void Render()
    {
        if (m_renderer)  // ← nullptr! Guard doesn't fix initialization bug
        {
            m_renderer->RenderNodes();  // Never executes
        }
    }
};

// Result: Nodes don't render (silent failure, no crash)
```

**Solution** (Phase 52 Pattern):
```cpp
class PrefabCanvas
{
public:
    void Initialize(Document* doc)
    {
        m_document = doc;
        // CRITICAL: Create ALL objects needed in Render()
        if (!m_renderer)
        {
            m_renderer = std::make_unique<ComponentNodeRenderer>();  // ← FIX!
        }
    }

    void Render()
    {
        if (m_renderer)  // Now always true (guaranteed by Initialize)
        {
            m_renderer->RenderNodes();  // Executes ✓
        }
    }
};
```

**Rule**: "Initialization Completeness"
- Constructor: Initialize basic members
- Initialize(): Create ALL dependent objects
- Render(): Can assume all objects exist
- If Initialize() doesn't create it, Render() won't have it

**Check**: Before calling Render(), ask:
- Does this method use m_childRenderer? → Must initialize it
- Does this method use m_nodeDrawer? → Must initialize it
- Any member accessed in Render() MUST be created in Initialize()

**Benefit**: Code works reliably without defensive `if (ptr)` checks everywhere.

---

### Pitfall 4: Hover & Context Menu - Wrong ImGui Phase (Silent Failure)

**Problem**: Context menu appears on wrong click or doesn't appear at all. Hover visual feedback missing.

**Anti-Pattern** (Phase 76 Root Cause):

```cpp
// WRONG: Input detection in RENDER phase
void YourCanvas::RenderNodeBox(const YourNode& node)
{
    // Problem: RenderNodeBox called 60 times/sec
    // ImGui::IsMouseClicked() is unreliable here

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))  // ← WRONG PHASE!
    {
        if (IsPointInNode(node)) {
            m_contextNodeId = node.nodeId;
            ImGui::OpenPopup("##context");  // Flag may not process
        }
    }
}

// Result: Context menu never appears (ImGui flag consumed early or ignored)
```

**Solution** (Phase 76 Pattern - Async Input/Output):

```cpp
// CORRECT: Separate input phase from render phase

// Phase 1: INPUT DETECTION (called in HandleNodeInteraction, not render)
void YourCanvas::HandleMouseInput(ImVec2 mousePos)
{
    // Update hover state every frame
    m_hoveredNodeId = GetNodeAtScreenPos(mousePos);

    // Async dispatch: Input detection HERE (input phase)
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && m_hoveredNodeId >= 0)
    {
        m_contextNodeId = m_hoveredNodeId;
        ImGui::OpenPopup("##node_context");  // Flag set in input phase ✓
    }
}

// Phase 2: RENDER (called in RenderContextMenu, render phase)
void YourCanvas::RenderContextMenu()
{
    // Render only, NO input detection
    if (ImGui::BeginPopup("##node_context"))
    {
        if (ImGui::MenuItem("Delete Node"))
        {
            m_document->DeleteNode(m_contextNodeId);
            MarkDirty();
        }
        ImGui::EndPopup();
    }
}

// Visual feedback (render phase - uses state from input phase)
void YourCanvas::RenderNodeBox(const YourNode& node)
{
    bool isHovered = (m_hoveredNodeId == node.nodeId);  // Read state, don't detect
    ImU32 borderColor = isHovered ? IM_COL32(255, 200, 0, 255) : IM_COL32(100, 100, 100, 255);
    drawList->AddRect(..., borderColor, 2.5f);  // Glow effect
}
```

**Rule**: "Async Input/Output Pattern"
1. **Input Phase** (HandleNodeInteraction):
   - Detect mouse clicks/hover
   - Update state members (m_hoveredNodeId, m_contextNodeId)
   - Call ImGui::OpenPopup()
2. **Render Phase** (RenderNodeBox, RenderContextMenu):
   - Read state members (don't detect input)
   - Render visual feedback (glow, thickness)
   - Render popups (BeginPopup, MenuItem)
3. **Never Mix**: Input detection and rendering MUST be in different methods called at different frame times

**ImGui Frame Cycle**:
```
ImGui::NewFrame()
  ├─ Input processing (mouse clicks recorded)
  ├─ UpdateInput() called (your HandleNodeInteraction)
  │  └─ You detect clicks here, call OpenPopup()
  ├─ Content rendering (RenderNodeBox called)
  │  └─ Render visual feedback using state
  ├─ Modal rendering (RenderContextMenu called)
  │  └─ ImGui processes popup flag from earlier, renders modal
  └─ EndFrame()
```

**Connection Hit Detection** (Bonus Pattern):
```cpp
// Bezier curve hit detection - 32-point sampling with 10px tolerance
float GetDistanceToConnection(const YourConnection& conn, ImVec2 screenPos)
{
    const float HIT_TOLERANCE = 10.0f;
    const int BEZIER_SAMPLES = 32;
    float minDistance = std::numeric_limits<float>::max();

    for (int i = 0; i <= BEZIER_SAMPLES; ++i)
    {
        float t = static_cast<float>(i) / BEZIER_SAMPLES;
        // Cubic Bezier: B(t) = (1-t)³P0 + 3(1-t)²tCP1 + 3(1-t)t²CP2 + t³P3
        ImVec2 point = EvaluateBezier(t, p1, cp1, cp2, p2);
        minDistance = std::min(minDistance, Distance(point, screenPos));
    }

    return minDistance;
}
```

**Benefit**: Reliable hover detection, working context menus, visual feedback at all zoom levels.

---

### Pitfall 5: Accessing Moved Objects (Undefined Behavior)

**Problem**: Code compiles but returns garbage (empty string, undefined value)

**Anti-Pattern** (Phase 51):
```cpp
// BROKEN: Undefined behavior
std::string tabIDToReturn;
{
    EditorTab tab = CreateTab(...);
    m_tabs.emplace_back(std::move(tab));  // tab is moved here
    tabIDToReturn = tab.tabID;  // ← UNDEFINED! tab is now empty
}
return tabIDToReturn;  // Returns garbage
```

**Solution** (Phase 51 Fix):
```cpp
// CORRECT: Save before move
std::string tabIDToReturn;
{
    EditorTab tab = CreateTab(...);
    tabIDToReturn = tab.tabID;  // Save FIRST
    m_tabs.emplace_back(std::move(tab));  // Move is safe
}
return tabIDToReturn;  // Returns actual value ✓
```

**Rule**: "Save Before Move"
1. Extract all values needed from object
2. Then call std::move()
3. Return/use extracted values

**Pattern**:
```cpp
// Wrong:
std::unique_ptr<Object> obj = CreateObject();
m_container.push_back(std::move(obj));
return obj->GetData();  // ← Undefined

// Right:
std::unique_ptr<Object> obj = CreateObject();
std::string data = obj->GetData();  // Save first
m_container.push_back(std::move(obj));
return data;  // ← Defined
```

**Note**: Compiler allows accessing moved objects (no error), but result is unpredictable. Not a compile error, but a logic bug.

**Benefit**: Predictable behavior, no mysterious empty strings or garbage values.

---

### Pitfall 5: Duplicate Function Definitions (LNK2005)

**Problem**: Build error - "Symbol already defined"

**Root Cause** (Common Pattern):
```cpp
// WRONG: Same method in two .cpp files
// File 1: VisualScriptEditorPanel_Canvas.cpp
void VisualScriptEditorPanel::RenderContextMenus() { ... }

// File 2: VisualScriptEditorPanel_RenderingCore.cpp
void VisualScriptEditorPanel::RenderContextMenus() { ... }

// Result: LNK2005 error at link time
```

**Solution**:
```cpp
// Step 1: Find ALL implementations
// grep for: "void ClassName::MethodName"

// Step 2: Keep only ONE
// Delete from all but one file

// Step 3: If both needed (unlikely), move to header as inline
// header:
// inline void ClassName::Method() { ... }
```

**Prevention**: When adding method implementation:
1. Search for method name in ALL .cpp files
2. Verify it's not already implemented
3. If declared in header + implemented in multiple .cpp, that's the bug

**Benefit**: Clean build, no linker confusion.

---

## Part 3: Framework-Specific Patterns

### Pattern 1: The IGraphRenderer Contract

**Interface** (What every graph type must implement):
```cpp
class IGraphRenderer
{
public:
    virtual std::string GetGraphType() const = 0;
    virtual bool Load(const std::string& filePath) = 0;
    virtual bool Save(const std::string& filePath) = 0;
    virtual void Render() = 0;  // Main entry point
};
```

**Why Matters**: TabManager calls these methods to:
1. Load files (passes file path)
2. Identify type (calls GetGraphType)
3. Render in tab (calls Render every frame)
4. Save on command (calls Save)

**Implementation Pattern**:
```cpp
class YourGraphRenderer : public GraphEditorBase  // Inherits IGraphRenderer
{
    std::string GetGraphType() const override
    {
        return "YourGraphType";  // Unique identifier
    }
    
    bool Load(const std::string& filePath) override
    {
        // 1. Create document
        // 2. Load from file
        // 3. Initialize canvas
        // 4. Create framework
        // Return success
    }
    
    bool Save(const std::string& filePath) override
    {
        // Delegate to document->Save()
    }
    
    void Render() override
    {
        // RenderGraphContent() + RenderFrameworkModals()
    }
};
```

**Critical**: ALL THREE methods must be implemented, or TabManager can't handle your type.

---

### Pattern 2: The CanvasFramework Delegation

**What It Provides**:
```cpp
// CanvasFramework gives you for FREE:
- [Save] button (calls ExecuteSave)
- [SaveAs] button (calls ExecuteSaveAs + shows modal)
- [Browse] button (opens file browser)
- Toolbar rendering (RenderToolbar)
- Modal rendering (RenderModals)
- Keyboard shortcuts (Ctrl+S for save, etc.)
```

**How to Use**:

```cpp
// In your renderer:
class YourGraphRenderer : public GraphEditorBase
{
private:
    std::unique_ptr<CanvasFramework> m_framework;
};

// In Load():
if (!m_framework)
    m_framework = std::make_unique<CanvasFramework>(m_document.get());

// In RenderCommonToolbar():
if (m_framework && m_framework->GetToolbar())
{
    m_framework->GetToolbar()->Render();  // Renders [Save] [SaveAs] [Browse]
}

// In RenderFrameworkModals() (called after content):
if (m_framework && m_framework->GetToolbar())
{
    m_framework->GetToolbar()->RenderModals();  // Renders SaveAs modal
}
```

**Benefit**: Don't code buttons yourself. Framework handles it.

**Common Mistake** (Don't Do):
```cpp
// WRONG: Reimplementing toolbar
ImGui::Button("Save##custom", ...);
if (ImGui::IsItemClicked()) {
    // Your save logic here
}

// RIGHT: Use framework
m_framework->GetToolbar()->Render();  // Already has Save button
```

---

### Pattern 3: Async Input/Output (Critical for ImGui)

**When**: Building any ImGui-based interaction (context menus, hover, selection)

**What It Means**:
- Input detection happens in one method/phase
- Output rendering happens in different method/phase
- ImGui expects this for reliable flag processing

**Why It Matters**:
ImGui maintains internal state (popup flags, focus, etc.) that MUST be managed in the correct frame cycle order. Mixing input detection and rendering breaks ImGui's assumptions.

**Pattern**:

```cpp
// ❌ WRONG (mixing phases):
void RenderNode()
{
    if (ImGui::IsMouseClicked(1))  // Input in render ❌
        ImGui::OpenPopup("menu");
    if (ImGui::BeginPopup("menu"))  // Output in render ❌
    {
        // Result: Popup flag consumed in wrong frame, doesn't appear
    }
}

// ✅ CORRECT (separate phases):
void HandleInput()  // Called before rendering
{
    if (ImGui::IsMouseClicked(1))  // Input detection ✓
        m_showContextMenu = true;
        ImGui::OpenPopup("menu");   // Flag set at correct time
}

void Render()  // Called during render phase
{
    RenderNodes();  // Visual feedback from m_hoveredNodeId

    // Render comes AFTER input processing
    if (ImGui::BeginPopup("menu"))  // ImGui processes flag ✓
    {
        ImGui::MenuItem("Delete");
    }
}
```

**Frame Cycle Details**:
```
ImGui::NewFrame()
  ├─ Internal state reset
  └─ Input listeners armed

Your Code:
  ├─ HandleInput()  ← Input phase (detect clicks, set flags)
  └─ Render()       ← Render phase (use flags, draw UI)
     ├─ RenderNodes()       ← Draw from state
     ├─ RenderContextMenu() ← Draw modals AFTER content
     └─ ImGui processes deferred flags here

ImGui::EndFrame()
  └─ Finalize rendering
```

**Key Application: Hover + Context Menu** (Phase 76):
```cpp
// Input: Detect hover and dispatch
void HandleMouseInput(ImVec2 pos)
{
    m_hoveredNodeId = GetNodeAtScreenPos(pos);      // Update state
    if (ImGui::IsMouseClicked(1) && m_hoveredNodeId >= 0)
    {
        ImGui::OpenPopup("node_context");            // Dispatch
    }
}

// Render: Draw visual feedback and popup
void Render()
{
    // Visual feedback uses state from input phase
    for (const auto& node : m_nodes)
    {
        bool isHovered = (m_hoveredNodeId == node.id);
        RenderNode(node, isHovered);                 // Uses state
    }

    // Modal rendered AFTER content (ImGui requirement)
    if (ImGui::BeginPopup("node_context"))
    {
        if (ImGui::MenuItem("Delete"))
            DeleteNode(m_contextNodeId);
        ImGui::EndPopup();
    }
}
```

**Benefit**: ImGui state management works correctly, modals appear reliably, hover feedback always synced.

---

### Pattern 4: Coordinate Transformation

**Problem**: Mouse at (100, 50) in screen space, but where is it in canvas?

**Formula** (Correct - Phase 29):
```cpp
ImVec2 ScreenToCanvas(ImVec2 screenPos)
{
    ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();
    
    // Canvas space = (screen - origin - pan offset) / zoom
    // IMPORTANT: Don't multiply offset by zoom!
    return (screenPos - canvasScreenPos - m_canvasOffset) / m_canvasZoom;
}
```

**Common Mistake** (Phase 29 Root Cause):
```cpp
// WRONG: Multiplying offset by zoom
return (screenPos - canvasScreenPos - m_canvasOffset * m_canvasZoom) / m_canvasZoom;
       // ← This double-applies zoom to offset!

// Result: Zoom level 2x → offset wrong by 2x → selection misaligned
```

**Why This Works** (The math):
```
Screen:  0           100           200
         ├─ origin ──┤
         
Canvas:  -50          0            50   (if offset=50, zoom=1)
         
Formula: canvas = (screen - origin - offset) / zoom
         canvas = (100 - 0 - 50) / 1 = 50 ✓
         
At zoom 2x (canvas zoomed in):
Formula: canvas = (100 - 0 - 50) / 2 = 25 ✓
         (canvas coordinate scaled, offset NOT scaled)
```

**Benefit**: Drag-drop, selection, all coordinate-dependent features work correctly at any zoom level.

---

### Pattern 4: The Dirty Flag System

**What It Does**:
```
User modifies graph
  ↓
Call MarkDirty() or m_document->SetDirty(true)
  ↓
GraphEditorBase::IsDirty() returns true
  ↓
TabManager sees dirty state:
  - Tab title shows asterisk: "MyGraph*"
  - Close button shows save prompt
  - Framework toolbar knows to enable Save button
```

**Implementation**:
```cpp
// In your renderer (inherited from GraphEditorBase):
void OnNodeDeleted(int nodeId)
{
    m_document->DeleteNode(nodeId);
    MarkDirty();  // Marks both document AND renderer as dirty
}

// MarkDirty() is inherited, does:
// m_document->SetDirty(true);
// (Framework checks IsDirty() for UI feedback)

// On Save, dirty flag cleared:
bool ExecuteSave(const std::string& path)
{
    m_document->Save(path);
    m_document->SetDirty(false);  // Clears "modified" indicator
}
```

**Benefit**: Save prompt appears when needed, clears when saved.

---

## Part 4: Common Integration Mistakes

### Mistake 1: Forgetting to Create Framework

**Symptom**: Save/SaveAs buttons missing or non-functional

**Root Cause**:
```cpp
// Missing in Load() or InitializeCanvasEditor():
if (!m_framework)
    m_framework = std::make_unique<CanvasFramework>(m_document.get());
// ← If this isn't called, toolbar buttons won't exist
```

**Fix**:
```cpp
bool YourGraphRenderer::Load(const std::string& filePath)
{
    // ... load document ...
    
    // CRITICAL: Must create framework
    if (!m_framework)
    {
        m_framework = std::make_unique<CanvasFramework>(m_document.get());
    }
    
    // Then toolbar will render with buttons
}
```

---

### Mistake 2: Not Implementing RenderFrameworkModals()

**Symptom**: SaveAs button clicked but modal never appears

**Root Cause**:
```cpp
// Missing in your renderer:
void YourGraphRenderer::RenderFrameworkModals()
{
    // Empty! Should delegate to framework
}

// Result: Framework has modal but nobody renders it
```

**Fix**:
```cpp
void YourGraphRenderer::RenderFrameworkModals()
{
    // Phase 45 Pattern: Render framework modals
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->RenderModals();
    }
}
```

---

### Mistake 3: Rendering Modals at Wrong Time

**Symptom**: Multiple modals on screen, weird ImGui warnings

**Root Cause**:
```cpp
// WRONG: Rendering modals too early in frame
void RenderTabBar()  // Called early
{
    m_framework->GetToolbar()->RenderModals();  // ← TOO EARLY!
}

// Result: ImGui gets confused about window stacking
```

**Fix**:
```cpp
// CORRECT: Render modals after content
void BlueprintEditorGUI::RenderFrame()
{
    RenderTabBar();        // Content first
    RenderActiveCanvas();  // More content
    
    // THEN modals
    EditorTab* tab = GetActiveTab();
    if (tab && tab->renderer)
        tab->renderer->RenderFrameworkModals();
}
```

---

### Mistake 4: Calling SetDocument() After GetToolbar()

**Symptom**: Framework toolbar buttons crash or behave strangely

**Root Cause**:
```cpp
// WRONG order:
m_framework->GetToolbar()->Render();  // Toolbar tries to access document
SetDocument((void*)m_document.get()); // But document was nullptr!
```

**Fix**:
```cpp
// CORRECT order:
SetDocument((void*)m_document.get());  // Set document FIRST
if (!m_framework)
    m_framework = std::make_unique<CanvasFramework>(m_document.get());
// Now framework has valid document reference
```

---

## Part 5: Testing Checklist

### Pre-Release Verification

```
ARCHITECTURE
  □ Inherits GraphEditorBase (not reimplementing base functionality)
  □ IGraphRenderer contract implemented (GetGraphType, Load, Save, Render)
  □ CanvasFramework created in Load()
  □ SetDocument() called after creating framework

DATA LAYER
  □ Load() reads JSON and creates nodes/connections
  □ Save() writes complete graph state to JSON
  □ Dirty flag set on modifications
  □ Load/Save roundtrip preserves all data

UI LAYER
  □ Canvas renders nodes and connections
  □ Selection works (click node → highlight)
  □ Dragging works (drag node → moves)
  □ Context menu appears (right-click → menu)
  □ Tab name shows asterisk when modified ("Name*")

TOOLBAR
  □ [Save] button saves current file
  □ [SaveAs] button shows modal + saves new file
  □ [Browse] button opens file browser
  □ [Grid] checkbox toggles grid
  □ [Reset View] button resets pan/zoom
  □ [Minimap] checkbox toggles minimap

MODALS & EVENTS
  □ Delete key removes selected nodes
  □ Escape key closes popups
  □ Ctrl+S saves file (if keyboard shortcut implemented)
  □ Right-click context menu works on nodes
  □ Right-click context menu works on connections (if applicable)

LOGGING
  □ NO console spam (no render loop logs)
  □ Logs appear on button clicks (OnSaveClicked, etc.)
  □ Logs appear on file operations (Load, Save)
  □ Logs appear on errors

BUILD
  □ 0 errors (including LNK2005 duplicate symbol)
  □ 0 warnings
  □ Clean rebuild succeeds
```

---

## Part 6: Performance Considerations

### Don't Create Objects in Render Loop

**Problem**: Memory leak + frame rate drop

**Anti-Pattern**:
```cpp
void RenderNodes()
{
    for (const auto& node : m_nodes)
    {
        auto renderer = std::make_unique<NodeRenderer>();  // ← 60 FPS!
        renderer->Render(node);
        // renderer destroyed at end of loop
    }
}
// Result: 60 * nodeCount allocations per second
```

**Solution**:
```cpp
void Initialize()
{
    m_nodeRenderer = std::make_unique<NodeRenderer>();
}

void RenderNodes()
{
    for (const auto& node : m_nodes)
    {
        m_nodeRenderer->Render(node);  // Reuse same renderer
    }
}
```

### Lazy Initialization

**Pattern**: Create expensive objects only when needed

```cpp
void RenderPropertyPanel()
{
    if (!m_propertyEditor)  // First time?
    {
        m_propertyEditor = std::make_unique<PropertyEditor>();  // Create once
    }
    m_propertyEditor->Render();  // Reuse
}
```

---

## Quick Reference

| Pitfall | Symptom | Solution |
|---------|---------|----------|
| Logging in render | Console spam | Log only on user actions |
| Frame ordering | Modal doesn't appear | Modals after content in frame |
| Uninitialized pointers | Silent failure | Initialize() creates all deps |
| Hover context menu | No menu, no visual feedback | Async pattern: input phase dispatch, render phase popup |
| Accessing moved objects | Undefined behavior | Save before move |
| Duplicate definitions | LNK2005 error | One impl per method |
| Missing framework | No toolbar | Create in Load() |
| Missing RenderFrameworkModals | Modal won't render | Implement delegation |
| Wrong modal timing | ImGui warnings | After content phase |
| Coordinate transform | Misaligned selection | Don't multiply offset by zoom |
| Dirty flag not set | Save prompt missing | Call MarkDirty() |
| Hover not detected | Silent failure - can't click connections | Bezier curve sampling (32 points, 10px tolerance) |
| Connection hover | Can't interact with lines | GetDistanceToConnection implementation |

---

## Conclusion

The Blueprint Editor Framework has evolved through 74 phases of implementation. These best practices and pitfalls represent the distilled wisdom of that evolution.

**Key Takeaway**: "The simplest solution that uses existing code correctly will work better than a complex solution that tries to improve things."

Follow these patterns, study existing implementations, and your next graph type will be production-ready in days, not weeks.

