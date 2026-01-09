# Blueprint JSON Schema Documentation

## Overview

This document defines the JSON schema requirements for all blueprint types in the Olympe Engine. All blueprints must include certain required fields for proper detection and loading in the Blueprint Editor.

---

## Common Fields (Required for ALL Blueprints)

Every blueprint file must include these fields at the root level:

```json
{
  "schema_version": 1 or 2,
  "type": "XXX",              // ⚠️ REQUIRED - Primary type identifier
  "name": "string",           // Required - Human-readable name
  "description": "string"     // Optional but recommended
}
```

### Field Descriptions

- **`schema_version`**: Integer (1 or 2)
  - Version 1: Original schema with direct field access
  - Version 2: Modern schema with data wrapper and enhanced metadata

- **`type`**: String (REQUIRED)
  - Primary field used by C++ parser to identify blueprint type
  - Must be one of the supported types (see below)
  - Takes precedence over deprecated `blueprintType` field

- **`name`**: String (REQUIRED)
  - Human-readable name displayed in editor
  - Should be descriptive and unique

- **`description`**: String (Optional)
  - Longer description of the blueprint's purpose
  - Displayed in Asset Browser and Inspector

---

## Supported Blueprint Types

### 1. EntityBlueprint (Schema v1)

**Purpose**: Define entity archetypes with components

**Required Fields**:
```json
{
  "schema_version": 1,
  "type": "EntityBlueprint",
  "name": "MyEntity",
  "components": []            // ⚠️ REQUIRED - Array of component definitions
}
```

**Example**:
```json
{
  "schema_version": 1,
  "type": "EntityBlueprint",
  "name": "SimpleCharacter",
  "description": "A basic character entity",
  "components": [
    {
      "type": "Position",
      "properties": {
        "position": { "x": 100.0, "y": 200.0, "z": 0.0 }
      }
    },
    {
      "type": "VisualSprite",
      "properties": {
        "spritePath": "Resources/entity_male.png"
      }
    }
  ]
}
```

---

### 2. EntityPrefab (Schema v2)

**Purpose**: Prefabricated entity templates with enhanced metadata

**Required Fields**:
```json
{
  "schema_version": 2,
  "type": "EntityPrefab",
  "name": "MyPrefab",
  "data": {
    "components": []          // ⚠️ REQUIRED - Array of component definitions
  }
}
```

**Optional Fields**:
```json
{
  "blueprintType": "EntityPrefab",  // For backward compatibility
  "metadata": {
    "author": "string",
    "created": "ISO8601 timestamp",
    "lastModified": "ISO8601 timestamp",
    "tags": ["tag1", "tag2"]
  }
}
```

**Example**:
```json
{
  "schema_version": 2,
  "type": "EntityPrefab",
  "blueprintType": "EntityPrefab",
  "name": "PlayerEntity",
  "description": "Playable character entity",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-01-08T10:00:00Z",
    "tags": ["Player", "Character"]
  },
  "data": {
    "components": [
      {
        "type": "Position_data",
        "properties": { "position": { "x": 0, "y": 0, "z": 0 } }
      },
      {
        "type": "Health_data",
        "properties": { "maxHealth": 100, "currentHealth": 100 }
      }
    ]
  }
}
```

---

### 3. BehaviorTree (Schema v2)

**Purpose**: AI behavior tree definitions

**Required Fields**:
```json
{
  "schema_version": 2,
  "type": "BehaviorTree",
  "name": "MyBehaviorTree",
  "data": {
    "rootNodeId": 1,          // ⚠️ REQUIRED - ID of the root node
    "nodes": []               // ⚠️ REQUIRED - Array of node definitions
  }
}
```

**Optional Fields**:
```json
{
  "blueprintType": "BehaviorTree",  // For backward compatibility
  "metadata": { /* ... */ },
  "editorState": {
    "zoom": 1.0,
    "scrollOffset": { "x": 0, "y": 0 }
  }
}
```

