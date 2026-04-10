# Phase 38: BehaviorTree Dynamic Execution Pins (Y-Axis Ordered Visualization)

## 🎯 Objective
Add **visual execution order indicators** to BT Sequence/Selector nodes using **vertical (Y-axis) positioning**:
- ✅ Top-to-bottom visual flow: highest node = first execution
- ✅ Auto-generate indices (1, 2, 3...n) on connection lines to branches
- ✅ Drag nodes vertically to reorder execution dynamically
- ✅ NO additional metadata storage (childIds order remains authority)

## 🔑 Key Innovation
**Position-based execution order** = Users see what they get:
- Visual: Child nodes from top→bottom
- Logic: childIds order = vertical sort by Y coordinate
- Rendering: Index labels on connection lines (e.g., "1", "2", "3")

---

## 📊 Analysis: VisualScript Sequence Node Architecture

### 1. **Data Structure (TaskNodeDefinition)**
```cpp
struct TaskNodeDefinition {
    // ...
    std::vector<std::string> DynamicExecOutputPins;  // Phase 24.0 (L153)
    // For VSSequence: stores pin names like "Out", "Out_1", "Out_2", ...
    // Dynamic pins = DynamicExecOutputPins (user-added at runtime)
    // Static pins  = GetExecOutputPins() base list (hardcoded "Out")
};
```

### 2. **Static vs Dynamic Pins**
- **Static pins** (immutable): `"Out"` (always present)
- **Dynamic pins** (user-managed): `"Out_1"`, `"Out_2"`, ... (added/removed via UI buttons)
- **Separation**: `numStaticPins = execOutputPins.size() - DynamicExecOutputPins.size()`

### 3. **UI Rendering** (VisualScriptNodeRenderer.cpp)
```cpp
// [+] button for adding pins (line 687)
if (hasDynamicPins && onAddPin) {
    if (ImGui::SmallButton("[+]"))
        onAddPin(nodeID, onAddPinUserData);
}

// [-] button for removing pins (line 643)
for (size_t i = 0; i < execOutputPins.size(); ++i) {
    if (hasDynamicPins && i >= numStaticPins && onRemovePin) {
        if (ImGui::SmallButton("[-]"))
            onRemovePin(nodeID, dynIdx, onRemovePinUserData);
    }
}
```

### 4. **Pin Naming Scheme**
- VSSequence: `"Out"`, `"Out_1"`, `"Out_2"`, ... (numbered after static pin)
- Switch: `"Case_0"`, `"Case_1"`, `"Case_2"`, ... (plus structured metadata via `switchCases[]`)

### 5. **Commands (Undo/Redo)**
- **AddDynamicPinCommand**: Adds pin to node's DynamicExecOutputPins
- **RemoveDynamicPinCommand**: Removes pin by index
- Both trigger UID regeneration and graph serialization

### 6. **JSON Persistence** (TaskGraphTemplate v4)
```json
{
  "nodes": [
    {
      "id": 4,
      "type": "VSSequence",
      "execOut": {
        "Out": [5],
        "Out_1": [6],
        "Out_2": [7]
      },
      "DynamicExecOutputPins": ["Out_1", "Out_2"]
    }
  ]
}
```

---

## 🎲 Current BehaviorTree Architecture

### 1. **Data Structure (BTNode)**
```cpp
struct BTNode {
    BTNodeType type = BTNodeType::Action;
    uint32_t id = 0;
    
    // For composite nodes (Selector, Sequence)
    std::vector<uint32_t> childIds;  // ← Children stored here
    
    // NO DynamicExecOutputPins equivalent!
    // ❌ Missing: Named execution ports
    // ❌ Missing: Reorderable branch labels
};
```

### 2. **Runtime Execution**
```cpp
// Sequence: executes children in order (all must succeed)
for (uint32_t childId : node.childIds) {
    BTStatus result = ExecuteBTNode(GetNode(childId), ...);
    if (result == BTStatus::Failure) return BTStatus::Failure;
}
return BTStatus::Success;

// Selector: executes children in order (first success wins)
for (uint32_t childId : node.childIds) {
    BTStatus result = ExecuteBTNode(GetNode(childId), ...);
    if (result == BTStatus::Success) return BTStatus::Success;
}
return BTStatus::Failure;
```

### 3. **Editor Gaps**
- ❌ No UI to visualize branch order
- ❌ No add/remove branch buttons
- ❌ No reorderable branch labels
- ❌ No branch naming/custom labels (like Switch cases)

---

## 🔧 Integration Strategy: BT Dynamic Execution Pins

### Phase 38.A: Data Model Enhancement

