# Blueprint JSON Schema Fit/Gap Analysis and Migration

## Overview

This document describes the JSON schema validation, normalization, and migration system for Olympe Engine blueprints. The system ensures all blueprint files have consistent structure and required fields, detecting and correcting common issues.

## Schema Versions

### Version 1 (Legacy)
- Flat structure with nodes/components at root level
- No metadata or editor state
- Type detection via structure only

### Version 2 (Current)
- Wrapped structure with `data` section
- Required fields: `schema_version`, `type`, `blueprintType`
- Optional but recommended: `metadata`, `editorState`
- Node positions included in blueprint

## Blueprint Types

The validator detects and validates the following blueprint types:

### 1. BehaviorTree
**Detection Heuristics:**
- Contains `rootNodeId` field
- Contains `nodes` array

**Required Fields:**
- `data.nodes` (array)
- `data.rootNodeId` (integer)

**Example Structure:**
```json
{
  "schema_version": 2,
  "type": "BehaviorTree",
  "blueprintType": "BehaviorTree",
  "name": "GuardPatrol",
  "metadata": { ... },
  "editorState": { ... },
  "data": {
    "rootNodeId": 1,
    "nodes": [ ... ]
  }
}
```

### 2. HFSM (Hierarchical Finite State Machine)
**Detection Heuristics:**
- Contains `states` array OR `initialState` field

**Required Fields:**
- `data.states` (array)
- `data.initialState` (string or integer)

**Example Structure:**
```json
{
  "schema_version": 2,
  "type": "HFSM",
  "blueprintType": "HFSM",
  "name": "NPCStateMachine",
  "metadata": { ... },
  "editorState": { ... },
  "data": {
    "initialState": "Idle",
    "states": [ ... ]
  }
}
```

### 3. EntityPrefab / EntityBlueprint
**Detection Heuristics:**
- Contains `components` array at root OR in `data`
- Contains `data.prefabName` field

**Required Fields:**
- `data.components` (array)

**Example Structure:**
```json
{
  "schema_version": 2,
  "type": "EntityPrefab",
  "blueprintType": "EntityPrefab",
  "name": "PlayerEntity",
  "metadata": { ... },
  "data": {
    "prefabName": "PlayerEntity",
    "components": [ ... ]
  }
}
```

### 4. UIBlueprint
**Detection Heuristics:**
- Contains `elements` array

**Required Fields:**
- `elements` (array)

### 5. Level
**Detection Heuristics:**
- Contains `worldSize` OR `entities` field

**Required Fields:**
- `worldSize` OR `entities`

### 6. Catalog
**Detection Heuristics:**
- Contains `catalogType` field

### 7. Template
**Detection Heuristics:**
- Contains `blueprintData` field

## Validation Features

### BlueprintValidator Methods

#### DetectType(json)
Detects blueprint type using heuristics when `type` field is missing.

**Priority:**
1. Check explicit `type` field
2. Use structural heuristics
3. Default to "Generic"

**Returns:** String type name

#### Normalize(json)
Adds missing required fields with sensible defaults.

**Changes Made:**
- Adds `schema_version: 2` if missing
- Detects and adds `type` if missing
- Adds `blueprintType` (matching type)
- Creates `metadata` object with defaults:
  - `author: "Unknown"`
  - `created: ""`
  - `lastModified: ""`
  - `tags: []`
- Creates `editorState` object with defaults:
  - `zoom: 1.0`
  - `scrollOffset: {x: 0, y: 0}`

**Returns:** Boolean (true if changes were made)

#### ValidateJSON(json, errors)
Validates blueprint has all required fields for its type.

**Returns:** Boolean (true if valid)
**Output:** `errors` string contains description if invalid

## Usage Examples

### Detecting Missing Type
```cpp
BlueprintValidator validator;
nlohmann::json blueprint = LoadBlueprint("guard_patrol.json");

// Detect type if missing
std::string type = validator.DetectType(blueprint);
std::cout << "Detected type: " << type << std::endl;
```

### Normalizing Blueprint
```cpp
BlueprintValidator validator;
nlohmann::json blueprint = LoadBlueprint("guard_patrol.json");

// Add missing fields
bool modified = validator.Normalize(blueprint);
if (modified) {
    std::cout << "Blueprint normalized, missing fields added" << std::endl;
    // Optionally save back to file
}
```

### Validating Blueprint
```cpp
BlueprintValidator validator;
nlohmann::json blueprint = LoadBlueprint("guard_patrol.json");

// Validate structure
std::string errors;
if (!validator.ValidateJSON(blueprint, errors)) {
    std::cerr << "Validation failed: " << errors << std::endl;
}
```

### Complete Workflow
```cpp
BlueprintValidator validator;
nlohmann::json blueprint = LoadBlueprint("example.json");

// 1. Normalize
bool normalized = validator.Normalize(blueprint);

// 2. Validate
std::string errors;
bool valid = validator.ValidateJSON(blueprint, errors);

// 3. Report
if (normalized) {
    std::cout << "Normalized blueprint (added missing fields)" << std::endl;
}
if (!valid) {
    std::cerr << "Validation errors: " << errors << std::endl;
}

// 4. Optionally save if normalized
if (normalized) {
    SaveBlueprint("example.json", blueprint);
}
```

## Integration Points

### Asset Loading
`BlueprintEditor::DetectAssetType()` uses `BlueprintValidator::DetectType()` to identify blueprint types when the `type` field is missing.

### Asset Scanning
When scanning the Blueprints directory, the validator logs warnings for files missing type information.

## Common Issues and Fixes

### Issue: Missing `type` Field
**Symptom:** Asset appears as "Generic" in browser
**Fix:** Validator detects type via structure and can add the field
**Action:** Call `Normalize()` on the blueprint

### Issue: Missing `metadata`
**Symptom:** No author or timestamp information
**Fix:** `Normalize()` adds default metadata structure
**Action:** Call `Normalize()` and optionally populate fields

### Issue: Missing `editorState`
**Symptom:** Editor zoom/scroll not persisted
**Fix:** `Normalize()` adds default editor state
**Action:** Call `Normalize()` to add defaults

### Issue: Catalog Files Missing `type`
**Symptom:** ActionTypes.json, ConditionTypes.json show as "Generic"
**Fix:** These are correctly identified as "Catalog" type
**Action:** Add `"type": "Catalog"` to catalog JSON files

## Migration from V1 to V2

For migrating v1 blueprints to v2 format, use `BlueprintMigrator` class:

```cpp
BlueprintMigrator migrator;
nlohmann::json v1Blueprint = LoadBlueprint("old_format.json");

// Check if migration needed
if (!migrator.IsV2(v1Blueprint)) {
    nlohmann::json v2Blueprint = migrator.MigrateToV2(v1Blueprint);
    SaveBlueprint("old_format.json", v2Blueprint);
}
```

The migrator handles:
- Structure wrapping (data section)
- Node position calculation
- Metadata and editor state addition
- Parameters structure migration

## Best Practices

1. **Always Normalize on Load:** Call `Normalize()` when loading blueprints to ensure consistency
2. **Validate Before Save:** Call `ValidateJSON()` before saving to catch errors early
3. **Log Warnings:** Use validator results to log issues for debugging
4. **Non-Destructive by Default:** Normalization adds fields but doesn't modify existing data
5. **Explicit Types:** Always include `type` field in new blueprints
6. **Version Tracking:** Set `schema_version` to 2 for all new blueprints

## Future Enhancements

- Auto-migration on asset scan with user confirmation
- Batch normalization tool for all blueprints
- Schema validation UI in editor
- JSON schema definitions for external validation
- Automatic backup before normalization
