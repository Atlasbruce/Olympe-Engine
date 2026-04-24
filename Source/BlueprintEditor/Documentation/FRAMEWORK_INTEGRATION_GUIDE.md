# Framework Integration Guide: Creating New Graph Types for Blueprint Editor

**Author:** Copilot  
**Date:** 2026-04-08  
**Status:** ✅ Phase 75 - Complete  
**Audience:** Blueprint Editor developers implementing new graph editor types

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Foundation](#architecture-foundation)
3. [Step-by-Step Integration](#step-by-step-integration)
4. [Reference Implementations](#reference-implementations)
5. [Common Patterns](#common-patterns)
6. [Testing & Verification](#testing--verification)
7. [Troubleshooting](#troubleshooting)

---

## Overview

The Blueprint Editor Framework provides a **unified system** for building graph-based editors with consistent UI, interaction patterns, and save/load functionality. This guide walks you through integrating a new graph type (like PlaceholderGraphRenderer, EntityPrefabRenderer, or BehaviorTreeRenderer) into the framework **end-to-end**.

### Key Principles

1. **Don't Reinvent**: Study existing implementations (VisualScript, EntityPrefab, BehaviorTree) BEFORE coding
2. **Separation of Concerns**: Data model separate from rendering separate from UI
3. **Pattern Reuse**: Toolbar, modals, tabbed layouts, minimap - all provided by framework
4. **Pragmatism Over Perfection**: Simple integration beats perfect architecture
5. **Incremental Development**: Build layer-by-layer (model → canvas → toolbar → save)

---

## Architecture Foundation

### Core Classes (Responsibility Model)

```
┌─────────────────────────────────────────────────────────┐
│           IGraphRenderer (Interface)                     │
│  - Load/Save contract                                   │
│  - Render pipeline entry point                          │
│  - Type identification                                  │
└─────────────────────────────────────────────────────────┘
                        ▲
                        │ (inherits)
                        │
┌─────────────────────────────────────────────────────────┐
│         GraphEditorBase (Abstract Base)                  │
│  - Selection management (m_selectedNodeIds)             │
│  - Dirty flag tracking                                  │
│  - Framework integration (m_framework, m_document)      │
│  - Common utilities (MarkDirty, GetDocument)            │
└─────────────────────────────────────────────────────────┘
                        ▲
                        │ (inherits)
                        │
┌─────────────────────────────────────────────────────────┐
│   YourGraphRenderer (Concrete Implementation)           │
│  - Load/Save (YourGraphDocument)                        │
│  - Render (Canvas + Toolbar + Properties)               │
│  - Interaction (selection, dragging, context menus)     │
└─────────────────────────────────────────────────────────┘
```

### Supporting Classes

| Class | Purpose | Responsibility |
|-------|---------|-----------------|
| `YourGraphDocument` | Data Model | Store nodes, connections, properties; serialize/deserialize |
| `YourCanvas` | ImGui Container | Render nodes, handle mouse/keyboard input, manage viewport |
| `YourNodeRenderer` | Node Drawing | Render individual nodes and connections with styling |
| `CanvasFramework` | Toolbar & Modals | Save/SaveAs/Browse buttons, keyboard shortcuts, modal dialogs |
| `YourPropertyEditor` | Properties Panel | Edit node properties, batch operations, Apply/Reset |

---

## Step-by-Step Integration

### Step 1: Create Data Model Class

**Purpose**: Define your graph's data structure (nodes, connections, properties)

**File Structure**:
```
Source/BlueprintEditor/YourEditor/
├── YourGraphDocument.h
├── YourGraphDocument.cpp
├── YourGraphRenderer.h
└── YourGraphRenderer.cpp
```

**Minimal YourGraphDocument**:

```cpp
// YourGraphDocument.h
class YourGraphDocument
{
public:
    // Core node operations
    YourNode* CreateNode(int x, int y);
    void DeleteNode(int nodeId);
    void UpdateNode(int nodeId, const YourNode& data);
    YourNode* GetNode(int nodeId);
    
    // Connection operations
    void CreateConnection(int fromId, int toId);
    void DeleteConnection(int connId);
    
    // Serialization
    bool Load(const std::string& filePath);
    bool Save(const std::string& filePath);
    
    // Accessors
    const std::vector<YourNode>& GetAllNodes() const;
    const std::vector<YourConnection>& GetAllConnections() const;
    
    // Dirty tracking (for UI feedback)
    void SetDirty(bool dirty);
    bool IsDirty() const;
    
    // File path
    std::string GetFilePath() const;
    void SetFilePath(const std::string& path);
    
private:
    std::vector<YourNode> m_nodes;
    std::vector<YourConnection> m_connections;
    int m_nextNodeId = 1;
    bool m_isDirty = false;
    std::string m_filePath;
};
```

**Load/Save Pattern** (JSON):

```cpp
bool YourGraphDocument::Load(const std::string& filePath)
{
    // 1. Load JSON file
    std::ifstream file(filePath);
    if (!file) return false;
    
    nlohmann::json j;
    file >> j;
    
    // 2. Parse schema (version-aware for future changes)
    int version = j["version"].get<int>();
    
    // 3. Deserialize nodes
    for (const auto& nodeJson : j["nodes"])
    {
        YourNode node;
        node.nodeId = nodeJson["id"];
        node.type = nodeJson["type"];
        node.posX = nodeJson["x"];
        node.posY = nodeJson["y"];
        // ... other properties
        m_nodes.push_back(node);
    }
    
    // 4. Deserialize connections
    for (const auto& connJson : j["connections"])
    {
        YourConnection conn;
        conn.connId = connJson["id"];
        conn.fromNodeId = connJson["from"];
        conn.toNodeId = connJson["to"];
        m_connections.push_back(conn);
    }
    
    m_filePath = filePath;
    m_isDirty = false;
    return true;
}

bool YourGraphDocument::Save(const std::string& filePath)
{
    nlohmann::json j;
    j["version"] = 1;
    
    // Serialize nodes
    auto& nodesArray = j["nodes"] = nlohmann::json::array();
    for (const auto& node : m_nodes)
    {
        nlohmann::json nodeJson;
        nodeJson["id"] = node.nodeId;
        nodeJson["type"] = node.type;
        nodeJson["x"] = node.posX;
        nodeJson["y"] = node.posY;
        // ... other properties
        nodesArray.push_back(nodeJson);
    }
    
    // Serialize connections
    auto& connsArray = j["connections"] = nlohmann::json::array();
    for (const auto& conn : m_connections)
    {
        nlohmann::json connJson;
        connJson["id"] = conn.connId;
        connJson["from"] = conn.fromNodeId;
        connJson["to"] = conn.toNodeId;
        connsArray.push_back(connJson);
    }
    
    // Write to file
    std::ofstream file(filePath);
    file << j.dump(2);
    
    m_filePath = filePath;
    m_isDirty = false;
    return true;
}
```

### Step 2: Create Canvas Class

**Purpose**: ImGui rendering container for your graph

**Pattern**:

```cpp
// YourCanvas.h
class YourCanvas
{
public:
    void Initialize(YourGraphDocument* document);
    void Render();
    void SetRenderer(YourGraphRenderer* renderer);
    
    // Viewport controls
    void ResetPanZoom();
    void SetGridVisible(bool visible);
    void SetMinimapVisible(bool visible);
    
    // Input handling
    void OnMouseMove(ImVec2 pos);
    void OnMouseDown(ImVec2 pos);
    void OnMouseUp(ImVec2 pos);
    void OnMouseScroll(float delta);
    void OnKeyDown(int key);
    
    // Selection
    int GetSelectedNodeId() const;
    void SetSelectedNodeId(int id);
    
    // Coordinate transformation
    ImVec2 ScreenToCanvas(ImVec2 screenPos);
    ImVec2 CanvasToScreen(ImVec2 canvasPos);
    
    // State accessors
    float GetCanvasZoom() const;
    ImVec2 GetCanvasOffset() const;
    
private:
    YourGraphDocument* m_document = nullptr;
    YourGraphRenderer* m_renderer = nullptr;
    
    // Viewport
    float m_canvasZoom = 1.0f;
    ImVec2 m_canvasOffset = {0, 0};
    
    // Selection
    int m_selectedNodeId = -1;
    
    // Rendering
    std::unique_ptr<YourNodeRenderer> m_nodeRenderer;
    bool m_gridVisible = true;
    bool m_minimapVisible = true;
};
```

**Rendering Loop Pattern**:

```cpp
void YourCanvas::Render()
{
    if (!m_document || !m_nodeRenderer) return;
    
    ImGui::BeginChild("YourCanvasChild", ImVec2(-1, -1), true);
    
    // 1. Render grid (background)
    RenderGrid();
    
    // 2. Handle input
    if (ImGui::IsWindowFocused())
    {
        HandlePanZoom();
        HandleNodeInteraction();
    }
    
    // 3. Render nodes and connections
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (const auto& node : m_document->GetAllNodes())
    {
        m_nodeRenderer->RenderNode(drawList, node, node.nodeId == m_selectedNodeId);
    }
    
    for (const auto& conn : m_document->GetAllConnections())
    {
        m_nodeRenderer->RenderConnection(drawList, conn);
    }
    
    // 4. Render minimap (optional, can use framework's CanvasMinimapRenderer)
    if (m_minimapVisible && m_canvas && m_canvas->GetCanvasEditor())
    {
        m_minimap.RenderMinimap();
    }
    
    ImGui::EndChild();
}
```

### Step 3: Create Renderer Class

**Purpose**: Main interface between TabManager and your graph editor

**Inherit from GraphEditorBase**:

```cpp
// YourGraphRenderer.h
class YourGraphRenderer : public GraphEditorBase
{
public:
    YourGraphRenderer();
    ~YourGraphRenderer();
    
    // IGraphRenderer interface (required)
    std::string GetGraphType() const override;  // Return "YourGraphType"
    bool Load(const std::string& filePath) override;
    bool Save(const std::string& filePath) override;
    
    // Rendering entry point (called by TabManager/BlueprintEditorGUI)
    void Render() override;
    
    // Framework integration (inherited from GraphEditorBase, implement these)
    void InitializeCanvasEditor() override;
    void RenderGraphContent() override;
    void RenderFrameworkModals() override;
    void RenderTypeSpecificToolbar() override;
    
private:
    // Data
    std::unique_ptr<YourGraphDocument> m_document;
    std::unique_ptr<YourCanvas> m_canvas;
    
    // Framework integration
    std::unique_ptr<CanvasFramework> m_framework;
    std::unique_ptr<YourPropertyEditorPanel> m_propertyEditor;
    
    // UI state
    std::vector<int> m_selectedNodeIds;
    bool m_gridVisible = true;
    bool m_minimapVisible = true;
};
```

**Load Method Pattern**:

```cpp
bool YourGraphRenderer::Load(const std::string& filePath)
{
    SYSTEM_LOG << "[YourGraphRenderer] Loading: " << filePath << "\n";
    
    // Phase 52 Pattern: Initialize must create ALL dependencies
    if (!m_document)
        m_document = std::make_unique<YourGraphDocument>();
    
    // Load from file
    if (!m_document->Load(filePath))
        return false;
    
    // Create canvas
    if (!m_canvas)
    {
        m_canvas = std::make_unique<YourCanvas>();
        m_canvas->Initialize(m_document.get());
        m_canvas->SetRenderer(this);
    }
    
    // Create framework for unified toolbar
    if (!m_framework)
        m_framework = std::make_unique<CanvasFramework>(m_document.get());
    
    // Create property editor
    if (!m_propertyEditor)
    {
        m_propertyEditor = std::make_unique<YourPropertyEditorPanel>();
        m_propertyEditor->Initialize(m_document.get());
    }
    
    // Set document in base class for framework access
    SetDocument((void*)m_document.get());
    
    SYSTEM_LOG << "[YourGraphRenderer] Loaded successfully\n";
    return true;
}
```

### Step 4: Implement Rendering Layout

**Pattern**: Three-column layout (Canvas + Palette + Properties)

```cpp
void YourGraphRenderer::RenderGraphContent()
{
    if (!m_canvas) return;
    
    // ---- TOOLBAR ----
    RenderCommonToolbar();        // Framework toolbar (Save, SaveAs, Browse)
    ImGui::Separator();
    RenderTypeSpecificToolbar();  // Your custom buttons
    ImGui::Separator();
    
    // ---- THREE-COLUMN LAYOUT ----
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float totalHeight = ImGui::GetContentRegionAvail().y;
    
    // Clamp properties panel width
    if (m_propertiesPanelWidth < 200.0f) m_propertiesPanelWidth = 200.0f;
    if (m_propertiesPanelWidth > totalWidth * 0.60f) m_propertiesPanelWidth = totalWidth * 0.60f;
    
    float handleWidth = 6.0f;
    float canvasAreaWidth = totalWidth - m_propertiesPanelWidth - handleWidth;
    
    // LEFT: Canvas + Palette
    ImGui::BeginChild("YourCanvasArea", ImVec2(canvasAreaWidth, 0), false);
    m_canvas->Render();
    ImGui::EndChild();
    
    // CENTER: Resize handle
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::Button("##resize", ImVec2(handleWidth, -1.0f));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_propertiesPanelWidth -= ImGui::GetIO().MouseDelta.x;
    }
    ImGui::PopStyleColor();
    
    // RIGHT: Properties Panel with Tabs
    ImGui::SameLine();
    ImGui::BeginChild("YourRightPanel", ImVec2(m_propertiesPanelWidth, 0), true);
    RenderRightPanelTabs();
    ImGui::EndChild();
}

void YourGraphRenderer::RenderRightPanelTabs()
{
    if (ImGui::BeginTabBar("RightPanelTabs"))
    {
        // Tab 0: Node Palette (drag-drop component list)
        if (ImGui::BeginTabItem("Components"))
        {
            if (!m_palette)
                m_palette = std::make_unique<YourNodePalette>();
            m_palette->Render();
            ImGui::EndTabItem();
        }
        
        // Tab 1: Properties (node editing)
        if (ImGui::BeginTabItem("Properties"))
        {
            if (m_propertyEditor)
            {
                m_propertyEditor->Render(m_selectedNodeIds);
                if (m_propertyEditor->HasChanges())
                {
                    ImGui::Separator();
                    if (ImGui::Button("Apply", ImVec2(80, 0)))
                    {
                        m_propertyEditor->ApplyChanges();
                        MarkDirty();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset", ImVec2(80, 0)))
                    {
                        m_propertyEditor->ResetChanges();
                    }
                }
            }
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}
```

### Step 5: Framework Integration

**Purpose**: Connect your renderer to the framework (toolbar, modals, save/load)

**Pattern**:

```cpp
// Your renderer must override these methods

void YourGraphRenderer::InitializeCanvasEditor()
{
    // Called once during initialization
    if (!m_document)
        m_document = std::make_unique<YourGraphDocument>();
    if (!m_canvas)
    {
        m_canvas = std::make_unique<YourCanvas>();
        m_canvas->Initialize(m_document.get());
    }
    if (!m_framework)
        m_framework = std::make_unique<CanvasFramework>(m_document.get());
    if (!m_propertyEditor)
    {
        m_propertyEditor = std::make_unique<YourPropertyEditorPanel>();
        m_propertyEditor->Initialize(m_document.get());
    }
}

void YourGraphRenderer::RenderCommonToolbar()
{
    // Framework toolbar (already implemented by base class)
    // Renders: [Save] [SaveAs] [Browse] buttons from CanvasFramework
    // Also: Grid toggle, Reset View, Minimap toggle
    
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->Render();
        ImGui::SameLine(0.0f, 20.0f);
        ImGui::Separator();
        ImGui::SameLine(0.0f, 20.0f);
    }
    
    // Common controls
    if (ImGui::Checkbox("Grid##toolbar", &m_gridVisible))
    {
        if (m_canvas) m_canvas->SetGridVisible(m_gridVisible);
    }
    ImGui::SameLine(0.0f, 10.0f);
    
    if (ImGui::Button("Reset View##btn", ImVec2(80, 0)))
    {
        if (m_canvas) m_canvas->ResetPanZoom();
    }
    ImGui::SameLine(0.0f, 10.0f);
    
    if (ImGui::Checkbox("Minimap##toolbar", &m_minimapVisible))
    {
        if (m_canvas) m_canvas->SetMinimapVisible(m_minimapVisible);
    }
}

void YourGraphRenderer::RenderFrameworkModals()
{
    // Phase 45 Pattern: Modal rendering must happen AFTER content in frame cycle
    // This dispatcher ensures modals from framework toolbar appear at correct time
    
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->RenderModals();
    }
}

void YourGraphRenderer::RenderTypeSpecificToolbar()
{
    // Add your custom buttons here
    // Example: Verify button, Run Graph button, Custom operations
}
```

### Step 6: Register Your Renderer

**Purpose**: Make TabManager aware of your graph type

**In TabManager.cpp**:

```cpp
std::unique_ptr<IGraphRenderer> TabManager::CreateRendererForFile(
    const std::string& filePath)
{
    // Detect file type from extension or content
    std::string type = DetectGraphType(filePath);
    
    if (type == "YourGraphType")
    {
        auto renderer = std::make_unique<YourGraphRenderer>();
        if (renderer->Load(filePath))
            return renderer;
    }
    
    // ... other types (VisualScript, BehaviorTree, EntityPrefab)
    
    return nullptr;
}
```

### Step 7: Testing & Verification

**Checklist**:

```
□ Model loads/saves JSON correctly
□ Canvas renders nodes and connections
□ Selection works (click node → highlight)
□ Toolbar buttons functional (Save, SaveAs, Browse)
□ Properties panel updates on node selection
□ Dirty flag set on modifications
□ Context menus work (right-click → delete)
□ Minimap renders and scales correctly
□ Build: 0 errors, 0 warnings
```

---

## Reference Implementations

### Full Examples Available In Codebase

| Implementation | Location | Features |
|-----------------|----------|----------|
| **VisualScript** | `Source/BlueprintEditor/VisualScriptEditorPanel*` | Complete reference, 3000+ lines, all framework features |
| **EntityPrefab** | `Source/BlueprintEditor/EntityPrefabEditor/` | Modern simplified pattern, drag-drop components, dynamic palette |
| **BehaviorTree** | `Source/BlueprintEditor/BehaviorTreeEditor/` | Tree-specific layout, node properties panel |
| **Placeholder** | `Source/BlueprintEditor/PlaceholderEditor/` | Minimal reference, Phase 74 features (verify, run graph) |

### Study Order (Recommended)

1. **Start with Placeholder** (simplest, newest patterns)
2. **Then EntityPrefab** (modern patterns, dynamic loading)
3. **Then BehaviorTree** (tree-specific features)
4. **Finally VisualScript** (complete reference, all advanced features)

---

## Common Patterns

### Pattern 1: Node Selection Synchronization

**Problem**: Canvas has single selection, but renderer needs vector of selected nodes

**Solution**:

```cpp
// In renderer:
std::vector<int> m_selectedNodeIds;

void YourGraphRenderer::SyncCanvasSelectionWithBase()
{
    if (m_canvas)
    {
        int canvasSelected = m_canvas->GetSelectedNodeId();
        if (canvasSelected >= 0)
        {
            m_selectedNodeIds.clear();
            m_selectedNodeIds.push_back(canvasSelected);
        }
    }
}
```

### Pattern 2: Coordinate Transformation

**Problem**: Mouse positions in screen space, need canvas space for hit detection

**Solution**:

```cpp
// In canvas:
ImVec2 ScreenToCanvas(ImVec2 screenPos)
{
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    // Convert to canvas-relative, then apply pan/zoom
    return (screenPos - canvasPos - m_canvasOffset) / m_canvasZoom;
}

// In node renderer:
bool IsPointInNode(ImVec2 point, const YourNode& node)
{
    return point.x >= node.posX && point.x < node.posX + node.width &&
           point.y >= node.posY && point.y < node.posY + node.height;
}
```

### Pattern 3: Dirty Flag Tracking

**Problem**: Need to know if document modified (for UI feedback, save prompt)

**Solution**:

```cpp
// In document:
void YourGraphDocument::SetDirty(bool dirty)
{
    m_isDirty = dirty;
    SYSTEM_LOG << "[YourGraphDocument] Dirty flag set to: " << dirty << "\n";
}

// In renderer (after any modification):
void YourGraphRenderer::OnNodeDeleted(int nodeId)
{
    m_document->DeleteNode(nodeId);
    MarkDirty();  // Inherited from GraphEditorBase
}

// Base class checks IsDirty() for:
// - Tab title asterisk (marks unsaved)
// - Save modal (prompt before exit)
// - Framework toolbar state
```

### Pattern 4: Context Menus

**Problem**: Right-click on node should show menu with Delete, Edit, etc.

**Solution** (from VisualScript):

```cpp
// In canvas input handling:
if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && IsNodeAtMouse())
{
    m_renderer->m_contextNodeID = hoveredNodeId;
    ImGui::OpenPopup("YourNodeContextMenu");
}

// In renderer rendering:
void YourGraphRenderer::RenderContextMenus()
{
    if (ImGui::BeginPopup("YourNodeContextMenu"))
    {
        if (ImGui::MenuItem("Delete Node"))
        {
            m_document->DeleteNode(m_contextNodeID);
            MarkDirty();
        }
        if (ImGui::MenuItem("Edit Properties"))
        {
            m_selectedNodeIds.clear();
            m_selectedNodeIds.push_back(m_contextNodeID);
        }
        ImGui::EndPopup();
    }
}
```

### Pattern 5: Drag-Drop Node Creation from Palette

**Problem**: Drag component from palette, drop on canvas to create node

**Solution**:

```cpp
// In palette rendering:
ImGui::SetDragDropSource();
ImGui::SetDragDropPayload("YOUR_NODE_TYPE", &nodeTypeValue, sizeof(int));
ImGui::EndDragDropSource();

// In canvas drag-drop target:
if (ImGui::BeginDragDropTarget())
{
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("YOUR_NODE_TYPE"))
    {
        int nodeType = *static_cast<const int*>(payload->Data);
        ImVec2 dropPos = ImGui::GetMousePos();
        ImVec2 canvasPos = ScreenToCanvas(dropPos);
        m_document->CreateNode(nodeType, canvasPos.x, canvasPos.y);
        MarkDirty();
    }
    ImGui::EndDragDropTarget();
}
```

### Pattern 6: Save/SaveAs Integration

**Problem**: Framework toolbar calls ExecuteSave, need to implement it

**Solution**:

```cpp
// Your renderer inherits from GraphEditorBase which provides:
// - m_framework (CanvasFramework with toolbar)
// - m_document (your graph document)
// - MarkDirty() method

// Implement ExecuteSave (can be inline or override):
bool YourGraphRenderer::ExecuteSave(const std::string& filePath)
{
    if (!m_document)
    {
        SYSTEM_LOG << "[YourGraphRenderer] ERROR: No document to save\n";
        return false;
    }

    m_document->SetFilePath(filePath);
    bool success = m_document->Save(filePath);

    if (success)
    {
        SYSTEM_LOG << "[YourGraphRenderer] ✓ Saved to: " << filePath << "\n";
        m_document->SetDirty(false);
    }
    else
    {
        SYSTEM_LOG << "[YourGraphRenderer] ✗ Save failed!\n";
    }

    return success;
}
```

### Pattern 7: Node Hover & Context Menu (Phase 76 Framework Feature)

**Problem**: Need visual feedback when mouse over node, and context menu on right-click

**Architecture** (Async Input/Output Pattern):
- Input phase: Detect hover + dispatch right-click
- Render phase: Render context menus from stored state
- Why separate: ImGui flag management requires correct frame timing

**Solution**:

**Step 1: Add State Members to Canvas**

```cpp
// YourCanvas.h
class YourCanvas
{
private:
    // Hover & context menu state
    int m_hoveredNodeId = -1;              // Current hover target
    int m_hoveredConnectionId = -1;        // Connection hover detection
    int m_contextNodeId = -1;              // Right-clicked node
    int m_contextConnectionId = -1;        // Right-clicked connection

    // Detection methods
    int GetNodeAtScreenPos(ImVec2 screenPos);  // Hit detection
    int GetConnectionAtScreenPos(ImVec2 screenPos);  // Bezier curve sampling
    float GetDistanceToConnection(const YourConnection& conn, ImVec2 screenPos);
};
```

**Step 2: Input Phase - Detect Hover & Dispatch Right-Click**

```cpp
// YourCanvas.cpp - Called in HandleNodeInteraction() / input handling phase
void YourCanvas::HandleMouseInput(ImVec2 mouseScreenPos)
{
    // Update hover state every frame
    m_hoveredNodeId = GetNodeAtScreenPos(mouseScreenPos);
    m_hoveredConnectionId = GetConnectionAtScreenPos(mouseScreenPos);

    // Async right-click dispatch (input phase only)
    // CRITICAL: Input detection MUST happen in input phase, not render phase
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        if (m_hoveredNodeId >= 0)
        {
            m_contextNodeId = m_hoveredNodeId;
            ImGui::OpenPopup("##canvas_node_context");
        }
        else if (m_hoveredConnectionId >= 0)
        {
            m_contextConnectionId = m_hoveredConnectionId;
            ImGui::OpenPopup("##canvas_connection_context");
        }
    }
}
```

**Step 3: Render Phase - Draw Context Menus**

```cpp
// YourCanvas.cpp - Called in Render() / render phase AFTER content
void YourCanvas::RenderContextMenus()
{
    // Node context menu (rendered async, no input detection here)
    if (ImGui::BeginPopup("##canvas_node_context"))
    {
        if (ImGui::MenuItem("Delete Node"))
        {
            m_document->DeleteNode(m_contextNodeId);
            m_contextNodeId = -1;
            MarkDirty();
        }
        if (ImGui::MenuItem("Select All"))
        {
            // Your selection logic
        }
        ImGui::EndPopup();
    }

    // Connection context menu
    if (ImGui::BeginPopup("##canvas_connection_context"))
    {
        if (ImGui::MenuItem("Delete Connection"))
        {
            m_document->DeleteConnection(m_contextConnectionId);
            m_contextConnectionId = -1;
            MarkDirty();
        }
        ImGui::EndPopup();
    }
}
```

**Step 4: Visual Feedback - Hover Glow & Thickness**

```cpp
// YourNodeRenderer.cpp - Render phase visual feedback
void YourNodeRenderer::RenderNode(ImDrawList* drawList, const YourNode& node)
{
    bool isHovered = (m_canvas->m_hoveredNodeId == node.nodeId);
    bool isSelected = (m_canvas->m_selectedNodeId == node.nodeId);

    // Color priority: Selected > Hovered > Normal
    ImU32 borderColor = isSelected ? IM_COL32(0, 255, 0, 255) :  // Green
                        isHovered ? IM_COL32(255, 200, 0, 255) :  // Yellow
                        IM_COL32(100, 100, 100, 255);              // Gray

    float borderWidth = isSelected ? 2.5f :
                        isHovered ? 2.5f :
                        1.5f;

    // Draw node box with glow
    drawList->AddRect(
        ImVec2(node.screenX, node.screenY),
        ImVec2(node.screenX + node.width, node.screenY + node.height),
        borderColor, 0.0f, ImDrawCornerFlags_All, borderWidth
    );

    // ... rest of node rendering
}

void YourNodeRenderer::RenderConnection(ImDrawList* drawList, const YourConnection& conn)
{
    bool isHovered = (m_canvas->m_hoveredConnectionId == conn.connId);

    // Thickness priority: Selected > Hovered > Normal
    float thickness = isHovered ? 3.0f : 2.0f;
    ImU32 color = isHovered ? IM_COL32(255, 200, 0, 255) :  // Yellow
                  IM_COL32(150, 150, 150, 255);              // Gray

    // Draw Bezier curve
    drawList->AddBezierCubic(
        fromScreenPos, controlPoint1, controlPoint2, toScreenPos,
        color, thickness, bezierSegments
    );
}
```

**Step 5: Hit Detection - AABB for Nodes, Bezier Sampling for Connections**

```cpp
// YourCanvas.cpp - Hit detection methods
int YourCanvas::GetNodeAtScreenPos(ImVec2 screenPos)
{
    for (const auto& node : m_document->GetAllNodes())
    {
        // AABB intersection test
        if (screenPos.x >= node.screenX && screenPos.x < node.screenX + node.width &&
            screenPos.y >= node.screenY && screenPos.y < node.screenY + node.height)
        {
            return node.nodeId;
        }
    }
    return -1;
}

int YourCanvas::GetConnectionAtScreenPos(ImVec2 screenPos)
{
    const float HIT_TOLERANCE = 10.0f;  // 10 pixel tolerance for UX

    for (const auto& conn : m_document->GetAllConnections())
    {
        float distance = GetDistanceToConnection(conn, screenPos);
        if (distance <= HIT_TOLERANCE)
            return conn.connId;
    }
    return -1;
}

float YourCanvas::GetDistanceToConnection(const YourConnection& conn, ImVec2 screenPos)
{
    // Bezier curve sampling: 32 points for good accuracy
    const int BEZIER_SAMPLES = 32;
    float minDistance = std::numeric_limits<float>::max();

    ImVec2 p1 = GetNodePortScreenPos(conn.fromNodeId, true);   // From node
    ImVec2 p2 = GetNodePortScreenPos(conn.toNodeId, false);    // To node

    // Control points (40% of horizontal distance)
    float ctrlOffset = (p2.x - p1.x) * 0.4f;
    ImVec2 cp1 = {p1.x + ctrlOffset, p1.y};
    ImVec2 cp2 = {p2.x - ctrlOffset, p2.y};

    // Sample Bezier curve at 32 points
    for (int i = 0; i <= BEZIER_SAMPLES; ++i)
    {
        float t = static_cast<float>(i) / BEZIER_SAMPLES;

        // Cubic Bezier formula: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
        float u = 1.0f - t;
        float u2 = u * u;
        float u3 = u2 * u;
        float t2 = t * t;
        float t3 = t2 * t;

        ImVec2 point = {
            u3 * p1.x + 3 * u2 * t * cp1.x + 3 * u * t2 * cp2.x + t3 * p2.x,
            u3 * p1.y + 3 * u2 * t * cp1.y + 3 * u * t2 * cp2.y + t3 * p2.y
        };

        float dx = screenPos.x - point.x;
        float dy = screenPos.y - point.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        minDistance = std::min(minDistance, distance);
    }

    return minDistance;
}
```

**Key Benefits**:
- ✅ Consistent across all graph types (framework feature)
- ✅ Reliable ImGui integration (async input/output pattern)
- ✅ Smooth user experience (hover visual feedback)
- ✅ Connection hit detection working (Bezier sampling)
- ✅ Right-click context menus functional
- ✅ Proven in PlaceholderGraphRenderer (Phase 76)

**Implementation Reference**: See `Source/BlueprintEditor/PlaceholderEditor/PlaceholderCanvas.cpp` (Phase 76 implementation) for complete working example.

---

## Testing & Verification

### Build Verification

```powershell
# Clean build to catch missing includes
cmake --build . --clean-first --config Debug

# Expected: 0 errors, 0 warnings
# If LNK2005 errors: Check for duplicate function definitions
# If CS#### errors: Check for missing #include directives
```

### Runtime Testing

**Test 1: File Loading**
```cpp
1. Right-click your graph file in file browser
2. Select "Open with Blueprint Editor"
3. Verify: Tab appears with graph loaded
4. Verify: Console shows "[YourGraphRenderer] Loaded successfully"
5. Verify: Nodes render on canvas
```

**Test 2: Node Interaction**
```cpp
1. Click node → should highlight (select)
2. Drag node → should move smoothly
3. Right-click node → context menu appears
4. Select "Delete" → node removed
5. Verify: m_dirty flag set (save prompt appears)
```

**Test 3: Toolbar Buttons**
```cpp
1. Modify graph (e.g., add node)
2. Click "Save" button → file saved
3. Click "SaveAs" button → modal appears, select path
4. Click "Browse" button → file browser opens
5. Verify: Console shows save operations
```

**Test 4: Save/Load Roundtrip**
```cpp
1. Create graph with 3 nodes + 2 connections
2. Add properties to nodes
3. Save to file
4. Close tab
5. Re-open file
6. Verify: All nodes, connections, properties restored
7. Verify: m_dirty flag cleared (no asterisk on tab)
```

### Debugging with Logging

**Strategic Log Placement** (follow Phase 46 pattern):

```cpp
// ✅ DO log these:
- Constructor/Destructor (initialization lifecycle)
- Load/Save entry/exit + file paths
- Button clicks (toolbar interactions)
- Node creation/deletion (graph mutations)
- Save status (success/failure)

// ❌ DON'T log these (60 FPS spam):
- RenderCanvas() every frame
- OnMouseMove() every frame  
- RenderGrid() every frame
- Any method called 60+ times per second
```

**Example**:

```cpp
// ✅ GOOD (logs only on user action):
void OnSaveClicked()
{
    SYSTEM_LOG << "[YourGraphRenderer] Save clicked\n";
    ExecuteSave(currentPath);
}

// ❌ BAD (logs 60 times per second):
void Render()
{
    SYSTEM_LOG << "[YourGraphRenderer] Rendering frame\n";  // SPAM!
}
```

---

## Troubleshooting

### Build Errors

**Error: LNK2005 - Symbol Already Defined**

**Cause**: Method implemented in multiple .cpp files

**Solution**:
```
1. Search for method name in all .cpp files
2. Keep only ONE implementation
3. Move others to header (inline) or delete duplicates
4. Common issue: RenderContextMenus() defined in both Canvas.cpp and RenderingCore.cpp
```

**Error: CS0246 - Undefined Method**

**Cause**: Missing #include or typo in method name

**Solution**:
```
1. Check if method exists in header file
2. Check if class instantiated (nullptr check)
3. Add missing #include for dependent classes
4. Verify spelling matches exactly
```

### Runtime Issues

**Problem: Nodes Don't Render**

**Checklist**:
```
□ Document loaded? (check Load() return value)
□ Canvas initialized? (m_canvas != nullptr)
□ Node renderer created? (m_nodeRenderer != nullptr)
□ Render() method called? (check RenderGraphContent)
□ Coordinate transformation correct? (verify ScreenToCanvas formula)
□ Phase 52 Initialization Pattern**: Canvas->Initialize() must create m_nodeRenderer

SOLUTION: Follow Phase 52 pattern in Load():
- Create document
- Create canvas
- Call canvas->Initialize(document)
- This ensures all dependencies created before Render()
```

**Problem: Save Button Doesn't Work**

**Checklist**:
```
□ CanvasFramework created? (m_framework != nullptr)
□ ExecuteSave() implemented? (check derived class)
□ m_document != nullptr when saving?
□ File permissions OK? (try write to temp directory)
□ Phase 45 Frame Ordering**: Modals rendered AFTER content
   - RenderGraphContent() first
   - RenderFrameworkModals() last
```

**Problem: Dirty Flag Never Set**

**Checklist**:
```
□ Call MarkDirty() after mutations? (inherited from GraphEditorBase)
□ m_document->SetDirty(true) called? (also set document dirty)
□ Dirty flag cleared on Load/Save? (prevents false "modified" indicators)
□ Property editor calling ApplyChanges()? (must call MarkDirty internally)
```

**Problem: Tab Doesn't Appear After Load**

**Checklist**:
```
□ Load() returns true? (check console logs)
□ SetDocument() called in Load()? (base class integration)
□ Renderer returned from TabManager::CreateRendererForFile? (registration)
□ File type detected correctly? (DetectGraphType logic)
□ Phase 51 Fix**: Don't access moved objects
   - Save tabID BEFORE std::move()
   - Return saved value, not moved object
```

---

## Best Practices Summary

### Do's ✅

1. **Study Existing Code First** - All patterns exist in VisualScript/EntityPrefab/BehaviorTree
2. **Use Framework Utilities** - Don't reinvent Save, Toolbar, Modals
3. **Separate Concerns** - Document (data) ≠ Canvas (UI) ≠ Renderer (orchestration)
4. **Log Strategically** - Only on user actions, not render loops
5. **Test End-to-End** - Load/Save/Modify/Save again
6. **Follow Phase Patterns** - Phases 44-51 codify lessons learned
7. **Initialize Completely** - Phase 52: Initialize() must create ALL dependencies

### Don'ts ❌

1. **Don't Recreate Framework APIs** - Use CanvasFramework, CanvasToolbarRenderer, etc.
2. **Don't Log in Render Loops** - Creates console spam, breaks debugging
3. **Don't Access Moved Objects** - Save before std::move(), use saved copy
4. **Don't Render Modals Too Early** - Wait for frame cycle (Phase 45 pattern)
5. **Don't Ignore Dirty Flags** - They drive save prompts and tab feedback
6. **Don't Hardcode File Paths** - Use DataManager::FindResourceRecursive()
7. **Don't Mix Concerns** - Keep model, rendering, interaction separate

---

## Quick Reference Checklist

### For New Graph Type Implementation

**Day 1 - Architecture**
- [ ] Create YourGraphDocument (model class)
- [ ] Create YourCanvas (ImGui container)
- [ ] Create YourGraphRenderer (inherits GraphEditorBase)
- [ ] Implement Load/Save in YourGraphDocument

**Day 2 - Rendering**
- [ ] Implement YourCanvas::Render() (nodes + connections)
- [ ] Implement YourGraphRenderer::RenderGraphContent() (layout)
- [ ] Add toolbar buttons (RenderTypeSpecificToolbar)
- [ ] Test basic rendering

**Day 3 - Interaction**
- [ ] Implement node selection (click)
- [ ] Implement node dragging
- [ ] Implement node deletion (Delete key, context menu)
- [ ] Implement connection creation
- [ ] Test all interactions

**Day 4 - Framework Integration**
- [ ] Create CanvasFramework (Save/SaveAs/Browse buttons)
- [ ] Implement RenderFrameworkModals()
- [ ] Create YourPropertyEditorPanel
- [ ] Test Save/Load roundtrip
- [ ] Register in TabManager

**Day 5 - Polish**
- [ ] Add minimap support
- [ ] Create node palette (drag-drop components)
- [ ] Add type-specific features (Verify button, etc.)
- [ ] Create documentation
- [ ] Full end-to-end testing

---

## Additional Resources

- **Framework API Reference**: See GraphEditorBase.h for inherited methods
- **UI Components**: CanvasFramework.h, CanvasToolbarRenderer.h, CanvasMinimapRenderer.h
- **Working Examples**: EntityPrefabRenderer, BehaviorTreeRenderer (code comments)
- **Phase Documentation**: See Project Management folder for Phase-specific details

---

**End of Framework Integration Guide**

For questions or issues, refer to the code comments in existing renderers or check the Project Management folder for phase-specific documentation.

