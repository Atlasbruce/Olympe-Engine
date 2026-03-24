# Action Parameters Panel - Implementation Guide

## What's Been Implemented

### 1. **ActionParametersPanel** (New Component)
   - Files: `Source/Editor/Panels/ActionParametersPanel.h/.cpp`
   - A dedicated ImGui panel for editing action node parameters
   - Supports multiple parameter types: String, Int, Float, Bool
   - Integrates with the Inspector Panel

### 2. **Enhanced AtomicTaskUIRegistry**
   - File: `Source/BlueprintEditor/AtomicTaskUIRegistry.h/.cpp`
   - Added `TaskParameter` struct for parameter metadata
   - Updated `TaskSpec` to include a vector of parameters
   - Pre-populated all action tasks with their parameters:

   **Examples:**
   - **Log Message** → `message` (String)
   - **Patrol Path** → `pathName` (String), `speed` (Float)
   - **Play Animation** → `animationName` (String), `speed` (Float), `loop` (Bool)
   - **Play Sound** → `soundName` (String), `volume` (Float), `loop` (Bool)
   - **Move To Goal** → `targetKey` (String), `speed` (Float)
   - And many more...

### 3. **InspectorPanel Integration**
   - File: `Source/BlueprintEditor/InspectorPanel.h/.cpp`
   - Added methods to select/display action nodes:
     - `SetSelectedActionNode(taskID, nodeName, parameters)`
     - `ClearSelectedActionNode()`
   - New `RenderActionNodeInspector()` method
   - Automatically shows action parameters when an action node is selected

## How to Use It

### In Your Node Graph Editor:

When a user clicks on an action node (like "Log Message" or "Patrol Path"), call:

```cpp
// Get the action node's properties
std::string taskID = actionNode.taskID;          // e.g., "log_message"
std::string nodeName = actionNode.displayName;   // e.g., "Log Message #1"
std::unordered_map<std::string, std::string> params;
params["message"] = "Debug information";
// ... populate other parameters

// Update the inspector to show parameters
InspectorPanel::Get().SetSelectedActionNode(taskID, nodeName, params);
```

### To Show Action Parameters in the Inspector:

The panel automatically displays:
- **Title section** with action name and task ID
- **Parameter inputs** for each parameter:
  - Text fields for String parameters
  - Drag controls for Float/Int
  - Checkboxes for Bool
- **Clear Selection** button

### Parameters are Read Back Via:

```cpp
// To get edited parameters after user modifications:
const auto& params = actionParametersPanel.GetParameters();
for (const auto& param : params) {
    std::string name = param.name;      // e.g., "message"
    std::string value = param.value;    // e.g., "Detected enemy!"
    bool modified = param.isDirty;      // Whether user edited it
}

// Or get a single parameter:
std::string messageValue = actionParametersPanel.GetParameterValue("message");

// Check if any parameter was modified:
if (actionParametersPanel.IsDirty()) {
    // Save changes, update node, etc.
    actionParametersPanel.ClearDirty();
}
```

## Task Registry - Available Actions

All the following actions now have parameters defined:

### Movement
- **Move To Goal** - `targetKey` (String), `speed` (Float)
- **Rotate To Face** - `targetKey` (String), `rotationSpeed` (Float)
- **Patrol Path** - `pathName` (String), `speed` (Float)

### Combat
- **Attack If Close** - `targetKey` (String), `range` (Float), `damage` (Float)
- **Perform Dodge** - `dodgeDistance` (Float), `dodgeSpeed` (Float)
- **Take Cover** - `searchRadius` (Float), `moveSpeed` (Float)

### Animation
- **Play Animation** - `animationName` (String), `speed` (Float), `loop` (Bool)
- **Stop Animation** - `fadeOutTime` (Float)
- **Blend Animation** - `animationNameA` (String), `animationNameB` (String), `blendWeight` (Float)

### Audio
- **Play Sound** - `soundName` (String), `volume` (Float), `loop` (Bool)
- **Stop Sound** - `soundName` (String), `fadeOutTime` (Float)
- **Set Volume** - `channelName` (String), `volume` (Float)

### Misc
- **Log Message** - `message` (String)
- **Set State** - `stateName` (String), `stateValue` (String)
- **Clear Target** - No parameters

## Files Modified

1. **Source/BlueprintEditor/AtomicTaskUIRegistry.h** - Added TaskParameter struct
2. **Source/BlueprintEditor/AtomicTaskUIRegistry.cpp** - Added parameter definitions to all tasks
3. **Source/BlueprintEditor/InspectorPanel.h** - Added action node support
4. **Source/BlueprintEditor/InspectorPanel.cpp** - Implemented RenderActionNodeInspector()
5. **OlympeBlueprintEditor/OlympeBlueprintEditor.vcxproj** - Added source files

## Files Created

1. **Source/Editor/Panels/ActionParametersPanel.h** - Header file
2. **Source/Editor/Panels/ActionParametersPanel.cpp** - Implementation

## Next Steps (Integration)

To fully activate this feature in your node editor:

1. In your **NodeGraphPanel** or action node handler, detect when an action node is selected
2. Call `InspectorPanel::Get().SetSelectedActionNode()` with the node's data
3. When the user modifies parameters, read them back and save to the node
4. Optionally, expand the parameter definitions in `AtomicTaskUIRegistry.cpp` with more details or different types

## Example Integration

```cpp
// In NodeGraphPanel or similar
void OnActionNodeSelected(const ActionNode& node) {
    // Prepare parameters map
    std::unordered_map<std::string, std::string> params;
    for (const auto& [key, value] : node.parameters) {
        params[key] = value;
    }

    // Show in inspector
    InspectorPanel::Get().SetSelectedActionNode(
        node.taskID,
        node.displayName,
        params
    );
}

// When user changes parameters and wants to save
void OnParametersChanged() {
    auto* panel = InspectorPanel::Get().GetActionPanel(); // Add this getter
    if (panel && panel->IsDirty()) {
        for (const auto& param : panel->GetParameters()) {
            selectedNode.parameters[param.name] = param.value;
        }
        panel->ClearDirty();
        SaveNode(selectedNode);
    }
}
```

