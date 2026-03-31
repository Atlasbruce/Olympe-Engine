/**
 * @file INTEGRATION_GUIDE_NodeProperties.md
 * @brief Guide for integrating While/ForEach/SubGraph property panels
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * This guide explains how to integrate the new node property rendering methods
 * into the existing RenderNodePropertiesPanel() function.
 */

# Integration Guide: While, ForEach, SubGraph Properties

## Overview
Three new node types now have full property panel support:
- **While** : Loop conditions configuration
- **ForEach** : List iteration setup
- **SubGraph** : File path and parameter bindings

## Integration Steps

### 1. Include the new header
In VisualScriptEditorPanel.h (or the file containing RenderNodePropertiesPanel):

```cpp
#include "VisualScriptEditorPanel_NodeProperties_Extensions.h"
```

### 2. Add cases to RenderNodePropertiesPanel()

In the main RenderNodePropertiesPanel() function (in VisualScriptEditorPanel_RenderingCore.cpp or similar), 
add these cases to the switch statement that handles different node types:

```cpp
void VisualScriptEditorPanel::RenderNodePropertiesPanel()
{
    if (m_selectedNodeID == NODE_INDEX_NONE)
    {
        ImGui::TextDisabled("(Select a node to view properties)");
        return;
    }

    // Find selected node
    const TaskNodeDefinition* nodePtr = nullptr;
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
        {
            nodePtr = &m_template.Nodes[i];
            break;
        }
    }

    if (!nodePtr)
        return;

    // Existing cases...
    switch (nodePtr->Type)
    {
        case TaskNodeType::EntryPoint:
            RenderEntryPointProperties();
            break;

        case TaskNodeType::AtomicTask:
            RenderAtomicTaskProperties();
            break;

        case TaskNodeType::Branch:
            RenderBranchProperties();
            break;

        // *** ADD THESE THREE CASES ***
        case TaskNodeType::While:
            RenderWhileNodeProperties();
            break;

        case TaskNodeType::ForEach:
            RenderForEachNodeProperties();
            break;

        case TaskNodeType::SubGraph:
            RenderSubGraphNodeProperties();
            break;
        // *** END NEW CASES ***

        // ... rest of existing cases ...

        default:
            ImGui::TextDisabled("Properties not yet implemented for this node type");
            break;
    }
}
```

### 3. Member variable setup

Ensure VisualScriptEditorPanel has these members (likely already present):

```cpp
class VisualScriptEditorPanel {
    // ... existing members ...

    // For property editing
    int32_t m_selectedNodeID = NODE_INDEX_NONE;
    TaskNodeDefinition* m_selectedNodePtr = nullptr;  // Mutable pointer for editing
    bool m_dirty = false;

    // New property panel methods
    void RenderWhileNodeProperties();
    void RenderForEachNodeProperties();
    void RenderSubGraphNodeProperties();
};
```

### 4. Update node selection handling

When user clicks on a node in the canvas, update the editor to set m_selectedNodePtr:

```cpp
void VisualScriptEditorPanel::OnNodeSelected(int32_t nodeID)
{
    m_selectedNodeID = nodeID;

    // Find mutable pointer to the node
    m_selectedNodePtr = nullptr;
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].NodeID == nodeID)
        {
            m_selectedNodePtr = &m_template.Nodes[i];
            break;
        }
    }
}
```

## Features by Node Type

### While Loop
```
Input Fields:
- Node Name (text)
- Multiple Conditions:
  - Left Operand: [Variable ▼] [input field]
  - Operator: [== ▼] (6 comparison operators)
  - Right Operand: [Variable ▼] [input field]
  - Delete button (X)
- Add Condition button

Output:
- Modifies m_template.Nodes[i].conditions
- Sets m_dirty = true
```

### ForEach
```
Input Fields:
- Node Name (text)
- List Variable: [text input]
- Available Blackboard Variables: [scrollable list for quick selection]

Output:
- Modifies m_template.Nodes[i].forEachVariable
- Sets m_dirty = true
```

### SubGraph
```
Input Fields:
- Node Name (text)
- SubGraph File Path: [text input] [Browse button]
- Input Parameters (table):
  - Parameter Name | Binding Type ▼ | Value/Variable input
  - Delete button per parameter
- Add Parameter button

Output:
- Modifies m_template.Nodes[i].SubGraphPath
- Modifies m_template.Nodes[i].Parameters map
- Sets m_dirty = true
```

## Data Structures

### While Node
```cpp
struct TaskNodeDefinition {
    std::vector<Condition> conditions;  // Array of loop conditions
    // ... other fields ...
};

struct Condition {
    std::string leftMode;       // "Pin" | "Variable" | "Const"
    std::string leftPin;        // If Pin mode
    std::string leftVariable;   // If Variable mode
    TaskValue leftConstValue;   // If Const mode

    std::string operatorStr;    // "==", "!=", "<", ">", "<=", ">="

    std::string rightMode;      // "Pin" | "Variable" | "Const"
    std::string rightPin;       // If Pin mode
    std::string rightVariable;  // If Variable mode
    TaskValue rightConstValue;  // If Const mode
};
```

### ForEach Node
```cpp
struct TaskNodeDefinition {
    std::string forEachVariable;  // Blackboard key for list variable
    // ... other fields ...
};
```

### SubGraph Node
```cpp
struct TaskNodeDefinition {
    std::string SubGraphPath;                                    // File path
    std::unordered_map<std::string, ParameterBinding> Parameters; // Input bindings
    // ... other fields ...
};

struct ParameterBinding {
    ParameterBindingType Type;      // Literal or LocalVariable
    TaskValue LiteralValue;         // If Literal
    std::string VariableName;       // If LocalVariable
};
```

## Compilation Notes

1. The new .cpp file includes imgui.h and TaskGraphTypes.h
2. Make sure the project includes the new file in its build configuration
3. The implementation uses standard ImGui widgets (InputText, Combo, Button, etc.)
4. Error handling is basic; consider adding validation in RenderNodePropertiesPanel()

## Future Enhancements

- File browser dialog for SubGraph path selection
- Syntax highlighting for condition expressions
- Parameter type validation based on SubGraph definition
- Condition preset management (reuse common conditions)
- Visual condition builder (drag-and-drop operands)

---

**Integration Date:** 2026-03-15
**Status:** Ready for integration
