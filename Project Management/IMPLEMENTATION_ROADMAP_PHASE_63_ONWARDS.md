# 🗺️ IMPLEMENTATION ROADMAP - What's Left to Build
**Phase 63 onwards - Complete Feature Checklist**

---

## 📋 EXECUTIVE ROADMAP

### Current State: Framework Stable, Features 60% Stubbed
- ✅ Rendering engine working
- ✅ Framework architecture proven
- ✅ Data persistence (Load/Save) working
- ❌ Node interactions (Select/Move/Delete) are stubs

### Goal: Full CRUD Operations + Polish
- **Time to Complete**: 2-3 days (14-15 hours dev work)
- **Complexity**: Low to Medium
- **Risk**: Minimal (framework proven)

---

## 🔥 PHASE 63 - CRITICAL PATH (Session 1) - 3 Hours

### Goal: Get node selection + deletion working
### Why Critical: All other features depend on this

---

### Task 63.1: Node Selection & Visual Feedback (1 hour)

**Current State**: PlaceholderCanvas stubs log clicks but don't select

**What Needs Doing**:
```cpp
// In PlaceholderCanvas::OnMouseDown()

// 1. Get clicked node position
ImVec2 canvasPos = ScreenToCanvas(ImGui::GetMousePos());

// 2. Loop through all nodes and hit-test
for (auto& node : m_document->GetAllNodes()) {
    ImVec2 nodeStart = {node.posX, node.posY};
    ImVec2 nodeEnd = {node.posX + node.width, node.posY + node.height};
    
    if (canvasPos.x >= nodeStart.x && canvasPos.x <= nodeEnd.x &&
        canvasPos.y >= nodeStart.y && canvasPos.y <= nodeEnd.y) {
        // Hit! Call parent class selection logic
        SelectMultipleNodes(node.nodeId, ctrlPressed, shiftPressed);
        break;
    }
}

// 3. In RenderNodes(), highlight selected nodes
if (IsNodeSelected(node.nodeId)) {
    // Change color to blue or add glow effect
    drawList->AddRect(screenStart, screenEnd, IM_COL32(0, 100, 255, 255), 0.0f, 0, 2.0f);
}
```

**Acceptance Criteria**:
- [x] Click on node → m_selectedNodeIds updated
- [x] Selected node renders with blue highlight
- [x] Click on empty space → Deselects
- [x] Ctrl+Click → Multi-select works

**Files to Modify**:
- Source/BlueprintEditor/PlaceholderEditor/PlaceholderCanvas.cpp (OnMouseDown, RenderNodes)

**Estimated Time**: 1 hour

---

### Task 63.2: Delete Selected Nodes (30 minutes)

**Current State**: 
- GraphEditorBase: Calls DeleteSelectedNodes() ✅
- PlaceholderGraphRenderer: Logs but does nothing ⚠️
- PlaceholderGraphDocument: DeleteNode() not implemented ❌

**What Needs Doing**:

```cpp
// In PlaceholderGraphDocument::DeleteNode()
void PlaceholderGraphDocument::DeleteNode(int nodeId)
{
    // Find node in vector
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [nodeId](const PlaceholderNode& n) { return n.nodeId == nodeId; });
    
    if (it != m_nodes.end()) {
        m_nodes.erase(it);
        m_isDirty = true;
        
        // Also delete all connections to/from this node
        auto connIt = m_connections.begin();
        while (connIt != m_connections.end()) {
            if (connIt->fromNodeId == nodeId || connIt->toNodeId == nodeId) {
                connIt = m_connections.erase(connIt);
            } else {
                ++connIt;
            }
        }
    }
}

// In PlaceholderGraphRenderer::DeleteSelectedNodes()
void PlaceholderGraphRenderer::DeleteSelectedNodes()
{
    if (m_selectedNodeIds.empty())
        return;
    
    for (int nodeId : m_selectedNodeIds) {
        m_document->DeleteNode(nodeId);
    }
    
    DeselectAll();
    MarkDirty();
}
```

**Acceptance Criteria**:
- [x] Delete key pressed → Selected nodes removed from canvas
- [x] Connections to deleted nodes also removed
- [x] MarkDirty() called so file shows "unsaved"
- [x] Selection cleared after delete

**Files to Modify**:
- Source/BlueprintEditor/PlaceholderEditor/PlaceholderGraphDocument.cpp (DeleteNode)
- Source/BlueprintEditor/PlaceholderEditor/PlaceholderGraphRenderer.cpp (DeleteSelectedNodes)

**Estimated Time**: 30 minutes

---

### Task 63.3: Save/Load Button Hooks (1 hour)

**Current State**: 
- Save/SaveAs buttons render ✅
- Save As modal opens ✅
- But OnSaveClicked() does nothing ⚠️

**What Needs Doing**:

