---
id: behavior-tree-dependency-loading
title: Behavior Tree Dependency Loading
sidebar_label: Dependency Loading
---

# Dependency-Aware Behavior Tree Loading System

## Overview

The dependency-aware behavior tree loading system automatically detects and loads required Behavior Trees during level loading. This eliminates manual BT registration, prevents runtime stuttering, and optimizes memory usage by loading only what's needed per level.

## Architecture

### New Components

1. **BehaviorTreeDependencyScanner** (`Source/AI/BehaviorTreeDependencyScanner.h/cpp`)
   - Scans level JSON to extract prefab types
   - Scans prefabs to find BehaviorTreeRuntime_data components
   - Extracts treePath properties to determine BT dependencies
   - Generates consistent IDs from BT paths using FNV-1a hashing

2. **BehaviorTreeManager Path Registry** (Updated `Source/AI/BehaviorTree.h/cpp`)
   - Maintains path-to-ID mapping (`m_pathToIdMap`)
   - New methods:
     - `GetTreeIdFromPath(treePath)` - Get tree ID from path
     - `IsTreeLoadedByPath(treePath)` - Check if tree is loaded
     - `GetTreeByPath(treePath)` - Get tree asset by path

3. **World Level Dependency Loading** (Updated `Source/World.h/cpp`)
   - New method: `LoadLevelDependencies(levelJson)`
   - Integrated into `LoadLevelFromTiled()` as Phase 2.5
   - Executes before resource preloading (Phase 3)

4. **PrefabFactory BT Instantiation** (Updated `Source/PrefabFactory.cpp`)
   - `InstantiateBehaviorTreeRuntime()` now maps treePath to treeId
   - Verifies tree is loaded during instantiation
   - Provides warnings if dependencies aren't met

## Loading Pipeline

```
1. Parse Level TMJ/JSON
   ↓
2. Extract Prefab Types Used
   ↓
3. Scan Prefabs for BT Dependencies
   ↓
4. Load Required Behavior Trees (with caching)
   ↓
5. Preload Resources
   ↓
6. Instantiate Entities (all dependencies ready ✅)
   ↓
7. Start Gameplay
```

## Usage

### In Entity Prefabs

Simply reference the behavior tree by path in your prefab JSON:

```json
{
    "type": "BehaviorTreeRuntime_data",
    "properties": {
        "treePath": "Blueprints/AI/npc_wander.json",
        "active": true
    }
}
```

The system will:
1. Detect this dependency during level loading
2. Automatically load the behavior tree
3. Generate a consistent ID from the path
4. Map the path to the ID for instantiation

### Manual Loading (Optional)

You can still manually load behavior trees if needed:

```cpp
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/custom_ai.json", treeId);
```

The path will be registered in the path-to-ID map automatically.

## Benefits

✅ **Zero Manual Registration** - No more forgetting to register BT files  
✅ **Per-Level Loading** - Only load what's needed (optimized memory)  
✅ **No Runtime Stuttering** - All assets loaded before gameplay  
✅ **Clear Debugging** - Detailed logs show exactly what's loaded  
✅ **Scalable** - Works with 1 BT or 100+ BTs  
✅ **Caching** - BTs remain cached across level reloads  
✅ **Backward Compatible** - Existing prefabs work without changes  

## Console Output Example

```
+===========================================================+
| LEVEL DEPENDENCY LOADING                                  |
+===========================================================+
Step 1/3: Extracting prefab types from level...
  -> Found 2 unique prefab type(s):
     - test_npc_wander
     - guard_entity

Step 2/3: Scanning prefabs for behavior tree dependencies...
[BTDepScanner] Scanning 2 prefabs for BT dependencies...
[BTDepScanner]   Found BT dependency: Blueprints/AI/npc_wander.json (ID=3245678912)
[BTDepScanner]   Found BT dependency: Blueprints/AI/guard_patrol.json (ID=1234567890)
[BTDepScanner] Found 2 unique BT dependencies

Step 3/3: Loading required behavior trees...
  [LOADING] Blueprints/AI/npc_wander.json (ID=3245678912)... SUCCESS
  [LOADING] Blueprints/AI/guard_patrol.json (ID=1234567890)... SUCCESS

+===========================================================+
| DEPENDENCY LOADING SUMMARY                                |
+===========================================================+
| Behavior Trees Required:   2
| Loaded This Session:       2
| Already Cached:            0
| Failed:                    0
+===========================================================+
```

