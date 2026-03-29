# Animation Editor Standalone Window - Test Plan

## Purpose
This document provides a comprehensive test plan for verifying the Animation Editor standalone window implementation.

## Prerequisites
- Olympe Engine build environment set up
- Test project with animation banks available in `GameData/Animations/Banks/`
- Optional: Second monitor for multi-monitor testing

## Test Environment Setup

### Build Steps
1. Open project in Visual Studio (Windows) or use CMake (Linux)
2. Build in Debug mode for detailed error messages
3. Verify no compilation errors or warnings related to AnimationEditorWindow

### Runtime Setup
1. Launch Olympe Engine executable
2. Load a test project with animation data
3. Keep console/log output visible for monitoring

## Test Cases

### TC001: Basic Window Opening
**Objective:** Verify F9 opens the Animation Editor in a separate window

**Steps:**
1. Launch the engine
2. Press F9

**Expected Results:**
- A new window titled "Animation Editor - Olympe Engine" appears
- Window size is 1280x720 pixels
- Window has dark gray background (RGB 45, 45, 48)
- Window is resizable
- Log shows: `[AnimationEditor] Standalone window created`
- Log shows: `AnimationEditorWindow: Opened`

**Pass Criteria:** ✅ New window appears with correct title and size

---

### TC002: Window Closing via F9
**Objective:** Verify F9 closes the Animation Editor window

**Steps:**
1. Open Animation Editor with F9
2. Press F9 again

**Expected Results:**
- Window disappears (but may remain in memory)
- Log shows: `AnimationEditorWindow: Closed`
- Main window remains functional

**Pass Criteria:** ✅ Window closes without crashes

---

### TC003: Window Closing via Close Button
**Objective:** Verify X button closes the Animation Editor window

**Steps:**
1. Open Animation Editor with F9
2. Click the X (close) button on the window title bar

**Expected Results:**
- Window closes
- Log shows: `AnimationEditorWindow: Closed`
- No crashes or errors

**Pass Criteria:** ✅ Window closes gracefully

---

### TC004: Window Independence
**Objective:** Verify the window can be moved independently

**Steps:**
1. Open Animation Editor with F9
2. Drag the window to different screen positions
3. If available, drag to second monitor
4. Resize the window by dragging edges

**Expected Results:**
- Window moves smoothly without affecting main window
- Window can be positioned anywhere on screen
- Window can be moved to second monitor
- Window resizes properly
- Main game window is unaffected

**Pass Criteria:** ✅ Window moves and resizes independently

---

### TC005: Multiple Editor Windows
**Objective:** Verify Animation Editor and BT Debugger can coexist

**Steps:**
1. Press F9 to open Animation Editor
2. Press F10 to open BT Debugger
3. Interact with both windows
4. Close both windows

**Expected Results:**
- Both windows open successfully
- Both windows are functional
- No interference between windows
- Input works correctly in each window
- Both windows close cleanly

**Pass Criteria:** ✅ Both windows work independently

---

### TC006: Blueprint Editor Compatibility
**Objective:** Verify F2 (Blueprint Editor) still works

**Steps:**
1. Press F2 to open Blueprint Editor (in main window)
2. Press F9 to open Animation Editor (separate window)
3. Interact with both editors
4. Close Animation Editor
5. Verify Blueprint Editor still works

**Expected Results:**
- Blueprint Editor renders in main window
- Animation Editor in separate window
- No visual conflicts or overlaps
- Both editors fully functional
- No crashes when switching between them

**Pass Criteria:** ✅ Both editors work without conflicts

---

### TC007: Input Handling - Mouse
**Objective:** Verify mouse input works correctly in Animation Editor

**Steps:**
1. Open Animation Editor with F9
2. Test mouse interactions:
   - Click buttons in menu bar
   - Click items in bank list panel
   - Click and drag in spritesheet panel
   - Use scroll wheel for zoom
   - Click in preview panel

**Expected Results:**
- All mouse clicks register correctly
- Dragging works (pan in spritesheet view)
- Scroll wheel works (zoom)
- Mouse input in Animation Editor doesn't affect main window
- Mouse input in main window doesn't affect Animation Editor

