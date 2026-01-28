# Testing Guide: Render Layer System

## Prerequisites

1. Open `Olympe Engine.sln` in Visual Studio
2. Build the solution (Ctrl+Shift+B)
3. Ensure the build completes without errors

## Test 1: Verify Compilation

**Objective**: Ensure the new code compiles without errors

**Steps**:
1. Clean the solution (Build → Clean Solution)
2. Rebuild the solution (Build → Rebuild Solution)
3. Check the Output window for any compilation errors

**Expected Result**:
✅ Build succeeds with 0 errors  
✅ All new enum types compile correctly  
✅ No linker errors

---

## Test 2: Verify Automatic Layer Assignment

**Objective**: Confirm that players are automatically assigned to the Characters layer

**Steps**:
1. Run the application (F5)
2. In the code, ensure player creation uses: `VideoGame::AddPlayerEntity("PlayerEntity")`
3. Check the console output for layer assignment messages

**Expected Output**:
```
World::SetEntityLayer: Entity [ID] assigned to layer 2 (z=2.0)
```

**Expected Result**:
✅ Player entity created successfully  
✅ Layer assignment message shows layer 2 (Characters)  
✅ Player's position.z is set to 2.0  

---

## Test 3: Verify Player Renders Above Ground

**Objective**: Ensure players appear visually above ground tiles

**Steps**:
1. Run the application
2. Load a level with both ground tiles and a player character
3. Observe the rendering order
4. Move the player across different ground tiles

**Expected Result**:
✅ Player is always rendered on top of ground tiles  
✅ Player never appears behind ground elements  
✅ Visual hierarchy is consistent regardless of position  

**Visual Check**:
- Ground tiles (z=0) should appear at depth ≈ 0-500 (depending on Y)
- Player (z=2) should appear at depth ≈ 20000+ (always above ground)

---

## Test 4: Test Layer Override

**Objective**: Verify explicit layer override functionality

**Steps**:
1. In code, create an entity with layer override:
   ```cpp
   EntityID testEntity = PrefabFactory::Get().CreateEntityFromPrefabName("PlayerEntity", RenderLayer::Flying);
   ```
2. Check console output for layer assignment

**Expected Output**:
```
PrefabFactory::CreateEntityFromPrefabName: Entity [ID] layer set to 3
World::SetEntityLayer: Entity [ID] assigned to layer 3 (z=3.0)
```

**Expected Result**:
✅ Entity created with overridden layer (Flying, z=3)  
✅ No automatic layer assignment message (efficient, single assignment)  
✅ Entity renders at Flying layer depth (~30000+)  

---

## Test 5: Runtime Layer Change

**Objective**: Verify dynamic layer changes work correctly

**Steps**:
1. Create a player entity
2. In code, change its layer at runtime:
   ```cpp
   World::Get().SetEntityLayer(playerID, RenderLayer::Effects);
   ```
3. Observe rendering order change
4. Change back to Characters layer:
   ```cpp
   World::Get().SetEntityLayer(playerID, RenderLayer::Characters);
   ```

**Expected Result**:
✅ Player moves to Effects layer (z=4, appears above other characters)  
✅ Player returns to Characters layer (z=2, normal depth)  
✅ Rendering order updates immediately  
✅ Console logs show each layer change  

---

## Test 6: Query Entity Layer

**Objective**: Test GetEntityLayer() functionality

**Steps**:
1. Create various entity types
2. Query their layers:
   ```cpp
   RenderLayer layer = World::Get().GetEntityLayer(entityID);
   ```
3. Verify the returned values match expected layers

**Expected Results**:
✅ Player entity returns RenderLayer::Characters (2)  
✅ Entities without Position_data return RenderLayer::Ground (0)  
✅ Query is fast and doesn't crash  

---

## Test 7: Multiple Entity Types

**Objective**: Verify different entity types get correct layers

**Steps**:
1. Create prefabs for different entity types (if available):
   - Player (should be Characters, z=2)
   - NPC (should be Characters, z=2)
   - Item (should be Objects, z=1)
   - Effect (should be Effects, z=4)
2. Spawn multiple entities
3. Check console logs for layer assignments

**Expected Results**:
✅ Each entity type receives its correct default layer  
✅ Visual hierarchy is maintained (effects > characters > objects > ground)  
✅ Entities of same type render in Y-order (isometric depth)  

---

## Test 8: Backward Compatibility

**Objective**: Ensure existing entities without EntityType still work

**Steps**:
1. Create an entity with old-style JSON (without proper EntityType)
2. Verify it doesn't crash
3. Check its assigned layer

**Expected Result**:
✅ Entity creates successfully  
✅ Defaults to Ground layer (z=0)  
✅ No crashes or errors  
✅ System is backward compatible  

---

## Test 9: Depth Calculation Verification

**Objective**: Verify the depth formula works correctly

**Steps**:
1. Create entities at different layers
2. Check their final depth values in the rendering system
3. Verify the formula: `depth = z * 10000 + y`