**Example**:
```json
{
  "schema_version": 2,
  "type": "BehaviorTree",
  "blueprintType": "BehaviorTree",
  "name": "GuardPatrolAI",
  "description": "Guard patrol behavior",
  "metadata": {
    "author": "Atlasbruce",
    "tags": ["AI", "BehaviorTree", "Guard"]
  },
  "editorState": {
    "zoom": 1.0,
    "scrollOffset": { "x": 0, "y": 0 }
  },
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "name": "Patrol Sequence",
        "type": "Sequence",
        "position": { "x": 200.0, "y": 300.0 },
        "children": [2, 3],
        "parameters": {}
      },
      {
        "id": 2,
        "name": "Move to Waypoint",
        "type": "Action",
        "position": { "x": 550.0, "y": 300.0 },
        "actionType": "MoveToWaypoint",
        "parameters": { "waypointIndex": 0 }
      }
    ]
  }
}
```

---

### 4. HFSM (Hierarchical Finite State Machine)

**Purpose**: State machine definitions for animation and logic

**Required Fields**:
```json
{
  "schema_version": 2,
  "type": "HFSM",
  "name": "MyStateMachine",
  "states": [],               // ⚠️ REQUIRED - Array of state definitions
  "initialState": "string"    // ⚠️ REQUIRED - Name of the initial state
}
```

**Optional Fields**:
```json
{
  "transitions": []           // Optional - Array of transition definitions
}
```

**Example**:
```json
{
  "schema_version": 2,
  "type": "HFSM",
  "name": "CharacterMovementFSM",
  "description": "Character movement state machine",
  "states": [
    {
      "name": "Idle",
      "type": "State",
      "onEnter": "PlayIdleAnimation",
      "onExit": "StopIdleAnimation"
    },
    {
      "name": "Walking",
      "type": "State",
      "onEnter": "PlayWalkAnimation"
    }
  ],
  "initialState": "Idle",
  "transitions": [
    {
      "from": "Idle",
      "to": "Walking",
      "condition": "VelocityGreaterThan(0.1)"
    },
    {
      "from": "Walking",
      "to": "Idle",
      "condition": "VelocityLessThan(0.1)"
    }
  ]
}
```

---

### 5. UIMenu

**Purpose**: UI menu layout and navigation definitions

**Required Fields**:
```json
{
  "schema_version": 2,
  "type": "UIMenu",
  "name": "MyMenu",
  "data": {
    "elements": []            // UI element definitions
  }
}
```

**Example**:
```json
{
  "schema_version": 2,
  "type": "UIMenu",
  "blueprintType": "UIMenu",
  "name": "MainMenu",
  "description": "Main menu UI",
  "data": {
    "menuName": "MainMenu",
    "elements": [
      {
        "id": "btn_newgame",
        "type": "Button",
        "position": { "x": 400, "y": 400 },
        "text": "New Game",
        "action": "StartNewGame"
      }
    ]
  }
}
```

---

### 6. AnimationGraph

**Purpose**: Animation state machine and transition definitions

**Required Fields**:
```json
{
  "schema_version": 2,
  "type": "AnimationGraph",
  "name": "MyAnimationGraph",
  "data": {
    "states": []              // Animation state definitions
  }
}
```

---

### 7. LevelDefinition

**Purpose**: Level layout and entity placement

**Required Fields**:
```json
{
  "schema_version": 2,
  "type": "LevelDefinition",
  "name": "MyLevel",
  "data": {
    "entities": []            // Entity instance definitions
  }
}
```

---

### 8. ScriptedEvent

**Purpose**: Scripted event sequences and triggers

**Required Fields**:
```json
{
  "schema_version": 2,
  "type": "ScriptedEvent",
  "name": "MyEvent",
  "data": {
    "triggers": [],           // Event triggers
    "actions": []             // Event actions
  }
}
```

---

## Backward Compatibility

### Deprecated `blueprintType` Field

The `blueprintType` field was used in early schema v2 implementations but is now deprecated in favor of the standardized `type` field.

**Current behavior:**
- C++ parser checks `type` field first (Priority 1)
- Falls back to `blueprintType` if `type` is missing (Priority 2)
- Logs a warning when using deprecated `blueprintType`
- Performs structural detection as final fallback (Priority 3)

**Migration Path:**
1. Add `"type"` field to all blueprints (matching `blueprintType` value)
2. Keep `"blueprintType"` temporarily for compatibility
3. Eventually remove `"blueprintType"` in future versions

**Example Migration:**
```json
// OLD (deprecated)
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "name": "MyAI"
}

// NEW (recommended)
{
  "schema_version": 2,
  "type": "BehaviorTree",
  "blueprintType": "BehaviorTree",  // Keep for compatibility
  "name": "MyAI"
}

// FUTURE (target)
{
  "schema_version": 2,
  "type": "BehaviorTree",
  "name": "MyAI"
}
```

