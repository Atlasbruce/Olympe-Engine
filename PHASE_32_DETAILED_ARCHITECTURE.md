# 📋 PHASE 32 - DETAILED ARCHITECTURE GUIDE

**Status:** 📋 À faire  
**Est. Duration:** 2-3 jours  
**Priority:** 🔴 HAUTE

---

## 🎯 OBJECTIF GLOBAL

Rendre l'EntityPrefab Editor **production-ready** avec:
1. ✅ Copy/Paste de nœuds et subgraphes
2. ✅ Undo/Redo system complet
3. ✅ Node organization (comments, groups, minimap)
4. ✅ Validation automatique des prefabs
5. ✅ Export to binary + runtime loader

---

## 📐 ARCHITECTURE GLOBALE - PHASE 32

```
PrefabCanvas (existing UI)
    │
    ├─ ClipboardManager (NEW - 32.1)
    │   ├── Copy nodes to clipboard
    │   ├── Paste with ID reassignment
    │   └── Serialize/deserialize
    │
    ├─ CommandHistory (NEW - 32.2)
    │   ├── m_undoStack: ICommand*
    │   ├── m_redoStack: ICommand*
    │   ├── ExecuteCommand()
    │   ├── Undo() / Redo()
    │   └── Command logging
    │
    ├─ NodeOrganization (NEW - 32.3)
    │   ├── CommentNodes[]
    │   ├── GroupNodes[]
    │   ├── Minimap renderer
    │   └── Alignment helpers
    │
    ├─ PrefabValidator (NEW - 32.4)
    │   ├── ValidateStructure()
    │   ├── CheckComponentTypes()
    │   ├── CheckConnections()
    │   └── GenerateReport()
    │
    └─ PrefabExporter (NEW - 32.5)
        ├── ExportToJSON()
        ├── ExportToBinary()
        └── CreateRuntimeFormat()
```

---

## 🔄 PHASE 32.1: COPY/PASTE

### Architecture

```
ClipboardManager (Singleton)
├── Copy(nodeIds, connections) → serialize to m_clipboard
├── Paste(targetPos) → deserialize, reassign IDs
├── GetClipboard() → ClipboardData
└── Clear()

ClipboardData
├── std::vector<ComponentNode> nodes
├── std::vector<Connection> connections
└── Vector referencePosition  // For offset calculation

Integration Points:
├── PrefabCanvas::OnKeyDown()
│   └── Ctrl+C → ClipboardManager::Copy(selectedNodes)
│   └── Ctrl+V → ClipboardManager::Paste(mousePos)
│
└── EntityPrefabGraphDocument
    └── All node IDs automatically remapped on paste
```

### Files to Create

**NEW: `Source/BlueprintEditor/Utilities/ClipboardManager.h`**
```cpp
namespace Olympe {
    struct ClipboardData {
        std::vector<ComponentNode> nodes;
        std::vector<std::pair<NodeId, NodeId>> connections;
        Vector referencePosition;  // For offset calc
    };

    class ClipboardManager {
    public:
        static ClipboardManager& Get();

        // Copy selected nodes to clipboard
        void Copy(const std::vector<NodeId>& nodeIds,
                 const std::vector<std::pair<NodeId, NodeId>>& connections);

        // Paste nodes at screen position (handles offset + ID remapping)
        std::vector<NodeId> Paste(EntityPrefabGraphDocument* doc,
                                 const Vector& pastePosition);

        bool HasData() const;
        void Clear();

    private:
        ClipboardData m_clipboard;
    };
}
```

