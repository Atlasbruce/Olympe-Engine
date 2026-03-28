# DEVELOPER GUIDE: VisualScriptEditorPanel Refactoring

**Version**: 1.0  
**Date**: Phase 5 Completion  
**Audience**: Developers extending or maintaining the Blueprint Editor

---

## TABLE OF CONTENTS

1. [Architecture Overview](#architecture-overview)
2. [File Organization](#file-organization)
3. [How to Add New Features](#how-to-add-new-features)
4. [How to Add New Methods](#how-to-add-new-methods)
5. [State Management](#state-management)
6. [Adding New Node Types](#adding-new-node-types)
7. [Phase 24 Integration](#phase-24-integration)
8. [Phase 24.3 Integration](#phase-243-integration)
9. [Common Patterns](#common-patterns)
10. [Debugging Tips](#debugging-tips)

---

## ARCHITECTURE OVERVIEW

### 10-File Modular Architecture

The refactored `VisualScriptEditorPanel` is split into 10 specialized files:

```
┌─────────────────────────────────────────────────────────────┐
│                  VisualScriptEditorPanel                    │
│              (Main class in .h file - unchanged)            │
└─────────────────────────────────────────────────────────────┘
                              │
         ┌────────────────────┼────────────────────┐
         │                    │                    │
    ┌────▼────┐  ┌──────────▼──────────┐  ┌──────▼─────┐
    │   Core  │  │   Lifecycle &      │  │ Interaction│
    │         │  │   Rendering        │  │            │
    └────┬────┘  │                    │  └──────┬─────┘
         │       └────────────────────┘         │
    ┌────▼──────┐  ┌─────────────────┐  ┌──────▼──────┐
    │ Canvas    │  │  Connections    │  │  Presets    │
    │ Rendering │  │  Link Mgmt      │  │  (Phase 24) │
    └────┬──────┘  └────────┬────────┘  └──────┬──────┘
         │                  │                  │
    ┌────▼──────┐  ┌────────▼────────┐  ┌─────▼──────┐
    │ FileOps   │  │ Verification    │  │ Blackboard │
    │ Save/Load │  │ (+ Phase 24.3)  │  │ Variables  │
    └───────────┘  └─────────────────┘  └────────────┘

    ┌─────────────────────────────────────────┐
    │    Rendering (Toolbar, Menus)           │
    └─────────────────────────────────────────┘

    ┌─────────────────────────────────────────┐
    │  NodeProperties (Largest file, UI logic)│
    └─────────────────────────────────────────┘
```

### Design Principles

1. **Single Responsibility**: Each file handles one functional domain
2. **Mechanical Split**: Zero logic changes from original monolithic file
3. **State Centralization**: All state members in main class header
4. **Method Independence**: Each method can be understood standalone
5. **Backward Compatibility**: 100% API compatible with original

---

## FILE ORGANIZATION

### File Mapping by Domain

| File | Domain | Methods | Purpose |
|------|--------|---------|---------|
| **_Core.cpp** | Lifecycle | 6 | Constructor, Initialize, Shutdown, Render dispatcher |
| **_Interaction.cpp** | User Actions | 4 | AddNode, RemoveNode, Undo/Redo |
| **_Connections.cpp** | Link Management | 9 | Create/delete links, pin queries, rebuild links |
| **_Presets.cpp** | Phase 24 Presets | 3 | Preset bank UI, operand editor |
| **_Verification.cpp** | Validation | 6 | Graph verification, condition editor, validation UI |
| **_Blackboard.cpp** | Variables | 6 | Local/global variable management |
| **_Canvas.cpp** | ImNodes Canvas | 5 | Canvas rendering, synchronization |
| **_FileOps.cpp** | File I/O | 8 | Load, save, serialize, viewport management |
| **_Rendering.cpp** | Main UI | 5 | Toolbar, palette, context menus, status bar |
| **_NodeProperties.cpp** | Properties Panel | 9 | Properties, condition editor, type helpers |

### Include Pattern

```cpp
// Each .cpp file follows this pattern:
#include "VisualScriptEditorPanel.h"    // Main class definition (first!)
#include <algorithm>                     // Standard library
#include <sstream>
// ... other includes organized by type
#include "ImGui.h"                       // Third-party UI
#include "ImNodes.h"
// ... Phase 24 specific includes
#include "ConditionPresetRegistry.h"

namespace Olympe {
    // All method implementations here
}
```

---

## HOW TO ADD NEW FEATURES

### Scenario 1: Add a New Panel (e.g., Shortcuts Panel)

**Step 1: Decide Which File**
- If it's a rendering panel → `_Rendering.cpp` or `_NodeProperties.cpp`
- If it's data management → Create new file or add to existing

**Step 2: Add to Header**
```cpp
// In VisualScriptEditorPanel.h (add to class):
private:
    void RenderShortcutsPanel();
    std::vector<std::string> m_shortcutBindings;
```

**Step 3: Implement in Appropriate .cpp File**
```cpp
// In VisualScriptEditorPanel_Rendering.cpp (or new file):
void VisualScriptEditorPanel::RenderShortcutsPanel()
{
    ImGui::TextDisabled("Keyboard Shortcuts");
    ImGui::Separator();

    for (const auto& binding : m_shortcutBindings)
    {
        ImGui::BulletText("%s", binding.c_str());
    }
}
```

**Step 4: Call from Dispatcher**
```cpp
// In RenderContent() - typically in _Core.cpp:
RenderShortcutsPanel();
```

**Step 5: Update CMakeLists.txt** (if creating new file)
```cmake
target_sources(BlueprintEditor PRIVATE
    # ... existing files ...
    VisualScriptEditorPanel_Shortcuts.cpp  # ADD THIS
)
```

---

### Scenario 2: Add a New Node Type

**Step 1: Define in Header**
```cpp
// In graph definition files (not VisualScriptEditorPanel):
enum class TaskNodeType {
    // ... existing types ...
    CustomAction,  // NEW
};
```

**Step 2: Add to AddNode Method**
```cpp
// In VisualScriptEditorPanel_Interaction.cpp:
int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    // ... existing code ...

    if (type == TaskNodeType::CustomAction) {
        def.NodeName = "Custom Action";
        // Initialize custom action pins
    }
}
```

**Step 3: Add Pin Definitions**
```cpp
// In VisualScriptEditorPanel_Connections.cpp:
std::vector<std::string> VisualScriptEditorPanel::GetExecOutputPins(TaskNodeType type)
{
    switch (type) {
        // ... existing cases ...
        case TaskNodeType::CustomAction:
            return {"Done", "Error"};
    }
}
```

**Step 4: Add Properties Panel**
```cpp
// In VisualScriptEditorPanel_NodeProperties.cpp:
void VisualScriptEditorPanel::RenderNodePropertiesPanel(const TaskNodeDefinition& node)
{
    // ... existing code ...
    case TaskNodeType::CustomAction:
        RenderCustomActionProperties(node);
        break;
}
```

---

### Scenario 3: Add a New Validation Rule

**Step 1: Add to Verification Engine**
```cpp
// In VisualScriptEditorPanel_Verification.cpp or VSGraphVerifier.cpp:
void VisualScriptEditorPanel::RunVerification()
{
    // ... existing code ...

    // Check: All custom actions have metadata
    for (const auto& node : m_template.Nodes) {
        if (node.Type == TaskNodeType::CustomAction && 
            node.CustomMetadata.empty()) {
            m_verificationResult.issues.push_back({
                VSVerificationSeverity::Warning,
                "CUST-001",
                "Custom action missing metadata"
            });
        }
    }
}
```

**Step 2: Display Results**
```cpp
// Automatic in RenderVerificationPanel() - no changes needed!
```

---

## HOW TO ADD NEW METHODS

### Adding a Public Method (exposed to users)

**Step 1: Add to Header**
```cpp
// In VisualScriptEditorPanel.h (public section):
public:
    std::vector<TaskNodeDefinition> GetNodesOfType(TaskNodeType type);
```

**Step 2: Implement in Appropriate .cpp File**
```cpp
// In VisualScriptEditorPanel_Canvas.cpp (if canvas-related):
std::vector<TaskNodeDefinition> VisualScriptEditorPanel::GetNodesOfType(TaskNodeType type)
{
    std::vector<TaskNodeDefinition> result;
    for (const auto& node : m_editorNodes) {
        if (node.def.Type == type) {
            result.push_back(node.def);
        }
    }
    return result;
}
```

**Step 3: Update CMakeLists.txt** (no changes needed if using existing files)

### Adding a Private Helper Method

**Step 1: Add to Header**
```cpp
// In VisualScriptEditorPanel.h (private section):
private:
    bool ValidateNodePosition(float x, float y);
```

**Step 2: Implement in Appropriate .cpp File**
```cpp
// In VisualScriptEditorPanel_Canvas.cpp:
bool VisualScriptEditorPanel::ValidateNodePosition(float x, float y)
{
    // Check bounds, validate against existing nodes, etc.
    return (x >= 0 && y >= 0);
}
```

**Step 3: Use in Other Methods**
```cpp
// In other methods:
if (!ValidateNodePosition(x, y)) {
    m_dirty = false;
    return;
}
```

---

## STATE MANAGEMENT

### Key State Members

All state is stored in the main class header (`VisualScriptEditorPanel.h`):

```cpp
// Graph Data
TaskGraphTemplate m_template;                    // Current blueprint
std::vector<VSEditorNode> m_editorNodes;        // Editor node cache
std::vector<VSEditorLink> m_editorLinks;        // Editor link cache

// Undo/Redo
UndoRedoStack m_undoStack;                      // Undo/redo history

// Selection State
int m_selectedNodeID;                           // Currently selected node
int m_focusNodeID;                              // Node to focus on

// UI State
bool m_dirty;                                   // Graph modified?
bool m_verificationDone;                        // Verification run?
VSGraphVerificationResult m_verificationResult; // Verification results

// Phase 24 Integration
ConditionPresetRegistry m_presetRegistry;       // Preset storage
DynamicDataPinManager* m_pinManager;            // Dynamic pin manager
EntityBlackboard m_entityBlackboard;            // Entity variables
```

### Modifying State from Multiple Files

**Rule**: State lives in header, all .cpp files can access it:

```cpp
// In VisualScriptEditorPanel_Interaction.cpp:
int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    // Access state from any file:
    m_template.Nodes.push_back(def);      // OK
    m_dirty = true;                        // OK
    m_undoStack.PushCommand(...);         // OK
}

// In VisualScriptEditorPanel_Rendering.cpp:
void VisualScriptEditorPanel::RenderToolbar()
{
    if (m_dirty) {  // Access same state
        ImGui::TextColored(..., "●");
    }
}
```

### Thread Safety

**Note**: Current implementation is NOT thread-safe. All access must be from UI thread:

```cpp
// CORRECT: Call from UI thread
VisualScriptEditorPanel::Render() {  // Called from ImGui context
    // Safe to access m_template, m_editorNodes, etc.
}

// INCORRECT: Don't call from worker threads
std::thread t([this]() {
    m_template.Nodes.clear();  // RACE CONDITION!
});
```

---

## ADDING NEW NODE TYPES

### Complete Walkthrough: Adding "Delay Node"

**1. Define the Node Type** (in graph headers, not VisualScriptEditorPanel)

```cpp
enum class TaskNodeType {
    // ... existing types ...
    Delay,  // NEW
};
```

**2. Add Node Creation Logic** (_Interaction.cpp)

```cpp
int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    // ... existing code ...

    if (type == TaskNodeType::Delay) {
        def.NodeName = "Delay";
        def.Type = TaskNodeType::Delay;

        // Add duration parameter
        ParameterBinding duration;
        duration.Type = ParameterBindingType::Literal;
        duration.LiteralValue = TaskValue(1.0f);  // 1 second default
        def.Parameters["Duration"] = duration;
    }
}
```

**3. Add Pin Definitions** (_Connections.cpp)

```cpp
std::vector<std::string> VisualScriptEditorPanel::GetExecOutputPins(TaskNodeType type)
{
    switch (type) {
        // ... existing cases ...
        case TaskNodeType::Delay:
            return {"Completed"};  // Single output when delay finishes
    }
}

std::vector<std::string> VisualScriptEditorPanel::GetDataInputPins(TaskNodeType type)
{
    switch (type) {
        // ... existing cases ...
        case TaskNodeType::Delay:
            return {"Duration"};  // Input pin for duration value
    }
}
```

**4. Add Properties UI** (_NodeProperties.cpp)

```cpp
void VisualScriptEditorPanel::RenderNodePropertiesPanel(const TaskNodeDefinition& node)
{
    switch (node.Type) {
        // ... existing cases ...
        case TaskNodeType::Delay:
            ImGui::TextDisabled("Delay Duration:");
            ImGui::SameLine();

            auto it = node.Parameters.find("Duration");
            if (it != node.Parameters.end()) {
                float dur = it->second.LiteralValue.AsFloat();
                if (ImGui::InputFloat("##delay_dur", &dur, 0.1f)) {
                    // Update template (requires UI callback)
                }
            }
            break;
    }
}
```

**5. Add Verification Rules** (if needed) (_Verification.cpp)

```cpp
void VisualScriptEditorPanel::RunVerification()
{
    // ... existing code ...

    for (const auto& node : m_template.Nodes) {
        if (node.Type == TaskNodeType::Delay) {
            auto it = node.Parameters.find("Duration");
            if (it == node.Parameters.end()) {
                m_verificationResult.issues.push_back({
                    VSVerificationSeverity::Error,
                    "DELAY-001",
                    "Delay node missing duration parameter"
                });
            }
        }
    }
}
```

**6. Done!**
- Node can be created, connected, edited, and verified
- No additional files needed (unless creating specialized UI)

---

## PHASE 24 INTEGRATION

### Using Condition Presets

**Creating a Preset in Code**

```cpp
// In VisualScriptEditorPanel_Presets.cpp:
ConditionPreset preset;
preset.id = "health_check";
preset.left = Operand(OperandMode::Variable, "Health");
preset.op = ComparisonOp::Greater;
preset.right = Operand(OperandMode::Const, 50.0);

m_presetRegistry.AddPreset(preset);
m_template.Presets.push_back(preset);  // Save to template for serialization
```

**Using a Preset in a Branch Node**

```cpp
// The branch node can reference presets via data pins
// This is handled automatically by Phase 24 integration
```

### Dynamic Data Pin Manager

**Getting Available Pins**

```cpp
// In VisualScriptEditorPanel_Presets.cpp:
std::vector<DynamicDataPin> pins = m_pinManager->GetAllPins();

for (const auto& pin : pins) {
    ImGui::TextUnformatted(pin.label.c_str());
}
```

**Creating Dynamic Pins**

```cpp
// Phase 24 automatically creates pins based on:
// - Active presets in registry
// - Dynamic pin definitions in node
// - Current editor state

// No manual pin creation needed - fully automated!
```

---

## PHASE 24.3 INTEGRATION

### Execution Testing

**Running Execution Test**

```cpp
// In VisualScriptEditorPanel_Verification.cpp:
// Phase 24.3 execution test (stub in current implementation)

void VisualScriptEditorPanel::RunExecutionTest()
{
    // When fully implemented, this would:
    // 1. Create GraphExecutionSimulator
    // 2. Run simulation on m_template
    // 3. Populate ExecutionTestPanel with results
    // 4. Display trace log and errors
}
```

**Accessing Test Results**

```cpp
// After test runs:
const auto& testPanel = GetExecutionTestPanel();
const auto& tracer = testPanel.GetLastTrace();

// Trace information:
const auto& events = tracer.GetEvents();
std::string log = tracer.GetTraceLog();
std::string summary = tracer.GetExecutionSummary();
```

---

## COMMON PATTERNS

### Pattern 1: Marking Graph as Dirty

```cpp
// Whenever state changes:
m_dirty = true;

// Before saving:
if (m_dirty) {
    Save();
}
```

### Pattern 2: Undo/Redo Commands

```cpp
// In VisualScriptEditorPanel_Interaction.cpp:
m_undoStack.PushCommand(
    std::unique_ptr<ICommand>(new AddNodeCommand(def)),
    m_template
);

// Automatic undo/redo calls other methods:
PerformUndo() {  // Calls undo on stack
    SyncEditorNodesFromTemplate();
    RebuildLinks();
}
```

### Pattern 3: ImGui UI Rendering

```cpp
// Always use ImGui patterns:
if (ImGui::Button("Action##id")) {
    // User clicked
}

ImGui::SetNextItemWidth(120.0f);
if (ImGui::InputText("##label", buf, sizeof(buf))) {
    // User edited
}
```

### Pattern 4: Phase 24 Property Binding

```cpp
// Getting variable value:
ParameterBinding binding = node.Parameters["varname"];
if (binding.Type == ParameterBindingType::Variable) {
    std::string varName = binding.VariableName;
    // Get value from blackboard
}
```

---

## DEBUGGING TIPS

### Debug Output

```cpp
// Use system logging:
SYSTEM_LOG << "[VisualScriptEditorPanel] Message here\n";

// Conditional logging:
if (node.Type == TaskNodeType::Branch) {
    SYSTEM_LOG << "[DEBUG] Branch node " << node.NodeID << "\n";
}
```

### Breakpoint Locations

**To debug node creation:**
- Set breakpoint in `VisualScriptEditorPanel_Interaction.cpp:AddNode()`

**To debug rendering:**
- Set breakpoint in `VisualScriptEditorPanel_Canvas.cpp:RenderCanvas()`

**To debug saving:**
- Set breakpoint in `VisualScriptEditorPanel_FileOps.cpp:SerializeAndWrite()`

### Common Issues

| Issue | Location | Fix |
|-------|----------|-----|
| Node doesn't appear on canvas | _Canvas.cpp:RenderCanvas() | Check RebuildLinks() |
| Link creation fails | _Connections.cpp:ConnectExec() | Verify pin UIDs |
| Save doesn't persist | _FileOps.cpp:SerializeAndWrite() | Check file path |
| Undo doesn't work | _Interaction.cpp:PerformUndo() | Verify command was pushed |

---

## BEST PRACTICES

1. **Keep Domain Separation**: Don't mix concerns across files
2. **Use Helper Methods**: Extract complex logic into separate methods
3. **Document Integration Points**: Comment where Phase 24/24.3 features are used
4. **Test in Context**: New features should work with existing graph operations
5. **Check Dirty Flag**: Always mark m_dirty=true when modifying state
6. **Use Namespacing**: Keep everything in Olympe:: namespace
7. **Follow Include Pattern**: Always #include "VisualScriptEditorPanel.h" first
8. **Respect State Locality**: Only modify state from UI thread

---

**End of Developer Guide**

For questions about specific features, see:
- MAINTENANCE.md (troubleshooting)
- PHASE24_INTEGRATION_GUIDE.md (Phase 24 features)
- PHASE3_VERIFICATION_REPORT.md (technical details)
