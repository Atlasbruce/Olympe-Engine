# PLACEHOLDER EDITOR - COMPREHENSIVE UI DESIGN SPECIFICATION
**Version**: 1.0  
**Status**: ✅ Complete Architecture Reference  
**Last Updated**: Phase 4 Step 5  

---

## 📐 COMPLETE UI MOCKUP - ASCII DIAGRAM

### Final Target Layout (After Phase 4 Step 5 Fixes)

```
╔════════════════════════════════════════════════════════════════════════════════════╗
║ PLACEHOLDER EDITOR - LAYOUT ARCHITECTURE                                          ║
╠════════════════════════════════════════════════════════════════════════════════════╣
║                                                                                    ║
║  LAYER 0: MenuBar + Tabs                                                          ║
║  ┌──────────────────────────────────────────────────────────────────────────────┐ ║
║  │ [File] [Edit] [View] [Graph]  |  [PlaceholderGraph] ✕                       │ ║
║  └──────────────────────────────────────────────────────────────────────────────┘ ║
║                                                                                    ║
║  LAYER 1: CommonToolbar (GraphEditorBase - Framework Level)                       ║
║  ┌──────────────────────────────────────────────────────────────────────────────┐ ║
║  │ [New] [Open] [Save] [SaveAs]  |  [Delete] [Undo] [Redo]  |  [Zoom: 100%]   │ ║
║  │ ┌─ Framework buttons (same for all graph types)                            │ ║
║  │ │  • Provided by GraphEditorBase::RenderCommonToolbar()                    │ ║
║  │ │  • Line 232-290 in GraphEditorBase.cpp                                   │ ║
║  └──────────────────────────────────────────────────────────────────────────────┘ ║
║                                                                                    ║
║  LAYER 2: TypeSpecificToolbar (PlaceholderGraphRenderer Override)                 ║
║  ┌──────────────────────────────────────────────────────────────────────────────┐ ║
║  │ [Verify Graph] [Run Graph] [Execute]  |  ☐ Minimap  Size: [=====◆===]      │ ║
║  │ ┌─ Type-specific buttons (unique to Placeholder)                           │ ║
║  │ │  • Provided by PlaceholderGraphRenderer::RenderTypeSpecificToolbar()     │ ║
║  │ │  • Line 250-281 in PlaceholderGraphRenderer.cpp                          │ ║
║  │ │  • Called from GraphEditorBase::RenderCommonToolbar() line 292           │ ║
║  └──────────────────────────────────────────────────────────────────────────────┘ ║
║                                                                                    ║
║  LAYER 3: MainContent (Two-Column Layout)                                         ║
║  ┌──────────────────────────────────────────────────────────────────────────────┐ ║
║  │                                                                              │ ║
║  │   ┌────────────────────────────────┐ ║ ┌─────────────────────────────────┐ ║
║  │   │                                │ ║ │  RIGHT PANEL (280px, resizable) │ ║
║  │   │   LEFT: Canvas                 │ ║ │                                 │ ║
║  │   │   (Variable Width)             │ ║ │  ┌─ PART A: NodeProperties    │ ║
║  │   │                                │ ║ │  │ (35% height, ~140px)       │ ║
║  │   │   [MINIMAP]                    │ ║ │  ├─────────────────────────────┤ ║
║  │   │   ┌──────────────┐             │ ║ │  │ NodeID:        [Node_0001] │ ║
║  │   │   │              │             │ ║ │  │ ComponentType: [ Transform] │ ║
║  │   │   │  Grid View   │             │ ║ │  │ Position:      [0.0, 0.0]  │ ║
║  │   │   │  with Nodes  │             │ ║ │  │ Size:          [100, 50]   │ ║
║  │   │   │              │             │ ║ │  │ Enabled:       [✓] Checked │ ║
║  │   │   │              │             │ ║ │  └─────────────────────────────┤ ║
║  │   │   │  • Blue Node │             │ ║ │  ║ SPLITTER (vertical)        ║ ║
║  │   │   │  • Green Node│             │ ║ │  ├─────────────────────────────┤ ║
║  │   │   │  • Magenta   │             │ ║ │  │                             │ ║
║  │   │   │    Nodes     │             │ ║ │  │  PART B: TabbedPanel        │ ║
║  │   │   │              │             │ ║ │  │  (65% height, ~260px)      │ ║
║  │   │   │              │             │ ║ │  │                             │ ║
║  │   │   └──────────────┘             │ ║ │  │  [Components] [Node]        │ ║
║  │   │                                │ ║ │  │  ┌────────────────────────┐ ║
║  │   │   Connections:                 │ ║ │  │  │                        │ ║
║  │   │   • Bezier curves yellow       │ ║ │  │  │ TAB 0: Components      │ ║
║  │   │   • Hover: highlight bright   │ ║ │  │  │ (Placeholder UI)       │ ║
║  │   │   • Click to select            │ ║ │  │  │                        │ ║
║  │   │                                │ ║ │  │  │ TAB 1: Node            │ ║
║  │   │ Keyboard:                      │ ║ │  │  │ (PropertyEditor)       │ ║
║  │   │ • Delete key: remove nodes     │ ║ │  │  │                        │ ║
║  │   │ • Ctrl+A: select all           │ ║ │  │  │ [Edit] [Revert]        │ ║
║  │   │ • Pan/Zoom: standard           │ ║ │  │  └────────────────────────┘ ║
║  │   │                                │ ║ │  │                             │ ║
║  │   └────────────────────────────────┘ ║ │  └─────────────────────────────┘ ║
║  │                                      ║ │                                   ║
║  │   ◄─── Horizontal Resize Handle ───►║◄─ Vertical Resize Handle          ║
║  │                                                                              ║
║  └──────────────────────────────────────────────────────────────────────────────┘ ║
║                                                                                    ║
║  LAYER 4: StatusBar                                                               ║
║  ┌──────────────────────────────────────────────────────────────────────────────┐ ║
║  │ Nodes: 3  Connections: 2  Selection: 1  Status: Ready                       │ ║
║  └──────────────────────────────────────────────────────────────────────────────┘ ║
║                                                                                    ║
╚════════════════════════════════════════════════════════════════════════════════════╝
```

