# Blueprint Editor - Command System Architecture

## Overview

The Blueprint Editor implements a robust command pattern for undo/redo functionality. This document describes the architecture, implementation details, and how to extend the system.

## Design Pattern

### Command Pattern

The system uses the classic **Command Pattern** with these key characteristics:

- **Encapsulation:** Each operation is wrapped in a command object
- **Reversibility:** Every command can be undone and redone
- **History:** Commands are stored in a stack for sequential undo/redo
- **Separation:** Business logic (commands) separated from UI (GUI)

### Class Hierarchy

```
EditorCommand (Abstract Base)
    ├── CreateNodeCommand
    ├── DeleteNodeCommand
    ├── MoveNodeCommand
    ├── LinkNodesCommand
    ├── UnlinkNodesCommand
    └── SetParameterCommand
```

## Core Classes

### EditorCommand (Abstract)

**Location:** `Source/BlueprintEditor/CommandSystem.h`

```cpp
class EditorCommand
{
public:
    virtual ~EditorCommand() = default;
    virtual void Execute() = 0;      // Perform the operation
    virtual void Undo() = 0;         // Reverse the operation
    virtual std::string GetDescription() const = 0;  // For UI display
};
```

**Design Principles:**
- Pure virtual interface
- No state in base class
- Description for user feedback

### CommandStack

**Location:** `Source/BlueprintEditor/CommandSystem.h`

```cpp
class CommandStack
{
private:
    std::vector<std::unique_ptr<EditorCommand>> m_UndoStack;
    std::vector<std::unique_ptr<EditorCommand>> m_RedoStack;
    size_t m_MaxStackSize = 100;  // Memory limit

public:
    void ExecuteCommand(std::unique_ptr<EditorCommand> cmd);
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    // ... other methods
};
```

**Key Features:**
- Dual stack architecture (undo + redo)
- Memory-bounded (100 commands max)
- Smart pointer ownership
- Thread-safe for single-threaded use

## Command Lifecycle

### Execution Flow

```
1. User Action (via GUI)
   ↓
2. Create Command Object
   auto cmd = std::make_unique<CreateNodeCommand>(params);
   ↓
3. Execute via CommandStack
   m_CommandStack->ExecuteCommand(std::move(cmd));
   ↓
4. Command Executes
   cmd->Execute();  // Performs the operation
   ↓
5. Added to Undo Stack
   m_UndoStack.push_back(std::move(cmd));
   ↓
6. Redo Stack Cleared
   m_RedoStack.clear();  // New action invalidates redo
```

### Undo Flow

```
1. User Presses Ctrl+Z
   ↓
2. Pop from Undo Stack
   auto cmd = std::move(m_UndoStack.back());
   m_UndoStack.pop_back();
   ↓
3. Execute Undo
   cmd->Undo();  // Reverses the operation
   ↓
4. Move to Redo Stack
   m_RedoStack.push_back(std::move(cmd));
```

### Redo Flow

```
1. User Presses Ctrl+Y
   ↓
2. Pop from Redo Stack
   auto cmd = std::move(m_RedoStack.back());
   m_RedoStack.pop_back();
   ↓
3. Re-Execute
   cmd->Execute();  // Performs the operation again
   ↓
4. Move to Undo Stack
   m_UndoStack.push_back(std::move(cmd));
```

## Concrete Commands

### CreateNodeCommand

**Purpose:** Create a new node in a behavior tree graph

**State:**
```cpp
std::string m_GraphId;
std::string m_NodeType;
std::string m_NodeName;
float m_PosX, m_PosY;
int m_CreatedNodeId;  // Set during Execute
```

**Execute:**
```cpp
void Execute() override {
    NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
    NodeType type = StringToNodeType(m_NodeType);
    m_CreatedNodeId = graph->CreateNode(type, m_PosX, m_PosY, m_NodeName);
}
```

**Undo:**
```cpp
void Undo() override {
    NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
    graph->DeleteNode(m_CreatedNodeId);
}
```

### DeleteNodeCommand

**Purpose:** Delete a node from a graph

**State:**
```cpp
std::string m_GraphId;
int m_NodeId;
json m_NodeData;  // Saved for undo
```

**Execute:**
```cpp
void Execute() override {
    NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
    const GraphNode* node = graph->GetNode(m_NodeId);
    
    // Save node data before deleting
    m_NodeData = SerializeNode(node);
    
    graph->DeleteNode(m_NodeId);
}
```