**NEW: `Source/BlueprintEditor/Utilities/ClipboardManager.cpp`**
```cpp
void ClipboardManager::Copy(const std::vector<NodeId>& nodeIds,
                           const std::vector<std::pair<NodeId, NodeId>>& connections) {
    m_clipboard.nodes.clear();
    m_clipboard.connections.clear();

    // Copy selected nodes
    for (NodeId id : nodeIds) {
        // Get node data from document and copy
        m_clipboard.nodes.push_back(getNodeData(id));
    }

    // Copy only connections BETWEEN selected nodes
    for (auto& conn : connections) {
        if (nodeIds.contains(conn.first) && nodeIds.contains(conn.second)) {
            m_clipboard.connections.push_back(conn);
        }
    }

    // Store first node position as reference (for offset calculation)
    if (!m_clipboard.nodes.empty()) {
        m_clipboard.referencePosition = m_clipboard.nodes[0].position;
    }
}

std::vector<NodeId> ClipboardManager::Paste(EntityPrefabGraphDocument* doc,
                                           const Vector& pastePosition) {
    if (!HasData()) return {};

    std::vector<NodeId> newNodeIds;
    std::unordered_map<NodeId, NodeId> idMap;  // Old ID → New ID

    // Calculate offset from reference position to paste position
    Vector offset = pastePosition - m_clipboard.referencePosition;

    // Create new nodes with offset
    for (const auto& node : m_clipboard.nodes) {
        ComponentNode newNode = node;
        newNode.position = node.position + offset;

        // Assign new ID
        NodeId newId = doc->CreateComponentNode(node.componentType, node.componentName);
        newNode.nodeId = newId;

        // Store mapping for connection remapping
        idMap[node.nodeId] = newId;
        newNodeIds.push_back(newId);
    }

    // Create connections with remapped IDs
    for (const auto& conn : m_clipboard.connections) {
        NodeId newSource = idMap[conn.first];
        NodeId newTarget = idMap[conn.second];
        doc->ConnectNodes(newSource, newTarget);
    }

    return newNodeIds;
}
```

### Files to Modify

**`PrefabCanvas.cpp`** - OnKeyDown()
```cpp
void PrefabCanvas::OnKeyDown(int keyCode) {
    // ... existing code ...

    if (m_ctrlPressed) {
        if (keyCode == 67) {  // Ctrl+C
            const auto& selected = m_document->GetSelectedNodes();
            const auto& connections = m_document->GetConnections();
            ClipboardManager::Get().Copy(selected, connections);
            SYSTEM_LOG << "[PrefabCanvas] Copied " << selected.size() << " nodes\n";
            return;
        }
        else if (keyCode == 86) {  // Ctrl+V
            Vector pastePos = ScreenToCanvas(m_currentMousePos.x, m_currentMousePos.y);
            auto newIds = ClipboardManager::Get().Paste(m_document, pastePos);

            // Select pasted nodes
            m_document->DeselectAll();
            for (NodeId id : newIds) {
                m_document->SelectNode(id);
            }
            SYSTEM_LOG << "[PrefabCanvas] Pasted " << newIds.size() << " nodes\n";
            return;
        }
    }
}
```

### Testing Checklist

- [ ] Copy single node → Paste → Verify new ID, position offset
- [ ] Copy multiple nodes → Paste → Verify all IDs unique, positions offset
- [ ] Copy nodes with connections → Paste → Verify internal connections preserved
- [ ] Paste multiple times → Verify no ID conflicts
- [ ] Copy, undo, paste again → Works correctly
- [ ] Clipboard persists across different files (session scope)

---

## 🔙 PHASE 32.2: UNDO/REDO

### Architecture - Command Pattern

```
ICommand (Abstract Base)
    ├── CreateNodeCommand
    ├── DeleteNodeCommand
    ├── ConnectNodesCommand
    ├── DisconnectNodesCommand
    ├── MoveNodeCommand
    ├── ModifyPropertyCommand
    └── (others as needed)

CommandHistory (Manager)
    ├── m_undoStack: std::vector<ICommand*>
    ├── m_redoStack: std::vector<ICommand*>
    ├── ExecuteCommand(ICommand*) → stack undo
    ├── Undo() → pop undo, push redo
    ├── Redo() → pop redo, push undo
    └── Clear() → cleanup

Integration
    └── EntityPrefabGraphDocument
        └── Wraps all modifications in commands
```

### Files to Create

**NEW: `Source/BlueprintEditor/Utilities/ICommand.h`**
```cpp
namespace Olympe {
    class ICommand {
    public:
        virtual ~ICommand() = default;
        virtual void Execute() = 0;
        virtual void Undo() = 0;
        virtual std::string GetDescription() const = 0;
    };
}
```