## Implementation Details

### FNV-1a Hash Function

The system uses FNV-1a (Fowler-Noll-Vo) hash to generate consistent IDs from paths:

```cpp
uint32_t hash = 2166136261u; // FNV-1a offset basis
for (char c : treePath) {
    hash ^= static_cast<uint32_t>(c);
    hash *= 16777619u; // FNV-1a prime
}
```

This ensures:
- Same path always generates the same ID
- Hash collisions are extremely rare (32-bit hash space)
- No need for manual ID assignment

### Path-to-ID Registry

The registry persists across level loads, enabling caching:

```cpp
std::map<std::string, uint32_t> m_pathToIdMap;
```

When a BT is loaded, its path is registered:
```cpp
m_pathToIdMap[filepath] = treeId;
```

When a prefab is instantiated, the path is resolved:
```cpp
uint32_t treeId = BehaviorTreeManager::Get().GetTreeIdFromPath(treePath);
```

## Testing

### Test Prefab

A test prefab is provided at `Blueprints/EntityPrefab/test_npc_wander.json` that demonstrates the system.

### Test Procedure

1. Create a level in Tiled with the test_npc_wander prefab
2. Load the level in the engine
3. Observe console output showing automatic BT loading
4. NPCs should spawn with functioning wander behavior
5. Reload the level - BTs should show as "CACHED"

### Expected Behavior

- **First Load**: BTs are loaded and logged as "SUCCESS"
- **Subsequent Loads**: BTs show as "CACHED" (not reloaded)
- **Missing BT**: Clear error message with path shown
- **Invalid Path**: Warning during prefab instantiation

## Error Handling

The system provides clear error messages:

1. **Missing BT File**:
   ```
   [ERROR] Failed to load behavior tree: Blueprints/AI/nonexistent.json
   Failed: 1
   ```

2. **Prefab Not Found**:
   ```
   [BTDepScanner] Warning: Prefab not found: unknown_prefab
   ```

3. **BT Not Loaded During Instantiation**:
   ```
   [PrefabFactory] WARNING: BehaviorTree not loaded: Blueprints/AI/custom.json (ID=123456)
   ```

## Future Enhancements

Potential improvements:
- Unload BTs when changing levels to free memory
- Hot-reload support for BT changes during development
- Parallel loading for multiple BTs
- Dependency validation during prefab scanning
- BT precompilation/validation at build time

## Migration Guide

### Old System (Manual Registration)

```cpp
// In game initialization:
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/idle.json", 1);
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_patrol.json", 2);

// In prefab:
{
    "type": "BehaviorTreeRuntime_data",
    "properties": {
        "treeAssetId": 1,  // Manual ID
        "active": true
    }
}
```

### New System (Automatic Loading)

```cpp
// No game initialization code needed!

// In prefab:
{
    "type": "BehaviorTreeRuntime_data",
    "properties": {
        "treePath": "Blueprints/AI/idle.json",  // Just the path
        "active": true
    }
}
```

The system automatically:
1. Detects the dependency
2. Loads the BT file
3. Generates and maps the ID

## Files Modified

- `Source/AI/BehaviorTreeDependencyScanner.h` (NEW)
- `Source/AI/BehaviorTreeDependencyScanner.cpp` (NEW)
- `Source/AI/BehaviorTree.h` (Updated)
- `Source/AI/BehaviorTree.cpp` (Updated)
- `Source/World.h` (Updated)
- `Source/World.cpp` (Updated)
- `Source/PrefabFactory.cpp` (Updated)
- `CMakeLists.txt` (Updated)

## License

This implementation is part of Olympe Engine V2 - 2025
© Nicolas Chereau