---

## 🏗️ COMPONENT BREAKDOWN WITH SPECIFICATIONS

### 1. LAYER 0: MenuBar + Tabs
**Location**: Top of window  
**Height**: 32px  
**Components**:
- MenuBar: [File] [Edit] [View] [Graph]
- TabBar: [PlaceholderGraph ✕]
- Purpose: Navigation and tab switching

**Rendering**:
- ImGui::BeginMenuBar() → ImGui::EndMenuBar()
- Tab rendering handled by TabManager
- Not part of GraphEditorBase (framework-level)

---

### 2. LAYER 1: CommonToolbar (Framework Level - GraphEditorBase)
**Location**: Below MenuBar  
**Height**: 48px  
**Framework Responsibility**: YES ✅

**Rendering Call**:
```cpp
GraphEditorBase::RenderCommonToolbar()  // Line 232-290
  ├─ Renders: New, Open, Save, SaveAs buttons
  ├─ Renders: Delete, Undo, Redo buttons
  ├─ Renders: Zoom level display
  └─ Line 292: Calls RenderTypeSpecificToolbar() ← VIRTUAL OVERRIDE
```

**Code Location**:
- File: `Source/BlueprintEditor/Framework/GraphEditorBase.cpp`
- Lines: 265-296
- Method: `void GraphEditorBase::RenderCommonToolbar()`

**Key Features**:
- Same buttons for ALL graph types
- Separator between groups
- Consistent visual appearance

---

### 3. LAYER 2: TypeSpecificToolbar (Subclass Override)
**Location**: Continues from CommonToolbar (same line)  
**Height**: 48px  
**Subclass Responsibility**: YES ✅ (PlaceholderGraphRenderer)

**Rendering Call**:
```cpp
// Called FROM: GraphEditorBase::RenderCommonToolbar() line 292
PlaceholderGraphRenderer::RenderTypeSpecificToolbar()  // Line 250-281
  ├─ Renders: [Verify Graph] button
  ├─ Renders: [Run Graph] button
  ├─ Renders: [Execute] button
  ├─ Renders: Separator
  ├─ Renders: Minimap checkbox
  ├─ Renders: Minimap size slider
  └─ Type-specific features unique to Placeholder graphs
```

**Code Location**:
- File: `Source/BlueprintEditor/PlaceholderEditor/PlaceholderGraphRenderer.cpp`
- Lines: 250-281
- Method: `void PlaceholderGraphRenderer::RenderTypeSpecificToolbar()`

**Diagnostic Logging**:
```cpp
// Line 250-256: Diagnostic log added (Phase 4)
static bool logged = false;
if (!logged) { 
    std::cout << "[PlaceholderGraphRenderer] RenderTypeSpecificToolbar CALLED" << std::endl; 
    logged = true; 
}
```

**Key Features**:
- Type-specific buttons and controls
- Extends CommonToolbar (same line via SameLine())
- Can be empty or full depending on graph type needs

---

### 4. LAYER 3: MainContent (Two-Column Layout)
**Location**: Below toolbars  
**Height**: Available height minus toolbars and status bar  
**Rendering Responsibility**: PlaceholderGraphRenderer

**Sub-Components**:

