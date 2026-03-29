# PHASE 24 & 24.3 INTEGRATION GUIDE

**Version**: 1.0  
**Date**: Phase 5 Completion  
**Audience**: Developers integrating Phase 24 features with refactored editor

---

## TABLE OF CONTENTS

1. [Phase 24 Overview](#phase-24-overview)
2. [Condition Presets Integration](#condition-presets-integration)
3. [Dynamic Data Pins](#dynamic-data-pins)
4. [Entity Blackboard](#entity-blackboard)
5. [NodeBranchRenderer Integration](#nodebranchrenderer-integration)
6. [Phase 24.3 Execution Testing](#phase-243-execution-testing)
7. [Complete Example](#complete-example)
8. [Troubleshooting](#troubleshooting)

---

## PHASE 24 OVERVIEW

### What is Phase 24?

Phase 24 (Condition Presets) extends the Blueprint Editor with:
- **Reusable Conditions**: Define once, use many times
- **Dynamic Data Pins**: Pins generated from presets/templates
- **Entity Blackboard**: Per-entity variable storage
- **Advanced Rendering**: Visual representations of complex conditions

### Files Using Phase 24

```
Phase 24 Integration Points in Refactored Editor:

┌─ VisualScriptEditorPanel_Presets.cpp
│  ├─ RenderPresetBankPanel()
│  ├─ RenderPresetItemCompact()
│  └─ RenderOperandEditor()
│
├─ VisualScriptEditorPanel_Connections.cpp
│  ├─ GetExecOutputPinsForNode()  [Dynamic pins]
│  └─ RebuildLinks()              [Dynamic pin handling]
│
├─ VisualScriptEditorPanel_Blackboard.cpp
│  ├─ RenderBlackboard()
│  ├─ RenderLocalVariablesPanel()
│  ├─ RenderGlobalVariablesPanel()
│  └─ GetVariablesByType()
│
└─ VisualScriptEditorPanel_Verification.cpp
   └─ RunVerification()            [Preset validation]
```

---

## CONDITION PRESETS INTEGRATION

### Creating & Managing Presets

**Location**: `VisualScriptEditorPanel_Presets.cpp`

```cpp
// Create a preset
ConditionPreset healthCheck;
healthCheck.id = "health_gt_50";
healthCheck.left = Operand(OperandMode::Variable, "Health");
healthCheck.op = ComparisonOp::Greater;
healthCheck.right = Operand(OperandMode::Const, 50.0);

// Add to registry (memory)
m_presetRegistry.AddPreset(healthCheck);

// Add to template (for serialization)
m_template.Presets.push_back(healthCheck);
```

### Preset UI Components

**RenderPresetBankPanel()**
```cpp
// Displays all available presets
// Features:
// - List all presets
// - Add new preset button
// - Edit existing presets
// - Delete presets

// Called from: RenderProperties() dispatcher
```

**RenderPresetItemCompact()**
```cpp
// Single preset UI row
// Format: [Index] [Left ▼] [Op ▼] [Right ▼] [Edit] [Dup] [X]
// 
// Features:
// - Inline operand editing
// - Operator selection
// - Duplicate/Delete buttons
// - Auto-sync to template

// Called from: RenderPresetBankPanel() for each preset
```

**RenderOperandEditor()**
```cpp
// Operand mode/value selector
// Unified dropdown with three sections:
// 1. [Pin-in] Available input pins
// 2. [Const] Constant value with numeric input
// 3. Variables (local + global sorted alphabetically)

// Features:
// - Automatic pin discovery from DynamicDataPinManager
// - Const value numeric editing
// - Local variable filtering by type
// - Global variable support (Phase 24)

// Called from: RenderPresetItemCompact() twice (left & right operands)
```

### Preset Synchronization

**Problem**: Presets stored in registry (memory) but need to survive save/load

**Solution**: `SyncPresetsFromRegistryToTemplate()`

```cpp
// In VisualScriptEditorPanel_FileOps.cpp:

void VisualScriptEditorPanel::SyncPresetsFromRegistryToTemplate()
{
    // Before saving:
    // 1. Copy all presets from registry to m_template.Presets
    // 2. TaskGraphSerializer includes presets in JSON
    // 3. On load, TaskGraphLoader deserializes presets
    // 4. LoadTemplate() repopulates registry from m_template.Presets
}

// Timeline:
Edit presets in UI
    ↓
RenderPresetItemCompact() → m_presetRegistry.UpdatePreset()
    ↓
On save: SyncPresetsFromRegistryToTemplate()
    ↓
SerializeAndWrite() → includes m_template.Presets
    ↓
JSON file saved with presets embedded
    ↓
On load: TaskGraphLoader → m_template.Presets populated
    ↓
LoadTemplate() → m_presetRegistry repopulated from m_template
```

### Using Presets in Branch Nodes

**Phase 24 Concept**: Branch nodes can reference presets via dynamic data pins

```cpp
// Creating a preset-based branch:

TaskNodeDefinition branchNode;
branchNode.Type = TaskNodeType::Branch;
branchNode.NodeID = AllocNodeID();

// Phase 24: Create dynamic data input pin for preset condition
DynamicDataPin presetPin;
presetPin.label = "health_check";  // Preset ID
presetPin.type = DataPinType::Condition;

branchNode.DynamicDataInputPins.push_back(presetPin);

// When branch evaluates:
// 1. Look up preset by ID ("health_check")
// 2. Get left operand value (from Variable "Health")
// 3. Get right operand value (50.0)
// 4. Evaluate: Health > 50
// 5. Route to "Then" or "Else" output
```

---

## DYNAMIC DATA PINS

### Overview

**Dynamic Pins**: Generated based on node type, presets, or configuration (not hardcoded)

### Pin Generation

```cpp
// In VisualScriptEditorPanel_Connections.cpp:

std::vector<std::string> VisualScriptEditorPanel::GetExecOutputPinsForNode(
    const TaskNodeDefinition& def)
{
    // Default: use static pins
    std::vector<std::string> pins = GetExecOutputPins(def.Type);

    // Phase 24: For Branch nodes, add dynamic pins
    if (def.Type == TaskNodeType::Branch) {
        // Add one output per DynamicDataInputPin
        for (size_t i = 0; i < def.DynamicDataInputPins.size(); ++i) {
            pins.push_back("Preset_" + std::to_string(i));
        }
    }

    return pins;
}
```

### Pin Availability

```cpp
// In RenderOperandEditor(), get available pins:

std::vector<DynamicDataPin> allPins = m_pinManager->GetAllPins();

// These are:
// - All output pins from all nodes
// - Filtered to appropriate types
// - Updated as graph changes
```

### Pin References in Conditions

```cpp
// Operand can reference a pin:

Operand condition_operand;
condition_operand.mode = OperandMode::Pin;
condition_operand.stringValue = "Node_5_OutputData";  // Pin label

// When evaluating:
// 1. Find node with output pin "OutputData"
// 2. Get runtime value of that pin
// 3. Use in comparison
```

---

## ENTITY BLACKBOARD

### Local vs. Global Variables

```cpp
// Local Blackboard (in m_template.Blackboard)
// - Per-blueprint scope
// - Serialized with graph
// - Accessible from UI in _Blackboard.cpp

BlackboardEntry localVar;
localVar.Key = "PlayerHealth";
localVar.Type = VariableType::Int;
localVar.Default = TaskValue(100);
localVar.IsGlobal = false;

m_template.Blackboard.push_back(localVar);

// Global Blackboard (in GlobalTemplateBlackboard)
// - Across all blueprints
// - Not serialized (or serialized separately)
// - Accessible via GlobalTemplateBlackboard::Get()

GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
const auto& globalVars = gtb.GetAllVariables();
```

### Displaying Variables

**Location**: `VisualScriptEditorPanel_Blackboard.cpp`

```cpp
void VisualScriptEditorPanel::RenderBlackboard()
{
    // Add variable button
    if (ImGui::Button("+##vsbbAdd")) {
        BlackboardEntry entry;
        entry.Key = "NewVariable";
        entry.Type = VariableType::Int;
        m_template.Blackboard.push_back(entry);
    }

    // List variables with edit UI
    for (int idx = (int)m_template.Blackboard.size() - 1; idx >= 0; --idx) {
        // Name input
        ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf));

        // Type selector
        ImGui::Combo("##bbtype", &typeIdx, typeLabels, 6);

        // Delete button
        if (ImGui::SmallButton("x##bbdel")) {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
        }
    }
}

void VisualScriptEditorPanel::RenderLocalVariablesPanel()
{
    // Show only local (IsGlobal == false)
    for (const auto& entry : m_template.Blackboard) {
        if (!entry.IsGlobal) {
            ImGui::BulletText("%s (%s)", 
                entry.Key.c_str(), 
                GetTypeLabel(entry.Type).c_str());
        }
    }
}

void VisualScriptEditorPanel::RenderGlobalVariablesPanel()
{
    // Show global variables from GlobalTemplateBlackboard
    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    for (const auto& gvar : gtb.GetAllVariables()) {
        ImGui::BulletText("%s (%s)", 
            gvar.Key.c_str(), 
            GetTypeLabel(gvar.Type).c_str());
    }
}
```

### Variable Usage in Presets

```cpp
// In RenderOperandEditor(), populate variable list:

std::vector<std::string> sortedVarNames;

// Local variables
for (const auto& entry : m_template.Blackboard) {
    if (entry.Type != VariableType::None && !entry.Key.empty()) {
        sortedVarNames.push_back(entry.Key);
    }
}

// Global variables
GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
for (const auto& gvar : gtb.GetAllVariables()) {
    sortedVarNames.push_back(gvar.Key);
}

// Add separator, sort, display in dropdown
```

---

## NODEBRANCHRENDERER INTEGRATION

### Purpose

NodeBranchRenderer (Phase 24) provides visual representation of branch conditions

### Integration Points

```cpp
// In VisualScriptEditorPanel.h (state):
std::unique_ptr<NodeBranchRenderer> m_branchRenderer;

// In Initialize():
m_branchRenderer = std::make_unique<NodeBranchRenderer>(
    m_presetRegistry,
    m_pinManager);

// In RenderCanvas() or node rendering:
if (node.Type == TaskNodeType::Branch) {
    m_branchRenderer->Render(node);  // Render branch visualization
}
```

### Rendering Branch Conditions

```cpp
// NodeBranchRenderer displays:
// - Condition preview text
// - Operator symbols (==, !=, <, >, <=, >=)
// - Left/right operands
// - Color coding (valid/invalid)

// Automatically integrates with:
// - ConditionPresetRegistry
// - DynamicDataPinManager
// - EntityBlackboard
// - Variable filtering
```

---

## PHASE 24.3 EXECUTION TESTING

### Overview

Phase 24.3 adds execution simulation and testing capabilities

### Components

**GraphExecutionSimulator**
```cpp
// Simulates node-by-node execution
// - Evaluates conditions
// - Traces execution path
// - Detects infinite loops
// - Reports unreachable nodes
```

**GraphExecutionTracer**
```cpp
// Records execution events
// - Entry/exit events
// - Condition results
// - Data pin resolutions
// - Error tracking
```

**ExecutionTestPanel**
```cpp
// ImGui panel displaying test results
// - Trace log (tabular)
// - Error list with severity
// - Execution summary
// - Full trace text output
```

### Integration in Refactored Editor

**Location**: `VisualScriptEditorPanel_Verification.cpp`

```cpp
// Stub for Phase 24.3 (ready for implementation):

std::vector<ValidationError> VisualScriptEditorPanel::RunExecutionTest()
{
    // When implemented:
    // 1. Create GraphExecutionSimulator
    // 2. Run simulation on m_template
    // 3. Populate ExecutionTestPanel with tracer results
    // 4. Display in properties or separate window

    return {};  // Placeholder
}

ExecutionTestPanel& VisualScriptEditorPanel::GetExecutionTestPanel()
{
    return m_executionTestPanel;
}
```

### Using Execution Tests

```cpp
// In future UI (not yet implemented in Phase 5):

// Button: "Test Execution"
// ↓
// RunExecutionTest() called
// ↓
// GraphExecutionSimulator runs node-by-node
// ↓
// GraphExecutionTracer records all events
// ↓
// ExecutionTestPanel displays results:
//   - Total steps: 42
//   - Errors: 0
//   - Status: SUCCESS
//   - Event log with each step
```

---

## COMPLETE EXAMPLE

### Creating a Simple Preset-Based Blueprint

```cpp
// Step 1: Initialize editor
VisualScriptEditorPanel editor;
editor.Initialize();

// Step 2: Add local variables (blackboard)
BlackboardEntry playerHealth;
playerHealth.Key = "Health";
playerHealth.Type = VariableType::Float;
playerHealth.Default = TaskValue(100.0f);
editor.m_template.Blackboard.push_back(playerHealth);

// Step 3: Create entry point
TaskNodeDefinition entry;
entry.Type = TaskNodeType::EntryPoint;
entry.NodeID = 1;
entry.NodeName = "Start";
editor.m_template.Nodes.push_back(entry);

// Step 4: Create preset condition
ConditionPreset healthCheck;
healthCheck.id = "is_alive";
healthCheck.left = Operand(OperandMode::Variable, "Health");
healthCheck.op = ComparisonOp::Greater;
healthCheck.right = Operand(OperandMode::Const, 0.0);

editor.m_presetRegistry.AddPreset(healthCheck);
editor.m_template.Presets.push_back(healthCheck);

// Step 5: Create branch node using preset
TaskNodeDefinition branch;
branch.Type = TaskNodeType::Branch;
branch.NodeID = 2;
branch.NodeName = "Is Alive?";

DynamicDataPin presetPin;
presetPin.label = "is_alive";
branch.DynamicDataInputPins.push_back(presetPin);

editor.m_template.Nodes.push_back(branch);

// Step 6: Connect nodes
editor.m_template.ExecConnections.push_back({
    1,              // Source node
    "Out",          // Source pin
    2,              // Target node
    "In"            // Target pin
});

// Step 7: Save
editor.m_currentFilePath = "alive_check.json";
editor.Save();

// Result: Preset-based blueprint saved with:
// - Local variable: Health = 100.0
// - Preset condition: is_alive (Health > 0)
// - Branch node using preset
```

### Using the Saved Blueprint

```cpp
// In another context:

VisualScriptEditorPanel editor2;
editor2.Initialize();
editor2.LoadTemplate("alive_check.json");

// Verify presets were loaded
assert(editor2.m_presetRegistry.GetPresetCount() > 0);

// Access preset
const ConditionPreset* preset = 
    editor2.m_presetRegistry.GetPreset("is_alive");

// Verify branch node references preset
const TaskNodeDefinition* branch = 
    editor2.m_template.GetNode(2);
assert(branch->Type == TaskNodeType::Branch);
assert(!branch->DynamicDataInputPins.empty());
assert(branch->DynamicDataInputPins[0].label == "is_alive");

// Success!
```

---

## TROUBLESHOOTING

### Issue: Preset Changes Don't Save

**Symptom**: Edit preset, save, reload - changes lost

**Cause**: SyncPresetsFromRegistryToTemplate() not called before save

**Fix**:
```cpp
// In VisualScriptEditorPanel_FileOps.cpp, Save() must:
void VisualScriptEditorPanel::Save()
{
    SyncCanvasFromTemplate();
    SyncPresetsFromRegistryToTemplate();  // ADD THIS
    ValidateAndCleanBlackboardEntries();
    SerializeAndWrite();
}
```

### Issue: Variables Don't Appear in Preset Editor

**Symptom**: RenderOperandEditor() dropdown is empty

**Cause**: m_template.Blackboard not populated or variables are invalid

**Fix**:
```cpp
// Verify variables:
for (const auto& entry : m_template.Blackboard) {
    if (entry.Key.empty() || entry.Type == VariableType::None) {
        SYSTEM_LOG << "Invalid variable: " << entry.Key << "\n";
        // BUG-001: Skip invalid entries
    }
}

// Add new variable:
BlackboardEntry newVar;
newVar.Key = "MyVar";          // Non-empty!
newVar.Type = VariableType::Int; // Not None!
m_template.Blackboard.push_back(newVar);
m_dirty = true;
```

### Issue: Branch Condition Preview Shows "[?]"

**Symptom**: RenderPresetItemCompact() shows "[Var: ?]" or "[Const: ?]"

**Cause**: BuildConditionPreview() can't interpret operand

**Fix**:
```cpp
// In VisualScriptEditorPanel_NodeProperties.cpp:

std::string VisualScriptEditorPanel::BuildConditionPreview(const Condition& cond)
{
    // Ensure operands are properly initialized:
    if (cond.leftMode == "Variable" && cond.leftVariable.empty()) {
        // BUG-029: Auto-initialize to first available
        cond.leftVariable = availableVars[0];
        m_dirty = true;
    }

    // Then preview will show proper value
}
```

### Issue: GlobalBlackboard Variables Not Showing

**Symptom**: RenderGlobalVariablesPanel() shows "(no global variables)"

**Cause**: GlobalTemplateBlackboard not initialized

**Fix**:
```cpp
// In Initialize():
GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
// Ensure global variables are populated from wherever they're stored
```

---

## BEST PRACTICES

1. **Always Sync Before Save**: Call SyncPresetsFromRegistryToTemplate()
2. **Validate Blackboard Entries**: Check for empty keys and None types
3. **Cache Pin Manager Results**: Don't call GetAllPins() every frame
4. **Handle Missing Presets Gracefully**: Check GetPreset() return value
5. **Use Forward Declarations**: Avoid circular includes
6. **Log Preset Operations**: Help with debugging

---

**End of Phase 24 & 24.3 Integration Guide**

References:
- DEVELOPER_GUIDE.md (general development)
- MAINTENANCE.md (troubleshooting)
- Original Phase 24 documentation