**Undo:**
```cpp
void Undo() override {
    NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
    // Recreate node from saved data
    RecreateNodeFromData(graph, m_NodeData);
}
```

### MoveNodeCommand

**Purpose:** Change a node's position

**State:**
```cpp
std::string m_GraphId;
int m_NodeId;
float m_OldX, m_OldY;
float m_NewX, m_NewY;
```

**Execute & Undo:** Simple position swap

## Integration Points

### BlueprintEditor Backend

**Location:** `Source/BlueprintEditor/blueprinteditor.h/cpp`

```cpp
class BlueprintEditor {
private:
    CommandStack* m_CommandStack;
    
public:
    void Undo() { m_CommandStack->Undo(); }
    void Redo() { m_CommandStack->Redo(); }
    bool CanUndo() const { return m_CommandStack->CanUndo(); }
    bool CanRedo() const { return m_CommandStack->CanRedo(); }
    
    CommandStack* GetCommandStack() { return m_CommandStack; }
};
```

### GUI Integration

**Location:** `Source/BlueprintEditor/BlueprintEditorGUI.cpp`

**Keyboard Shortcuts:**
```cpp
void HandleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Ctrl+Z: Undo
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift) {
        if (BlueprintEditor::Get().CanUndo()) {
            BlueprintEditor::Get().Undo();
        }
    }
    
    // Ctrl+Y: Redo
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
        if (BlueprintEditor::Get().CanRedo()) {
            BlueprintEditor::Get().Redo();
        }
    }
}
```

**Menu Integration:**
```cpp
if (ImGui::BeginMenu("Edit")) {
    bool canUndo = backend.CanUndo();
    std::string undoLabel = canUndo 
        ? "Undo: " + backend.GetLastCommandDescription()
        : "Undo";
    
    if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUndo)) {
        backend.Undo();
    }
}
```

## Memory Management

### Stack Size Limit

**Problem:** Unlimited history consumes memory

**Solution:** Limit stack to 100 commands

```cpp
void CommandStack::ExecuteCommand(std::unique_ptr<EditorCommand> cmd) {
    cmd->Execute();
    m_UndoStack.push_back(std::move(cmd));
    m_RedoStack.clear();
    
    // Enforce limit
    if (m_UndoStack.size() > m_MaxStackSize) {
        m_UndoStack.erase(m_UndoStack.begin());
    }
}
```

### Smart Pointers

**Ownership:** CommandStack owns all commands via `std::unique_ptr`

**Benefits:**
- Automatic cleanup
- No memory leaks
- Clear ownership semantics
- Move semantics for efficiency

## Extending the System

### Adding a New Command

**Step 1:** Define command class in `CommandSystem.h`

```cpp
class YourNewCommand : public EditorCommand
{
public:
    YourNewCommand(/* parameters */);
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;
    
private:
    // State needed for execute/undo
    std::string m_Data;
};
```

**Step 2:** Implement in `CommandSystem.cpp`

```cpp
YourNewCommand::YourNewCommand(/* parameters */)
    : m_Data(data)
{
}

void YourNewCommand::Execute() {
    // Perform operation
    // Save any state needed for undo
}

void YourNewCommand::Undo() {
    // Reverse operation using saved state
}

std::string YourNewCommand::GetDescription() const {
    return "Your Action Description";
}
```

**Step 3:** Use in code

```cpp
auto cmd = std::make_unique<YourNewCommand>(params);
BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
```

### Command Design Guidelines

✅ **DO:**
- Save all state needed for undo in Execute()
- Make Undo() reverse Execute() exactly
- Keep descriptions clear and concise
- Handle errors gracefully (invalid IDs, etc.)
- Test both Execute and Undo thoroughly

❌ **DON'T:**
- Access global mutable state
- Perform side effects in constructor
- Rely on external state for undo
- Throw exceptions (return error codes instead)
- Create commands that can't be undone

## Common Patterns

### Memento Pattern Integration

For complex state:

```cpp
class ComplexCommand : public EditorCommand {
private:
    json m_StateBeforeExecute;  // Memento
    json m_StateAfterExecute;   // Memento
    
public:
    void Execute() override {
        m_StateBeforeExecute = CaptureCurrentState();
        PerformOperation();
        m_StateAfterExecute = CaptureCurrentState();
    }
    
    void Undo() override {
        RestoreState(m_StateBeforeExecute);
    }
};
```

### Composite Commands

For operations affecting multiple objects:

```cpp
class CompositeCommand : public EditorCommand {
private:
    std::vector<std::unique_ptr<EditorCommand>> m_Commands;
    
public:
    void AddCommand(std::unique_ptr<EditorCommand> cmd) {
        m_Commands.push_back(std::move(cmd));
    }
    
    void Execute() override {
        for (auto& cmd : m_Commands) {
            cmd->Execute();
        }
    }
    
    void Undo() override {
        // Undo in reverse order
        for (auto it = m_Commands.rbegin(); it != m_Commands.rend(); ++it) {
            (*it)->Undo();
        }
    }
};
```

### Macro Commands

For atomic multi-step operations:

```cpp
// Example: "Create Node and Link It" as single undoable operation
auto macroCmd = std::make_unique<CompositeCommand>();
macroCmd->AddCommand(std::make_unique<CreateNodeCommand>(...));
macroCmd->AddCommand(std::make_unique<LinkNodesCommand>(...));
ExecuteCommand(std::move(macroCmd));
```

## Performance Considerations

### Command Creation Cost

- Commands are lightweight objects
- Created on demand (not pre-allocated)
- Minimal overhead per operation

### Memory Usage

**Per Command:** ~100-200 bytes average
**100 Commands:** ~10-20 KB total
**Negligible:** For modern systems

### Optimization Opportunities

1. **Command Compression:** Merge consecutive similar commands
   ```cpp
   // Instead of 100 "Move Node" commands
   // Store one "Move Node" with final position
   ```

2. **Lazy State Capture:** Only save state when needed
   ```cpp
   void Execute() {
       // Don't save state until first undo request
       if (m_StateNotYetSaved) {
           m_State = CaptureState();
       }
   }
   ```

3. **State Diffing:** Store diffs instead of full state
   ```cpp
   json m_StateDiff;  // Only changed properties
   ```

## Testing Strategy

### Unit Tests

```cpp
TEST(CreateNodeCommand, ExecuteCreatesNode) {
    auto cmd = std::make_unique<CreateNodeCommand>(...);
    cmd->Execute();
    ASSERT_TRUE(NodeExists());
}

TEST(CreateNodeCommand, UndoDeletesNode) {
    auto cmd = std::make_unique<CreateNodeCommand>(...);
    cmd->Execute();
    int nodeId = GetCreatedNodeId();
    cmd->Undo();
    ASSERT_FALSE(NodeExists(nodeId));
}
```

### Integration Tests

```cpp
TEST(CommandStack, ExecuteUndo) {
    CommandStack stack;
    auto cmd = std::make_unique<CreateNodeCommand>(...);
    
    stack.ExecuteCommand(std::move(cmd));
    ASSERT_TRUE(NodeExists());
    
    stack.Undo();
    ASSERT_FALSE(NodeExists());
}
```

## Troubleshooting

### Command Not Working

**Check:**
1. Is Execute() actually performing the operation?
2. Is Undo() properly reversing Execute()?
3. Is the command being added to the stack?
4. Are there error messages in console?

### Undo/Redo Behaving Strangely

**Common Issues:**
1. **State not saved:** Execute() must save state before modifying
2. **Order problems:** Composite commands must undo in reverse
3. **External state changed:** Commands rely on external state that changed

### Memory Leaks

**Use Valgrind/AddressSanitizer:**
```bash
valgrind --leak-check=full ./OlympeEngine
```

**Check:**
- All commands using smart pointers?
- CommandStack properly clearing stacks?
- No circular references?

## Future Enhancements

### Planned Features

1. **Command Compression**
   - Merge consecutive similar commands
   - Reduce memory usage

2. **Command Macros**
   - Record sequences of commands
   - Replay as single operation

3. **Persistent Undo**
   - Save undo history to disk
   - Restore on editor restart

4. **Selective Undo**
   - Undo specific commands, not just last one
   - Non-linear undo history

5. **Command Threading**
   - Execute expensive commands asynchronously
   - Show progress indication

## References

### Design Patterns
- **Command Pattern:** Gang of Four Design Patterns
- **Memento Pattern:** For state capture
- **Composite Pattern:** For macro commands

### Similar Implementations
- Unreal Engine: Transaction system
- Unity: Undo system (EditorUtility.SetDirty)
- Qt: QUndoCommand and QUndoStack

---

**See Also:**
- [Keyboard Shortcuts Guide](KEYBOARD_SHORTCUTS.md)
- [Templates System Architecture](TEMPLATES_ARCHITECTURE.md)
- [Blueprint Editor Overview](../QUICKSTART.md)
