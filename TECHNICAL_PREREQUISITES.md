# Technical Prerequisites - Testing Guide

This guide explains how to test and validate the technical prerequisites for the Olympe Blueprint Editor.

## Overview

The technical prerequisites (Phase 0) include:
1. **ImGui & ImNodes Integration** - Visual UI libraries for the Blueprint Editor
2. **JSON Helper API** - Centralized JSON operations
3. **Unified Asset Loader** - Single API for all asset types

## Quick Validation

### 1. Test ImGui & ImNodes Integration

Run the integration test to verify ImGui and ImNodes are properly set up:

```bash
cd Examples

# Compile the test
g++ -std=c++17 \
    -I../Source/third_party \
    -I../Source/third_party/imgui \
    -I../Source/third_party/imnodes \
    imgui_imnodes_test.cpp \
    ../Source/third_party/imgui/imgui.cpp \
    ../Source/third_party/imgui/imgui_demo.cpp \
    ../Source/third_party/imgui/imgui_draw.cpp \
    ../Source/third_party/imgui/imgui_tables.cpp \
    ../Source/third_party/imgui/imgui_widgets.cpp \
    ../Source/third_party/imnodes/imnodes.cpp \
    -o imgui_imnodes_test

# Run the test
./imgui_imnodes_test
```

**Expected Output:**
```
=== Olympe Engine - ImGui & ImNodes Integration Test ===

[ImGui Test]
  Creating ImGui context...
  ✓ ImGui context created successfully!
  ✓ ImGui version: 1.92.6 WIP
  ✓ ImGuiIO initialized
  ✓ Font atlas built (512x128)
  ✓ ImGui frame rendered (simulated)

[ImNodes Test]
  Creating ImNodes context...
  ✓ ImNodes context created successfully!
  ✓ ImNodes node created (simulated)
  ✓ ImNodes context destroyed

[Cleanup]
  ✓ ImGui context destroyed

=== All Tests Passed! ===
ImGui and ImNodes are properly integrated and ready to use.
```

### 2. Test Asset Loader API

Run the asset loader test to verify the unified asset API:

```bash
cd Examples

# Compile the test
g++ -std=c++17 \
    -I../Source \
    -I../Source/third_party \
    asset_loader_test.cpp \
    -o asset_loader_test

# Run the test
./asset_loader_test
```

**Expected Output:**
```
=== Olympe Engine - Asset Loader Test ===

[Test AssetType Conversion]
  ✓ AssetType to string conversion
  ✓ String to AssetType conversion

[Test AssetInfo]
  ✓ AssetInfo serialized
  ✓ AssetInfo deserialized

[Test EntityBlueprint]
  ✓ Entity blueprint saved
  ✓ Entity blueprint loaded
  ✓ Entity blueprint data verified

[Test BehaviorTree]
  ✓ Behavior tree saved
  ✓ Behavior tree loaded
  ✓ Behavior tree data verified

[Test HFSM]
  ✓ HFSM saved
  ✓ HFSM loaded
  ✓ HFSM data verified

[Test Asset Validation]
  ✓ Valid asset passes validation
  ✓ Asset without 'type' fails validation
  ✓ Asset without 'name' fails validation

=== All Tests Passed! ===
Asset Loader API is working correctly.
```

## Detailed Component Documentation

### ImGui & ImNodes

**Location:** `Source/third_party/imgui/` and `Source/third_party/imnodes/`

**Usage:**
```cpp
#include "third_party/imgui/imgui.h"
#include "third_party/imnodes/imnodes.h"

// Initialize
IMGUI_CHECKVERSION();
ImGuiContext* ctx = ImGui::CreateContext();
ImNodesContext* nodes_ctx = ImNodes::CreateContext();

// Use in your application loop
ImGui::NewFrame();
ImGui::Begin("My Window");
ImGui::Text("Hello, World!");
ImGui::End();

ImNodes::BeginNodeEditor();
// ... create nodes
ImNodes::EndNodeEditor();

ImGui::Render();
// ... render with your backend
```