**Pass Criteria:** ✅ Mouse input works correctly

---

### TC008: Input Handling - Keyboard
**Objective:** Verify keyboard input works correctly

**Steps:**
1. Open Animation Editor with F9
2. Test keyboard interactions:
   - Type in text fields (e.g., bank ID, sequence name)
   - Press Tab to navigate between fields
   - Press Enter in dialogs
   - Press Escape to cancel dialogs

**Expected Results:**
- Text input works in all fields
- Tab navigation works
- Keyboard shortcuts work
- No key presses leak to main window

**Pass Criteria:** ✅ Keyboard input works correctly

---

### TC009: Window Reopening
**Objective:** Verify window state persists across open/close cycles

**Steps:**
1. Open Animation Editor with F9
2. Move window to different position
3. Resize window
4. Close with F9
5. Reopen with F9

**Expected Results:**
- Window reopens quickly (already created in memory)
- Window position preserved (optional, depends on implementation)
- Window size may reset or be preserved
- No crashes or memory issues

**Pass Criteria:** ✅ Window reopens successfully

---

### TC010: Application Shutdown
**Objective:** Verify clean shutdown with window open

**Steps:**
1. Open Animation Editor with F9
2. Load/edit some animation data
3. Close the main application (not just the window)

**Expected Results:**
- Application closes cleanly
- Log shows: `[AnimationEditor] Separate window destroyed`
- Log shows: `AnimationEditorWindow: Destroyed`
- No crashes or memory leaks reported
- No SDL/ImGui errors in log

**Pass Criteria:** ✅ Clean shutdown without errors

---

### TC011: Rapid Open/Close
**Objective:** Verify stability with rapid toggling

**Steps:**
1. Press F9 rapidly 10 times (open/close/open/close...)
2. Verify no crashes or hangs
3. Check log for errors

**Expected Results:**
- Window opens and closes rapidly without issues
- No memory leaks
- No crashes
- Log shows alternating Opened/Closed messages

**Pass Criteria:** ✅ No crashes or errors

---

### TC012: ImGui Context Isolation
**Objective:** Verify ImGui contexts are properly isolated

**Steps:**
1. Open Blueprint Editor (F2)
2. Open Animation Editor (F9)
3. Open BT Debugger (F10)
4. Interact with UI elements in each:
   - Drag windows within each editor
   - Open/close panels
   - Use menus
5. Close all editors

**Expected Results:**
- No UI corruption or overlaps
- Each editor's UI elements work independently
- No conflicts between ImGui contexts
- All editors render correctly
- Clean shutdown

**Pass Criteria:** ✅ No context conflicts

---

### TC013: Animation Editor Features
**Objective:** Verify all Animation Editor features work in separate window

**Test the following features in the separate window:**

#### TC013a: Bank List Panel
- [ ] Bank list displays correctly
- [ ] Clicking banks selects them
- [ ] Scrolling works if many banks

#### TC013b: Spritesheet Panel
- [ ] Spritesheet displays correctly
- [ ] Grid overlay toggles (if implemented)
- [ ] Pan and zoom work
- [ ] Frame selection works

#### TC013c: Sequence Panel
- [ ] Sequence list displays
- [ ] Adding new sequences works
- [ ] Editing sequence properties works
- [ ] Deleting sequences works

#### TC013d: Preview Panel
- [ ] Preview displays animation
- [ ] Play/Pause/Stop controls work
- [ ] Speed slider works
- [ ] Frame counter updates

#### TC013e: Menu Bar
- [ ] File menu opens
- [ ] Edit menu opens
- [ ] View menu opens
- [ ] All menu items functional

#### TC013f: Dialogs
- [ ] New Bank dialog works
- [ ] Add Spritesheet dialog works
- [ ] Add Sequence dialog works
- [ ] File browser dialogs work (if implemented)

**Pass Criteria:** ✅ All features work as before

---

### TC014: Memory Leak Testing (Optional)
**Objective:** Verify no memory leaks

**Tools Required:** Valgrind (Linux) or Visual Studio Memory Profiler (Windows)