**NEW: `Source/BlueprintEditor/Utilities/CommandHistory.h`**
```cpp
namespace Olympe {
    class CommandHistory {
    public:
        void ExecuteCommand(ICommand* command);
        void Undo();
        void Redo();
        bool CanUndo() const;
        bool CanRedo() const;
        void Clear();

    private:
        std::vector<ICommand*> m_undoStack;
        std::vector<ICommand*> m_redoStack;
        static const int MAX_HISTORY = 100;
    };
}
```

**NEW: `Source/BlueprintEditor/EntityPrefabEditor/Commands/`**

Create following files in new `Commands/` directory:
- `CreateNodeCommand.h/cpp`
- `DeleteNodeCommand.h/cpp`
- `ConnectNodesCommand.h/cpp`
- `DisconnectNodesCommand.h/cpp`
- `MoveNodeCommand.h/cpp`
- `ModifyPropertyCommand.h/cpp`

**Example: `MoveNodeCommand.h`**
```cpp
namespace Olympe {
    class MoveNodeCommand : public ICommand {
    public:
        MoveNodeCommand(EntityPrefabGraphDocument* doc,
                       NodeId nodeId,
                       const Vector& oldPos,
                       const Vector& newPos)
            : m_doc(doc), m_nodeId(nodeId), m_oldPos(oldPos), m_newPos(newPos) {}

        void Execute() override {
            if (auto node = m_doc->GetNode(m_nodeId)) {
                node->position = m_newPos;
            }
        }

        void Undo() override {
            if (auto node = m_doc->GetNode(m_nodeId)) {
                node->position = m_oldPos;
            }
        }

        std::string GetDescription() const override {
            return "Move Node";
        }

    private:
        EntityPrefabGraphDocument* m_doc;
        NodeId m_nodeId;
        Vector m_oldPos, m_newPos;
    };
}
```

### Files to Modify

**`EntityPrefabGraphDocument.h`**
```cpp
class EntityPrefabGraphDocument {
private:
    std::unique_ptr<CommandHistory> m_commandHistory;  // NEW
};
```

**`PrefabCanvas.cpp`** - OnMouseUp() for node drag
```cpp
void PrefabCanvas::OnMouseUp(int button, float x, float y) {
    // ... existing drag end code ...

    if (m_interactionMode == CanvasInteractionMode::DraggingNode) {
        // OLD: Just update position
        // NEW: Wrap in command for undo

        for (NodeId id : m_document->GetSelectedNodes()) {
            ComponentNode* node = m_document->GetNode(id);
            if (node && node->position != m_nodeOldPosition) {
                auto cmd = new MoveNodeCommand(m_document, id, m_nodeOldPosition, node->position);
                m_commandHistory->ExecuteCommand(cmd);
            }
        }
    }
}
```

**`PrefabCanvas.cpp`** - OnKeyDown()
```cpp
void PrefabCanvas::OnKeyDown(int keyCode) {
    // ... existing code ...

    if (keyCode == 90 && m_ctrlPressed) {  // Ctrl+Z
        m_commandHistory->Undo();
        return;
    }
    else if (keyCode == 89 && m_ctrlPressed) {  // Ctrl+Y
        m_commandHistory->Redo();
        return;
    }
}
```

### Testing Checklist

- [ ] Move node → Ctrl+Z → Node returns to old position
- [ ] Undo multiple moves → Stack unwinds correctly
- [ ] Undo, then make new change → Redo stack clears
- [ ] Undo all the way → Nothing breaks
- [ ] Create node → Delete node → Undo delete → Node restored
- [ ] Max history (100 commands) → Oldest discarded

---

## 🎨 PHASE 32.3: NODE ORGANIZATION

### Features to Implement

#### 3a. Comment Nodes
```
CommentNode
├── text: std::string
├── position: Vector
├── size: Vector (250x150 default)
├── color: ImVec4 (yellow by default)
└── isMinimized: bool
```

**Key Points:**
- No component logic (purely decorative)
- Double-click to edit text
- Right-click → Change color
- Delete key to remove