**Extend BTNode with dynamic branch metadata:**
```cpp
struct BTNode {
    // ...existing fields...
    
    // NEW (Phase 38): Dynamic branch management for Sequence/Selector
    std::vector<std::string> BranchLabels;  // Custom labels per child (e.g., ["Check Health", "Attack", "Retreat"])
    
    // Helper: Get label for child at index
    std::string GetBranchLabel(size_t childIndex) const {
        if (childIndex < BranchLabels.size() && !BranchLabels[childIndex].empty())
            return BranchLabels[childIndex];
        return "Branch_" + std::to_string(childIndex);
    }
    
    // Helper: Set label for child at index
    void SetBranchLabel(size_t childIndex, const std::string& label) {
        if (childIndex >= BranchLabels.size())
            BranchLabels.resize(childIndex + 1);
        BranchLabels[childIndex] = label;
    }
};
```

### Phase 38.B: Execution Pin Management

**Map childIds to execution ports (similar to VSSequence):**
```
BT Sequence/Selector Node:
┌─────────────────────────────────┐
│ Type: Sequence                  │
├─────────────────────────────────┤
│ IN: Entry                       │
│ OUT: Branch_0 → childIds[0]     │  (e.g., "Check Health")
│      Branch_1 → childIds[1]     │  (e.g., "Attack")
│      Branch_2 → childIds[2]     │  (e.g., "Retreat")
│ [+] to add new branch           │
└─────────────────────────────────┘
```

**Static vs Dynamic branching:**
- ✅ **Static branches**: childIds vector (core execution paths)
- ✅ **Dynamic UI**: BranchLabels for naming/reordering UI (not data, but metadata)
- ✅ **Reordering**: Modify childIds order → triggers re-render + serialization

### Phase 38.C: Editor UI Rendering

**In BTNodeRenderer (analogous to VisualScriptNodeRenderer):**
```cpp
// Render Sequence/Selector node branches
if (node.type == BTNodeType::Sequence || node.type == BTNodeType::Selector) {
    for (size_t i = 0; i < node.childIds.size(); ++i) {
        std::string branchLabel = node.GetBranchLabel(i);
        
        // Render branch port
        ImGui::Text("%s", branchLabel.c_str());
        ImGui::SameLine();
        
        // [-] button to remove branch
        if (ImGui::SmallButton("[-]##remove_branch_" + std::to_string(i))) {
            RemoveBranchCommand cmd(node.id, i);
            cmd.Execute(behaviorTree);
        }
        
        // Drag target to reorder
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BT_BRANCH")) {
                int sourceIdx = *(int*)payload->Data;
                ReorderBranchCommand cmd(node.id, sourceIdx, i);
                cmd.Execute(behaviorTree);
            }
            ImGui::EndDragDropTarget();
        }
    }
    
    // [+] button to add branch
    if (ImGui::SmallButton("[+]##add_branch")) {
        AddBranchCommand cmd(node.id);
        cmd.Execute(behaviorTree);
    }
}
```

### Phase 38.D: JSON Persistence

**BehaviorTree JSON schema (v2):**
```json
{
  "nodes": [
    {
      "id": 3,
      "type": "Sequence",
      "name": "Combat Sequence",
      "childIds": [4, 5, 6],
      "branchLabels": ["Check Health", "Attack", "Retreat"],
      "position": {"x": 100.0, "y": 150.0}
    },
    {
      "id": 4,
      "type": "Action",
      "actionType": "CheckHealth",
      "name": "Health Check"
    }
  ]
}
```

---

## 🏗️ Implementation Roadmap

### Step 1: Extend BTNode Data Structure
- Add `BranchLabels` vector + helper methods
- Update JSON schema version to v2

### Step 2: Commands for Dynamic Branch Management
- **AddBranchCommand**: Add child + auto-generate label
- **RemoveBranchCommand**: Remove child at index
- **ReorderBranchCommand**: Swap child order (drag-drop)
- **SetBranchLabelCommand**: Edit branch name

### Step 3: Serialization
- Update TaskGraphLoader to parse `branchLabels`
- Update BehaviorTree::SaveToFile() to persist labels
- Add migration logic for old BT files (v1 → v2)

### Step 4: Editor UI Integration
- Extend BTNodeRenderer with branch visualization
- Add [+] [-] buttons per branch
- Add drag-drop reordering support
- Add property panel for branch label editing

### Step 5: Property Panel
- New tab: "Branch Manager" in BTNodePropertyPanel
- List branches with reorder handles
- Inline edit for branch labels
- Live preview in canvas

---

## 📋 Comparison: VisualScript vs BehaviorTree