**References:**
- [ImGui GitHub](https://github.com/ocornut/imgui)
- [ImNodes GitHub](https://github.com/Nelarius/imnodes)

### JSON Helper

**Location:** `Source/json_helper.h`

**Features:**
- Safe JSON loading and saving
- Type-safe accessors with default values
- Array and object iteration
- Validation functions

**Usage Examples:**
```cpp
#include "json_helper.h"

// Load a JSON file
json data;
if (JsonHelper::LoadJsonFromFile("config.json", data))
{
    // Safe accessors with defaults
    std::string name = JsonHelper::GetString(data, "name", "default");
    int value = JsonHelper::GetInt(data, "value", 0);
    float x = JsonHelper::GetFloat(data, "x", 0.0f);
    
    // Check and iterate arrays
    if (JsonHelper::IsArray(data, "items"))
    {
        JsonHelper::ForEachInArray(data, "items", 
            [](const json& item, size_t index)
            {
                std::cout << "Item " << index << std::endl;
            });
    }
}

// Save JSON with formatting
json output;
output["name"] = "example";
output["value"] = 42;
JsonHelper::SaveJsonToFile("output.json", output, 4);
```

### Unified Asset Loader

**Location:** `Source/AssetLoader.h`

**Supported Asset Types:**
- Entity Blueprints
- Prefabs
- Behavior Trees (BT)
- Hierarchical Finite State Machines (HFSM)
- Templates
- Custom assets

**Usage Examples:**

#### Loading Any Asset
```cpp
#include "AssetLoader.h"
using namespace Olympe::Assets;

json asset;
AssetInfo info;
if (AssetLoader::LoadAssetWithInfo("asset.json", asset, info))
{
    std::cout << "Loaded " << AssetTypeToString(info.type) 
              << ": " << info.name << std::endl;
}
```

#### Loading Specific Asset Types
```cpp
// Entity Blueprint
json blueprint;
if (EntityBlueprintLoader::LoadEntityBlueprint("entity.json", blueprint))
{
    // Use the blueprint
}

// Behavior Tree
json bt;
if (BehaviorTreeLoader::LoadBehaviorTree("behavior.json", bt))
{
    // Use the behavior tree
}

// HFSM
json hfsm;
if (HFSMLoader::LoadHFSM("statemachine.json", hfsm))
{
    // Use the state machine
}
```

#### Saving Assets
```cpp
// Create asset JSON
json asset;
asset["schema_version"] = 1;
asset["type"] = "EntityBlueprint";
asset["name"] = "MyEntity";
asset["components"] = json::array();

// Save it
AssetLoader::SaveAsset("my_entity.json", asset);
```

#### Asset Validation
```cpp
// Validate required fields
if (AssetLoader::ValidateAsset(asset, {"components"}))
{
    std::cout << "Asset is valid" << std::endl;
}
```

## Asset JSON Schema

All assets should follow this base schema:

```json
{
  "schema_version": 1,
  "type": "EntityBlueprint|Prefab|BehaviorTree|HFSM|Template",
  "name": "AssetName",
  "description": "Optional description",
  // ... asset-specific fields
}
```

### Entity Blueprint Schema
```json
{
  "schema_version": 1,
  "type": "EntityBlueprint",
  "name": "MyEntity",
  "description": "Description",
  "components": [
    {
      "type": "Position",
      "properties": {
        "x": 0.0,
        "y": 0.0
      }
    }
  ]
}
```

### Behavior Tree Schema
```json
{
  "schema_version": 1,
  "type": "BehaviorTree",
  "name": "MyBehaviorTree",
  "description": "Description",
  "nodes": [
    {
      "id": 1,
      "type": "Selector",
      "children": [2, 3]
    }
  ]
}
```

### HFSM Schema
```json
{
  "schema_version": 1,
  "type": "HFSM",
  "name": "MyStateMachine",
  "description": "Description",
  "states": [
    {
      "name": "Idle",
      "transitions": []
    }
  ]
}
```

## Troubleshooting

### ImGui/ImNodes Not Found
- Ensure submodules are initialized: `git submodule update --init --recursive`
- Check include paths in your build configuration
- Verify files exist in `Source/third_party/imgui/` and `Source/third_party/imnodes/`

### Compilation Errors
- Use C++17 or later: `-std=c++17`
- Include all necessary ImGui source files (not just headers)
- Include both ImGui and ImNodes directories in include path

### Asset Loading Failures
- Check file paths are correct
- Verify JSON is valid (use a JSON validator)
- Ensure required fields (`type`, `name`) are present
- Check console output for detailed error messages

## Next Steps

After validating the technical prerequisites:

1. **Integrate ImGui/ImNodes in OlympeBlueprintEditor** - Add visual UI to the editor
2. **Use AssetLoader API** - Replace direct JSON operations with the unified API
3. **Create Visual Node Editor** - Implement behavior tree and HFSM editors
4. **Add Asset Templates** - Create template assets for common patterns

## Additional Resources

- [BUILD.md](../BUILD.md) - Complete build instructions
- [README.md](../README.md) - Project overview
- [ARCHITECTURE.md](../ARCHITECTURE.md) - Engine architecture
- [Blueprints/README.md](../Blueprints/README.md) - Blueprint system documentation

---

**Olympe Engine V2 - Technical Prerequisites Complete**  
*Ready for Blueprint Editor Phase 1-9 Implementation*
