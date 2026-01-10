# Blueprint Editor - Testing Guide

## Build Instructions

1. Open `Olympe Engine.sln` in Visual Studio 2019 or later
2. Ensure the build configuration includes `OLYMPE_BLUEPRINT_EDITOR_ENABLED` preprocessor definition (already configured)
3. Build the solution (F7 or Build → Build Solution)
4. Run the application (F5 or Debug → Start Debugging)

## Quick Test Procedure

### 1. Opening the Editor
- Press **F2** to toggle the Blueprint Editor
- You should see a menu bar appear at the top
- Multiple panels should appear (Asset Browser, Inspector, etc.)

### 2. Panel Interactivity Test (Part A)
**What to test:**
- Click and drag panel title bars → panels should move
- Drag panel edges → panels should resize
- Type in any text fields → keyboard input should work
- Click panels → mouse should not affect the game world behind
- Close editor (F2) and reopen → panel positions should be restored from imgui.ini

**Expected behavior:**
- All panels should be freely moveable and resizable
- No game controls should activate when clicking on panels
- Panel layout persists between sessions

**Success indicators:**
- ✅ Panels move smoothly when dragged
- ✅ Panels resize when edges are dragged
- ✅ Text input works in all fields
- ✅ imgui.ini file is updated when panels are moved
- ✅ Game world doesn't respond to clicks on panels

### 3. Runtime Entity Tracking Test (Part B)
**What to test:**
- Open the **Asset Browser** panel (View → Asset Browser if not visible)
- Scroll to the bottom to find "Runtime Entities" section
- Look for existing entities in the list
- If the game creates entities dynamically, verify they appear immediately

**Expected behavior:**
- Runtime Entities section shows all active entities
- Each entity displays name and component count
- Entity list updates in real-time as entities are created/destroyed
- Hovering over an entity shows tooltip with ID and component list

**Success indicators:**
- ✅ All runtime entities are listed
- ✅ Entity count is accurate
- ✅ Newly created entities appear immediately
- ✅ Destroyed entities disappear immediately
- ✅ Tooltips show correct information

### 4. Panel Synchronization Test (Part C)
**What to test:**
- Open Asset Browser, Inspector, Node Graph, and Entities panels
- Click on an entity in the Asset Browser's "Runtime Entities" section
- Observe all other panels

**Expected behavior:**
- **Inspector Panel**: Updates to show selected entity's components
- **Node Graph Panel**: Shows entity name at the top
- **Entities Panel**: Highlights the selected entity
- All panels should update simultaneously

**Try from different panels:**
1. Select entity from Asset Browser → All panels update
2. Select entity from Entities Panel → All panels update
3. Selection should be consistent across all panels

**Success indicators:**
- ✅ Selecting entity in Asset Browser updates Inspector
- ✅ Inspector shows correct components for selected entity
- ✅ Node Graph shows selected entity name
- ✅ Entities Panel highlights the selection
- ✅ All panels stay synchronized

### 5. Menu System Test (Part D)

#### File Menu
- **New Blueprint (Ctrl+N)**: Creates new blueprint, clears current
- **Open Blueprint (Ctrl+O)**: Opens example blueprint (file dialog pending)
- **Save (Ctrl+S)**: Saves current blueprint (enabled only when blueprint loaded)
- **Save As (Ctrl+Shift+S)**: Saves with different name
- **Reload Assets**: Refreshes asset tree
- **Exit Editor (F2)**: Closes the editor