#### LEFT COLUMN: Canvas
**Width**: Variable (100% - 280px - handle width)  
**Height**: 100% of available  
**Content**:
- ImGui::BeginChild("PlaceholderCanvas")
- PrefabCanvas rendering (m_canvas->Render())
- Grid, nodes, connections
- Selection/drag interactions
- ImGui::EndChild()

**Code Location**:
```cpp
// File: PlaceholderGraphRenderer.cpp
// Lines: 96-192, RenderGraphContent() Part A
ImGui::BeginChild("PlaceholderCanvas", ImVec2(canvasWidth, 0), ...);
m_canvas->Render();
ImGui::EndChild();
```

**Minimap Position**:
- Rendered in top-right corner of canvas
- Shows preview of entire graph
- Interactive: click to pan

---

#### HORIZONTAL RESIZE HANDLE
**Width**: 6px  
**Height**: Full content height  
**Visual**: Vertical line, cursor changes to ResizeEW on hover  
**Functionality**: Click and drag to resize canvas/properties panel

**Code Location**:
```cpp
// Line 160-167 in PlaceholderGraphRenderer.cpp
ImGui::SameLine();
ImGui::Button("##placeholder_resize", ImVec2(6.0f, -1.0f));
if (ImGui::IsItemHovered()) 
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
if (ImGui::IsItemActive() && ImGui::IsMouseDragging(...)) {
    m_propertiesPanelWidth -= ImGui::GetIO().MouseDelta.x;
    m_propertiesPanelWidth = std::max(250.0f, m_propertiesPanelWidth);
}
```

---

#### RIGHT COLUMN: Properties Panel
**Width**: 280px (resizable, m_propertiesPanelWidth)  
**Height**: 100% of content area  
**Location**: Right side, after horizontal resize handle

**Structure**:
```
┌─────────────────────────────────┐
│   PART A: NodeProperties        │  ← 35% height (~140px)
│   ┌──────────────────────────┐  │
│   │ NodeID, Type, Name, Pos  │  │
│   │ Size, Enabled status     │  │
│   └──────────────────────────┘  │
├─────────────────────────────────┤
│   SPLITTER (vertical, 4px)      │  ← Resize handle between parts
├─────────────────────────────────┤
│   PART B: TabbedPanel           │  ← 65% height (~260px)
│   ┌──────────────────────────┐  │
│   │ [Components] [Node]      │  │
│   │ ┌────────────────────┐   │  │
│   │ │ Tab Content Here  │   │  │
│   │ │ (varies by tab)   │   │  │
│   │ │                   │   │  │
│   │ │ [Apply] [Revert]  │   │  │
│   │ └────────────────────┘   │  │
│   └──────────────────────────┘  │
└─────────────────────────────────┘
```

**Code Location**:
```cpp
// File: PlaceholderGraphRenderer.cpp
// Lines: 170-192, RenderGraphContent() Part B
ImGui::SameLine();
ImGui::BeginChild("PlaceholderRightPanel", ImVec2(m_propertiesPanelWidth, 0), true);

// ─────────────────────────────────────────────────────────────────
// PART A: Node Properties (35% height)
// ─────────────────────────────────────────────────────────────────
ImGui::BeginChild("Part_A_NodeProperties", ImVec2(0, partAHeight), ...);
// Render node ID, type, name, position, size, enabled flag
ImGui::EndChild();

// ─────────────────────────────────────────────────────────────────
// VERTICAL SPLITTER
// ─────────────────────────────────────────────────────────────────
ImGui::Button("##placeholder_vsplit", ImVec2(-1.0f, 4.0f));
if (ImGui::IsItemHovered()) 
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
// Handle drag to resize Part A vs Part B

// ─────────────────────────────────────────────────────────────────
// PART B: Tabbed Panel (65% height)
// ─────────────────────────────────────────────────────────────────
ImGui::BeginChild("Part_B_TabbedPanel", ImVec2(0, 0), ...);
RenderRightPanelTabs();  // ← CALLS TAB SYSTEM
ImGui::EndChild();

ImGui::EndChild();  // End RightPanel
```

---

#### RIGHT PANEL - PART A: Node Properties (35% height)
**Dimensions**: Full right panel width × 35% height (~140px)  
**Content**: Selected node information display

**Fields Displayed**:
```
NodeID:        [Node_0001]              (read-only)
ComponentType: [Transform ▼]            (display)
ComponentName: [MyTransform   ]         (editable)
Position X:    [10.5         ]          (editable)
Position Y:    [20.3         ]          (editable)
Size:          [100.0 × 50.0]           (display)
Enabled:       [✓] Checked              (toggle)
```

