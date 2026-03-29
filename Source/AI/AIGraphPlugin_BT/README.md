# AIGraphPlugin_BT - Behavior Tree Graph Plugin

## Overview

AIGraphPlugin_BT is the first specific plugin for NodeGraphCore, implementing a complete Behavior Tree editor with validation, compilation, and UI components.

## Architecture

This plugin transforms the generic NodeGraphCore into a functional Behavior Tree editor by providing:

1. **BTNodeRegistry** - Central registry of all BT node types with metadata
2. **BTGraphValidator** - Validation system enforcing BT structural rules
3. **BTGraphCompiler** - Compiler from GraphDocument to BehaviorTreeAsset
4. **BTNodePalette** - ImGui palette for drag-and-drop node creation

## Node Types (18+)

### Composites (Flow Control)
- **BT_Selector** - OR logic, executes children until one succeeds
- **BT_Sequence** - AND logic, executes children until one fails
- **BT_Parallel** - Executes all children simultaneously

### Decorators (Modifiers)
- **BT_Inverter** - Inverts child result (SUCCESS <-> FAILURE)
- **BT_Repeater** - Repeats child N times
- **BT_UntilSuccess** - Repeats child until it succeeds
- **BT_UntilFailure** - Repeats child until it fails
- **BT_Cooldown** - Limits execution frequency

### Conditions (Boolean Checks)
- **BT_CheckBlackboardValue** - Compares blackboard value
- **BT_HasTarget** - Checks if entity has a target
- **BT_IsTargetInRange** - Checks distance to target
- **BT_CanSeeTarget** - Checks line of sight

### Actions (Leaf Execution)
- **BT_Wait** - Waits for N seconds
- **BT_WaitRandomTime** - Waits for random duration
- **BT_SetBlackboardValue** - Modifies blackboard value
- **BT_MoveToTarget** - Moves entity towards target
- **BT_MoveToPosition** - Moves entity to position
- **BT_AttackTarget** - Attacks current target
- **BT_PlayAnimation** - Plays animation
- **BT_EmitSound** - Emits sound effect

## Validation Rules

The validator enforces 7 rules:

1. **Root Unique** - Exactly one node without parent
2. **No Cycles** - Graph must be acyclic (DFS detection)
3. **Valid Children Count** - Respects min/max per node type
4. **No Orphans** - All nodes connected to root (BFS check)
5. **Valid Types** - All node types registered
6. **Decorators** - Exactly 1 child
7. **Composites** - Minimum 1 child

## Usage

### Registry Query
```cpp
#include "AI/AIGraphPlugin_BT/BTNodeRegistry.h"

auto& registry = BTNodeRegistry::Get();
auto allTypes = registry.GetAllNodeTypes();
const BTNodeTypeInfo* info = registry.GetNodeTypeInfo("BT_Selector");
```

### Validation
```cpp
#include "AI/AIGraphPlugin_BT/BTGraphValidator.h"

auto messages = BTGraphValidator::ValidateGraph(&graphDocument);
for (const auto& msg : messages) {
    if (msg.severity == BTValidationSeverity::Error) {
        // Handle error
    }
}
```

### Compilation
```cpp
#include "AI/AIGraphPlugin_BT/BTGraphCompiler.h"

BehaviorTreeAsset asset;
std::string error;
if (BTGraphCompiler::Compile(&graphDocument, asset, error)) {
    // Success - use asset
} else {
    // Error - check error message
}
```

### UI Palette
```cpp
#include "AI/AIGraphPlugin_BT/BTNodePalette.h"

BTNodePalette palette;
bool isOpen = true;
palette.Render(&isOpen);

if (palette.IsDragging()) {
    std::string nodeType = palette.GetDraggedNodeType();
    // Create node of this type
}
```

## Testing

Run the test suite:
```bash
# Build with tests enabled
cmake -DBUILD_TESTS=ON ..
make TestAIGraphPlugin_BT
./TestAIGraphPlugin_BT
```

Expected output: All 10 tests pass

## Dependencies

- **NodeGraphCore** (Phase 1.1) - Required
- **BehaviorTree.h** - Runtime BT structures
- **ImGui** - For UI palette
- **json_helper.h** - For JSON operations (not used yet, but required by coding rules)

## Coding Standards

This plugin strictly follows:
- C++14 standard (no C++17/20 features)
- All code in `namespace Olympe::AI`
- Uses `SYSTEM_LOG` for logging
- JSON helpers for all JSON operations
- No direct `json["key"]` access
- No emojis in logs

## Files

```
Source/AI/AIGraphPlugin_BT/
├── BTNodeRegistry.h          - Node type registry interface
├── BTNodeRegistry.cpp        - Registry implementation (18+ types)
├── BTGraphValidator.h        - Validation system interface
├── BTGraphValidator.cpp      - Validation rules implementation
├── BTGraphCompiler.h         - Compiler interface
├── BTGraphCompiler.cpp       - GraphDocument to BehaviorTreeAsset
├── BTNodePalette.h           - UI palette interface
├── BTNodePalette.cpp         - ImGui palette implementation
├── TestAIGraphPlugin_BT.cpp  - Test suite (10 tests)
└── README.md                 - This file
```

## Integration

The plugin is automatically included via CMakeLists.txt:
- All `Source/AI/*.cpp` files are built
- No manual CMake changes needed

## Future Enhancements

Potential Phase 2 improvements:
- Runtime node type registration API
- Custom node type plugins
- Visual scripting integration
- Performance profiling
- Blackboard editor integration

## License

Part of Olympe Engine V2 - 2025