| Feature | VisualScript Sequence | BT Sequence/Selector | Status |
|---------|----------------------|----------------------|--------|
| Static pins | "Out" (hardcoded) | childIds core structure | ✅ Exists |
| Dynamic pins | DynamicExecOutputPins[] | BranchLabels[] | 🔧 New |
| Pin naming | "Out", "Out_1", "Out_2" | "Branch_0", "Check Health" | 🔧 New |
| UI [+] button | ✅ Implemented | ❌ Missing | 🔧 To add |
| UI [-] button | ✅ Implemented | ❌ Missing | 🔧 To add |
| Reordering | Not in VisualScript | 🔧 Drag-drop for BT | 🔧 New |
| Undo/Redo | ✅ Commands exist | ⚠️ Partial | 🔧 Extend |
| JSON persistence | ✅ DynamicExecOutputPins | ❌ No branchLabels | 🔧 New |

---

## 🎨 UX Flow: End User Perspective

### Scenario: Creating a Combat Sequence

1. **Create node**: Right-click canvas → "New Sequence" → node appears
   - Default: 1 child (empty placeholder)

2. **Add branches**: Click [+] button below node
   - Adds new branch: "Branch_1"
   - Auto-creates new child Action node

3. **Label branches**: Click branch label
   - Opens inline editor
   - Sets "Check Health", "Attack", "Retreat"
   - Labels show in canvas

4. **Reorder branches**: Drag branch label upward/downward
   - Reorders childIds array
   - Execution order changes immediately
   - Connections follow child reorder

5. **Remove branches**: Click [-] on unwanted branch
   - Removes child + connection
   - Auto-validates tree

6. **Save**: Ctrl+S saves to JSON
   - branchLabels persisted
   - childIds order preserved
   - Tree can be reopened with exact layout

---

## ⚙️ Technical Considerations

### A. Coordinate with Existing Systems
- **UndoRedoStack**: Already supports AddDynamicPinCommand pattern → reuse for branches
- **JSON Loader**: TaskGraphLoader has v4 parser → extend for BT v2
- **BTNodeRegistry**: Has all BT node types → add branch label support

### B. Runtime vs Editor
- **Runtime**: childIds order is THE source of truth (no changes needed)
- **Editor**: BranchLabels metadata only (runtime ignores them)
- **Serialization**: Both are persisted for round-trip fidelity

### C. Backward Compatibility
- BT v1 files load without error (branchLabels default to empty)
- Auto-generate labels on load: "Branch_0", "Branch_1", ...
- Save as v2 on edit

### D. Performance
- **Rendering**: One label + two buttons per branch (negligible)
- **Reordering**: O(n) vector swap (very fast)
- **Persistence**: JSON array serialization (standard)

---

## 📁 Files to Modify

1. **Source/AI/BehaviorTree.h**
   - Extend BTNode with BranchLabels + helpers

2. **Source/AI/BehaviorTree.cpp**
   - Implement label helpers
   - Update SaveToFile() for branchLabels

3. **Source/NodeGraphShared/BehaviorTreeAdapter.h**
   - Add commands: AddBranchCommand, RemoveBranchCommand, ReorderBranchCommand

4. **Source/BlueprintEditor/BTNodeRenderer.cpp** (NEW/EXTEND)
   - Render branch labels + [+] [-] buttons
   - Add drag-drop reordering

5. **Source/BlueprintEditor/BTNodePropertyPanel.cpp**
   - New "Branch Manager" tab
   - Inline label editing + reordering

6. **Source/BlueprintEditor/BehaviorTreeRenderer.cpp**
   - Hook command execution for branch operations
   - Update dirty flag on branch changes

7. **Source/AI/BehaviorTreeLoader.cpp** (if exists) or TaskGraphLoader
   - Parse branchLabels from JSON v2
   - Migrate v1 → v2 on load

---

## 🚀 Next Steps

1. **Prototype Phase 38.A**: Modify BTNode structure, test serialization
2. **Implement Phase 38.B-C**: Commands + UI buttons
3. **Test Phase 38.D**: JSON round-trip + backward compatibility
4. **Polish**: Property panel + drag-drop reordering
5. **Integration**: Full editor workflow (add/remove/reorder/label/save)

---

## 📌 Key Takeaway

**The VisualScript DynamicExecOutputPins pattern is DIRECTLY APPLICABLE to BehaviorTree branches**, with one key difference:

- **VisualScript**: Pins are the execution ports (abstract)
- **BehaviorTree**: Branches ARE the children (concrete, with IDs)

We just need to **add metadata (BranchLabels) + UI controls** to make branches as reorderable and labeled as VisualScript pins. 🎯