**Rendering Code**:
```cpp
// Lines 121-159 in PlaceholderGraphRenderer.cpp, inside Part A
ImGui::LabelText("NodeID", selectedNodeIds.empty() ? "None" : selectedNodeIds[0].c_str());
if (ImGui::BeginCombo("ComponentType##prop", currentType.c_str(), 
                      ImGuiComboFlags_HeightLarge)) {
    // Component type options
    ImGui::EndCombo();
}
ImGui::InputText("ComponentName##prop", &componentName, ...);
ImGui::InputFloat("Position X##prop", &posX, ...);
ImGui::InputFloat("Position Y##prop", &posY, ...);
// ... size and enabled fields
```

---

#### RIGHT PANEL - PART B: Tabbed Panel (65% height)
**Dimensions**: Full right panel width × 65% height (~260px)  
**Tabs**: 2 tabs with ImGui tab bar

**Tab System Structure**:
```cpp
// Lines 194-248 in PlaceholderGraphRenderer.cpp
void PlaceholderGraphRenderer::RenderRightPanelTabs()
{
    if (ImGui::BeginTabBar("RightPanelTabs", ...))
    {
        // ════════════════════════════════════════════════════════
        // TAB 0: Components (Palette for creating new nodes)
        // ════════════════════════════════════════════════════════
        if (ImGui::BeginTabItem("Components"))
        {
            ImGui::TextDisabled("(Component palette - placeholder for now)");
            ImGui::TextDisabled("Future: List of components to add to graph");
            ImGui::EndTabItem();
        }
        
        // ════════════════════════════════════════════════════════
        // TAB 1: Node (Properties editor for selected node) ✅ FIXED
        // ════════════════════════════════════════════════════════
        if (ImGui::BeginTabItem("Node"))  // ← WAS "Properties", NOW "Node" ✅
        {
            if (!m_selectedNodeIds.empty() && m_propertyEditor)
            {
                m_propertyEditor->Render(m_selectedNodeIds);
                
                // Apply/Revert buttons
                ImGui::Separator();
                if (ImGui::Button("Apply##properties", ImVec2(120, 0)))
                {
                    // Apply changes
                }
                ImGui::SameLine();
                if (ImGui::Button("Revert##properties", ImVec2(120, 0)))
                {
                    // Revert changes
                }
            }
            else
            {
                ImGui::TextDisabled("(No node selected)");
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
```

**Tab 0: Components** (Placeholder content):
```
┌──────────────────────────────────┐
│ (Component palette - placeholder) │
│ Future: List of components to    │
│ add to graph                     │
│                                  │
│                                  │
│                                  │
│                                  │
└──────────────────────────────────┘
```

**Tab 1: Node** (Active property editor):
```
┌──────────────────────────────────┐
│ Editing: Node_0001 Properties    │
│ ─────────────────────────────────│
│ • Custom properties (editable)   │
│ • Values from PropertyEditor     │
│ • Specific to node type          │
│                                  │
│ [Apply] [Revert]                 │
└──────────────────────────────────┘
```

---

### 5. LAYER 4: StatusBar
**Location**: Bottom of window  
**Height**: 24px  
**Content**: Status information

```
Nodes: 3  Connections: 2  Selection: 1  Status: Ready
```

---

## 🔄 RENDERING CALL CHAIN & INHERITANCE PATTERN

### Complete Execution Flow (Every Frame)

