# Behavior Tree Debugger - Testing Guide

## Overview
This guide covers testing the improved BT Debugger with separate SDL3 window and enhanced layout algorithm.

## Build Instructions

### Windows (Visual Studio)
```batch
# Open project
Olympe Engine.sln

# Build configuration: Debug or Release
# Platform: x64

# Build solution (Ctrl+Shift+B)
```

### Linux/macOS (CMake)
```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

---

## Test Plan

### 1. Basic Window Operations

#### Test 1.1: Open Debugger Window
**Steps:**
1. Launch Olympe Engine
2. Load a level with AI entities (behavior trees)
3. Press **F10**

**Expected:**
- ✅ Separate window appears with title "Behavior Tree Runtime Debugger - Independent Window"
- ✅ Window size: 1800x1200 pixels
- ✅ Window is independent (not embedded in main engine window)
- ✅ Console output: `[BTDebugger] ✅ Separate window created successfully!`
- ✅ Console output: `[BTDebugger] Window can be moved to second monitor`

**Failure Indicators:**
- ❌ Window doesn't appear
- ❌ Crash with SDL error
- ❌ Window appears inside main engine window

---

#### Test 1.2: Close Debugger Window (F10)
**Steps:**
1. With debugger open, press **F10** again

**Expected:**
- ✅ Debugger window closes
- ✅ Console output: `[BTDebugger] F10: Debugger window closed`
- ✅ Console output: `[BTDebugger] Destroying separate window...`
- ✅ Main engine window remains open and functional

**Failure Indicators:**
- ❌ Window doesn't close
- ❌ Crash on close
- ❌ Main window becomes unresponsive

---

#### Test 1.3: Close Debugger Window (X Button)
**Steps:**
1. With debugger open, click the **X** button (close button) on debugger window

**Expected:**
- ✅ Debugger window closes
- ✅ Same behavior as pressing F10
- ✅ No crashes or errors

**Failure Indicators:**
- ❌ Window doesn't respond to X button
- ❌ Crash on close
- ❌ Both windows close

---

#### Test 1.4: Multiple Open/Close Cycles
**Steps:**
1. Press F10 (open)
2. Press F10 (close)
3. Repeat 5-10 times

**Expected:**
- ✅ Window opens/closes reliably each time
- ✅ No memory leaks (check Task Manager)
- ✅ No performance degradation
- ✅ No console errors

**Failure Indicators:**
- ❌ Window fails to open after several cycles
- ❌ Memory usage increases continuously
- ❌ Performance degrades over time

---

### 2. Window Independence

#### Test 2.1: Move to Second Monitor
**Steps:**
1. Open debugger (F10)
2. If available, drag window to second monitor
3. Interact with both windows

**Expected:**
- ✅ Window moves smoothly to second monitor
- ✅ Both windows remain functional
- ✅ Events routed correctly to each window
- ✅ No input conflicts

**Failure Indicators:**
- ❌ Window cannot be moved
- ❌ Window disappears when moved
- ❌ Input stops working on one window

---

#### Test 2.2: Resize Window
**Steps:**
1. Open debugger (F10)
2. Drag window edges to resize
3. Try various sizes (small, large, maximized)

**Expected:**
- ✅ Window resizes smoothly
- ✅ ImGui panels adjust to new size
- ✅ No rendering artifacts
- ✅ All UI elements remain accessible

**Failure Indicators:**
- ❌ Window doesn't resize
- ❌ UI elements disappear or overlap incorrectly
- ❌ Crash on resize

---

#### Test 2.3: Minimize/Restore Window
**Steps:**
1. Open debugger (F10)
2. Minimize debugger window
3. Restore debugger window from taskbar

**Expected:**
- ✅ Window minimizes correctly
- ✅ Window restores correctly
- ✅ Content renders properly after restore
- ✅ No crashes

**Failure Indicators:**
- ❌ Window doesn't minimize/restore
- ❌ Content is corrupted after restore
- ❌ Crash on restore

---

### 3. Layout Algorithm

#### Test 3.1: No Node Overlaps
**Steps:**
1. Open debugger with complex behavior tree (10+ nodes)
2. Zoom out to see entire tree
3. Check for node overlaps

**Expected:**
- ✅ No nodes overlap (even with long names)
- ✅ Minimum 2.5 abstract units spacing between nodes
- ✅ All node names are readable

**Failure Indicators:**
- ❌ Nodes overlap partially or completely
- ❌ Node names are hidden behind other nodes

---

#### Test 3.2: Reduced Edge Crossings
**Steps:**
1. Open debugger with complex tree (multiple children per node)
2. Observe edge connections between nodes
3. Compare with previous version (if available)

**Expected:**
- ✅ Significantly fewer edge crossings
- ✅ Graph is easier to read
- ✅ Parent-child relationships are clear

**Failure Indicators:**
- ❌ Many edges cross unnecessarily
- ❌ Graph is hard to read
- ❌ Cannot trace parent-child relationships

---

#### Test 3.3: Layout Performance
**Steps:**
1. Open debugger with large tree (50+ nodes)
2. Measure time to compute layout (check console)
3. Switch between entities

**Expected:**
- ✅ Layout computes in < 100ms
- ✅ No noticeable lag
- ✅ Smooth entity switching

**Failure Indicators:**
- ❌ Layout takes > 500ms
- ❌ Visible lag when switching entities
- ❌ Frame rate drops significantly

---

### 4. UI Functionality

#### Test 4.1: Entity List
**Steps:**
1. Open debugger
2. Check entity list panel (left side)
3. Select different entities

**Expected:**
- ✅ All AI entities listed
- ✅ Entity selection works
- ✅ Graph updates when entity selected
- ✅ Current entity highlighted

**Failure Indicators:**
- ❌ Entity list is empty
- ❌ Cannot select entities
- ❌ Graph doesn't update

---

#### Test 4.2: Node Graph Panel
**Steps:**
1. Select entity with behavior tree
2. Check node graph (center panel)
3. Use zoom controls (mouse wheel)
4. Pan view (drag with mouse)

**Expected:**
- ✅ Behavior tree graph visible
- ✅ Zoom in/out works (mouse wheel)
- ✅ Pan works (drag)
- ✅ Current node highlighted
- ✅ Connections visible

**Failure Indicators:**
- ❌ Graph is blank
- ❌ Zoom/pan doesn't work
- ❌ Nodes not visible
- ❌ Connections missing

---

#### Test 4.3: Inspector Panel
**Steps:**
1. Select entity
2. Check inspector panel (right side)
3. Verify blackboard values
4. Check execution log

**Expected:**
- ✅ Runtime info displayed
- ✅ Blackboard variables visible
- ✅ Execution log updates
- ✅ Timestamps accurate

**Failure Indicators:**
- ❌ Inspector is blank
- ❌ Blackboard not visible
- ❌ Execution log doesn't update

---

#### Test 4.4: Menu Bar
**Steps:**
1. Click "View" menu
2. Adjust sliders (spacing, zoom)
3. Click "Actions" menu
4. Try menu items (Fit to View, Center, Reset Zoom)

**Expected:**
- ✅ View menu works
- ✅ Spacing sliders functional
- ✅ Actions menu works
- ✅ All menu items respond
- ✅ "Window Mode: Separate (Independent)" text visible

**Failure Indicators:**
- ❌ Menus don't open
- ❌ Sliders don't work
- ❌ Actions don't execute

---

### 5. Stability Tests

#### Test 5.1: Main Window Responsiveness
**Steps:**
1. Open debugger (F10)
2. Interact with main engine window
3. Move camera, select objects, open menus

**Expected:**
- ✅ Main window remains fully responsive
- ✅ Game continues running normally
- ✅ No input conflicts
- ✅ Frame rate stays normal

**Failure Indicators:**
- ❌ Main window becomes slow
- ❌ Input doesn't work in main window
- ❌ Game freezes
- ❌ Frame rate drops significantly

---

#### Test 5.2: Extended Use
**Steps:**
1. Open debugger
2. Leave running for 10+ minutes
3. Switch entities multiple times
4. Zoom/pan extensively

**Expected:**
- ✅ No memory leaks (check Task Manager)
- ✅ Performance remains stable
- ✅ No crashes or errors
- ✅ UI remains responsive

**Failure Indicators:**
- ❌ Memory usage increases continuously
- ❌ Performance degrades over time
- ❌ Crashes after extended use

---

#### Test 5.3: Edge Cases
**Steps:**
1. Open debugger with NO AI entities
2. Open debugger with entity that has NO behavior tree
3. Open debugger with INVALID tree ID

**Expected:**
- ✅ Debugger opens without crash
- ✅ Entity list shows appropriate entities
- ✅ Graph shows "No tree" or similar message
- ✅ No crashes or errors

**Failure Indicators:**
- ❌ Crash when no entities present
- ❌ Crash with invalid tree
- ❌ Debugger becomes unusable

---

## Performance Benchmarks

### Layout Algorithm
- **Small trees (< 10 nodes)**: < 5ms
- **Medium trees (10-30 nodes)**: < 15ms
- **Large trees (30-50 nodes)**: < 25ms
- **Very large trees (50+ nodes)**: < 50ms

### Window Operations
- **Window creation**: < 20ms
- **Window destruction**: < 10ms
- **Per-frame render**: < 2ms

### Memory Usage
- **Debugger closed**: 0 bytes overhead
- **Debugger open (empty)**: < 50KB overhead
- **Debugger open (10 entities)**: < 500KB overhead

---

## Known Limitations

1. **SDL3 Backend**: Does NOT support ImGui multi-viewport (ConfigFlags_ViewportsEnable)
2. **Window Position**: Not saved between sessions
3. **Layout Algorithm**: May be slow (> 100ms) for trees with 100+ nodes
4. **Debug Mode Only**: `#ifdef DEBUG_BT_LAYOUT` disables edge crossing counter in release