---

## Detection Priority

The Blueprint Editor uses the following priority order to detect blueprint types:

1. **Explicit `type` field** (Highest priority)
   - Direct string match for type identification
   - Works for both schema v1 and v2

2. **Fallback to `blueprintType` field**
   - Used when `type` is missing
   - Logs deprecation warning
   - Schema v2 compatibility layer

3. **Schema v2 structural detection (`data` wrapper)**
   - Checks for `data.rootNodeId` + `data.nodes` → BehaviorTree
   - Checks for `data.components` → EntityPrefab

4. **Schema v1 structural detection (direct fields)**
   - Checks for `rootNodeId` + `nodes` → BehaviorTree
   - Checks for `states` or `initialState` → HFSM
   - Checks for `components` → EntityBlueprint

5. **Generic fallback**
   - When no type can be determined

---

## Best Practices

### 1. Always Include `type` Field
```json
{
  "type": "BehaviorTree",  // ✅ REQUIRED
  "name": "MyAI"
}
```

### 2. Use Descriptive Names
```json
{
  "name": "GuardPatrolAI",     // ✅ Good - descriptive
  "name": "AI_001"             // ❌ Bad - not descriptive
}
```

### 3. Add Descriptions
```json
{
  "description": "Guard AI that patrols between waypoints"  // ✅ Helpful
}
```

### 4. Use Metadata for Organization
```json
{
  "metadata": {
    "author": "YourName",
    "tags": ["AI", "Guard", "Combat"]
  }
}
```

### 5. Maintain Consistency
- Use consistent naming conventions across similar blueprints
- Group related blueprints in folders (AI/, EntityPrefab/, UI/)
- Keep component property names consistent

---

## Validation Rules

The Blueprint Editor validates blueprints according to these rules:

1. **Required Fields**: Must have `type` and `name`
2. **Type-Specific Requirements**: Each type has required data fields (see above)
3. **JSON Syntax**: Must be valid JSON
4. **Schema Version**: Must be 1 or 2
5. **Component Types**: Component types must be registered in the engine
6. **Node IDs**: In BehaviorTrees, node IDs must be unique
7. **State Names**: In HFSMs, state names must be unique

---

## Error Messages

Common validation errors and their meanings:

- **"Failed to load JSON file"**: Invalid JSON syntax
- **"Unknown asset type"**: Missing or invalid `type` field
- **"Missing required field: components"**: EntityBlueprint/Prefab without components
- **"Missing required field: rootNodeId"**: BehaviorTree without root node
- **"Invalid schema_version"**: Must be 1 or 2

---

## Schema Migration Tools

### BlueprintMigrator Class

The engine includes a `BlueprintMigrator` class that can automatically upgrade blueprints:

- Converts schema v1 → v2
- Adds missing `type` fields
- Wraps data in `data` object for v2
- Adds metadata structure

See `Source/BlueprintEditor/BlueprintMigrator.cpp` for implementation details.

---

## Examples Repository

Complete example blueprints are available in:
- `Blueprints/AI/` - BehaviorTree examples
- `Blueprints/EntityPrefab/` - EntityPrefab examples
- `Blueprints/UI/` - UIMenu examples
- `Blueprints/example_entity_simple.json` - Schema v1 example
- `Blueprints/example_entity_complete.json` - Complex entity example

---

## Related Documentation

- **ARCHITECTURE.md** - Overall Blueprint Editor architecture
- **BLUEPRINT_EDITOR_USER_GUIDE.md** - User guide for the editor
- **HOTFIX_BLUEPRINT_LOADING.md** - Loading system fixes
- **BlueprintMigrator.h** - Migration API documentation

---

## Version History

- **v2.1.0** (2026-01-09): Added standardized `type` field requirement
- **v2.0.0** (2026-01-08): Introduced schema v2 with `data` wrapper
- **v1.0.0** (2025-XX-XX): Initial schema v1 implementation

---

## Support

For questions or issues with blueprint schemas:
1. Check this documentation first
2. Review example blueprints in `Blueprints/` directory
3. Check validation error messages in console
4. Refer to ARCHITECTURE.md for technical details