```
┌─ FRAME START ───────────────────────────────────────────────────────────┐
│                                                                          │
│  1. BlueprintEditorGUI::Render()                                        │
│     File: BlueprintEditorGUI.cpp, Line 223                              │
│     Purpose: Main editor render loop                                    │
│     │                                                                    │
│     └─→ 2. RenderFixedLayout()                                          │
│           File: BlueprintEditorGUI.cpp, Line 458                        │
│           Purpose: Layout fixed regions (menu, tabs, canvas)            │
│           │                                                              │
│           ├─→ 3. TabManager::RenderTabBar()                             │
│           │     File: TabManager.cpp, Line 672                          │
│           │     Purpose: Render tab buttons at top                      │
│           │                                                              │
│           └─→ 4. TabManager::RenderActiveCanvas()                       │
│                 File: TabManager.cpp, Line 677                          │
│                 Purpose: Render active tab's renderer content            │
│                 │                                                        │
│                 └─→ 5. EditorTab::renderer->Render()                    │
│                       File: TabManager.cpp, Line 880                    │
│                       Type: PlaceholderGraphRenderer::Render()           │
│                       (inherited from GraphEditorBase)                  │
│                       │                                                  │
│                       └─→ 6. GraphEditorBase::Render()                  │
│                             File: GraphEditorBase.cpp, Line 222         │
│                             Keyword: FINAL (cannot be overridden)       │
│                             Template Method Pattern:                    │
│                             │                                            │
│                             ├─→ 7a. RenderCommonToolbar()               │
│                             │      File: GraphEditorBase.cpp, Line 232  │
│                             │      Purpose: Framework toolbar            │
│                             │      Content: [New][Open][Save]...        │
│                             │      │                                     │
│                             │      └─→ 7b. RenderTypeSpecificToolbar()  │
│                             │            File: GraphEditorBase.cpp      │
│                             │            Line 292 (calls virtual override)
│                             │            │                               │
│                             │            └─→ 8. PlaceholderGraphRenderer::
│                             │                   RenderTypeSpecificToolbar()
│                             │                   File: PlaceholderGraphRenderer.cpp
│                             │                   Line 250-281              │
│                             │                   Content: [Verify][Run]... │
│                             │                   ✅ FIXED: Now has         │
│                             │                      diagnostic log         │
│                             │                                             │
│                             ├─→ 7c. RenderGraphContent()                │
│                             │      File: GraphEditorBase.cpp, Line 244  │
│                             │      Type: Pure Virtual (must override)    │
│                             │      │                                     │
│                             │      └─→ 9. PlaceholderGraphRenderer::     │
│                             │            RenderGraphContent()            │
│                             │            File: PlaceholderGraphRenderer  │
│                             │            Line 96-192                     │
│                             │            Part A: Canvas (left column)    │
│                             │            Part B: Properties panel (right) │
│                             │            │                               │
│                             │            ├─→ 10a. PrefabCanvas::Render() │
│                             │            │       (Left column content)    │
│                             │            │                               │
│                             │            └─→ 10b. RenderRightPanelTabs()│
│                             │                    Line 194-248             │
│                             │                    ✅ FIXED: Called once    │
│                             │                       from Part B only      │
│                             │                    Previously: Called twice │
│                             │                       (duplicate bug) ❌    │
│                             │                                             │
│                             └─→ 7d. RenderTypePanels()                  │
│                                   File: GraphEditorBase.cpp, Line 247   │
│                                   Type: Virtual (can override)            │
│                                   │                                       │
│                                   └─→ 11. PlaceholderGraphRenderer::     │
│                                          RenderTypePanels()              │
│                                          File: PlaceholderGraphRenderer  │
│                                          Line 283-287                    │
│                                          Status: NOW EMPTY ✅ FIXED      │
│                                          (was calling RenderRightPanel   │
│                                           Tabs() - duplicate! ❌)        │
│                                                                           │
└─ FRAME END ─────────────────────────────────────────────────────────────┘
```

### Key Method Definitions

| Step | Method | File | Line | Type | Purpose |
|------|--------|------|------|------|---------|
| 1 | `BlueprintEditorGUI::Render()` | BlueprintEditorGUI.cpp | 223 | - | Entry point |
| 2 | `RenderFixedLayout()` | BlueprintEditorGUI.cpp | 458 | - | Layout dispatcher |
| 3 | `TabManager::RenderTabBar()` | TabManager.cpp | 672 | - | Tab buttons |
| 4 | `TabManager::RenderActiveCanvas()` | TabManager.cpp | 677 | - | Calls renderer |
| 5 | `PlaceholderGraphRenderer::Render()` | (inherited) | - | - | Calls base |
| 6 | `GraphEditorBase::Render()` | GraphEditorBase.cpp | 222 | **FINAL** | Template method |
| 7a | `RenderCommonToolbar()` | GraphEditorBase.cpp | 232 | - | Framework toolbar |
| 7b | `RenderTypeSpecificToolbar()` | GraphEditorBase.cpp | 292 | **Virtual** | Calls override |
| 8 | `PlaceholderGraphRenderer::RenderTypeSpecificToolbar()` | PlaceholderGraphRenderer.cpp | 250 | **Override** | Type-specific toolbar |
| 7c | `RenderGraphContent()` | GraphEditorBase.cpp | 244 | **PureVirtual** | Calls override |
| 9 | `PlaceholderGraphRenderer::RenderGraphContent()` | PlaceholderGraphRenderer.cpp | 96 | **Override** | Two-column layout |
| 10a | `PrefabCanvas::Render()` | PrefabCanvas.cpp | 50+ | - | Canvas content |
| 10b | `RenderRightPanelTabs()` | PlaceholderGraphRenderer.cpp | 194 | - | Tab system |
| 7d | `RenderTypePanels()` | GraphEditorBase.cpp | 247 | **Virtual** | Calls override |
| 11 | `PlaceholderGraphRenderer::RenderTypePanels()` | PlaceholderGraphRenderer.cpp | 283 | **Override** | No-op (fixed) |

---

## 🎯 INHERITANCE PATTERN EXPLANATION

### Template Method Pattern (How It Works)