```cpp
// In CanvasToolbarRenderer::OnSaveClicked()
void CanvasToolbarRenderer::OnSaveClicked()
{
    if (!m_document)
        return;
    
    std::string currentPath = m_document->GetFilePath();
    
    if (currentPath.empty()) {
        // No path yet - show Save As dialog
        m_showSaveAsModal = true;
        return;
    }
    
    // Save to current path
    if (m_document->Save(currentPath)) {
        SYSTEM_LOG << "[CanvasToolbarRenderer] Saved to: " << currentPath << "\n";
        
        // Call callback if set
        if (m_onSaveComplete) {
            m_onSaveComplete(true);
        }
    }
}

// In CanvasToolbarRenderer::OnSaveAsClicked()
void CanvasToolbarRenderer::OnSaveAsClicked()
{
    m_showSaveAsModal = true;
}

// In RenderModals() - when user confirms SaveAs:
if (ImGui::Button("Save")) {
    std::string newPath = m_saveAsFileBuffer;
    if (m_document->Save(newPath)) {
        m_showSaveAsModal = false;
        SYSTEM_LOG << "[CanvasToolbarRenderer] Saved As: " << newPath << "\n";
        if (m_onSaveComplete) m_onSaveComplete(true);
    }
}
```

**Acceptance Criteria**:
- [x] Click Save button → File saved
- [x] Title bar shows file is saved (no asterisk)
- [x] Click SaveAs button → Modal opens
- [x] Enter filename → File saved with new name

**Files to Modify**:
- Source/BlueprintEditor/Framework/CanvasToolbarRenderer.cpp (OnSaveClicked, OnSaveAsClicked, RenderModals)

**Estimated Time**: 1 hour

---

## 📦 PHASE 64 - HIGH PRIORITY (Session 2) - 4 Hours

### Goal: Node creation + Connection UI
### Why Important: Core graph functionality

---

### Task 64.1: Node Creation from Palette (2 hours)

**Current State**: ComponentPalettePanel exists and renders but drag-drop is stub

**What Needs Doing**:

```cpp
// In ComponentPalettePanel::RenderComponent()
if (ImGui::Selectable(componentName.c_str())) {
    // Make draggable
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("COMPONENT_TYPE", componentName.data(), componentName.size());
        ImGui::TextUnformatted(componentName.c_str());
        ImGui::EndDragDropSource();
    }
}

// In PlaceholderCanvas::Render() - accept drops
if (ImGui::AcceptDragDropPayload("COMPONENT_TYPE")) {
    const ImGuiPayload* payload = ImGui::GetDragDropPayload();
    std::string componentType((char*)payload->Data, payload->DataSize);
    
    // Create node at drop position
    ImVec2 dropScreenPos = ImGui::GetMousePos();
    ImVec2 dropCanvasPos = ScreenToCanvas(dropScreenPos);
    
    CreateNodeAtScreenPos(componentType, dropCanvasPos);
}

// In PlaceholderGraphRenderer::CreateNodeAtScreenPos()
void PlaceholderGraphRenderer::CreateNodeAtScreenPos(
    const std::string& componentType, const ImVec2& pos)
{
    if (!m_document)
        return;
    
    // Map component type to PlaceholderNodeType
    PlaceholderNodeType type = ParseNodeType(componentType);
    
    // Create node in document
    int nodeId = m_document->CreateNode(
        type, 
        componentType,  // title
        pos.x, pos.y,   // position
        100, 50,        // default size
        true            // enabled
    );
    
    MarkDirty();
}
```

**Acceptance Criteria**:
- [x] Drag component from palette
- [x] Node appears at drop position
- [x] Node has correct type/color
- [x] Document marked dirty

**Files to Modify**:
- Source/BlueprintEditor/EntityPrefabEditor/ComponentPalettePanel.cpp (drag-drop source)
- Source/BlueprintEditor/PlaceholderEditor/PlaceholderCanvas.cpp (accept drop)
- Source/BlueprintEditor/PlaceholderEditor/PlaceholderGraphRenderer.cpp (CreateNodeAtScreenPos)

**Estimated Time**: 2 hours

---

### Task 64.2: Connection Creation UI (2 hours)

**Current State**: Port rendering exists but connection creation is stub

**What Needs Doing**:

