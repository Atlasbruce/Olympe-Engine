# Testing Guide: Unified Entity Instantiation System

## Overview
This guide describes how to test the new unified entity instantiation system that centralizes entity creation through `PrefabFactory::CreateEntityWithOverrides()`.

## Changes Summary
1. **PrefabFactory**: New `CreateEntityWithOverrides()` method for unified entity creation with level parameter overrides
2. **World (Pass3)**: Refactored to use unified system for static objects
3. **World (Pass4)**: Refactored to use unified system for dynamic objects with Player registration
4. **VideoGame**: New `RegisterLoadedPlayerEntity()` method for level-loaded players
5. **Helper Methods**: Added utilities for property extraction and placeholder creation

## Test Cases

### Test Case 1: Player Spawn from Level
**Objective**: Verify that Player entities loaded from a level are fully functional

**Test Level**: `Gamedata/Levels/isometric_quest.tmj`
- Contains player entities: `player_1`, `player_2`, `player_3` (type: "player")

**Steps**:
1. Launch the game
2. Load level: `isometric_quest.tmj`
3. Wait for level loading to complete

**Expected Results**:
- ✅ Player entities created with all components from `PlayerEntity.json` blueprint
- ✅ Players visible on screen at their defined positions
- ✅ Players registered in `VideoGame::m_playersEntity`
- ✅ Player controls functional (keyboard/gamepad input works)
- ✅ Camera follows player(s)
- ✅ Viewport configured correctly for number of players
- ✅ Console log shows: "✅ Player N registered from level (Entity: X)"

**Log Output to Verify**:
```
[DEBUG] Pass 4 Entry - Analyzing categorized objects:
  - player_1 (type: player)
  - player_2 (type: player)
  -> Creating: player_1 [PlayerEntity]
    -> Created with 10 components
✅ Player 1 registered from level (Entity: 42)
  -> Creating: player_2 [PlayerEntity]
    -> Created with 10 components
✅ Player 2 registered from level (Entity: 43)
```

### Test Case 2: Static Objects with Overrides (Pass3)
**Objective**: Verify static objects are created with property overrides applied

**Test Data**: Create a test level with static objects having property overrides
```json
{
  "type": "Item",
  "name": "health_potion_1",
  "x": 100,
  "y": 200,
  "properties": [
    { "name": "health_value", "value": 50 }
  ]
}
```

**Expected Results**:
- ✅ Entity created with `ItemEntity` blueprint components
- ✅ Position set to (100, 200)
- ✅ `health_value` property override applied
- ✅ Entity visible at correct position

### Test Case 3: Dynamic Objects with Custom Properties (Pass4)
**Objective**: Verify dynamic objects support custom property overrides

**Test Data**: NPC with health and speed overrides
```json
{
  "type": "NPC",
  "name": "merchant_1",
  "x": 300,
  "y": 400,
  "properties": [
    { "name": "health", "value": 150 },
    { "name": "speed", "value": 50.0 }
  ]
}
```

**Expected Results**:
- ✅ NPC entity created with all `NPCEntity` blueprint components
- ✅ Position set to (300, 400)
- ✅ Health override applied (150 instead of default)
- ✅ Speed override applied (50.0 instead of default)
- ✅ Console shows override confirmation

**Log Output**:
```
  -> Creating: merchant_1 [NPCEntity]
    -> Created with 8 components
    -> Position: (300.0, 400.0, 0.0) [zOrder preserved]
```

### Test Case 4: Missing Prefab Handling
**Objective**: Verify graceful handling of missing prefabs

**Test Data**: Object with non-existent prefab type
```json
{
  "type": "UnknownType",
  "name": "mystery_object",
  "x": 500,
  "y": 600
}
```

**Expected Results**:
- ✅ Red placeholder entity created at position (500, 600)
- ✅ Entity has Identity_data and Position_data components
- ✅ Entity has VisualEditor_data with red marker icon
- ✅ No crash or exception
- ✅ Console log shows warning: "/!\  PLACEHOLDER: Created red marker for missing prefab 'UnknownType'"

### Test Case 5: Backward Compatibility
**Objective**: Verify existing entity creation methods still work

**Test**:
1. Call `VideoGame::AddPlayerEntity()` manually (not from level)
2. Call `PrefabFactory::CreateEntity()` with legacy prefab name

**Expected Results**:
- ✅ Manual player creation still works
- ✅ Legacy CreateEntity() still works
- ✅ No regressions in existing code paths

## Manual Testing Steps

### Step 1: Build the Project
```bash
# Open Visual Studio
# Load: Olympe Engine.sln
# Build Solution (Ctrl+Shift+B)
# Verify: 0 errors, 0 warnings (or only expected warnings)
```

### Step 2: Run the Engine
```bash
# Run in Debug mode (F5)
# Engine should start without errors
```

### Step 3: Load Test Level
1. In the engine, load level: `isometric_quest.tmj`
2. Observe console output for Pass 3 and Pass 4 execution
3. Check for player registration messages

### Step 4: Verify Player Functionality
1. Check that player entity is visible
2. Test keyboard controls (WASD/Arrow keys)
3. Verify camera follows player
4. Test gamepad controls if available

### Step 5: Inspect Entities
1. Open Blueprint Editor (if available)
2. Navigate to Entity Inspector
3. Find player entities loaded from level
4. Verify all components are present
5. Check component values match blueprint + overrides

## Console Log Checklist

During level loading, verify these log entries appear:

```
✅ Expected Logs:
- "[DEBUG] Pass 3 Entry:"
- "[DEBUG] Pass 4 Entry - Analyzing categorized objects:"
- "-> Creating: player_1 [PlayerEntity]"
- "-> Created with 10 components"
- "✅ Player N registered from level (Entity: X)"
- "Player N bound to controller M" (if gamepad connected)
- "PASS 4 COMPLETE"

❌ Error Logs to Watch For:
- "x Failed to create entity"
- "X Entity missing required player components"
- "/!\ Failed to instantiate component"
- "Invalid blueprint"
```

## Regression Testing

### Areas to Verify:
1. **Existing Levels**: Load all existing levels and verify no regressions
2. **Manual Player Creation**: Call `AddPlayerEntity()` and verify it still works
3. **Static Objects**: Verify items, waypoints, triggers still work
4. **Dynamic Objects**: Verify NPCs, enemies still work
5. **Patrol Paths**: Verify NPC/enemy patrol paths still link correctly

## Performance Considerations

### Metrics to Monitor:
- Level load time should not increase significantly
- Memory usage should remain stable
- No memory leaks during entity creation/destruction

## Known Limitations

1. Player entities in levels must have type "Player" or "PlayerEntity"
2. Property overrides only support: int, float, bool, string
3. Complex types (arrays, objects) in overrides may require special handling

## Success Criteria

All test cases pass AND:
- ✅ No crashes during entity creation
- ✅ No memory leaks
- ✅ Players from levels are fully functional
- ✅ Property overrides applied correctly
- ✅ Missing prefabs handled gracefully
- ✅ Backward compatibility maintained
- ✅ Console logs are clear and informative