#### 3b. Node Grouping & Alignment
- Ctrl+G: Visual grouping (bounding box around nodes)
- Align Left/Center/Right: Horizontal alignment
- Align Top/Middle/Bottom: Vertical alignment
- Distribute H/V: Even spacing

#### 3c. Minimap
```
Minimap
├── Shows entire graph view (small thumbnail)
├── Rectangle = current viewport
├── Click to pan to location
├── Toggle with button
└── Auto-update on changes
```

### Files to Create

**NEW: `Source/BlueprintEditor/EntityPrefabEditor/CommentNode.h`**
```cpp
struct CommentNode {
    NodeId nodeId;
    std::string text;
    Vector position;
    Vector size;
    ImVec4 color;
    bool isMinimized;
};
```

**NEW: `Source/BlueprintEditor/EntityPrefabEditor/Minimap.h`**
```cpp
class Minimap {
public:
    void Render(ImDrawList* drawList, 
               const PrefabCanvas* canvas,
               const EntityPrefabGraphDocument* doc);

    bool IsHovered() const;
    ImVec2 GetClickPosition() const;

private:
    ImVec2 m_minimapPos;
    ImVec2 m_minimapSize;
};
```

### Integration Points

**`PrefabCanvas.h`**
```cpp
class PrefabCanvas {
private:
    std::vector<CommentNode> m_comments;
    std::unique_ptr<Minimap> m_minimap;
    bool m_showMinimap;
};
```

**`PrefabCanvas::Render()`**
```cpp
// Add to rendering loop
RenderComments();
if (m_showMinimap) {
    m_minimap->Render(ImGui::GetWindowDrawList(), this, m_document);
}
```

---

## ✅ PHASE 32.4: VALIDATION

### Validation Rules

```
struct ValidationReport {
    bool isValid;
    std::vector<ValidationError> errors;
    std::vector<ValidationWarning> warnings;
    std::vector<ValidationInfo> infos;
};

ValidationError
├── nodeId: NodeId (if applicable)
├── message: std::string
└── severity: ErrorLevel (Critical, Warning, Info)
```

**Rules to Check:**
1. All nodes have valid componentType (registered in registry)
2. No duplicate node IDs
3. All connections point to existing nodes
4. No circular dependencies (if tree structure required)
5. Required components present (Identity, Position)
6. Parameter types match schema
7. No orphaned nodes (disconnected from main graph)

### Files to Create

**NEW: `Source/BlueprintEditor/EntityPrefabEditor/PrefabValidator.h`**
```cpp
class PrefabValidator {
public:
    ValidationReport Validate(const EntityPrefabGraphDocument* doc);

private:
    void CheckNodeTypes(const EntityPrefabGraphDocument* doc);
    void CheckConnections(const EntityPrefabGraphDocument* doc);
    void CheckRequiredComponents(const EntityPrefabGraphDocument* doc);
    void CheckParameters(const EntityPrefabGraphDocument* doc);

    ValidationReport m_report;
};
```

### Integration

**`EntityPrefabRenderer.cpp`** - RenderValidationPanel()
```cpp
void EntityPrefabRenderer::RenderValidationPanel() {
    ImGui::Begin("Validation");

    if (ImGui::Button("Validate")) {
        PrefabValidator validator;
        m_validationReport = validator.Validate(m_document);
    }

    // Display errors with red background
    // Display warnings with yellow background
    // Display infos with blue background

    ImGui::End();
}
```

---

## 💾 PHASE 32.5: EXPORT & RUNTIME

### Binary Format Specification

```
PrefabFile Format (.prefab)
├── Header (16 bytes)
│   ├── Magic: "PFAB" (4 bytes) = 0x50464142
│   ├── Version: uint16 = 0x0100
│   ├── NodeCount: uint32
│   └── ConnectionCount: uint32
│
├── Nodes Array
│   ├── For each node:
│   │   ├── NodeID: uint32
│   │   ├── ComponentType length: uint16
│   │   ├── ComponentType: string (variable)
│   │   ├── ComponentName length: uint16
│   │   ├── ComponentName: string
│   │   ├── Position: Vector (x, y, z = 3×float)
│   │   ├── Size: Vector
│   │   ├── Enabled: bool
│   │   ├── ParameterCount: uint16
│   │   └── For each parameter:
│   │       ├── Key length: uint16
│   │       ├── Key: string
│   │       └── Value (serialized based on type)
│   └── (repeat for all nodes)
│
└── Connections Array
    └── For each connection:
        ├── SourceNodeID: uint32
        └── TargetNodeID: uint32
```