---

## Troubleshooting

### Problem: Window doesn't appear
**Possible Causes:**
- SDL3 initialization failed
- Graphics driver issue
- Multi-monitor setup issue

**Solutions:**
- Check console for SDL errors
- Update graphics drivers
- Try moving main window and pressing F10 again

---

### Problem: Nodes overlap
**Possible Causes:**
- Very long node names
- Extremely wide tree (> 20 nodes in one layer)
- Zoom level too low

**Solutions:**
- Increase node spacing (View menu)
- Zoom in
- Use layout direction: Left-to-Right

---

### Problem: Performance issues
**Possible Causes:**
- Very large tree (> 100 nodes)
- Multiple entities with large trees
- Debug build (non-optimized)

**Solutions:**
- Use Release build
- Filter entities (only show active)
- Close debugger when not needed

---

### Problem: Input conflicts
**Possible Causes:**
- Both windows focused simultaneously
- Event routing issue

**Solutions:**
- Click on window to ensure focus
- Close and reopen debugger
- Restart engine if issue persists

---

## Reporting Issues

When reporting issues, please include:

1. **System Info**: OS, SDL3 version, GPU
2. **Build Config**: Debug/Release, compiler version
3. **Steps to Reproduce**: Exact sequence of actions
4. **Console Output**: Copy all `[BTDebugger]` messages
5. **Screenshots**: If visual issue, include screenshot
6. **Tree Complexity**: Number of nodes in tree

---

## Success Criteria

### Must Pass
- ✅ Window opens/closes reliably
- ✅ No crashes or memory leaks
- ✅ No node overlaps in complex trees
- ✅ Main window remains responsive

### Should Pass
- ✅ Fewer edge crossings than before
- ✅ Window can move to second monitor
- ✅ Layout computes in < 100ms
- ✅ All UI features functional

### Nice to Have
- ✅ Layout computes in < 50ms
- ✅ No edge crossings at all
- ✅ Supports 100+ node trees
- ✅ Window position persists

---

## Final Checklist

Before marking issue as resolved, verify:

- [ ] All "Must Pass" criteria met
- [ ] All "Should Pass" criteria met
- [ ] No new console errors introduced
- [ ] Performance within benchmarks
- [ ] Documentation updated
- [ ] Screenshots taken showing improvements

---

## Contact

For questions or issues with testing:
- Check console output for `[BTDebugger]` messages
- Review implementation summary in PR description
- Report issues in GitHub issue tracker