**Steps:**
1. Run engine under memory profiler
2. Open/close Animation Editor 20 times
3. Interact with editor features
4. Close application
5. Check memory report

**Expected Results:**
- No memory leaks detected
- All allocated memory properly freed
- SDL resources cleaned up
- ImGui resources cleaned up

**Pass Criteria:** ✅ No memory leaks

---

### TC015: Error Handling
**Objective:** Verify graceful error handling

**Steps:**
1. Simulate SDL window creation failure (if possible)
2. Simulate renderer creation failure (if possible)
3. Check error logs

**Expected Results:**
- Errors logged clearly: `[AnimationEditor] Failed to create window: <error>`
- Application doesn't crash
- Graceful fallback or error message
- Subsequent attempts handle failure state

**Pass Criteria:** ✅ Errors handled gracefully

---

## Performance Tests

### PT001: Rendering Performance
**Objective:** Verify no performance degradation

**Steps:**
1. Monitor FPS with Animation Editor closed
2. Open Animation Editor with F9
3. Monitor FPS with Animation Editor open
4. Interact with editor
5. Compare FPS

**Expected Results:**
- Minimal FPS impact (<5% drop acceptable)
- No stuttering or lag
- Smooth rendering in both windows

**Pass Criteria:** ✅ Acceptable performance

---

### PT002: Memory Usage
**Objective:** Verify reasonable memory usage

**Steps:**
1. Note memory usage with Animation Editor closed
2. Open Animation Editor
3. Note memory usage increase
4. Close Animation Editor
5. Verify memory is released

**Expected Results:**
- Reasonable memory increase (< 50MB for window/context)
- Memory released when window closed or app quits
- No memory accumulation over multiple open/close cycles

**Pass Criteria:** ✅ Reasonable memory usage

---

## Regression Tests

### RT001: Existing Features
**Objective:** Verify existing features still work

**Test:**
- [ ] Main game loop
- [ ] Blueprint Editor (F2)
- [ ] BT Debugger (F10)
- [ ] Tiled Level Loader (F3)
- [ ] Game rendering
- [ ] Entity systems
- [ ] Physics
- [ ] Audio

**Pass Criteria:** ✅ No regressions

---

## Test Results Summary Template

```
Date: __________
Tester: __________
Build: __________
Platform: Windows / Linux

Test Case Results:
[ ] TC001 - PASS / FAIL / SKIP
[ ] TC002 - PASS / FAIL / SKIP
[ ] TC003 - PASS / FAIL / SKIP
[ ] TC004 - PASS / FAIL / SKIP
[ ] TC005 - PASS / FAIL / SKIP
[ ] TC006 - PASS / FAIL / SKIP
[ ] TC007 - PASS / FAIL / SKIP
[ ] TC008 - PASS / FAIL / SKIP
[ ] TC009 - PASS / FAIL / SKIP
[ ] TC010 - PASS / FAIL / SKIP
[ ] TC011 - PASS / FAIL / SKIP
[ ] TC012 - PASS / FAIL / SKIP
[ ] TC013 - PASS / FAIL / SKIP
[ ] TC014 - PASS / FAIL / SKIP
[ ] TC015 - PASS / FAIL / SKIP
[ ] PT001 - PASS / FAIL / SKIP
[ ] PT002 - PASS / FAIL / SKIP
[ ] RT001 - PASS / FAIL / SKIP

Issues Found:
1. ___________________________
2. ___________________________
3. ___________________________

Overall Result: PASS / FAIL

Notes:
_________________________________
_________________________________
```

## Known Limitations
- Window position/size may not persist across sessions (not implemented)
- Unsaved changes dialog is placeholder (logs only, no UI dialog)
- Requires Windows environment with SDL3 installed for building

## Success Criteria
- All TC001-TC013 tests pass
- No crashes or errors
- Performance is acceptable (PT001, PT002)
- No regressions (RT001)
- Clean memory usage (no leaks)

## Failure Criteria
- Any crash during normal operation
- ImGui context conflicts or corruption
- Window fails to open/close
- Major performance degradation
- Memory leaks detected
- Existing features broken