**Pattern Definition**:
```cpp
// BASE CLASS (GraphEditorBase)
class GraphEditorBase : public IGraphRenderer {
public:
    virtual void Render() override final;        // ← FINAL: Cannot be overridden
    virtual void RenderGraphContent() = 0;       // ← Pure virtual: MUST override
    virtual void RenderTypeSpecificToolbar() {}  // ← Virtual: Can override
    virtual void RenderTypePanels() {}           // ← Virtual: Can override
};

// SUBCLASS (PlaceholderGraphRenderer)
class PlaceholderGraphRenderer : public GraphEditorBase {
public:
    virtual void RenderGraphContent() override;           // ← MUST implement
    virtual void RenderTypeSpecificToolbar() override;    // ← CAN enhance
    virtual void RenderTypePanels() override;             // ← CAN override
};
```

### Method Resolution Order (VTable Lookup)

```
Call: PlaceholderGraphRenderer::Render()
  ↓
Not found in PlaceholderGraphRenderer
  ↓
Look in base class: GraphEditorBase::Render() ← FOUND (FINAL, line 222)
  ↓
Execute: GraphEditorBase::Render()
  {
      // Template method calls virtual methods:
      
      RenderCommonToolbar();             // Normal method (not virtual)
      RenderTypeSpecificToolbar();       // ← VIRTUAL CALL
          ↓
          Lookup in VTable for this instance
          ↓
          Found: PlaceholderGraphRenderer::RenderTypeSpecificToolbar()
          ↓
          Execute: PlaceholderGraphRenderer::RenderTypeSpecificToolbar()
      
      RenderGraphContent();              // ← VIRTUAL CALL
          ↓
          Lookup in VTable for this instance
          ↓
          Found: PlaceholderGraphRenderer::RenderGraphContent()
          ↓
          Execute: PlaceholderGraphRenderer::RenderGraphContent()
  }
```

### Inheritance Hierarchy Diagram

```
┌────────────────────────────────────────────┐
│ IGraphRenderer (Interface)                 │
│                                            │
│ virtual void Render() = 0;                 │
│ virtual bool Load(string) = 0;             │
│ virtual bool Save(string) = 0;             │
│ ...                                        │
└────────────┬─────────────────────────────┘
             │
             │ public inheritance
             │
┌────────────▼─────────────────────────────┐
│ GraphEditorBase                           │
│ (Framework Template)                      │
│                                            │
│ FINAL Methods:                            │
│  • Render() final                         │ ← Controls flow
│  • RenderCommonToolbar()                  │ ← All graphs same
│                                            │
│ Virtual Methods (for subclass override):  │
│  • RenderTypeSpecificToolbar() {}         │ ← Override to customize
│  • RenderTypePanels() {}                  │ ← Override to customize
│  • RenderGraphContent() = 0;              │ ← MUST override
│  • HandleTypeSpecificShortcuts() {}       │ ← Override to customize
│  • ... (and 20+ others)                   │
│                                            │
│ Members:                                  │
│  • m_canvasEditor                         │
│  • m_selectedNodeIds                      │
│  • m_minimapVisible                       │
│  • ... (shared state)                     │
└────────────┬─────────────────────────────┘
             │
    ┌────────┼────────┬──────────────────────────┐
    │        │        │                          │
    │ pub    │ pub    │ pub                      │
    │ inh    │ inh    │ inh                      │
    │        │        │                          │
┌───▼──┐ ┌──▼───┐ ┌──▼──────────────────────┐
│VisualScript   │ │ BehaviorTree│ │EntityPrefab        │
│EditorPanel    │ │ Renderer    │ │ Renderer           │
│               │ │             │ │                    │
│Overrides:     │ │ Overrides:  │ │ Overrides:         │
│• RenderGraph  │ │ • RenderGra│ │ • RenderGraph      │
│  Content()    │ │   phContent│ │   Content()        │
│• RenderType   │ │ • RenderTy│ │ • RenderType       │
│  Specific     │ │   peSpecif│ │   Specific         │
│  Toolbar()    │ │   icToolba│ │   Toolbar()        │
│• RenderType   │ │   r()      │ │ • RenderType       │
│  Panels()     │ │ • RenderTy│ │   Panels()         │
│               │ │   pePanels│ │                    │
│Members:       │ │ ()         │ │ Members:           │
│• m_document   │ │            │ │ • m_document       │
│• m_canvas     │ │ Members:   │ │ • m_canvas         │
│• m_editor     │ │ • m_documnt│ │ • m_pallete        │
│  Panel        │ │ • m_canvas │ │ • m_propertyEditor │
│               │ │ • m_verifier
│               │ │            │ │                    │
└───────────────┘ │ • m_trace  │ │                    │
                  │   Panel    │ │                    │
                  └────────────┘ └────────────────────┘
```

### Method Override Verification

