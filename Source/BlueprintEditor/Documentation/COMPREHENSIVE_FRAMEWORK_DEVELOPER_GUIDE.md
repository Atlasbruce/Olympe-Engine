# 🏗️ Blueprint Editor Framework - Comprehensive Developer Guide

**Status**: ✅ Phase 76+ Complete  
**Version**: 2.0 (Framework Synthesis)  
**Audience**: Developers implementing new graph types  
**Time to Implementation**: 6-8 hours for experienced developers  
**Last Updated**: 2026-04-08

---

## 📋 Quick Navigation

- [1. Executive Summary](#1-executive-summary) - What the framework does in 60 seconds
- [2. Feature Inventory](#2-feature-inventory) - What you get for free
- [3. Architecture Overview](#3-architecture-overview) - Why this design
- [4. Complete API Reference](#4-complete-api-reference) - Every method & signature
- [5. Data Flow Models](#5-data-flow-models) - How requests flow through system
- [6. Step-by-Step Implementation Guide](#6-step-by-step-implementation-guide) - Build a new graph type
- [7. Real Example Walkthrough](#7-real-example-walkthrough) - PlaceholderCanvas deep dive
- [8. Code Templates](#8-code-templates) - Copy-paste ready implementations
- [9. Testing & Verification](#9-testing--verification) - Ensure it works
- [10. Troubleshooting Guide](#10-troubleshooting-guide) - When things break
- [Appendix A: Common Mistakes](#appendix-a-common-mistakes) - What not to do
- [Appendix B: Performance Optimization](#appendix-b-performance-optimization) - Speed up your implementation

---

## 1. Executive Summary

### What The Framework Does

The Blueprint Editor Framework is a **reusable, multi-graph-type rendering system** that provides:

**Problem Solved**:
- Before: Each graph type needed separate save, load, toolbar, modal, minimap implementation
- After: New graph type gets all of this automatically

**Key Insight**:
> "Don't reimplement features. Inherit from framework → implement Load/Save/Render → get toolbar, modals, minimap, keyboard shortcuts, file browser for free"

### Framework Handles (You Don't Implement)

```
✅ Automatically Provided:
  └─ [Save] button + keyboard shortcut (Ctrl+S)
  └─ [SaveAs] button + file browser modal
  └─ [Browse] button + file selection dialog
  └─ [Grid] toggle + grid rendering
  └─ [Reset View] button + zoom/pan reset
  └─ [Minimap] toggle + viewport visualization
  └─ Tab management (dirty flag, asterisk indicator)
  └─ Keyboard shortcuts (Escape to close popups)
  └─ File routing + type detection
  └─ Dirty flag tracking
  └─ File caching (prevent duplicate loads)
  └─ Context menu support (right-click infrastructure)
  └─ Hover + selection state management
```

### You Implement (Framework Doesn't Know)

```
❌ You Must Implement:
  └─ Load graph from file (parse your JSON format)
  └─ Save graph to file (serialize your data structure)
  └─ Render nodes visually (ImGui/ImNodes drawing)
  └─ Handle interactions (click, drag, delete)
  └─ Define connections between nodes
  └─ Implement domain-specific logic
```

### The Trade-Off Pattern

```
┌─────────────────────────────────────────┐
│  Framework                              │
│  ├─ [Save] [SaveAs] [Browse] buttons   │ <- You get these
│  ├─ Grid, Minimap, Reset View          │
│  ├─ Tab management & dirty flags        │
│  └─ File I/O infrastructure             │
│                                          │
│  Your Implementation                    │
│  ├─ Render your specific nodes          │ <- You implement
│  ├─ Handle your interactions            │
│  ├─ Parse your JSON format              │
│  └─ Serialize your data                 │
└─────────────────────────────────────────┘

Result: 80% infrastructure provided + 20% domain-specific code
       = Production-ready graph type in < 1 day
```

---

## 2. Feature Inventory

### 2.1 Core Framework Features

#### Feature: Unified Toolbar
**What it does**: Renders [Save] [SaveAs] [Browse] [Grid] [Reset View] [Minimap] buttons

```
┌─────────────────────────────────────────────────────────────┐
│ [Save] [SaveAs] [Browse] | [Grid ☑] | [Reset View] [Mini☑] │
└─────────────────────────────────────────────────────────────┘
```

**Automatic handling**:
- [Save] → Calls your Load/Save implementation
- [SaveAs] → Opens file browser modal → Calls Save with new path
- [Browse] → Opens file browser (no action, just selection)
- [Grid] → Toggles grid rendering (your canvas uses CanvasGridRenderer)
- [Reset View] → Resets pan/zoom to default (your canvas handles via ICanvasEditor)
- [Minimap] → Toggles minimap visibility (your canvas uses CanvasMinimapRenderer)

**You don't implement this** - it's CanvasFramework doing all the work.

---

#### Feature: Save/SaveAs Modals
**What it does**: Dialog boxes for Save confirmation and SaveAs file selection

```
Save Flow:
1. User clicks [Save] button
2. Framework calls your ExecuteSave()
3. Your ExecuteSave() calls m_document->Save(filepath)
4. Result: File saved, dirty flag cleared
5. Tab title updates (asterisk disappears)

SaveAs Flow:
1. User clicks [SaveAs] button
2. Framework opens file browser modal
3. User selects destination
4. Framework calls your ExecuteSaveAs()
5. Your ExecuteSaveAs() calls m_document->Save(newFilepath)
6. Result: New file saved, tab renamed, dirty flag cleared
```

**File Browser Implementation**:
- Uses platform-native file dialogs (Windows OpenFileDialog)
- Returns full filepath selected by user
- Your code: Just call the filepath provided

---

#### Feature: Tab Management System
**What it does**: Manages multiple open files in tabs

```
┌──────────────────────────────────────────────┐
│ [File1.bt] [File2*ep] [MyGraph.vs]  [File3] │
│    ▲          ▲                       ▲      │
│   Active    Dirty                  Others    │
└──────────────────────────────────────────────┘

Tab States:
- Active: Currently displayed
- Dirty: Modified but not saved (shows *)
- Inactive: In background
- Cached: Loaded in memory for fast switching
```

**What framework provides**:
- Tab rendering with names
- Dirty flag indicator (asterisk)
- Tab switching
- Close with save prompt
- Caching to prevent reloading

**What you provide**:
- GetGraphType() - returns "YourGraphType"
- Load(filepath) - loads and populates your renderer
- Save(filepath) - saves current state to file
- Render() - draws on screen

---

#### Feature: Dirty Flag Tracking
**What it does**: Tracks if graph has unsaved changes

```cpp
// Automatic tracking via framework
User modifies graph
  ↓ (any node added/deleted/modified)
  ↓
Framework detects change
  ↓
MarkDirty() called
  ↓
Tab shows asterisk: "MyGraph*"
  ↓
Close button prompts: "Save changes?"
  ↓
[Save] button highlights as available
```

**Implementation**:
```cpp
// In your node deletion handler:
void DeleteNode(int nodeId)
{
    m_document->DeleteNode(nodeId);
    MarkDirty();  // Framework sees this, updates UI
}

// On save:
bool ExecuteSave(const std::string& filepath)
{
    m_document->Save(filepath);
    m_document->SetDirty(false);  // Clears asterisk
    return true;
}
```

---

#### Feature: Keyboard Shortcuts
**What it does**: Global keyboard bindings handled automatically

```
Ctrl+S         → Save current file
Escape         → Close open popups
Ctrl+A         → Select all nodes (optional, your implementation)
Ctrl+D         → Delete selected (optional, your implementation)
Delete         → Delete selected (standard, you handle this)
```

**Framework handles**: Ctrl+S routes to ExecuteSave()  
**You handle**: Everything else (implement in your renderer)

---

#### Feature: File Type Detection & Routing
**What it does**: Routes files to correct renderer based on type

```cpp
User double-clicks "myfile.bp.json"
  ↓
TabManager::OpenFileInTab(filepath)
  ├─ Detects type: ".bt" → BehaviorTree
  ├─ Creates BehaviorTreeRenderer
  ├─ Calls Load(filepath)
  ├─ Adds to tabs
  └─ Returns tab ID

// Your code just implements Load():
bool BehaviorTreeRenderer::Load(const std::string& filepath)
{
    // Parse JSON, create nodes, populate m_document
    // Return success
}
```

**Supported Types** (extensible):
- `.vs` → VisualScript
- `.bt` → BehaviorTree
- `.ep` → EntityPrefab
- Custom: Implement GetGraphType() returning your type

---

#### Feature: Grid Rendering
**What it does**: Displays background grid with consistent appearance

```
Grid Customization:
  - Spacing: 24 pixels (standardized)
  - Color: #3F3F47FF (dark gray, imnodes native)
  - Background: #26262FFF (dark blue)
  - Zoom-aware: Scales appropriately
  - Pan-aware: Moves with canvas offset
```

**Your code**:
```cpp
// In PrefabCanvas::RenderGrid():
CanvasGridRenderer::RenderGrid(
    m_drawList,
    canvasPos,
    canvasSize,
    m_canvasOffset,
    m_canvasZoom,
    CanvasGridRenderer::Style_VisualScript  // Predefined style
);
```

---

#### Feature: Minimap Navigation
**What it does**: Shows bird's-eye view of entire graph + viewport rectangle

```
┌─────────────────────┐
│ Minimap (top-right) │
│ ┌──────────────────┐ │
│ │ ◻   ◻   ◻   ◻    │ │  (Nodes as tiny dots)
│ │ ◻ ┌────────────┐ │ │  (Viewport as rectangle)
│ │ ◻ │ viewport  │ │ │
│ │   └────────────┘ │ │
│ └──────────────────┘ │
└─────────────────────┘
```

**Your code** (automatic via CanvasMinimapRenderer):
```cpp
// In your Render():
if (m_canvas)
{
    m_canvas->RenderMinimap();  // Minimap renders automatically
}
```

---

### 2.2 Framework Infrastructure Features

#### Feature: Canvas Editor Abstraction (ICanvasEditor)
**What it does**: Unified pan/zoom/coordinate system for all canvas types

```cpp
Interface provides:
  - Pan(dx, dy) - Move canvas by pixel offset
  - SetZoom(scale, centerPoint) - Zoom with mouse position as center
  - GetZoom() - Current zoom level
  - ScreenToCanvas(screenPos) - Convert mouse position to graph coordinates
  - CanvasToScreen(canvasPos) - Convert back for rendering
  - GetGridSpacing() - Grid size at current zoom
  - RenderGrid() - Draw background
  - RenderMinimap() - Draw bird's-eye view
```

**Implementation detail**: You don't call these directly - your canvas inherits from CustomCanvasEditor and gets them automatically.

---

#### Feature: Async Input/Output Pattern (Critical for ImGui)
**What it does**: Separates input detection from rendering for reliable ImGui behavior

```
WRONG (Mixes phases):
void Render()
{
    if (ImGui::IsMouseClicked(1))      // ❌ Input in render phase
        ImGui::OpenPopup("menu");
    if (ImGui::BeginPopup("menu"))     // ❌ Output in render phase
        ImGui::MenuItem("Delete");     // Result: Doesn't work reliably
}

CORRECT (Separate phases):
void HandleInput()  // Input phase (before rendering)
{
    if (ImGui::IsMouseClicked(1) && nodeHovered)
        ImGui::OpenPopup("menu");      // ✅ Flag set at right time
}

void Render()  // Render phase (after input)
{
    RenderNodes();  // Draw visual feedback using state
    if (ImGui::BeginPopup("menu"))     // ✅ ImGui processes flag correctly
        ImGui::MenuItem("Delete");     // Result: Works reliably
}
```

**Why this matters**: ImGui maintains internal state that must be managed in correct frame order. Violating this breaks modals, context menus, hover feedback.

---

#### Feature: State Separation (Hover vs Selection vs Context)
**What it does**: Tracks 3 independent states for reliable interaction

```cpp
m_hoveredNodeId      // Which node mouse is over (every frame updated)
m_selectedNodeId     // Which node clicked (stays until new click)
m_contextNodeId      // Which node right-clicked (for context menu)

Visual feedback:
  Hovered:  Yellow glow, thicker line
  Selected: Blue highlight, keeps on-screen
  Context:  Right-click menu appears (async dispatch)
```

**Pattern**:
```cpp
// Hover (updates every frame)
m_hoveredNodeId = GetNodeAtScreenPos(mousePos);

// Selection (updates on click)
if (ImGui::IsMouseClicked(0) && m_hoveredNodeId >= 0)
    m_selectedNodeId = m_hoveredNodeId;

// Context (async dispatch)
if (ImGui::IsMouseClicked(1) && m_hoveredNodeId >= 0)
{
    m_contextNodeId = m_hoveredNodeId;
    ImGui::OpenPopup("##context");  // Dispatch for async rendering
}
```

---

#### Feature: Coordinate Transformation
**What it does**: Converts between screen coordinates (pixels) and canvas coordinates (logical)

```
Screen Space: Absolute window coordinates
             0          mouse X=100        200
             ├─ origin ──┤
             
Canvas Space: Logical coordinates after pan/zoom
             -50         0            50   (after pan=50, zoom=1)


Formula (CORRECT):
canvas = (screen - origin - pan) / zoom

Why NOT multiply pan by zoom:
  - Pan offset is in screen pixels (independent of zoom)
  - Zoom scales the DISTANCE from origin, not the offset itself
  - Multiplying offset by zoom breaks at zoom != 1.0
```

**Example**:
```cpp
ImVec2 ScreenToCanvas(ImVec2 screenPos)
{
    ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();  // (0, 100)
    
    // Correct formula: (screen - origin - offset) / zoom
    return (screenPos - canvasScreenPos - m_canvasOffset) / m_canvasZoom;
}

// At zoom 2.0, mouse at (100, 100):
// canvas = (100 - 0 - 50) / 2.0 = 25 ✓ (not 0, not 100)
```

---

#### Feature: Connection Hit Detection
**What it does**: Determines if user clicked on a curve (not just endpoints)

```
Bezier Curve Sampling (32 points):
  ├─ Start point (0% along curve)
  ├─ Sample point 1 (3% along)
  ├─ Sample point 2 (6% along)
  ├─ ...
  └─ End point (100% along)

For each sample: Calculate distance to mouse position
Return minimum distance found (in pixels)

Hit tolerance: 10 pixels (adjustable)
Result: Can click anywhere along line, not just endpoints
```

**Implementation**:
```cpp
float GetDistanceToConnection(const Connection& conn, ImVec2 screenPos)
{
    const float HIT_TOLERANCE = 10.0f;
    const int BEZIER_SAMPLES = 32;
    float minDistance = std::numeric_limits<float>::max();

    for (int i = 0; i <= BEZIER_SAMPLES; ++i)
    {
        float t = static_cast<float>(i) / BEZIER_SAMPLES;
        ImVec2 point = EvaluateBezier(t, p1, cp1, cp2, p2);
        minDistance = std::min(minDistance, Distance(point, screenPos));
    }
    
    return minDistance < HIT_TOLERANCE ? minDistance : std::numeric_limits<float>::max();
}
```

---

#### Feature: Dirty Flag System
**What it does**: Tracks modification state for save prompts

```cpp
// Document tracks dirty state
m_document->SetDirty(true);     // Mark as modified

// Framework shows visual feedback
// Tab shows asterisk: "MyGraph*"
// Close button prompts to save

// On save, framework clears it
m_document->SetDirty(false);    // Clears asterisk

// At any time, check state
bool IsDirty() const { return m_document->IsDirty(); }

// GraphEditorBase provides convenience
MarkDirty();  // Sets both document and renderer dirty state
```

---

### 2.3 Integration Features

#### Feature: IGraphRenderer Contract
**What it does**: Defines what every graph renderer must implement

```cpp
class IGraphRenderer
{
    // Tell TabManager what type you are
    virtual std::string GetGraphType() const = 0;
    
    // Load file into your renderer
    virtual bool Load(const std::string& filePath) = 0;
    
    // Save renderer state to file
    virtual bool Save(const std::string& filePath) = 0;
    
    // Render on screen (called every frame)
    virtual void Render() = 0;
    
    // Render framework modals (called after content)
    virtual void RenderFrameworkModals() = 0;
};
```

**Why this matters**: TabManager uses this interface to handle ANY graph type without knowing specifics.

---

#### Feature: GraphEditorBase Foundation
**What it does**: Common base class with selection, dirty flag, framework integration

```cpp
class GraphEditorBase : public IGraphRenderer
{
    // Selection management
    int m_selectedNodeId;
    bool SelectNode(int nodeId);
    int GetSelectedNode() const;
    
    // Dirty flag
    bool IsDirty() const;
    void MarkDirty();
    void SetDirty(bool dirty);
    
    // Framework integration
    CanvasFramework* GetFramework() const;
    void SetDocument(GraphDocument* doc);
    
    // To override:
    virtual bool Load(const std::string& filepath) = 0;
    virtual bool Save(const std::string& filepath) = 0;
    virtual void Render() = 0;
};
```

**Benefit**: Don't reimplement selection/dirty/framework - inherit these automatically.

---

#### Feature: CanvasFramework Integration
**What it does**: Provides toolbar, modals, keyboard shortcuts

```cpp
// In your renderer:
std::unique_ptr<CanvasFramework> m_framework;

// In Load():
if (!m_framework)
    m_framework = std::make_unique<CanvasFramework>(m_document.get());

// In RenderCommonToolbar():
if (m_framework && m_framework->GetToolbar())
    m_framework->GetToolbar()->Render();  // [Save] [SaveAs] [Browse] etc.

// In RenderFrameworkModals():
if (m_framework && m_framework->GetToolbar())
    m_framework->GetToolbar()->RenderModals();  // SaveAs dialog, etc.
```

**What you get**:
- Toolbar renders automatically
- Modals handle file dialogs
- Callbacks route to your ExecuteSave()
- Keyboard shortcuts work automatically

---

## 3. Architecture Overview

### 3.1 System Architecture Diagram

```
┌────────────────────────────────────────────────────────────────┐
│                        BlueprintEditorGUI                       │
│  (Main window, tab bar, active canvas rendering)                │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  TabManager (Routes files to correct renderer)           │  │
│  │  ├─ DetectGraphType(filepath)                            │  │
│  │  │  └─ Returns "VisualScript" | "BehaviorTree" | ...     │  │
│  │  ├─ CreateRenderer(graphType)                            │  │
│  │  │  └─ Instantiates correct renderer                     │  │
│  │  └─ OpenFileInTab(filepath)                              │  │
│  │     └─ Loads file, creates tab, shows tab                │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Active Tab (Currently displayed)                        │  │
│  │                                                           │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │  YourGraphRenderer (inherits GraphEditorBase)      │  │  │
│  │  │                                                     │  │  │
│  │  │  ┌──────────────────────────────────────────────┐  │  │  │
│  │  │  │  RenderCommonToolbar()                       │  │  │  │
│  │  │  │  ├─ RenderCommonToolbar.h methods            │  │  │  │
│  │  │  │  └─ Framework toolbar renders here           │  │  │  │
│  │  │  └──────────────────────────────────────────────┘  │  │  │
│  │  │                                                     │  │  │
│  │  │  ┌──────────────────────────────────────────────┐  │  │  │
│  │  │  │  RenderGraphContent()                        │  │  │  │
│  │  │  │  ├─ RenderCanvas() + nodes + connections    │  │  │  │
│  │  │  │  ├─ RenderProperties()                       │  │  │  │
│  │  │  │  └─ RenderContextMenus() (input dispatch)    │  │  │  │
│  │  │  └──────────────────────────────────────────────┘  │  │  │
│  │  │                                                     │  │  │
│  │  │  ┌──────────────────────────────────────────────┐  │  │  │
│  │  │  │  RenderFrameworkModals()                     │  │  │  │
│  │  │  │  └─ Renders SaveAs modal + error dialogs     │  │  │  │
│  │  │  └──────────────────────────────────────────────┘  │  │  │
│  │  │                                                     │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
└────────────────────────────────────────────────────────────────┘

Render Frame Cycle (Per frame):
1. BlueprintEditorGUI::RenderFrame()
   ├─ RenderTabBar()              [Let user choose tab]
   ├─ RenderActiveCanvas()        [Render active tab's renderer]
   │  └─ activeTab->renderer->Render()
   │     ├─ RenderCommonToolbar()      [Toolbar rendering]
   │     ├─ RenderGraphContent()       [Canvas + properties]
   │     └─ Return to parent
   └─ RenderFrameworkModals()    [Render modals AFTER content]
      └─ activeTab->renderer->RenderFrameworkModals()
         └─ Toolkit->RenderModals()    [SaveAs dialog, etc.]
```

---

### 3.2 Class Hierarchy

```
┌─────────────────────────────────────┐
│     IGraphRenderer (Interface)       │
│                                      │
│ + GetGraphType(): string             │
│ + Load(filepath): bool               │
│ + Save(filepath): bool               │
│ + Render(): void                     │
│ + RenderFrameworkModals(): void      │
└─────────────────────────┬────────────┘
                          │ inherits
                          ▼
┌─────────────────────────────────────┐
│    GraphEditorBase (Base Class)      │
│                                      │
│ # m_selectedNodeId                   │
│ # m_document: GraphDocument*         │
│ # m_framework: CanvasFramework*      │
│                                      │
│ + GetSelectedNode(): int             │
│ + SelectNode(id): bool               │
│ + MarkDirty(): void                  │
│ + IsDirty(): bool                    │
│ + GetFramework(): CanvasFramework*   │
└─────────────────────────┬────────────┘
                          │ inherits
        ┌─────────────────┼─────────────────┐
        ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ VisualScript │  │BehaviorTree  │  │EntityPrefab  │
│   Renderer   │  │   Renderer   │  │   Renderer   │
└──────────────┘  └──────────────┘  └──────────────┘

Each concrete renderer:
  - Implements Load() for their file format
  - Implements Save() for their serialization
  - Implements Render() for their visualization
  - Implements RenderFrameworkModals() delegating to framework
  - Inherits selection, dirty flag, toolbar from GraphEditorBase
```

---

### 3.3 Data Model Pattern

```
Your Renderer:                          Framework:
┌──────────────────────┐               ┌─────────────────┐
│  YourGraphRenderer   │               │ GraphEditorBase │
│                      │               │                 │
│ # m_document    ────────────┐        │ # m_framework   │
│                      │       │        │                 │
└──────────────────────┘       │        └─────────────────┘
                               │
                               ▼
                      ┌─────────────────────────┐
                      │ YourGraphDocument       │
                      │                         │
                      │ - nodes: vector<Node>   │
                      │ - connections: vector   │
                      │ - metadata: map         │
                      │                         │
                      │ + Load(filepath)        │
                      │ + Save(filepath)        │
                      │ + DeleteNode(id)        │
                      │ + ConnectNodes(...)     │
                      │ + GetDirty(): bool      │
                      │ + SetDirty(bool)        │
                      └─────────────────────────┘

Responsibilities:
  YourGraphDocument: Data structure, serialization, validation
  YourGraphRenderer: Visualization, user interaction, framework integration
  GraphEditorBase:   Selection, dirty flag, framework delegation
```

---

### 3.4 Render Loop Flow (Critical)

```
ImGui::NewFrame()
    ↓
BlueprintEditorGUI::RenderFrame()
    ├─ [PHASE 1: TABS]
    │  └─ RenderTabBar()
    │     ├─ Display all tabs
    │     ├─ User can click to switch
    │     └─ No content rendering here
    │
    ├─ [PHASE 2: CONTENT]
    │  └─ RenderActiveCanvas()
    │     └─ activeTab->renderer->Render()
    │        ├─ RenderCommonToolbar()      [Toolbar + buttons]
    │        │  └─ Framework->Render() displays [Save] [SaveAs] etc.
    │        ├─ RenderGraphContent()       [Canvas + nodes + interactions]
    │        │  ├─ RenderCanvas()          [Background grid]
    │        │  ├─ RenderNodes()           [Node boxes + connections]
    │        │  ├─ HandleNodeInteraction() [INPUT DISPATCH - RIGHT-CLICK]
    │        │  │  └─ ImGui::OpenPopup("##context") [Sets flag here]
    │        │  └─ RenderProperties()      [Right panel]
    │        └─ Return to BlueprintEditorGUI
    │
    ├─ [PHASE 3: MODALS - CRITICAL]
    │  └─ RenderFrameworkModals() [MUST be after all content]
    │     └─ Framework->RenderModals()
    │        ├─ SaveAs dialog modal
    │        ├─ Error messages
    │        └─ (ImGui now processes flags set earlier)
    │
    └─ (Note: Context menu also renders here)
       └─ if (ImGui::BeginPopup("##context"))  [Renders popup from flag set earlier]
          └─ MenuItem("Delete")

ImGui::EndFrame()
    ↓
ImGui::Render()

KEY INSIGHT:
  ❌ WRONG: Set ImGui::OpenPopup() and BeginPopup() in same Render() call
  ✅ CORRECT: Set popup flag in HandleNodeInteraction() (input phase)
             Render popup in RenderFrameworkModals() (render phase)
```

---

## 4. Complete API Reference

### 4.1 GraphEditorBase - Core Base Class

**Header**: `Source/BlueprintEditor/Framework/GraphEditorBase.h`

```cpp
class GraphEditorBase : public IGraphRenderer
{
public:
    // Constructor
    GraphEditorBase(const std::string& name);
    virtual ~GraphEditorBase();

    // IGraphRenderer implementation
    virtual std::string GetGraphType() const = 0;  // Override: return your type
    virtual bool Load(const std::string& filePath) = 0;
    virtual bool Save(const std::string& filePath) = 0;
    virtual void Render() = 0;
    virtual void RenderFrameworkModals() = 0;

    // Selection Management
    bool SelectNode(int nodeId);
    int GetSelectedNode() const;
    bool IsNodeSelected(int nodeId) const;
    void ClearSelection();

    // Dirty Flag Management
    void MarkDirty();
    bool IsDirty() const;
    
    // Framework Integration
    CanvasFramework* GetFramework() const;
    void SetDocument(void* docPtr);  // Pass m_document.get()
    
    // Protected access
protected:
    std::unique_ptr<GraphDocument> m_document;
    std::unique_ptr<CanvasFramework> m_framework;
    int m_selectedNodeId = -1;
};
```

**Common Usage Pattern**:

```cpp
class YourGraphRenderer : public GraphEditorBase
{
public:
    YourGraphRenderer(const std::string& name)
        : GraphEditorBase(name) {}

    std::string GetGraphType() const override
    {
        return "YourGraphType";  // Unique identifier
    }

    bool Load(const std::string& filePath) override
    {
        m_document = std::make_unique<YourGraphDocument>();
        if (!m_document->Load(filePath))
            return false;

        // Initialize framework
        if (!m_framework)
            m_framework = std::make_unique<CanvasFramework>(m_document.get());

        return true;
    }

    bool Save(const std::string& filePath) override
    {
        return m_document->Save(filePath);
    }

    void Render() override
    {
        RenderCommonToolbar();  // Toolbar
        RenderGraphContent();    // Canvas
    }

    void RenderFrameworkModals() override
    {
        if (m_framework && m_framework->GetToolbar())
            m_framework->GetToolbar()->RenderModals();
    }

private:
    void RenderCommonToolbar()
    {
        if (m_framework && m_framework->GetToolbar())
            m_framework->GetToolbar()->Render();
    }

    void RenderGraphContent()
    {
        // Your rendering code here
    }
};
```

---

### 4.2 IGraphRenderer - Interface Contract

**Header**: `Source/BlueprintEditor/Framework/GraphEditorBase.h` (defined as interface)

```cpp
class IGraphRenderer
{
public:
    virtual ~IGraphRenderer() = default;

    // Identify graph type for routing
    virtual std::string GetGraphType() const = 0;
    // Returns: "VisualScript", "BehaviorTree", "YourGraphType", etc.
    // Used by: TabManager to route files

    // Load from file
    virtual bool Load(const std::string& filePath) = 0;
    // Params: Full filepath to graph file
    // Returns: true on success, false on failure
    // Called by: TabManager when user opens file
    // Your impl: Parse JSON/XML, populate m_document, initialize UI

    // Save to file
    virtual bool Save(const std::string& filePath) = 0;
    // Params: Full filepath where to save
    // Returns: true on success, false on failure
    // Called by: CanvasFramework when user clicks [Save]
    // Your impl: Serialize m_document to JSON/XML

    // Main render entry point
    virtual void Render() = 0;
    // Called by: BlueprintEditorGUI every frame for active tab
    // Params: None (read ImGui state yourself)
    // Your impl: Should call:
    //   - RenderCommonToolbar() for [Save] [SaveAs] etc.
    //   - RenderGraphContent() for nodes, connections, properties
    //   - Handle input dispatch (HandleNodeInteraction)

    // Render modals (SaveAs dialog, etc.)
    virtual void RenderFrameworkModals() = 0;
    // Called by: BlueprintEditorGUI AFTER Render() completes
    // MUST be after Render() for ImGui frame ordering (CRITICAL!)
    // Your impl: Delegate to m_framework->GetToolbar()->RenderModals()
};
```

---

### 4.3 CanvasFramework - Toolbar & Modal System

**Header**: `Source/BlueprintEditor/Framework/CanvasFramework.h`

```cpp
class CanvasFramework
{
public:
    // Constructor - pass your document
    explicit CanvasFramework(GraphDocument* document);
    ~CanvasFramework();

    // Get toolbar renderer (for rendering buttons)
    CanvasToolbarRenderer* GetToolbar() const;

    // Callbacks (implement in your renderer, call from CanvasFramework)
    std::function<bool(const std::string& filePath)> OnSaveAs;
    std::function<bool(const std::string& filePath)> OnSave;

    // Check dirty flag
    bool GetDocumentDirty() const;
    void SetDocumentDirty(bool dirty);
};

class CanvasToolbarRenderer
{
public:
    // Main render function - displays toolbar buttons
    void Render();
    // Renders: [Save] [SaveAs] [Browse] [Grid ☑] [Reset View] [Mini ☑]
    // Calls: ExecuteSave() when [Save] clicked
    // Calls: Shows SaveAs modal when [SaveAs] clicked

    // Render modals (SaveAs dialog, error messages)
    void RenderModals();
    // Called AFTER Render() completes (different frame phase!)
    // Shows: File browser modal, error dialogs

    // Configure toolbar
    void SetGridVisible(bool visible);
    bool IsGridVisible() const;
    
    void SetMinimapVisible(bool visible);
    bool IsMinimapVisible() const;
};
```

**Usage Pattern**:

```cpp
// In your Load():
m_framework = std::make_unique<CanvasFramework>(m_document.get());

// In your RenderCommonToolbar():
if (m_framework && m_framework->GetToolbar())
{
    m_framework->GetToolbar()->Render();  // [Save] [SaveAs] etc. appear
}

// In your RenderFrameworkModals():
if (m_framework && m_framework->GetToolbar())
{
    m_framework->GetToolbar()->RenderModals();  // SaveAs modal, etc.
}

// Wire up callbacks (optional, framework handles defaults)
m_framework->OnSave = [this](const std::string& path) {
    return ExecuteSave(path);
};

m_framework->OnSaveAs = [this](const std::string& path) {
    return ExecuteSaveAs(path);
};
```

---

### 4.4 TabManager - File Routing & Tab Management

**Header**: `Source/BlueprintEditor/TabManager.h`

```cpp
class TabManager
{
public:
    static TabManager& Instance();

    // Open file in tab (main entry point)
    std::string OpenFileInTab(const std::string& filePath);
    // Params: Full file path
    // Returns: Tab ID (empty string if failed)
    // Logic:
    //   1. Detect file type (GetGraphType routing)
    //   2. Create appropriate renderer
    //   3. Call Load() on renderer
    //   4. Create tab entry
    //   5. Caches loaded file to prevent reload

    // Query tabs
    EditorTab* GetTabByID(const std::string& tabID);
    EditorTab* GetActiveTab() const;
    
    // Dirty flag management
    void MarkTabDirty(const std::string& tabID, bool isDirty);
    bool IsTabDirty(const std::string& tabID) const;
    
    // Type detection (extensible)
    std::string DetectGraphType(const std::string& filePath);
    // Returns: "VisualScript", "BehaviorTree", "YourGraphType"
    // Based on: File extension or content inspection

    // Internal (do not call):
    IGraphRenderer* CreateRenderer(const std::string& graphType);
};
```

**Internally Called By**:
- `BlueprintEditorGUI::OnFileDoubleClicked(filepath)`
- `BlueprintEditorGUI::OnFileMenuOpenClicked(filepath)`

**You Don't Call This Directly** - Framework uses it internally to route files.

---

### 4.5 CustomCanvasEditor - Pan/Zoom/Coordinate System

**Header**: `Source/BlueprintEditor/Utilities/CustomCanvasEditor.h`

```cpp
class CustomCanvasEditor
{
public:
    CustomCanvasEditor(const std::string& name, ImVec2 screenPos, ImVec2 size);

    // Pan (translate canvas)
    void Pan(float dx, float dy);
    ImVec2 GetPanOffset() const;
    void SetPanOffset(ImVec2 offset);

    // Zoom (scale canvas)
    void SetZoom(float scale, ImVec2 centerPoint = ImVec2(0, 0));
    void ZoomToFit(const std::vector<ImRect>& bounds);
    float GetZoom() const;
    float GetMinZoom() const;  // 0.1x
    float GetMaxZoom() const;  // 3.0x

    // Coordinate transformation
    ImVec2 ScreenToCanvas(ImVec2 screenPos) const;
    ImVec2 CanvasToScreen(ImVec2 canvasPos) const;

    // Grid
    void RenderGrid();
    ImVec2 GetGridSpacing() const;

    // Minimap
    void RenderMinimap();
    void SetMinimapVisible(bool visible);

    // Rendering
    void BeginRender();
    void EndRender();
    ImDrawList* GetDrawList();

    // Reset
    void ResetViewport();  // Reset to default pan/zoom

protected:
    ImVec2 m_canvasOffset;
    float m_canvasZoom = 1.0f;
    ImVec2 m_canvasSize;
    ImVec2 m_canvasScreenPos;
};
```

**Your Canvas Inherits From This**:

```cpp
class PrefabCanvas : public CustomCanvasEditor
{
public:
    PrefabCanvas(const std::string& name, ImVec2 screenPos, ImVec2 size)
        : CustomCanvasEditor(name, screenPos, size) {}

    void Render()
    {
        BeginRender();
        
        RenderGrid();          // From CustomCanvasEditor
        RenderNodes();         // Your implementation
        RenderConnections();   // Your implementation
        
        RenderMinimap();       // From CustomCanvasEditor
        
        EndRender();
    }
};
```

---

### 4.6 CanvasGridRenderer - Standardized Grid

**Header**: `Source/BlueprintEditor/Utilities/CanvasGridRenderer.h`

```cpp
class CanvasGridRenderer
{
public:
    // Predefined styles (all canvas types use Style_VisualScript)
    enum class Style
    {
        Style_VisualScript,    // 24px major, dark background
        Style_Compact,         // 12px major, darker background
        Custom                 // User-defined
    };

    struct StyleConfig
    {
        float majorSpacing = 24.0f;
        ImU32 backgroundColor = IM_COL32(38, 38, 47, 255);
        ImU32 majorLineColor = IM_COL32(63, 63, 71, 255);
        ImU32 minorLineColor = IM_COL32(45, 45, 55, 255);
        float minorDivisor = 1.0f;  // 1.0 = no minor lines
    };

    // Static rendering
    static void RenderGrid(
        ImDrawList* drawList,
        ImVec2 canvasPos,
        ImVec2 canvasSize,
        ImVec2 offset,
        float zoom,
        Style style
    );

    // Get style configuration
    static StyleConfig GetStyle(Style style);
};
```

**Your Code** (usually in your Canvas::RenderGrid()):

```cpp
void PrefabCanvas::RenderGrid()
{
    CanvasGridRenderer::RenderGrid(
        GetDrawList(),
        m_canvasScreenPos,
        m_canvasSize,
        m_canvasOffset,
        m_canvasZoom,
        CanvasGridRenderer::Style_VisualScript
    );
}
```

---

### 4.7 CanvasMinimapRenderer - Bird's-Eye View

**Header**: `Source/BlueprintEditor/Utilities/CanvasMinimapRenderer.h`

```cpp
class CanvasMinimapRenderer
{
public:
    enum class MinimapPosition { TopLeft, TopRight, BottomLeft, BottomRight };

    // Update with current graph state
    void UpdateNodes(const std::vector<YourNode>& nodes);
    void UpdateViewport(ImVec2 canvasSize, ImVec2 canvasOffset, float zoom);

    // Rendering (for ImGui-based canvas)
    void RenderCustom(ImDrawList* drawList, ImVec2 canvasSize);
    
    // Rendering (for ImNodes-based canvas)
    void RenderImNodes();

    // Configuration
    void SetVisible(bool visible);
    bool IsVisible() const;
    void SetSize(float sizeRatio);  // 0.05 to 0.5 of canvas
    void SetPosition(MinimapPosition pos);

    // Styling
    void SetBackgroundColor(ImU32 color);
    void SetNodeColor(ImU32 color);
    void SetViewportColor(ImU32 color);
};
```

**Your Code** (usually in your Canvas::Render()):

```cpp
void PrefabCanvas::Render()
{
    // ... render nodes, connections ...

    // Render minimap
    if (m_minimap)
    {
        m_minimap->UpdateNodes(m_nodes);
        m_minimap->UpdateViewport(m_canvasSize, m_canvasOffset, m_canvasZoom);
        m_minimap->RenderCustom(GetDrawList(), m_canvasSize);
    }
}
```

---

## 5. Data Flow Models

### 5.1 User Opens File (Complete Flow)

```
User double-clicks "mygraph.bt" in file browser
    ↓
BlueprintEditorGUI::OnFileDoubleClicked(filepath)
    ├─ filePath = "C:/graphs/mygraph.bt"
    └─ TabManager::OpenFileInTab(filePath)
       ├─ Step 1: Detect type
       │  └─ DetectGraphType("mygraph.bt")
       │     ├─ Check extension: ".bt" → BehaviorTree
       │     ├─ Check content: JSON parsing if ambiguous
       │     └─ Return "BehaviorTree"
       │
       ├─ Step 2: Create renderer
       │  └─ CreateRenderer("BehaviorTree")
       │     ├─ new BehaviorTreeRenderer()
       │     └─ Return renderer instance
       │
       ├─ Step 3: Load file
       │  └─ renderer->Load(filePath)
       │     ├─ m_document = new BehaviorTreeGraphDocument()
       │     ├─ m_document->Load("C:/graphs/mygraph.bt")
       │     │  ├─ Read JSON file
       │     │  ├─ Parse nodes array
       │     │  ├─ Parse connections array
       │     │  └─ Populate m_nodes, m_connections
       │     ├─ m_framework = new CanvasFramework(m_document)
       │     └─ Return true (success)
       │
       ├─ Step 4: Create tab
       │  ├─ EditorTab tab
       │  │  ├─ tab.tabID = "bt_0001" (unique ID)
       │  │  ├─ tab.filename = "mygraph.bt"
       │  │  ├─ tab.renderer = renderer
       │  │  └─ tab.isDirty = false
       │  └─ m_tabs.push_back(tab)
       │
       ├─ Step 5: Cache file
       │  └─ m_loadedFilePaths["C:/graphs/mygraph.bt"] = renderer
       │
       └─ Return tab ID: "bt_0001"
            └─ BlueprintEditorGUI shows new tab in tab bar
               └─ Tab label: "mygraph.bt"
                  └─ Next frame: Calls renderer->Render()

Result: User sees graph rendered on screen
```

---

### 5.2 User Saves File (Complete Flow)

```
User clicks [Save] button
    ↓
CanvasToolbarRenderer::OnSaveClicked()
    ├─ Get current filepath from m_document
    ├─ Check if filepath empty (new file)
    │  ├─ If empty: Trigger SaveAs instead
    │  └─ If not empty: Continue
    └─ Call ExecuteSave(currentPath)
       └─ Your Renderer::ExecuteSave(filepath)
          ├─ Step 1: Validate document
          │  └─ if (!m_document) return false;
          │
          ├─ Step 2: Log dirty state before save
          │  └─ SYSTEM_LOG << "Dirty before save: " << IsDirty();
          │
          ├─ Step 3: Call backend save
          │  └─ m_document->Save(filepath)
          │     ├─ Serialize m_nodes to JSON
          │     ├─ Serialize m_connections to JSON
          │     ├─ Serialize metadata to JSON
          │     ├─ Write to file system
          │     └─ Return true
          │
          ├─ Step 4: Clear dirty flag
          │  └─ m_document->SetDirty(false)
          │     └─ Tab asterisk disappears: "mygraph.bt*" → "mygraph.bt"
          │
          ├─ Step 5: Log successful save
          │  └─ SYSTEM_LOG << "Save complete, file: " << filepath;
          │
          └─ Return true (success)
             └─ TabManager notified
                └─ Tab UI updated
                   └─ Asterisk removed from tab title

Result: File saved to disk, dirty flag cleared, UI updated
```

---

### 5.3 User Right-Clicks Node (Context Menu Flow)

```
Frame N (Input Phase):
─────────────────────

User mouse position: (100, 100) in window

BlueprintEditorGUI::RenderFrame()
    └─ activeTab->renderer->Render()
       └─ RenderGraphContent()
          └─ HandleNodeInteraction()  ← INPUT DISPATCH PHASE
             ├─ Get mouse position: ImGui::GetMousePos() = (100, 100)
             │
             ├─ m_hoveredNodeId = GetNodeAtScreenPos((100, 100))
             │  └─ Calls ScreenToCanvas(screenPos)
             │     ├─ Formula: (screen - origin - offset) / zoom
             │     ├─ Example: (100 - 0 - 50) / 1.0 = 50 (canvas X)
             │     └─ Returns node at canvas position 50
             │        └─ m_hoveredNodeId = 5 (node ID)
             │
             ├─ Check for right-click: ImGui::IsMouseClicked(ImGuiMouseButton_Right)
             │  └─ User DID right-click
             │
             ├─ if (m_hoveredNodeId >= 0)  // && right-click
             │  ├─ m_contextNodeId = m_hoveredNodeId;  // Save which node
             │  └─ ImGui::OpenPopup("##node_context");  ← FLAG SET HERE
             │
             └─ Return to Render()

RenderGraphContent() returns
    └─ RenderFrameworkModals()  ← RENDER PHASE (different frame point)
       └─ RenderContextMenu()
          ├─ if (ImGui::BeginPopup("##node_context"))  ← FLAG PROCESSED HERE
          │  ├─ ImGui sees flag set earlier ✓
          │  ├─ Creates popup window
          │  ├─ ImGui::MenuItem("Delete Node")
          │  │  └─ User clicks → calls DeleteNode(m_contextNodeId)
          │  ├─ ImGui::MenuItem("Rename")
          │  │  └─ User clicks → shows rename dialog
          │  └─ ImGui::EndPopup()
          │
          └─ Popup rendered on screen ✓

Result: Context menu appears when user right-clicked
        User can interact with menu items

Key Pattern:
  ❌ WRONG: if (right-click) OpenPopup() then BeginPopup() in SAME frame
  ✅ CORRECT: Set flag in input phase, render popup in render phase (async)
```

---

### 5.4 User Drags Node (Interaction Flow)

```
Frame N:
────────
User starts drag:
  Mouse down on node at screen position (100, 100)
  
  HandleNodeInteraction()
    ├─ m_hoveredNodeId = GetNodeAtScreenPos((100, 100)) = 5
    ├─ if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    │  ├─ if (m_hoveredNodeId >= 0)
    │  │  └─ m_draggedNodeId = m_hoveredNodeId = 5
    │  │     m_dragStartPos = (100, 100)
    │  └─ m_isDragging = true
    └─ Store state for next frame

RenderGraphContent()
  └─ RenderNodes()
     ├─ For each node:
     │  ├─ if (node.id == m_draggedNodeId)
     │  │  ├─ Color = BRIGHT_BLUE (dragging highlight)
     │  │  └─ Thickness = 3.0f (thicker border)
     │  └─ Render node box
     │
     └─ Visual feedback: Node box highlights

Frame N+1:
──────────
User moves mouse: (150, 120)

HandleNodeInteraction()
  ├─ m_hoveredNodeId = GetNodeAtScreenPos((150, 120)) = 5 (still over node)
  ├─ if (m_isDragging)
  │  ├─ Screen delta: (150-100, 120-100) = (50, 20)
  │  ├─ Convert to canvas: (50-0, 20-0) / 1.0 = (50, 20)
  │  ├─ New canvas position: original + delta
  │  │  └─ m_document->GetNode(5)->position += (50, 20)
  │  ├─ Mark dirty:
  │  │  └─ MarkDirty()  // Tab shows asterisk
  │  └─ Update m_dragStartPos for next frame
  │
  └─ Node position in m_document updated

RenderGraphContent()
  └─ RenderNodes()
     └─ Node renders at NEW position on canvas

Result: Node follows mouse movement
        Graph marked as modified (dirty flag)

Frame N+2:
──────────
User releases mouse

HandleNodeInteraction()
  ├─ if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
  │  ├─ m_isDragging = false
  │  └─ m_draggedNodeId = -1
  │
  └─ Drag operation complete

RenderGraphContent()
  └─ RenderNodes()
     └─ Node renders with normal highlighting (not bright blue)
        └─ Drag complete, back to normal appearance

Result: Node stays at new position
        Changes saved to m_document
        Awaiting user [Save] to persist to disk
```

---

## 6. Step-by-Step Implementation Guide

### 6.0 Pre-Implementation Checklist

Before starting, ensure you have:

```
✓ File structure created:
  └─ Source/BlueprintEditor/YourEditor/
     ├─ YourGraphDocument.h/cpp
     ├─ YourGraphRenderer.h/cpp
     ├─ YourCanvas.h/cpp
     └─ YourPropertyPanel.h/cpp (optional)

✓ Dependencies available:
  └─ #include "Framework/GraphEditorBase.h"
  └─ #include "Framework/CanvasFramework.h"
  └─ #include "Utilities/CustomCanvasEditor.h"
  └─ #include "Utilities/CanvasGridRenderer.h"
  └─ #include "Utilities/CanvasMinimapRenderer.h"

✓ Project file updated:
  └─ OlympeBlueprintEditor.vcxproj.filters
     └─ Added your new files

✓ Build verification:
  └─ Existing framework files compile (should already work)
```

---

### 6.1 Step 1: Create Data Model (YourGraphDocument)

**File**: `YourGraphEditor/YourGraphDocument.h`

```cpp
#pragma once

#include <vector>
#include <string>
#include <map>
#include "nlohmann/json.hpp"

// Node structure (your domain-specific node)
struct YourNode
{
    int nodeId;
    std::string nodeType;  // e.g., "StartNode", "ActionNode", "ConditionNode"
    std::string nodeName;
    
    float positionX, positionY;  // Canvas position
    float width, height;         // Node size
    bool enabled = true;
    
    std::map<std::string, std::string> properties;  // Custom properties
};

// Connection structure
struct YourConnection
{
    int connectionId;
    int sourceNodeId;
    int targetNodeId;
    int sourcePortIndex = 0;
    int targetPortIndex = 0;
};

// Main document class
class YourGraphDocument
{
public:
    YourGraphDocument() = default;
    ~YourGraphDocument() = default;

    // Serialization
    bool Load(const std::string& filePath);
    bool Save(const std::string& filePath) const;

    // Node management
    YourNode* CreateNode(const std::string& nodeType, float x, float y);
    bool DeleteNode(int nodeId);
    YourNode* GetNode(int nodeId) const;
    const std::vector<YourNode>& GetNodes() const { return m_nodes; }

    // Connection management
    YourConnection* ConnectNodes(int sourceId, int targetId);
    bool DisconnectNodes(int connectionId);
    const std::vector<YourConnection>& GetConnections() const { return m_connections; }

    // Dirty flag
    void SetDirty(bool dirty) { m_isDirty = dirty; }
    bool GetDirty() const { return m_isDirty; }

    // Metadata
    void SetMetadata(const std::string& key, const std::string& value);
    std::string GetMetadata(const std::string& key) const;

private:
    std::vector<YourNode> m_nodes;
    std::vector<YourConnection> m_connections;
    std::map<std::string, std::string> m_metadata;
    bool m_isDirty = false;
    int m_nextNodeId = 1;
    int m_nextConnectionId = 1;
};
```

**File**: `YourGraphEditor/YourGraphDocument.cpp`

```cpp
#include "YourGraphDocument.h"
#include <fstream>
#include <sstream>

bool YourGraphDocument::Load(const std::string& filePath)
{
    try
    {
        // Read JSON file
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        nlohmann::json j;
        file >> j;
        file.close();

        // Parse nodes
        if (j.contains("nodes") && j["nodes"].is_array())
        {
            for (const auto& nodeJson : j["nodes"])
            {
                YourNode node;
                node.nodeId = nodeJson["nodeId"].get<int>();
                node.nodeType = nodeJson["nodeType"].get<std::string>();
                node.nodeName = nodeJson["nodeName"].get<std::string>();
                node.positionX = nodeJson["positionX"].get<float>();
                node.positionY = nodeJson["positionY"].get<float>();
                node.width = nodeJson["width"].get<float>();
                node.height = nodeJson["height"].get<float>();
                node.enabled = nodeJson.value("enabled", true);

                if (nodeJson.contains("properties"))
                {
                    for (auto& [key, value] : nodeJson["properties"].items())
                        node.properties[key] = value.get<std::string>();
                }

                m_nodes.push_back(node);
                m_nextNodeId = std::max(m_nextNodeId, node.nodeId + 1);
            }
        }

        // Parse connections
        if (j.contains("connections") && j["connections"].is_array())
        {
            for (const auto& connJson : j["connections"])
            {
                YourConnection conn;
                conn.connectionId = connJson["connectionId"].get<int>();
                conn.sourceNodeId = connJson["sourceNodeId"].get<int>();
                conn.targetNodeId = connJson["targetNodeId"].get<int>();
                conn.sourcePortIndex = connJson.value("sourcePortIndex", 0);
                conn.targetPortIndex = connJson.value("targetPortIndex", 0);

                m_connections.push_back(conn);
                m_nextConnectionId = std::max(m_nextConnectionId, conn.connectionId + 1);
            }
        }

        m_isDirty = false;
        return true;
    }
    catch (const std::exception& ex)
    {
        // SYSTEM_LOG << "Error loading file: " << ex.what();
        return false;
    }
}

bool YourGraphDocument::Save(const std::string& filePath) const
{
    try
    {
        nlohmann::json j;

        // Serialize nodes
        nlohmann::json nodesArray = nlohmann::json::array();
        for (const auto& node : m_nodes)
        {
            nlohmann::json nodeJson;
            nodeJson["nodeId"] = node.nodeId;
            nodeJson["nodeType"] = node.nodeType;
            nodeJson["nodeName"] = node.nodeName;
            nodeJson["positionX"] = node.positionX;
            nodeJson["positionY"] = node.positionY;
            nodeJson["width"] = node.width;
            nodeJson["height"] = node.height;
            nodeJson["enabled"] = node.enabled;

            if (!node.properties.empty())
            {
                nlohmann::json propsJson;
                for (const auto& [key, value] : node.properties)
                    propsJson[key] = value;
                nodeJson["properties"] = propsJson;
            }

            nodesArray.push_back(nodeJson);
        }
        j["nodes"] = nodesArray;

        // Serialize connections
        nlohmann::json connectionsArray = nlohmann::json::array();
        for (const auto& conn : m_connections)
        {
            nlohmann::json connJson;
            connJson["connectionId"] = conn.connectionId;
            connJson["sourceNodeId"] = conn.sourceNodeId;
            connJson["targetNodeId"] = conn.targetNodeId;
            connJson["sourcePortIndex"] = conn.sourcePortIndex;
            connJson["targetPortIndex"] = conn.targetPortIndex;

            connectionsArray.push_back(connJson);
        }
        j["connections"] = connectionsArray;

        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open())
            return false;

        file << j.dump(2);  // Pretty-print with 2-space indent
        file.close();

        return true;
    }
    catch (const std::exception& ex)
    {
        // SYSTEM_LOG << "Error saving file: " << ex.what();
        return false;
    }
}

YourNode* YourGraphDocument::CreateNode(const std::string& nodeType, float x, float y)
{
    YourNode node;
    node.nodeId = m_nextNodeId++;
    node.nodeType = nodeType;
    node.nodeName = nodeType + "_" + std::to_string(node.nodeId);
    node.positionX = x;
    node.positionY = y;
    node.width = 100.0f;    // Default width
    node.height = 60.0f;    // Default height
    node.enabled = true;

    m_nodes.push_back(node);
    m_isDirty = true;
    return &m_nodes.back();
}

bool YourGraphDocument::DeleteNode(int nodeId)
{
    // Remove node
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [nodeId](const YourNode& n) { return n.nodeId == nodeId; });
    
    if (it == m_nodes.end())
        return false;

    m_nodes.erase(it);

    // Remove connections involving this node
    auto connIt = std::remove_if(m_connections.begin(), m_connections.end(),
        [nodeId](const YourConnection& c) {
            return c.sourceNodeId == nodeId || c.targetNodeId == nodeId;
        });
    m_connections.erase(connIt, m_connections.end());

    m_isDirty = true;
    return true;
}

YourNode* YourGraphDocument::GetNode(int nodeId) const
{
    for (auto& node : const_cast<std::vector<YourNode>&>(m_nodes))
    {
        if (node.nodeId == nodeId)
            return &node;
    }
    return nullptr;
}

YourConnection* YourGraphDocument::ConnectNodes(int sourceId, int targetId)
{
    // Prevent self-connections
    if (sourceId == targetId)
        return nullptr;

    // Prevent duplicate connections
    for (auto& conn : m_connections)
    {
        if (conn.sourceNodeId == sourceId && conn.targetNodeId == targetId)
            return &conn;
    }

    YourConnection conn;
    conn.connectionId = m_nextConnectionId++;
    conn.sourceNodeId = sourceId;
    conn.targetNodeId = targetId;

    m_connections.push_back(conn);
    m_isDirty = true;
    return &m_connections.back();
}

bool YourGraphDocument::DisconnectNodes(int connectionId)
{
    auto it = std::find_if(m_connections.begin(), m_connections.end(),
        [connectionId](const YourConnection& c) { return c.connectionId == connectionId; });
    
    if (it == m_connections.end())
        return false;

    m_connections.erase(it);
    m_isDirty = true;
    return true;
}

void YourGraphDocument::SetMetadata(const std::string& key, const std::string& value)
{
    m_metadata[key] = value;
}

std::string YourGraphDocument::GetMetadata(const std::string& key) const
{
    auto it = m_metadata.find(key);
    return it != m_metadata.end() ? it->second : "";
}
```

---

### 6.2 Step 2: Create Canvas (YourCanvas)

**File**: `YourGraphEditor/YourCanvas.h`

```cpp
#pragma once

#include "../Utilities/CustomCanvasEditor.h"
#include <vector>
#include <memory>

class YourGraphDocument;
struct YourNode;
struct YourConnection;

class YourCanvas : public CustomCanvasEditor
{
public:
    YourCanvas(const std::string& name, ImVec2 screenPos, ImVec2 size, YourGraphDocument* document);

    // Main render
    void Render() override;

    // Input handling (async pattern)
    void HandleNodeInteraction(ImVec2 mousePos);
    void RenderContextMenu();

    // Rendering
    void RenderNodes();
    void RenderConnections();

    // Hit detection
    int GetNodeAtScreenPos(ImVec2 screenPos) const;
    float GetDistanceToConnection(const YourConnection& conn, ImVec2 screenPos) const;

    // Dirty flag
    void MarkDirty() { if (m_document) m_document->SetDirty(true); }

protected:
    YourGraphDocument* m_document = nullptr;
    
    // State tracking (async pattern)
    int m_hoveredNodeId = -1;
    int m_selectedNodeId = -1;
    int m_contextNodeId = -1;
    
    // Dragging state
    int m_draggedNodeId = -1;
    ImVec2 m_dragStartPos;
    bool m_isDragging = false;
};
```

**File**: `YourGraphEditor/YourCanvas.cpp`

```cpp
#include "YourCanvas.h"
#include "YourGraphDocument.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

YourCanvas::YourCanvas(const std::string& name, ImVec2 screenPos, ImVec2 size, YourGraphDocument* document)
    : CustomCanvasEditor(name, screenPos, size), m_document(document)
{
}

void YourCanvas::Render()
{
    if (!m_document)
        return;

    BeginRender();

    // Render background grid
    RenderGrid();

    // Render connections first (so they appear behind nodes)
    RenderConnections();

    // Render nodes
    RenderNodes();

    // Input handling (async dispatch - set flags here)
    ImVec2 mousePos = ImGui::GetMousePos();
    HandleNodeInteraction(mousePos);

    // Render minimap
    RenderMinimap();

    EndRender();
}

void YourCanvas::HandleNodeInteraction(ImVec2 mousePos)
{
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        return;

    // Update hover state
    m_hoveredNodeId = GetNodeAtScreenPos(mousePos);

    // Left-click: Select node
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (m_hoveredNodeId >= 0)
        {
            m_selectedNodeId = m_hoveredNodeId;
            m_draggedNodeId = m_hoveredNodeId;
            m_dragStartPos = mousePos;
            m_isDragging = true;
        }
        else
        {
            m_selectedNodeId = -1;
        }
    }

    // Left-drag: Move selected node
    if (m_isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        if (m_draggedNodeId >= 0)
        {
            ImVec2 delta = ImGui::GetMousePos() - m_dragStartPos;
            delta = ImVec2(delta.x / m_canvasZoom, delta.y / m_canvasZoom);

            if (auto node = m_document->GetNode(m_draggedNodeId))
            {
                node->positionX += delta.x;
                node->positionY += delta.y;
                m_dragStartPos = ImGui::GetMousePos();
                MarkDirty();
            }
        }
    }

    // Left-release: End drag
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_isDragging = false;
        m_draggedNodeId = -1;
    }

    // Right-click: Context menu dispatch (async)
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        if (m_hoveredNodeId >= 0)
        {
            m_contextNodeId = m_hoveredNodeId;
            ImGui::OpenPopup("##node_context");  // Flag set for async rendering
        }
    }

    // Delete key: Delete selected node
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        if (m_selectedNodeId >= 0)
        {
            m_document->DeleteNode(m_selectedNodeId);
            m_selectedNodeId = -1;
            MarkDirty();
        }
    }
}

void YourCanvas::RenderContextMenu()
{
    // This is called AFTER HandleNodeInteraction (async pattern)
    // ImGui flag was set earlier, now we render
    if (ImGui::BeginPopup("##node_context"))
    {
        if (ImGui::MenuItem("Delete Node"))
        {
            if (m_document)
                m_document->DeleteNode(m_contextNodeId);
            MarkDirty();
        }

        if (ImGui::MenuItem("Rename Node"))
        {
            // Optional: Implement rename dialog
        }

        ImGui::EndPopup();
    }
}

void YourCanvas::RenderNodes()
{
    if (!m_document)
        return;

    auto drawList = GetDrawList();
    if (!drawList)
        return;

    for (const auto& node : m_document->GetNodes())
    {
        // Convert canvas position to screen position
        ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node.positionX, node.positionY));
        ImVec2 nodeScreenSize = ImVec2(node.width * m_canvasZoom, node.height * m_canvasZoom);
        ImVec2 nodeScreenMax = ImVec2(nodeScreenPos.x + nodeScreenSize.x, nodeScreenPos.y + nodeScreenSize.y);

        // Determine color based on state
        ImU32 borderColor = IM_COL32(100, 100, 100, 255);
        float borderThickness = 1.5f;

        if (node.nodeId == m_hoveredNodeId)
        {
            borderColor = IM_COL32(255, 200, 0, 255);  // Yellow for hover
            borderThickness = 2.5f;
        }

        if (node.nodeId == m_selectedNodeId)
        {
            borderColor = IM_COL32(0, 150, 255, 255);  // Blue for selection
            borderThickness = 2.5f;
        }

        // Draw node box
        ImU32 backgroundColor = IM_COL32(50, 50, 60, 255);
        drawList->AddRectFilled(nodeScreenPos, nodeScreenMax, backgroundColor);
        drawList->AddRect(nodeScreenPos, nodeScreenMax, borderColor, 0.0f, 0, borderThickness);

        // Draw text (node name)
        ImVec2 textPos = ImVec2(nodeScreenPos.x + 5.0f, nodeScreenPos.y + 5.0f);
        drawList->AddText(textPos, IM_COL32(200, 200, 200, 255), node.nodeName.c_str());
    }
}

void YourCanvas::RenderConnections()
{
    if (!m_document)
        return;

    auto drawList = GetDrawList();
    if (!drawList)
        return;

    for (const auto& conn : m_document->GetConnections())
    {
        auto sourceNode = m_document->GetNode(conn.sourceNodeId);
        auto targetNode = m_document->GetNode(conn.targetNodeId);

        if (!sourceNode || !targetNode)
            continue;

        // Get node positions in screen space
        ImVec2 sourcePos = CanvasToScreen(ImVec2(sourceNode->positionX + sourceNode->width, sourceNode->positionY + sourceNode->height / 2));
        ImVec2 targetPos = CanvasToScreen(ImVec2(targetNode->positionX, targetNode->positionY + targetNode->height / 2));

        // Draw Bezier curve
        ImVec2 controlPointOffset = ImVec2((targetPos.x - sourcePos.x) * 0.4f, 0.0f);
        ImVec2 cp1 = ImVec2(sourcePos.x + controlPointOffset.x, sourcePos.y);
        ImVec2 cp2 = ImVec2(targetPos.x - controlPointOffset.x, targetPos.y);

        ImU32 lineColor = IM_COL32(150, 150, 150, 255);

        // Check if hovered
        float distToConnection = GetDistanceToConnection(conn, ImGui::GetMousePos());
        if (distToConnection < 10.0f)
        {
            lineColor = IM_COL32(255, 200, 0, 255);  // Yellow when hovered
        }

        drawList->AddBezierCubic(sourcePos, cp1, cp2, targetPos, lineColor, 2.0f, 32);
    }
}

int YourCanvas::GetNodeAtScreenPos(ImVec2 screenPos) const
{
    if (!m_document)
        return -1;

    for (const auto& node : m_document->GetNodes())
    {
        ImVec2 nodeScreenPos = CanvasToScreen(ImVec2(node.positionX, node.positionY));
        ImVec2 nodeScreenMax = ImVec2(nodeScreenPos.x + node.width * m_canvasZoom, nodeScreenPos.y + node.height * m_canvasZoom);

        if (screenPos.x >= nodeScreenPos.x && screenPos.x <= nodeScreenMax.x &&
            screenPos.y >= nodeScreenPos.y && screenPos.y <= nodeScreenMax.y)
        {
            return node.nodeId;
        }
    }

    return -1;
}

float YourCanvas::GetDistanceToConnection(const YourConnection& conn, ImVec2 screenPos) const
{
    auto sourceNode = m_document->GetNode(conn.sourceNodeId);
    auto targetNode = m_document->GetNode(conn.targetNodeId);

    if (!sourceNode || !targetNode)
        return std::numeric_limits<float>::max();

    ImVec2 sourcePos = CanvasToScreen(ImVec2(sourceNode->positionX + sourceNode->width, sourceNode->positionY + sourceNode->height / 2));
    ImVec2 targetPos = CanvasToScreen(ImVec2(targetNode->positionX, targetNode->positionY + targetNode->height / 2));

    const int BEZIER_SAMPLES = 32;
    float minDistance = std::numeric_limits<float>::max();

    ImVec2 controlPointOffset = ImVec2((targetPos.x - sourcePos.x) * 0.4f, 0.0f);
    ImVec2 cp1 = ImVec2(sourcePos.x + controlPointOffset.x, sourcePos.y);
    ImVec2 cp2 = ImVec2(targetPos.x - controlPointOffset.x, targetPos.y);

    for (int i = 0; i <= BEZIER_SAMPLES; ++i)
    {
        float t = static_cast<float>(i) / BEZIER_SAMPLES;
        ImVec2 point = ImGui::GetBezierQuadraticCurvePoint(t, sourcePos, cp1, targetPos);
        
        float dx = point.x - screenPos.x;
        float dy = point.y - screenPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        minDistance = std::min(minDistance, dist);
    }

    return minDistance;
}
```

---

### 6.3 Step 3: Create Renderer (YourGraphRenderer)

**File**: `YourGraphEditor/YourGraphRenderer.h`

```cpp
#pragma once

#include "../Framework/GraphEditorBase.h"
#include <memory>

class YourGraphDocument;
class YourCanvas;

class YourGraphRenderer : public GraphEditorBase
{
public:
    YourGraphRenderer(const std::string& name = "YourGraphEditor");
    ~YourGraphRenderer();

    // IGraphRenderer contract
    std::string GetGraphType() const override { return "YourGraphType"; }
    bool Load(const std::string& filePath) override;
    bool Save(const std::string& filePath) override;
    void Render() override;
    void RenderFrameworkModals() override;

private:
    std::unique_ptr<YourCanvas> m_canvas;

    void RenderCommonToolbar();
    void RenderGraphContent();
    void ExecuteSave();
};
```

**File**: `YourGraphEditor/YourGraphRenderer.cpp`

```cpp
#include "YourGraphRenderer.h"
#include "YourGraphDocument.h"
#include "YourCanvas.h"
#include "../Framework/CanvasFramework.h"
#include "imgui.h"

YourGraphRenderer::YourGraphRenderer(const std::string& name)
    : GraphEditorBase(name)
{
}

YourGraphRenderer::~YourGraphRenderer()
{
}

bool YourGraphRenderer::Load(const std::string& filePath)
{
    // Create document
    auto document = std::make_unique<YourGraphDocument>();
    
    // Load from file
    if (!document->Load(filePath))
        return false;

    // Set document (required for framework)
    m_document = std::move(document);
    SetDocument(m_document.get());

    // Create framework (provides toolbar, modals)
    if (!m_framework)
        m_framework = std::make_unique<CanvasFramework>(m_document.get());

    // Create canvas
    m_canvas = std::make_unique<YourCanvas>(
        "YourGraphCanvas",
        ImVec2(0, 40),           // Position below toolbar
        ImVec2(800, 600),        // Default size
        m_document.get()
    );

    return true;
}

bool YourGraphRenderer::Save(const std::string& filePath)
{
    if (!m_document)
        return false;

    return m_document->Save(filePath);
}

void YourGraphRenderer::Render()
{
    RenderCommonToolbar();
    RenderGraphContent();
}

void YourGraphRenderer::RenderFrameworkModals()
{
    // Render framework modals (SaveAs dialog, etc.)
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->RenderModals();
    }

    // Render your custom modals
    m_canvas->RenderContextMenu();
}

void YourGraphRenderer::RenderCommonToolbar()
{
    // Render toolbar with buttons provided by framework
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->Render();
    }

    ImGui::Separator();
}

void YourGraphRenderer::RenderGraphContent()
{
    // ImGui layout: Toolbar at top, canvas filling rest
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    if (m_canvas)
    {
        m_canvas->Render();
    }
}
```

---

### 6.4 Step 4: Register with TabManager

**File**: `TabManager.cpp` (Modify existing detection logic)

```cpp
// In TabManager::DetectGraphType():
std::string TabManager::DetectGraphType(const std::string& filePath)
{
    // Check file extension
    if (filePath.ends_with(".your"))     // Your custom extension
        return "YourGraphType";
    
    if (filePath.ends_with(".bt"))
        return "BehaviorTree";
    
    if (filePath.ends_with(".vs"))
        return "VisualScript";
    
    if (filePath.ends_with(".ep"))
        return "EntityPrefab";

    return "Unknown";
}

// In TabManager::CreateRenderer():
IGraphRenderer* TabManager::CreateRenderer(const std::string& graphType)
{
    if (graphType == "YourGraphType")
        return new YourGraphRenderer("YourGraphEditor");
    
    if (graphType == "BehaviorTree")
        return new BehaviorTreeRenderer("BehaviorTreeEditor");
    
    if (graphType == "VisualScript")
        return new VisualScriptEditorPanel("VisualScriptEditor");
    
    if (graphType == "EntityPrefab")
        return new EntityPrefabRenderer("EntityPrefabEditor");

    return nullptr;  // Unknown type
}
```

---

### 6.5 Step 5: Build and Test

**Compile**:
```bash
# Build solution
msbuild OlympeBlueprintEditor.sln /p:Configuration=Debug

# Expected result: 0 errors, 0 warnings
```

**Test Manually**:

```
1. Create test file:
   └─ Create "TestGraph.your" with sample JSON:
      {
        "nodes": [
          {
            "nodeId": 1,
            "nodeType": "StartNode",
            "nodeName": "Start",
            "positionX": 100,
            "positionY": 100,
            "width": 80,
            "height": 60,
            "enabled": true
          }
        ],
        "connections": []
      }

2. Run editor:
   └─ Launch BlueprintEditorStandalone
   └─ Click File → Open
   └─ Select TestGraph.your
   └─ Verify:
      ✓ Tab appears: "TestGraph.your"
      ✓ Canvas renders with grid
      ✓ Node renders as box
      ✓ Toolbar shows [Save] [SaveAs] [Browse]
      ✓ Can drag node (moves on screen)
      ✓ Right-click node → context menu appears
      ✓ Click [Delete] → node removed, tab shows asterisk
      ✓ Click [Save] → file saved, asterisk removed

3. Verify interactions:
   ✓ Left-click node → highlight blue
   ✓ Hover node → glow yellow
   ✓ Drag → node follows mouse
   ✓ Right-click → context menu appears
   ✓ Delete key → selected node removed
   ✓ Ctrl+S → saves file
```

---

## 7. Real Example Walkthrough

### 7.1 PlaceholderCanvas - Complete Working Example

**Location**: `Source/BlueprintEditor/PlaceholderEditor/PlaceholderCanvas.h/cpp`

This is a production-ready example demonstrating ALL framework patterns.

**Key Features Demonstrated**:

```
✓ Async Input/Output Pattern (Phase 76)
  └─ HandleNodeInteraction() - input dispatch
  └─ RenderContextMenu() - async rendering

✓ State Separation
  └─ m_hoveredNodeId - hover state
  └─ m_selectedNodeId - selection state
  └─ m_contextNodeId - context menu state

✓ Coordinate Transformation
  └─ ScreenToCanvas() - mouse to graph coords
  └─ Correct formula: (screen - origin - offset) / zoom

✓ Hit Detection
  └─ AABB for nodes
  └─ Bezier sampling for connections (32 points, 10px tolerance)

✓ Visual Feedback
  └─ Hover: Yellow glow, thick border
  └─ Selection: Blue highlight
  └─ Context: Menu popup

✓ Framework Integration
  └─ Inherits from GraphEditorBase
  └─ Uses CustomCanvasEditor for pan/zoom
  └─ Uses CanvasGridRenderer for grid
  └─ Uses CanvasMinimapRenderer for minimap
```

**Study This File To**:
- Understand correct async pattern
- See coordinate transformation in action
- Learn hit detection implementation
- Copy state management pattern
- Understand frame cycle integration

---

## 8. Code Templates

### 8.1 Minimal Renderer Template

Copy-paste this and fill in `TODOs`:

```cpp
// File: YourGraphEditor/YourGraphRenderer.h
#pragma once
#include "../Framework/GraphEditorBase.h"

class YourGraphRenderer : public GraphEditorBase
{
public:
    YourGraphRenderer(const std::string& name = "YourEditor");

    std::string GetGraphType() const override { return "YOUR_TYPE"; }
    bool Load(const std::string& filePath) override;
    bool Save(const std::string& filePath) override;
    void Render() override;
    void RenderFrameworkModals() override;
};

// File: YourGraphEditor/YourGraphRenderer.cpp
#include "YourGraphRenderer.h"
#include "../Framework/CanvasFramework.h"

YourGraphRenderer::YourGraphRenderer(const std::string& name)
    : GraphEditorBase(name) {}

bool YourGraphRenderer::Load(const std::string& filePath)
{
    // TODO: Create and load your document
    // m_document = std::make_unique<YourDocument>();
    // if (!m_document->Load(filePath)) return false;

    SetDocument(m_document.get());

    if (!m_framework)
        m_framework = std::make_unique<CanvasFramework>(m_document.get());

    return true;
}

bool YourGraphRenderer::Save(const std::string& filePath)
{
    // TODO: Implement save
    // return m_document->Save(filePath);
    return false;
}

void YourGraphRenderer::Render()
{
    // TODO: Render toolbar
    if (m_framework && m_framework->GetToolbar())
        m_framework->GetToolbar()->Render();

    // TODO: Render your canvas/content
}

void YourGraphRenderer::RenderFrameworkModals()
{
    if (m_framework && m_framework->GetToolbar())
        m_framework->GetToolbar()->RenderModals();
}
```

---

### 8.2 Canvas with Hover Pattern Template

```cpp
// State members (add to your Canvas class)
int m_hoveredNodeId = -1;
int m_contextNodeId = -1;

// Input phase (call from Render(), before visual rendering)
void HandleNodeInteraction(ImVec2 mousePos)
{
    // Update hover
    m_hoveredNodeId = GetNodeAtScreenPos(mousePos);

    // Right-click dispatch (async - flag set here)
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        if (m_hoveredNodeId >= 0)
        {
            m_contextNodeId = m_hoveredNodeId;
            ImGui::OpenPopup("##context");  // ← Set flag here
        }
    }
}

// Render phase (call from RenderFrameworkModals() or equivalent)
void RenderContextMenu()
{
    // Render phase - ImGui processes flag set earlier
    if (ImGui::BeginPopup("##context"))  // ← Render here
    {
        if (ImGui::MenuItem("Delete"))
        {
            DeleteNode(m_contextNodeId);
        }
        ImGui::EndPopup();
    }
}

// Visual feedback (call from Render())
void RenderNodes()
{
    for (const auto& node : m_nodes)
    {
        ImU32 color = IM_COL32(100, 100, 100, 255);  // Normal

        if (node.id == m_hoveredNodeId)
            color = IM_COL32(255, 200, 0, 255);      // Hover yellow

        DrawNode(node, color);
    }
}
```

---

### 8.3 Async Frame Cycle Integration

```cpp
// In your main render function (called per frame):

void MyRenderer::Render()
{
    // PHASE 1: Content (input dispatch)
    HandleNodeInteraction(ImGui::GetMousePos());  // Input dispatch
    RenderToolbar();                              // Toolbar
    RenderCanvas();                               // Canvas + visual feedback
    
    // Phase 1 is complete - ImGui flags are set
}

// Called AFTER Render() completes:
void MyRenderer::RenderFrameworkModals()
{
    // PHASE 2: Modals (async rendering)
    RenderContextMenu();         // Popup flag processed here
    RenderPropertyPanel();       // Other modals
    
    // Framework modals
    if (m_framework && m_framework->GetToolbar())
        m_framework->GetToolbar()->RenderModals();
}

// In BlueprintEditorGUI (calls both phases in correct order):
void BlueprintEditorGUI::RenderFrame()
{
    // Phase 1: Content
    activeTab->renderer->Render();  // Input dispatch + rendering
    
    // Phase 2: Modals (MUST be after content)
    activeTab->renderer->RenderFrameworkModals();  // Async popups
}
```

---

## 9. Testing & Verification

### 9.1 Testing Checklist

```
Pre-Release Verification (Copy-Paste Ready):

COMPILATION
  ☐ Build with 0 errors
  ☐ Build with 0 warnings
  ☐ Clean rebuild succeeds
  ☐ No missing includes

FUNCTIONALITY
  ☐ File loads correctly (JSON parsed)
  ☐ Nodes render on canvas
  ☐ Grid renders behind nodes
  ☐ Can pan canvas (middle-mouse drag)
  ☐ Can zoom canvas (scroll wheel)

INTERACTION
  ☐ Left-click node → selects (blue highlight)
  ☐ Drag node → moves on canvas
  ☐ Right-click node → context menu appears
  ☐ Context menu item works (e.g., Delete)
  ☐ Delete key removes selected node
  ☐ Escape key closes popups

TOOLBAR
  ☐ [Save] button saves file
  ☐ [SaveAs] button shows file dialog
  ☐ [Browse] button opens file browser
  ☐ [Grid] checkbox toggles grid
  ☐ [Reset View] resets pan/zoom
  ☐ [Minimap] checkbox toggles minimap
  ☐ Ctrl+S saves file

DIRTY FLAG
  ☐ Modify graph → tab shows asterisk (*)
  ☐ Save file → asterisk disappears
  ☐ Close modified file → prompts to save
  ☐ Cancel close → tab stays open

FRAME CYCLE
  ☐ No ImGui warnings in console
  ☐ Hover feedback appears reliably
  ☐ Context menus appear on right-click
  ☐ Multiple popups stack correctly
  ☐ No modal stacking issues

FILE I/O
  ☐ Save → file written to disk
  ☐ Load → file read correctly
  ☐ Round-trip: Load → Modify → Save → Load preserves data
  ☐ Invalid file → graceful error (no crash)
  ☐ Missing file → error message shown
```

### 9.2 Automated Test Template

```cpp
// File: Tests/YourGraphRendererTests.cpp
#include <gtest/gtest.h>
#include "YourGraphRenderer.h"

class YourGraphRendererTest : public ::testing::Test
{
protected:
    YourGraphRenderer renderer;
};

TEST_F(YourGraphRendererTest, LoadValidFile)
{
    bool result = renderer.Load("path/to/valid/file.your");
    EXPECT_TRUE(result);
}

TEST_F(YourGraphRendererTest, SaveCreatesFile)
{
    renderer.Load("path/to/valid/file.your");
    bool result = renderer.Save("output.your");
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists("output.your"));
}

TEST_F(YourGraphRendererTest, RoundTripPreservesData)
{
    // Load
    renderer.Load("input.your");
    
    // Save to different location
    renderer.Save("output.your");
    
    // Load again
    YourGraphRenderer renderer2;
    renderer2.Load("output.your");
    
    // Verify nodes match
    EXPECT_EQ(renderer.GetNodes().size(), renderer2.GetNodes().size());
}
```

---

## 10. Troubleshooting Guide

### Issue: Build Fails with "Undefined reference to GraphEditorBase"

**Symptom**: Linker error, cannot find GraphEditorBase symbols

**Solution**:
```cpp
// Verify you have the correct include
#include "../Framework/GraphEditorBase.h"

// Verify GraphEditorBase.cpp is in project
// (add to OlympeBlueprintEditor.vcxproj if missing)

// Verify inheritance syntax
class YourRenderer : public GraphEditorBase  // ✓ Correct
{
    // NOT: class YourRenderer : GraphEditorBase (missing public)
};
```

---

### Issue: Context Menu Never Appears

**Symptom**: Right-click works but popup doesn't show

**Root Cause**: Mixing input and render phases (Phase 76 Pitfall #4)

**Solution**:
```cpp
// ❌ WRONG (same frame):
void Render()
{
    if (ImGui::IsMouseClicked(1))
        ImGui::OpenPopup("menu");  // Set flag
    if (ImGui::BeginPopup("menu"))  // Render flag
        ImGui::MenuItem("Delete");
}

// ✅ CORRECT (separate methods):
void HandleInput()  // Input phase
{
    if (ImGui::IsMouseClicked(1))
        ImGui::OpenPopup("menu");  // Set flag
}

void RenderFrameworkModals()  // Render phase
{
    if (ImGui::BeginPopup("menu"))  // Render flag
        ImGui::MenuItem("Delete");
}

// In main frame cycle:
void BlueprintEditorGUI::RenderFrame()
{
    activeTab->renderer->Render();          // Calls HandleInput()
    activeTab->renderer->RenderFrameworkModals();  // Calls RenderContextMenu()
}
```

---

### Issue: Node Dragging Misaligned at Different Zoom Levels

**Symptom**: Drag works at zoom 1.0x but breaks at 2.0x or 0.5x

**Root Cause**: Incorrect coordinate transformation formula

**Solution**:
```cpp
// ❌ WRONG (multiplies offset by zoom):
ImVec2 ScreenToCanvas(ImVec2 screenPos)
{
    return (screenPos - origin - offset * zoom) / zoom;
    //                          ^^^^^ WRONG!
}

// ✅ CORRECT (offset is zoom-independent):
ImVec2 ScreenToCanvas(ImVec2 screenPos)
{
    return (screenPos - origin - offset) / zoom;
    //                          ^^^^^ Correct (no multiply)
}

// Why:
// - offset is in screen pixels (pan amount in pixels)
// - zoom scales the COORDINATE distance, not the offset
// - Multiplying offset by zoom applies zoom twice
```

---

### Issue: Save Button Doesn't Work

**Symptom**: Click [Save], nothing happens, no error message

**Root Cause**: CanvasFramework not initialized

**Solution**:
```cpp
bool YourGraphRenderer::Load(const std::string& filePath)
{
    // Load document first
    m_document = std::make_unique<YourGraphDocument>();
    if (!m_document->Load(filePath))
        return false;

    // CRITICAL: Create framework BEFORE using it
    if (!m_framework)
        m_framework = std::make_unique<CanvasFramework>(m_document.get());
    // ↑ If this line is missing, toolbar won't work

    return true;
}

bool YourGraphRenderer::Save(const std::string& filePath)
{
    // CRITICAL: Must have valid document
    if (!m_document)
        return false;

    return m_document->Save(filePath);
}
```

---

### Issue: Nodes Don't Render

**Symptom**: Canvas shows grid but no nodes visible (Phase 52)

**Root Cause**: Canvas not initialized with document

**Solution**:
```cpp
// In your renderer Load():
m_canvas = std::make_unique<YourCanvas>(
    "CanvasName",
    ImVec2(0, 40),
    ImVec2(800, 600),
    m_document.get()  // ← Pass document pointer!
);
// If document pointer is null, rendering is skipped

// In RenderGraphContent():
if (m_canvas)
{
    m_canvas->Render();  // ← Must call every frame
}
```

---

### Issue: File Not Saving to Correct Location

**Symptom**: [SaveAs] saves to wrong path, or file not found after save

**Root Cause**: Relative path not resolved

**Solution**:
```cpp
bool YourGraphDocument::Save(const std::string& filePath) const
{
    // ❌ WRONG: Relative path might fail
    std::ofstream file(filePath);

    // ✅ CORRECT: Use absolute path
    std::filesystem::path absPath = std::filesystem::absolute(filePath);
    std::ofstream file(absPath.string());
    
    if (!file.is_open())
        return false;

    // ... write JSON ...
    file.close();
    return true;
}
```

---

### Issue: Memory Leak When Opening Many Files

**Symptom**: Memory usage grows, not released when closing tabs

**Root Cause**: Renderers not properly deleted, documents held in memory

**Solution**:
```cpp
// In TabManager, when closing tab:
void TabManager::CloseTab(const std::string& tabID)
{
    auto it = std::find_if(m_tabs.begin(), m_tabs.end(),
        [&tabID](const EditorTab& tab) { return tab.tabID == tabID; });
    
    if (it != m_tabs.end())
    {
        // Delete renderer (frees its document and canvas)
        if (it->renderer)
            delete it->renderer;  // ← Important!
        
        m_tabs.erase(it);
    }
}

// In your renderer destructor:
YourGraphRenderer::~YourGraphRenderer()
{
    // Unique pointers auto-delete
    // m_document.reset();  // Usually not needed, unique_ptr handles it
    // m_canvas.reset();
}
```

---

## Appendix A: Common Mistakes

### Mistake 1: Logging in Render Loop (Console Spam)

**Problem**: Console flooded with 60 FPS logs, can't see actual errors

**Example**:
```cpp
// ❌ WRONG:
void RenderNodes()
{
    SYSTEM_LOG << "Rendering nodes";  // 60x per second!
    for (const auto& node : m_nodes)
        DrawNode(node);
}

// ✅ CORRECT:
void OnNodeAdded(int nodeId)
{
    SYSTEM_LOG << "Node added: " << nodeId;  // 1x per action
    m_document->CreateNode(...);
}
```

---

### Mistake 2: Duplicate Function Definitions (LNK2005)

**Problem**: Build error "Symbol already defined"

**Example**:
```cpp
// ❌ WRONG (same method in two files):
// File 1: Canvas.cpp
void Canvas::HandleInput(ImVec2 pos) { /* impl */ }

// File 2: CanvasLogic.cpp
void Canvas::HandleInput(ImVec2 pos) { /* different impl */ }

// ✅ CORRECT:
// File: Canvas.h (declaration)
class Canvas {
    void HandleInput(ImVec2 pos);
};

// File: Canvas.cpp (ONE implementation)
void Canvas::HandleInput(ImVec2 pos) { /* impl */ }
```

---

### Mistake 3: Accessing Moved Objects (Undefined Behavior)

**Problem**: Code compiles but returns garbage

**Example**:
```cpp
// ❌ WRONG:
EditorTab tab = CreateTab();
m_tabs.push_back(std::move(tab));
return tab.tabID;  // ← Undefined! tab is now empty

// ✅ CORRECT:
EditorTab tab = CreateTab();
std::string tabID = tab.tabID;  // Save FIRST
m_tabs.push_back(std::move(tab));
return tabID;  // ← Defined
```

---

### Mistake 4: Forgetting to Initialize Framework

**Problem**: Save/SaveAs buttons missing

**Example**:
```cpp
// ❌ WRONG (missing framework creation):
bool Load(const std::string& filePath) {
    m_document->Load(filePath);
    return true;
    // m_framework never created - toolbar won't work!
}

// ✅ CORRECT:
bool Load(const std::string& filePath) {
    m_document->Load(filePath);
    
    if (!m_framework)
        m_framework = std::make_unique<CanvasFramework>(m_document.get());
    
    return true;
}
```

---

### Mistake 5: Not Implementing RenderFrameworkModals()

**Problem**: SaveAs modal never appears

**Example**:
```cpp
// ❌ WRONG (empty implementation):
void RenderFrameworkModals() override
{
    // Empty! Framework modal is never rendered
}

// ✅ CORRECT:
void RenderFrameworkModals() override
{
    if (m_framework && m_framework->GetToolbar())
        m_framework->GetToolbar()->RenderModals();
}
```

---

## Appendix B: Performance Optimization

### Avoid Creating Objects in Render Loop

```cpp
// ❌ WRONG (memory leak + frame drops):
void RenderNodes()
{
    for (const auto& node : m_nodes)
    {
        auto renderer = std::make_unique<NodeRenderer>();  // ← 60 allocations/sec
        renderer->Render(node);
    }
}

// ✅ CORRECT (reuse renderer):
void Render()
{
    // Create once in Initialize()
    if (!m_nodeRenderer)
        m_nodeRenderer = std::make_unique<NodeRenderer>();

    // Reuse in render loop
    for (const auto& node : m_nodes)
        m_nodeRenderer->Render(node);
}
```

---

### Cache Complex Calculations

```cpp
// ❌ WRONG (recalculate every frame):
void RenderNodes()
{
    for (const auto& node : m_nodes)
    {
        ImVec2 screenPos = ScreenToCanvas(node.canvasPos);  // 60x per frame
        DrawNode(screenPos);
    }
}

// ✅ CORRECT (cache when needed):
void Render()
{
    // Update cache only when needed (pan/zoom changed)
    if (m_viewportChanged)
        UpdateScreenPositionCache();

    for (const auto& node : m_nodes)
        DrawNode(m_cachedScreenPositions[node.id]);
}
```

---

### Use Lazy Initialization

```cpp
// ✅ GOOD (create only when needed):
void RenderPropertyPanel()
{
    if (!m_propertyEditor)
        m_propertyEditor = std::make_unique<PropertyEditor>();

    if (m_propertyEditor->IsVisible())
        m_propertyEditor->Render();
}
```

---

### Limit Bezier Sampling for Hit Detection

```cpp
// ❌ WRONG (expensive every frame):
void RenderConnections()
{
    for (const auto& conn : connections)
    {
        float dist = GetDistanceToConnection(conn, mouse);  // Expensive!
        // Checks distance EVERY CONNECTION EVERY FRAME
    }
}

// ✅ CORRECT (only when needed):
void HandleNodeInteraction(ImVec2 mousePos)
{
    if (!ImGui::IsMouseClicked())
        return;  // Skip expensive check if not clicking

    // Only compute hit detection on click
    for (const auto& conn : connections)
    {
        float dist = GetDistanceToConnection(conn, mousePos);
        if (dist < 10.0f)
        {
            m_selectedConnection = conn.id;
            break;
        }
    }
}
```

---

## Final Checklist - Ready to Implement?

Before you start, verify you have:

```
✓ Read Sections 1-3 (understand what/why)
✓ Understood Section 5 (data flows)
✓ Reviewed Section 6.1-6.2 (document and canvas)
✓ Reviewed Section 7 (PlaceholderCanvas example)
✓ Copied templates from Section 8
✓ Bookmarked Section 9 (testing)
✓ Bookmarked Section 10 (troubleshooting)

Development Time Estimate:
  6-8 hours: Following this guide + PlaceholderCanvas reference
  Typical problems: 2-3 resolved by Section 10 checklist
  Build time: 5-10 minutes
  First test: 30 minutes

Success Metric:
  File loads → renders on canvas → can interact → saves correctly
  All checked in 30 minutes? Implementation complete.
```

---

**Good luck! The framework has proven architecture through 76 phases. Follow the patterns, reference PlaceholderCanvas, and you'll have a production-ready graph type in a day.**

**Need help? Reference FRAMEWORK_BEST_PRACTICES.md for pitfalls and FRAMEWORK_INTEGRATION_GUIDE.md for additional patterns.**