**Test procedure:**
1. Click File → New Blueprint → Should create blank blueprint
2. Press Ctrl+S → Should save (or show it's disabled if no blueprint)
3. Click Reload Assets → Asset tree should refresh
4. Press F2 → Editor should close

#### Edit Menu
- **Undo (Ctrl+Z)**: Currently disabled (stub)
- **Redo (Ctrl+Y)**: Currently disabled (stub)
- **Add Component**: Opens component selection dialog
- **Remove Component**: Removes selected component
- **Preferences**: Opens preferences dialog

**Test procedure:**
1. Click Edit → Add Component → Component dialog should open
2. Click Edit → Preferences → Preferences dialog should appear

#### View Menu
- Test each panel toggle:
  - Click View → Asset Browser → Panel should show/hide
  - Repeat for all other panels
- Click "Reset Layout" → All panels should become visible

**Test procedure:**
1. Hide Asset Browser via View menu → Panel should disappear
2. Show Asset Browser again → Panel should reappear
3. Hide multiple panels, then click Reset Layout → All should reappear

#### Help Menu
- **Documentation**: Prints message to console
- **Keyboard Shortcuts**: Shows shortcuts dialog
- **About**: Shows about dialog

**Test procedure:**
1. Click Help → Keyboard Shortcuts → Dialog should display all shortcuts
2. Click Help → About → Should show version and feature info

**Success indicators:**
- ✅ All menu items are clickable
- ✅ Keyboard shortcuts work (Ctrl+N, Ctrl+S, F2, etc.)
- ✅ Disabled items are grayed out appropriately
- ✅ Panel toggles work correctly
- ✅ Dialogs open and display correct information

## Common Issues and Solutions

### Issue: Panels are not interactive
**Cause**: ImGui event processing not working
**Solution**: Verify ImGui_ImplSDL3_ProcessEvent() is called in SDL_AppEvent()
**Check**: Look for "ImGui_ImplSDL3_ProcessEvent(&event)" near line 118 in OlympeEngine.cpp

### Issue: Runtime entities not appearing
**Cause**: OLYMPE_BLUEPRINT_EDITOR_ENABLED not defined
**Solution**: Check preprocessor definitions in project settings
**Expected**: Should be defined for all configurations (Debug/Release, x86/x64)

### Issue: Selection not synchronizing
**Cause**: Panels not reading from BlueprintEditor backend
**Solution**: Verify all panels call BlueprintEditor::Get().GetSelectedEntity()
**Check**: Grep for "GetSelectedEntity" in panel files

### Issue: Panel positions not persisting
**Cause**: imgui.ini is read-only or not accessible
**Solution**: Ensure imgui.ini in working directory has write permissions
**Location**: Same directory as executable

## Performance Checks

- **Frame rate**: Should remain stable with editor open
- **Memory**: Entity list should not leak when entities are created/destroyed
- **Responsiveness**: Panel updates should be immediate (not laggy)

## Verification Checklist

Before marking PR as complete, verify:

- [ ] All panels are draggable and resizable
- [ ] Input events are properly captured by ImGui
- [ ] Runtime entities appear in Asset Browser
- [ ] Entity count updates dynamically
- [ ] Selecting entity updates all panels simultaneously
- [ ] All menu items are functional
- [ ] Panel visibility toggles work
- [ ] Keyboard shortcuts work (F2, Ctrl+N, Ctrl+S)
- [ ] Dialogs (About, Shortcuts, Preferences) open correctly
- [ ] No crashes or exceptions during testing
- [ ] Panel layout persists between sessions (imgui.ini)

## Screenshot Checklist

Capture these screenshots for documentation:

1. **Panel Interactivity**: Editor with multiple panels open, showing drag handles
2. **Runtime Entities**: Asset Browser showing Runtime Entities section with entities listed
3. **Panel Sync**: Side-by-side view of Asset Browser selection and Inspector showing same entity
4. **Menu System**: Each menu (File, Edit, View, Help) expanded showing all options
5. **Dialogs**: Keyboard Shortcuts dialog and About dialog
6. **Full Layout**: Complete editor view with all panels visible and arranged

## Regression Testing

Ensure existing functionality still works:

- [ ] Game still runs normally with editor closed
- [ ] F2 toggle works reliably
- [ ] Blueprint loading/saving still works
- [ ] Component editing still works
- [ ] No performance degradation in game loop
- [ ] No memory leaks

## Next Steps After Testing

1. Document any issues found
2. Take requested screenshots
3. Update README with usage instructions
4. Consider user feedback for improvements
5. Plan implementation of undo/redo system
6. Plan native file dialog integration

## Phase: Tab Persistence and Selection Testing

### Test Case: Tab Order Persistence
**Objective:** Verify tabs maintain consistent order across operations

**Steps:**
1. Open 3 behavior trees: "GuardPatrol", "NPCIdle", "Investigation"
2. Note the tab order (left to right)
3. Close the editor (F2) and reopen
4. Verify tabs appear in same order

**Expected:**
- Tab order is deterministic and consistent
- Tabs appear in insertion order

### Test Case: Active Tab Persistence
**Objective:** Verify selected tab remains active

**Steps:**
1. Open 3 graphs
2. Click on the middle tab
3. Wait several frames (render cycles)
4. Verify middle tab stays selected

**Expected:**
- Selected tab does not change without user action
- No automatic re-selection to first tab

### Test Case: Smart Tab Selection on Close
**Objective:** Verify intelligent neighbor selection when closing tab

**Steps:**
1. Open 5 graphs: A, B, C, D, E
2. Select tab C (middle)
3. Close tab C
4. Verify tab D is now selected (next neighbor)
5. Select tab A (first)
6. Close tab A
7. Verify tab B is now selected

**Expected:**
- Closing a tab selects next tab if available
- If no next tab, selects previous tab
- Selection is predictable and user-friendly

## Phase: DnD Safety Testing

### Test Case: Valid Node Creation
**Objective:** Verify normal DnD operations work

**Steps:**
1. Open Node Graph Editor
2. Drag "MoveTo" action from palette
3. Drop on canvas
4. Verify node created at mouse position

**Expected:**
- Node appears at drop location
- No errors in console
- Node has correct type

### Test Case: Invalid Payload Handling
**Objective:** Verify graceful handling of bad payloads

**Steps:**
1. Attempt to drag an invalid or corrupted payload (testing scenario)
2. Check console for error messages
3. Verify no crash occurs

**Expected:**
- Error logged to console
- Tooltip shows "Invalid node type"
- No crash or undefined behavior

### Test Case: Unknown Type Validation
**Objective:** Verify catalog validation works

**Steps:**
1. Modify a blueprint to reference non-existent ActionType
2. Try to create node via DnD
3. Check error handling

**Expected:**
- Error message: "Invalid ActionType: <name>"
- Node creation blocked
- Tooltip shown to user

## Phase: CRUD Capability Gating

### Test Case: Runtime Mode (Read-Only)
**Objective:** Verify CRUD operations hidden in Runtime mode

**Steps:**
1. Initialize editor in Runtime mode:
   ```cpp
   BlueprintEditor::Get().InitializeRuntimeEditor();
   ```
2. Open a behavior tree
3. Right-click on a node
4. Verify context menu options

**Expected:**
- "Edit" available (read-only)
- "Duplicate" NOT shown
- "Delete" NOT shown
- Delete key does nothing
- Right-click on canvas shows no creation menu

### Test Case: Standalone Mode (Full CRUD)
**Objective:** Verify all CRUD operations available

**Steps:**
1. Initialize editor in Standalone mode:
   ```cpp
   BlueprintEditor::Get().InitializeStandaloneEditor();
   ```
2. Open a behavior tree
3. Right-click on a node
4. Verify context menu options

**Expected:**
- "Edit", "Duplicate", "Delete" all shown
- Delete key removes node
- Ctrl+D duplicates node
- Right-click on canvas shows node creation menu
- Link creation works

### Test Case: Mode Switching
**Objective:** Verify mode changes take effect

**Steps:**
1. Start in Standalone mode
2. Verify CRUD available
3. Switch to Runtime mode
4. Verify CRUD hidden

**Expected:**
- UI updates immediately when mode changes
- No restart required
- Capabilities correctly gated

## Phase: JSON Validation Testing

### Test Case: Type Detection
**Objective:** Verify heuristic type detection

**Steps:**
1. Create JSON file without `type` field:
   ```json
   {
     "rootNodeId": 1,
     "nodes": [...]
   }
   ```
2. Load in editor
3. Check detected type

**Expected:**
- Type detected as "BehaviorTree" via heuristics
- Warning logged: "Type field missing, detected as BehaviorTree"

### Test Case: Normalization
**Objective:** Verify missing fields are added

**Steps:**
1. Create minimal JSON:
   ```json
   {
     "rootNodeId": 1,
     "nodes": []
   }
   ```
2. Load and normalize
3. Check added fields

**Expected:**
- `schema_version: 2` added
- `type: "BehaviorTree"` added
- `metadata: {...}` added with defaults
- `editorState: {...}` added with defaults

### Test Case: Validation Errors
**Objective:** Verify validation catches issues

**Steps:**
1. Create invalid BehaviorTree (missing nodes array)
2. Run validation
3. Check error message

**Expected:**
- Validation returns false
- Error: "BehaviorTree missing 'nodes' array"
- Informative error message for user

## Phase: Entity-Independent Rendering

### Test Case: No Entity Selected
**Objective:** Verify graph renders without entity

**Steps:**
1. Open Node Graph Editor
2. Ensure no entity selected (selectedEntity == 0)
3. Open a behavior tree
4. Verify graph renders

**Expected:**
- Graph renders normally
- Top banner shows: "Editing BehaviorTree Asset (no entity context)"
- All nodes and links visible
- No error or blocked rendering

### Test Case: With Entity Selected
**Objective:** Verify entity info is informational only

**Steps:**
1. Select an entity from Runtime Entities
2. Open a behavior tree
3. Verify graph renders with entity info

**Expected:**
- Top banner shows: "Editing for Entity: <name> (ID: <id>)"
- Graph still renders fully
- Entity selection is purely informational
- Can still edit graph (in Standalone mode)