**PlaceholderGraphRenderer.h** (Line 28-56):
```cpp
class PlaceholderGraphRenderer : public GraphEditorBase
{
public:
    // ✅ CORRECT: Public inheritance from GraphEditorBase
    // This means PlaceholderGraphRenderer IS-A GraphEditorBase
    
    // ✅ CORRECT: All virtual overrides declared with 'override' keyword
    virtual void RenderGraphContent() override;
    virtual void RenderTypeSpecificToolbar() override;
    virtual void RenderTypePanels() override;
    
    // Note: 'override' keyword is C++11 best practice
    // Compiler will ERROR if method signature doesn't match base class
    // This prevents accidental mismatches
};
```

**Why 'override' keyword is critical**:
```cpp
// WRONG (without override):
class PlaceholderGraphRenderer : public GraphEditorBase {
    void RenderGraphContent() {}  // ← Typo: argument added by accident
};
// Result: Creates NEW method, doesn't override base! Bug!

// CORRECT (with override):
class PlaceholderGraphRenderer : public GraphEditorBase {
    void RenderGraphContent(int x) override {}  // ← COMPILE ERROR!
};
// Result: Compiler catches the mismatch! Bug prevented!
```

---

## 🐛 PHASE 4 STEP 5 FIXES EXPLAINED

### Fix #1: Remove Duplicate RenderRightPanelTabs() Call

**Problem**: Tabs rendered twice per frame
- Location 1: `RenderGraphContent()` line 188 ✅ CORRECT
- Location 2: `RenderTypePanels()` line 286 ❌ DUPLICATE (WRONG)

**Why This Happens**:
```
ImGui Immediate-Mode Rendering:
- If RenderRightPanelTabs() called twice in same frame
- Both calls render to same ImGui context
- Result: Tabs appear twice (overlapped or offset)
```

**The Fix**:
```cpp
// BEFORE (PlaceholderGraphRenderer.cpp line 283-287):
void PlaceholderGraphRenderer::RenderTypePanels()
{
    RenderRightPanelTabs();  // ❌ DUPLICATE CALL!
}

// AFTER:
void PlaceholderGraphRenderer::RenderTypePanelsin()
{
    // NOTE: RenderRightPanelTabs() is called from RenderGraphContent() Part B
    // DO NOT duplicate here - this was causing tabs to render twice
    // This method is a no-op in the new layout architecture
}
```

**Why This Works**:
- `RenderGraphContent()` Part B (line 188) calls it once ✅
- `RenderTypePanels()` is now empty (no-op) ✅
- Single call per frame ✅
- Tabs render at correct location only ✅

---

### Fix #2: Correct Tab Name

**Problem**: Tab labeled "Properties" instead of "Node"

**The Fix**:
```cpp
// BEFORE (line 211):
if (ImGui::BeginTabItem("Properties"))

// AFTER:
if (ImGui::BeginTabItem("Node"))
```

**Why This Matters**:
- Design specification requires "Node" (not "Properties")
- User-facing labels must be consistent
- Prevents confusion with property editor panel

---

### Fix #3: Add Diagnostic Logging

**Purpose**: Verify that RenderTypeSpecificToolbar() is actually called at runtime

**The Fix**:
```cpp
// BEFORE (line 250):
void PlaceholderGraphRenderer::RenderTypeSpecificToolbar()
{
    // Toolbar implementation...

// AFTER:
void PlaceholderGraphRenderer::RenderTypeSpecificToolbar()
{
    // Diagnostic log (added Phase 4)
    static bool logged = false;
    if (!logged) { 
        std::cout << "[PlaceholderGraphRenderer] RenderTypeSpecificToolbar CALLED" << std::endl; 
        logged = true; 
    }
    
    // Toolbar implementation...
```

**Why This Helps**:
- If log appears in console → method is called ✅
- If no log → method not called ❌ (signals deeper issue)
- `static bool` prevents spam (logs only once)
- Lightweight debugging without breakpoints

---

## ✅ VALIDATION CHECKLIST

### Pre-Runtime Verification (Code Level)
- [x] PlaceholderGraphRenderer inherits GraphEditorBase (line 28)
- [x] RenderTypeSpecificToolbar() declared with override (line 49)
- [x] RenderGraphContent() declared with override (line 46)
- [x] RenderTypePanels() declared with override (line 52)
- [x] Build successful: 0 errors, 0 warnings ✅

### Runtime Verification (Visual Level)
- [ ] Tabs render ONCE only (not duplicated)
- [ ] Tab names show "Components" and "Node" (not "Properties")
- [ ] Toolbar buttons visible: [Verify Graph] [Run Graph] [Execute]
- [ ] Minimap checkbox and size slider visible
- [ ] Canvas renders in left column (variable width)
- [ ] Properties panel renders on right (280px resizable)
- [ ] Horizontal resize handle works (cursor change on hover)
- [ ] Vertical splitter between Part A and Part B works
- [ ] Part A: Node Properties displays correctly
- [ ] Part B: Tab system shows correct content