### Files to Create

**NEW: `Source/PrefabExporter.h`**
```cpp
class PrefabExporter {
public:
    static bool ExportToJSON(const EntityPrefabGraphDocument* doc,
                            const std::string& filepath);

    static bool ExportToBinary(const EntityPrefabGraphDocument* doc,
                              const std::string& filepath);
};
```

**NEW: `Source/PrefabRuntimeLoader.h`**
```cpp
class PrefabRuntimeLoader {
public:
    static bool LoadFromJSON(const std::string& filepath,
                            EntityPrefabGraphDocument* outDoc);

    static bool LoadFromBinary(const std::string& filepath,
                              EntityPrefabGraphDocument* outDoc);
};
```

### Integration with PrefabFactory

**`PrefabFactory.h`** - Add methods:
```cpp
public:
    EntityID CreateFromPrefabFile(const std::string& filepath,
                                const LevelInstanceParameters& params = {});

private:
    PrefabRuntimeLoader m_loader;
```

---

## 📋 IMPLEMENTATION CHECKLIST - PHASE 32

### Phase 32.1 (Copy/Paste)
- [ ] Create ClipboardManager singleton
- [ ] Implement Copy() with node serialization
- [ ] Implement Paste() with ID remapping
- [ ] Add Ctrl+C/V handlers to PrefabCanvas
- [ ] Test copy/paste single and multiple nodes
- [ ] Test copy/paste with connections
- [ ] Commit with message: "Phase 32.1: Copy/Paste nodes (Ctrl+C/V)"

### Phase 32.2 (Undo/Redo)
- [ ] Create ICommand base class
- [ ] Create CommandHistory manager
- [ ] Implement MoveNodeCommand
- [ ] Implement CreateNodeCommand
- [ ] Implement DeleteNodeCommand
- [ ] Implement ConnectNodesCommand
- [ ] Add Ctrl+Z/Y handlers
- [ ] Test undo/redo operations
- [ ] Test max history limit (100 commands)
- [ ] Commit with message: "Phase 32.2: Undo/Redo system (Ctrl+Z/Y)"

### Phase 32.3 (Organization)
- [ ] Create CommentNode struct
- [ ] Implement comment rendering and editing
- [ ] Create Minimap class
- [ ] Implement minimap rendering
- [ ] Add alignment/distribution commands
- [ ] Test all organization features
- [ ] Commit with message: "Phase 32.3: Node organization (comments, minimap, alignment)"

### Phase 32.4 (Validation)
- [ ] Create PrefabValidator class
- [ ] Implement validation rules (8+ checks)
- [ ] Create ValidationReport struct
- [ ] Create validation UI panel
- [ ] Add visual feedback for errors
- [ ] Test validation on sample prefabs
- [ ] Commit with message: "Phase 32.4: Prefab validation system"

### Phase 32.5 (Export)
- [ ] Create PrefabExporter class
- [ ] Implement JSON export
- [ ] Implement binary export
- [ ] Create PrefabRuntimeLoader class
- [ ] Implement JSON load
- [ ] Implement binary load
- [ ] Integrate with PrefabFactory
- [ ] Test export/import round-trip
- [ ] Commit with message: "Phase 32.5: Export to binary & runtime loader"

---

## 🎯 NEXT STEPS

1. **Start with Phase 32.1** (Copy/Paste)
   - Simplest to implement
   - Immediately useful
   - ~1-2 days

2. **Then Phase 32.2** (Undo/Redo)
   - More complex (command pattern)
   - Most important for UX
   - ~2-3 days

3. **Then Phase 32.5** (Export)
   - Connects editor → game
   - Critical for pipeline
   - ~2 days

4. **Then Phase 32.3 & 32.4** (Polish)
   - Organization & validation
   - ~1-2 days each

---

**Estimated Total Duration:** 2-3 weeks for Phase 32 complete