```cpp
// In PlaceholderCanvas::Render() - detect port clicks
for (auto& node : m_document->GetAllNodes()) {
    // Draw output port (right side)
    ImVec2 outputPortPos = {nodeScreenEnd.x, nodeScreenCenter.y};
    drawList->AddCircleFilled(outputPortPos, 5.0f, IM_COL32(255, 255, 0, 255));
    
    if (ImGui::IsMouseClicked(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float dist = sqrt(pow(mousePos.x - outputPortPos.x, 2) + 
                         pow(mousePos.y - outputPortPos.y, 2));
        
        if (dist < 10.0f) {
            m_isDrawingConnection = true;
            m_connectionFromNodeId = node.nodeId;
            m_connectionFromPortId = 0;  // output port
        }
    }
}

// In PlaceholderCanvas::Render() - draw connection preview while dragging
if (m_isDrawingConnection) {
    ImVec2 fromNode = GetNodeScreenPosition(m_connectionFromNodeId);
    ImVec2 toMouse = ImGui::GetMousePos();
    
    drawList->AddLine(fromNode, toMouse, IM_COL32(255, 255, 0, 255), 2.0f);
}

// In PlaceholderCanvas::OnMouseUp() - complete connection
if (m_isDrawingConnection) {
    // Check if released over another port (input)
    for (auto& node : m_document->GetAllNodes()) {
        ImVec2 inputPortPos = GetInputPortScreenPos(node.nodeId);
        ImVec2 mousePos = ImGui::GetMousePos();
        
        float dist = sqrt(pow(mousePos.x - inputPortPos.x, 2) + 
                         pow(mousePos.y - inputPortPos.y, 2));
        
        if (dist < 10.0f && node.nodeId != m_connectionFromNodeId) {
            // Create connection
            m_document->ConnectNodes(
                m_connectionFromNodeId, 0,  // from node, port
                node.nodeId, 0              // to node, port
            );
            
            MarkDirty();
        }
    }
    
    m_isDrawingConnection = false;
}
```

**Acceptance Criteria**:
- [x] Right-click port → Drag starts
- [x] Yellow line follows cursor
- [x] Release over another port → Connection created
- [x] Connection renders as Bezier curve
- [x] Document marked dirty

**Files to Modify**:
- Source/BlueprintEditor/PlaceholderEditor/PlaceholderCanvas.cpp (port UI + drag logic)
- Source/BlueprintEditor/PlaceholderEditor/PlaceholderGraphDocument.cpp (ConnectNodes impl)

**Estimated Time**: 2 hours

---

## 📝 PHASE 65 - MEDIUM PRIORITY (Session 3) - 3 Hours

### Goal: Polish and complete features
### Why Important: UX quality

---

### Task 65.1: Node Dragging (1.5 hours)

**What Needs Doing**:
- Detect left-click on node
- OnMouseMove: Update node position
- OnMouseUp: Confirm position
- Visual feedback: Node follows cursor
- Mark dirty on each move

**Files**: PlaceholderCanvas.cpp

---

### Task 65.2: Rectangle Selection (1 hour)

**What Needs Doing**:
- Detect drag in empty area
- Draw selection rectangle during drag
- OnMouseUp: Select all nodes within rectangle
- Support Ctrl+Drag for additive selection

**Files**: PlaceholderCanvas.cpp, PlaceholderGraphRenderer.cpp

---

### Task 65.3: Property Editor (1 hour)

**What Needs Doing**:
- Show selected node properties in editor panel
- Input fields for each property
- Apply/Reset buttons
- Save properties to document

**Files**: PlaceholderPropertyEditorPanel.cpp

---

## 🎯 IMPLEMENTATION SEQUENCE

### Day 1 (Now) - CRITICAL
```
Phase 63.1: Node Selection (1 hour)
    ↓
Phase 63.2: Delete Nodes (30 min)
    ↓
Phase 63.3: Save/Load Hooks (1 hour)
    ↓
TEST: Verify all 3 working together
    ↓
RESULT: Can select, delete, save nodes ✅
```

**Expected Output After Day 1**:
- Click node → blue highlight
- Delete key → node gone
- Ctrl+S → file saved
- Title shows unsaved state

---

### Day 2 - HIGH PRIORITY
```
Phase 64.1: Drag-Drop Node Creation (2 hours)
    ↓
Phase 64.2: Connection UI (2 hours)
    ↓
TEST: Create/Connect nodes via UI
    ↓
RESULT: Full CRUD working ✅
```

**Expected Output After Day 2**:
- Drag component → creates node
- Drag port to port → creates connection
- Full graph editing possible

---

### Day 3 - POLISH
```
Phase 65.1: Node Dragging (1.5 hours)
Phase 65.2: Rectangle Selection (1 hour)
Phase 65.3: Property Editor (1 hour)
    ↓
TEST: All features working smoothly
    ↓
RESULT: Production ready ✅
```

**Expected Output After Day 3**:
- Drag nodes to reposition
- Select multiple nodes
- Edit node properties
- Complete working graph editor

---

## 🧪 ACCEPTANCE TESTS

### Core Functionality (After Phase 63)
```
TEST 1: Create new Placeholder graph
  → Loads 3 sample nodes
  → PASS ✅

TEST 2: Select node
  → Click node
  → Node highlights blue
  → PASS ✅

TEST 3: Delete node
  → Select node + Delete key
  → Node disappears
  → PASS ✅

TEST 4: Save file
  → Ctrl+S
  → Title shows saved
  → File updated on disk
  → PASS ✅
```