**Test Cases**:
| Entity      | Z | Y    | Expected Depth | Result |
|-------------|---|------|----------------|--------|
| Ground Tile | 0 | 100  | 100            |        |
| Item        | 1 | 100  | 10100          |        |
| Player      | 2 | 100  | 20100          |        |
| Effect      | 4 | 100  | 40100          |        |

**Expected Result**:
✅ All depth calculations match the formula  
✅ Entities render in correct order  
✅ Y-position provides intra-layer sorting  

---

## Test 10: Performance Check

**Objective**: Ensure layer system doesn't impact performance

**Steps**:
1. Spawn 100+ entities of various types
2. Monitor frame rate
3. Check for any performance degradation

**Expected Result**:
✅ No noticeable performance impact  
✅ Layer assignment happens once at creation (efficient)  
✅ Frame rate remains stable  

---

## Test 11: Console Log Verification

**Objective**: Verify proper logging of layer operations

**Steps**:
1. Enable console output
2. Create various entities
3. Review log messages

**Expected Log Messages**:
```
PrefabFactory::CreateEntityFromBlueprint: Created entity [ID] from blueprint 'PlayerEntity' (10 components, 0 failed)
World::SetEntityLayer: Entity [ID] assigned to layer 2 (z=2.0)
```

**Expected Result**:
✅ Layer assignments are logged  
✅ Messages are clear and informative  
✅ No error messages for valid operations  

---

## Test 12: Edge Cases

**Objective**: Test boundary conditions and error handling

### Test 12.1: Entity without Position_data
```cpp
EntityID entity = World::Get().CreateEntity();
World::Get().AddComponent<Identity_data>(entity, "Test", "Test", "Player");
// Try to set layer without Position_data
World::Get().SetEntityLayer(entity, RenderLayer::Characters);
```

**Expected**: Warning logged, no crash

### Test 12.2: Invalid EntityType string
JSON with: `"entityType": "InvalidType"`

**Expected**: Defaults to EntityType::None, assigns Ground layer

### Test 12.3: Missing entityType in JSON
JSON without entityType field

**Expected**: Defaults to EntityType::None, assigns Ground layer

---

## Regression Testing

**Objective**: Ensure existing functionality still works

**Tests**:
1. ✅ Existing prefabs still load
2. ✅ Entities without layer info still render
3. ✅ Depth sorting still works for same-layer entities
4. ✅ Camera rendering still functions correctly
5. ✅ Player movement and controls still work

---

## Visual Validation Checklist

When running the application, verify visually:

- [ ] Players appear above ground tiles at all times
- [ ] Items on ground appear above tiles but below players
- [ ] Effects (if present) appear above characters
- [ ] UI elements appear on top of everything
- [ ] Multiple players on same layer sort correctly by Y position
- [ ] No z-fighting or flickering between layers
- [ ] Smooth transitions when changing entity layers

---

## Known Issues to Watch For

1. **Player behind ground**: Indicates layer not assigned correctly
2. **Depth flickering**: May indicate z-fighting (same depth values)
3. **Layer changes not visible**: Check Position_data component exists
4. **Crash on entity creation**: Check all components compile correctly

---

## Success Criteria Summary

The implementation is successful if:

✅ Code compiles without errors  
✅ Players spawn on Characters layer (z=2)  
✅ Players render above ground tiles consistently  
✅ Layer override works correctly  
✅ Runtime layer changes work  
✅ Query functions return correct values  
✅ Different entity types get appropriate layers  
✅ Backward compatibility maintained  
✅ Depth formula verified  
✅ No performance degradation  
✅ Proper error handling  
✅ Clear console logging  

---

## Troubleshooting

### Issue: Player renders behind ground

**Diagnosis**:
- Check PlayerEntity.json has `"entityType": "Player"`
- Verify console shows layer 2 assignment
- Check Position_data exists on entity

**Fix**:
- Update prefab JSON with correct entityType
- Ensure PrefabFactory.cpp includes layer assignment code

### Issue: Layer changes not visible

**Diagnosis**:
- Check entity has Position_data component
- Verify SetEntityLayer() is being called
- Check console for error messages

**Fix**:
- Add Position_data component if missing
- Verify entity ID is valid

### Issue: Compilation errors

**Diagnosis**:
- Check for missing includes
- Verify enum definitions match usage
- Look for typos in enum values

**Fix**:
- Add missing `#include <unordered_map>` if needed
- Verify enum names are correct

---

## Quick Smoke Test (5 minutes)

For rapid validation:

1. ✅ Build succeeds
2. ✅ Application runs
3. ✅ Player spawns
4. ✅ Player renders above ground
5. ✅ Console shows layer assignment messages

If all pass → System working correctly!

---

## Reporting Issues

If you encounter issues:

1. Note the exact error message
2. Check the console logs
3. Verify prefab JSON structure
4. Check entity has required components
5. Review Documentation/RENDER_LAYERS.md
