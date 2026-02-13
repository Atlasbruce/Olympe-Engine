---
id: component-overrides
title: Component Scoped Overrides
sidebar_label: Component Overrides
---

# Component-Scoped TMJ Overrides

## Overview

This feature extends the Olympe Engine's TMJ (Tiled Map JSON) loading capabilities to support component-scoped property overrides, preventing cross-component property overwrites while maintaining full backward compatibility.

## Problem Statement

Previously, TMJ object properties were stored in a flat structure, which could cause issues when multiple components had parameters with the same name. For example:

```
Transform component: speed = 5.0 (movement speed)
Physics component: speed = 50.0 (velocity)
```

With the old flat structure, the last-set value would overwrite the first, causing incorrect behavior.

## Solution

### 1. Component-Scoped Override Storage

Extended `LevelInstanceParameters` to include a new `componentOverrides` field:

```cpp
struct LevelInstanceParameters {
    std::string objectName;
    std::string objectType;
    Vector position;
    
    // Legacy flat properties (backward compatibility)
    std::map<std::string, ComponentParameter> properties;
    
    // NEW: Component-scoped overrides
    std::map<std::string, std::map<std::string, ComponentParameter>> componentOverrides;
    
    std::vector<ObjectReference> objectReferences;
};
```

### 2. Automatic TMJ Property Parsing

The `TiledToOlympe::PropertiesToOverrides` function now supports dot notation for component scoping:

**TMJ Property Name Format:**
- `Component.parameter` → Scoped to specific component
- `parameter` → Flat property (legacy support)

**Examples:**
```
Transform.width = 32          → overrides["Transform"]["width"] = 32
Transform.height = 64         → overrides["Transform"]["height"] = 64
Physics.mass = 10.0           → overrides["Physics"]["mass"] = 10.0
health = 100                  → overrides["health"] = 100 (flat, legacy)
```

### 3. Priority-Based Parameter Resolution

The `ParameterResolver::ExtractComponentParameters` now uses a two-tier priority system:

**Priority 1 (HIGHEST):** Component-scoped overrides
- Checks `instanceParams.componentOverrides[componentType]`
- Direct component-to-parameter mapping
- No cross-component interference

**Priority 2 (FALLBACK):** Schema-based flat properties
- Checks `instanceParams.properties` using component schema
- Legacy behavior for backward compatibility
- Only applies if not already set by component overrides

### 4. JSON Override Structure Support

The `World::ExtractCustomProperties` function now handles both:

**Nested structure (component-scoped):**
```json
{
  "Transform": {
    "width": 32,
    "height": 64,
    "speed": 5.0
  },
  "Physics": {
    "mass": 10.0,
    "speed": 50.0
  }
}
```

**Flat structure (legacy):**
```json
{
  "width": 32,
  "health": 100,
  "level": 5
}
```

## Usage Examples

### In Tiled Editor

When creating objects in Tiled, use dot notation in custom property names:

1. Select an object
2. Add custom properties:
   - Name: `Transform.width`, Value: `32` (float)
   - Name: `Transform.height`, Value: `64` (float)
   - Name: `Physics.mass`, Value: `10.0` (float)
   - Name: `Health.maxHealth`, Value: `100` (int)

### In Code

The component-scoped overrides are automatically handled during level loading:

```cpp
// Load TMJ level
TiledLevelLoader loader;
TiledMap tiledMap = loader.LoadFromFile("level.tmj");

// Convert to Olympe format
TiledToOlympe converter;
LevelDefinition level;
converter.Convert(tiledMap, level);

// EntityInstance overrides are now component-scoped
for (auto& entity : level.entities) {
    // overrides["Transform"]["width"] is separate from
    // overrides["Physics"]["width"] - no conflicts!
}
```

## Benefits

1. **No Cross-Component Overwrites**: Each component maintains its own parameter namespace
2. **Automatic TMJ Field Mapping**: Dot notation automatically creates component scope
3. **Full Backward Compatibility**: Existing levels with flat properties continue to work
4. **C++14 Compatible**: Uses only C++14 standard features, no modern C++ dependencies
5. **Transparent Integration**: No changes required to existing code using the API

## Testing

Run the example test to verify the implementation:

```bash
# Compile
g++ -std=c++14 -o test_overrides Examples/TiledLoader/example_component_overrides.cpp

# Run
./test_overrides
```

Expected output:
```
✓✓✓ SUCCESS: No cross-component overwrites detected!
    Transform.speed and Physics.speed coexist independently.
```

## Files Modified

1. `Source/ParameterResolver.h` - Added `componentOverrides` field to `LevelInstanceParameters`
2. `Source/ParameterResolver.cpp` - Updated `ExtractComponentParameters` with priority system
3. `Source/World.cpp` - Updated `ExtractCustomProperties` to parse nested JSON structures
4. `Source/TiledLevelLoader/src/TiledToOlympe.cpp` - Updated `PropertiesToOverrides` to parse dot notation

## Migration Guide

### For New Levels

Use component-scoped property names in Tiled:
```
Transform.width = 32
Transform.height = 64
Physics.mass = 10.0
```

### For Existing Levels

No changes required! Flat properties are still supported and will be automatically mapped to components using the parameter schema system.

### For Custom Code

If you manually construct `LevelInstanceParameters`, you can now use:

```cpp
LevelInstanceParameters params("MyEntity", "Enemy");

// NEW: Component-scoped (recommended)
params.componentOverrides["Transform"]["width"] = ComponentParameter::FromFloat(32.0f);
params.componentOverrides["Physics"]["mass"] = ComponentParameter::FromFloat(10.0f);

// LEGACY: Flat properties (still supported)
params.properties["health"] = ComponentParameter::FromInt(100);
```

## Performance Impact

Minimal to none:
- Component override lookup is O(log n) map access (same as before)
- No additional allocations during parameter resolution
- Schema-based extraction only runs when needed (fallback)

## Future Enhancements

Potential improvements for future versions:
- Visual property editor in Blueprint Editor with component grouping
- Validation warnings for unknown component names
- Auto-completion for component names in Tiled (via external tooling)
- Migration tool to convert flat properties to component-scoped format