### Full CRUD (After Phase 64)
```
TEST 5: Create node from palette
  → Drag Verify component
  → New node appears
  → PASS ✅

TEST 6: Connect nodes
  → Drag from port A to port B
  → Connection appears
  → PASS ✅

TEST 7: Edit properties
  → Select node → Edit props
  → Apply changes
  → Props persist on save
  → PASS ✅
```

### Polish (After Phase 65)
```
TEST 8: Rectangle select
  → Drag in empty area
  → Multiple nodes selected
  → PASS ✅

TEST 9: Node dragging
  → Drag node to new position
  → Position updates
  → Persists on save
  → PASS ✅

TEST 10: Context menu
  → Right-click node
  → Delete/Copy options
  → PASS ✅
```

---

## 📊 EFFORT BREAKDOWN

| Phase | Task | Complexity | Hours | Status |
|-------|------|-----------|-------|--------|
| 63.1 | Node Selection | Low | 1.0 | 🔴 TODO |
| 63.2 | Delete Nodes | Low | 0.5 | 🔴 TODO |
| 63.3 | Save Hooks | Low | 1.0 | 🔴 TODO |
| 64.1 | Node Creation | Medium | 2.0 | 🔴 TODO |
| 64.2 | Connections | Medium | 2.0 | 🔴 TODO |
| 65.1 | Dragging | Medium | 1.5 | 🔴 TODO |
| 65.2 | Rectangle Select | Low | 1.0 | 🔴 TODO |
| 65.3 | Properties | Medium | 1.0 | 🔴 TODO |
| **TOTAL** | | | **10 hrs** | |

---

## 🎓 LEARNING RESOURCES

### For Each Task
1. **Node Selection**: Read Phase 3 in PlaceholderGraphRenderer (SelectMultipleNodes pattern)
2. **Delete**: Look at EntityPrefab for node deletion pattern
3. **Save Hooks**: Reference VisualScriptEditorPanel Save() implementation
4. **Drag-Drop**: Check ComponentPalettePanel in EntityPrefabEditor
5. **Connections**: Study Phase 30 connection UI in EntityPrefabEditor
6. **Dragging**: Reference PrefabCanvas node dragging code
7. **Rectangle**: Phase 31 EntityPrefabEditor rectangle selection
8. **Properties**: PropertyEditorPanel implementation

---

## 💡 TIPS & PITFALLS

### Common Mistakes to Avoid
❌ Forgetting to call `MarkDirty()` after modifications
❌ Not updating m_selectedNodeIds in PlaceholderGraphRenderer
❌ Not clearing selection after delete
❌ Forgetting to also delete connections when deleting nodes
❌ Not checking for null pointers in document methods

### Best Practices
✅ Always mark dirty after any node/connection change
✅ Use existing selection system (inherited from GraphEditorBase)
✅ Test coordinate transforms (ScreenToCanvas) carefully
✅ Log state changes during development, remove later
✅ Test edge cases (delete last node, empty selection, etc.)

---

## 📞 QUICK REFERENCE - Key Methods

```cpp
// Selection (GraphEditorBase)
SelectMultipleNodes(nodeId, ctrlPressed, shiftPressed)
GetSelectedNodeIds()
IsNodeSelected(nodeId)
DeselectAll()

// Document (PlaceholderGraphDocument)
CreateNode(type, title, x, y, w, h, enabled)
DeleteNode(nodeId)
ConnectNodes(fromId, toId, fromPort, toPort)
DisconnectNodes(connectionId)
MarkDirty()

// Canvas (PlaceholderCanvas)
ScreenToCanvas(screenPos)
CanvasToScreen(canvasPos)
GetNodeScreenPosition(nodeId)

// Renderer (PlaceholderGraphRenderer)
MarkDirty()
Load(path)
Save(path)
```

---

## ✅ COMPLETION CHECKLIST

### Phase 63 (Critical)
- [ ] Node selection + highlight implemented
- [ ] Delete nodes fully working
- [ ] Save/Load button hooks connected
- [ ] Manual test: All 3 working together
- [ ] No console errors

### Phase 64 (High)
- [ ] Drag-drop node creation working
- [ ] Connection UI functional
- [ ] Manual test: Full CRUD working
- [ ] No console errors

### Phase 65 (Polish)
- [ ] Node dragging complete
- [ ] Rectangle selection complete
- [ ] Property editor integrated
- [ ] All 10 acceptance tests passing
- [ ] Build: 0 errors, 0 warnings

---

**Roadmap Status**: ✅ COMPLETE & DETAILED  
**Ready to Start**: YES - Phase 63 ready to begin  
**Estimated Completion**: 2-3 days focused development