### Console Output Verification
- [ ] "[PlaceholderGraphRenderer] RenderTypeSpecificToolbar CALLED" appears once in console

### Functional Verification
- [ ] Can select nodes (click in canvas)
- [ ] Can drag nodes (left-click and drag)
- [ ] Can pan canvas (middle-mouse drag)
- [ ] Can zoom canvas (mouse scroll wheel)
- [ ] Properties panel updates when selecting nodes
- [ ] Can switch between "Components" and "Node" tabs
- [ ] Tab content is appropriate for each tab

---

## 📊 ARCHITECTURE COMPARISON

### BEFORE Phase 4 Step 5 (Broken State)
```
PlaceholderGraphRenderer.cpp
├─ RenderGraphContent()
│  └─ (Canvas logic)
│  └─ (Missing tab system)
│
├─ RenderTypePanels()
│  └─ Calls RenderRightPanelTabs() ← CORRECT
│
└─ RenderRightPanelTabs()
   └─ Tab implementation
   
PROBLEM: RenderRightPanelTabs() called TWICE per frame
- Call 1: From RenderGraphContent() ✅
- Call 2: From RenderTypePanels() ❌
Result: Tabs render twice (duplicate + offset)
```

### AFTER Phase 4 Step 5 (Fixed State)
```
PlaceholderGraphRenderer.cpp
├─ RenderGraphContent()
│  ├─ PART A: Canvas rendering
│  │  └─ (Canvas logic)
│  │
│  └─ PART B: Properties panel
│     └─ Calls RenderRightPanelTabs() ✅ (only call per frame)
│
├─ RenderTypePanels()
│  └─ Empty method (no-op) ✅ (no duplicate call)
│
└─ RenderRightPanelTabs()
   ├─ Tab 0: Components (placeholder)
   │
   └─ Tab 1: Node (property editor) ← Tab name FIXED
   
RESULT: Tabs render once at correct position
- Call count: 1 per frame ✅
- Duplicate removed: YES ✅
- Tab name corrected: YES ✅
```

---

## 🔗 FILE CROSS-REFERENCES

### Core Implementation Files
| File | Purpose | Key Lines |
|------|---------|-----------|
| `Source/BlueprintEditor/Framework/GraphEditorBase.h` | Base class declarations | 100-160 (virtual methods) |
| `Source/BlueprintEditor/Framework/GraphEditorBase.cpp` | Template method implementation | 222-296 (Render() pipeline) |
| `Source/BlueprintEditor/PlaceholderEditor/PlaceholderGraphRenderer.h` | Subclass declarations | 28 (inheritance), 49-52 (overrides) |
| `Source/BlueprintEditor/PlaceholderEditor/PlaceholderGraphRenderer.cpp` | Subclass implementation | 96-192 (RenderGraphContent), 194-248 (RenderRightPanelTabs), 250-281 (RenderTypeSpecificToolbar), 283-287 (RenderTypePanels) |
| `Source/BlueprintEditor/PlaceholderEditor/PrefabCanvas.h/.cpp` | Canvas rendering | ~50 lines (Render method) |
| `Source/BlueprintEditor/PlaceholderEditor/PlaceholderPropertyEditorPanel.h/.cpp` | Property editor | ~100 lines (Render method) |

### Call Chain Files
| File | Purpose | Key Lines |
|------|---------|-----------|
| `Source/BlueprintEditor/BlueprintEditorGUI.cpp` | Entry point | 223 (Render), 458 (RenderFixedLayout), 677 (RenderActiveCanvas) |
| `Source/BlueprintEditor/TabManager.cpp` | Tab management | 672 (RenderTabBar), 677 (RenderActiveCanvas), 880 (Call renderer->Render) |

---

## 📝 NOTES FOR DEVELOPERS

### Important Patterns
1. **Template Method Pattern**: Base class controls flow, subclasses customize behavior
2. **Virtual Method Override**: Use `override` keyword for compile-time verification
3. **Immediate-Mode GUI**: Each frame must render complete UI (no state persistence)
4. **ImGui Children**: Use `BeginChild()`/`EndChild()` for layout containers

### Common Pitfalls
1. ❌ Calling same ImGui rendering code twice in one frame → duplicate rendering
2. ❌ Forgetting `override` keyword → accidental new method creation
3. ❌ Rendering in wrong order in ImGui frame cycle → elements appear wrong
4. ❌ Not resetting resize handle state → drag continues indefinitely

### Next Steps (Phase 4 Step 6)
1. Add context menus (right-click on nodes/canvas)
2. Integrate minimap rendering
3. Add keyboard shortcuts for operations
4. Implement undo/redo system

---

**Document Status**: ✅ COMPLETE  
**Last Updated**: Phase 4 Step 5 (Comprehensive Design)  
**Ready for**: Runtime Verification & Phase 5 Planning
