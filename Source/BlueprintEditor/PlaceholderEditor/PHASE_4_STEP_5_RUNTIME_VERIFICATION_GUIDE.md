# Phase 4 Step 5 - Runtime Verification Guide
**Date**: Current Session  
**Build Status**: ✅ 0 errors, 0 warnings  
**Features Ready for Testing**: Feature #1 (Toolbar), Feature #2 (Rectangle Selection), Minimap Overlay

---

## 📋 Quick Build Summary

### What Was Fixed This Session (All 3 Build Phases)

**Build Phase 1: Initial Compilation** ❌ FAILED
- Issue: 28 compilation errors
- Causes: SYSTEM_LOG syntax + CanvasMinimapRenderer API mismatches

**Build Phase 2: SYSTEM_LOG Removal** ✅ PARTIAL SUCCESS
- Fixed: Removed 16 lines of SYSTEM_LOG diagnostics
- Result: ~6 errors eliminated
- Remaining: 22 errors (CanvasMinimapRenderer API issues)

**Build Phase 3: API Signature Fixes** ✅ COMPLETE SUCCESS
- Fixed: PlaceholderCanvas.h lines 55, 60 (SetSize, SetPosition)
- Fixed: PlaceholderCanvas.cpp line 454 (UpdateNodes tuple conversion)
- Fixed: PlaceholderCanvas.cpp line 480 (UpdateViewport 8-param signature)
- Fixed: PlaceholderCanvas.cpp line 483 (RenderCustom screen coordinates)
- Result: **0 errors, 0 warnings** ✅

---

## 🎯 Runtime Verification Checklist

### Step 1: Launch Editor with Placeholder Graph

**Action**: 
1. Start Olympe-Engine application
2. Open Blueprint Editor (Placeholder Graph type)
3. Navigate to/create a Placeholder graph file

**Expected Console Output**:
```
[PlaceholderGraphRenderer] Loading: <file_path>
[PlaceholderGraphRenderer] Loaded successfully
[PlaceholderGraphRenderer] InitializeCanvasEditor
```

**Visual Inspection**:
- Editor should load without errors
- No error dialogs

---

### Step 2: Verify Feature #1 - Framework Toolbar Visibility

**What to Look For**:

#### Expected UI Layout (Top to Bottom):
```
┌─────────────────────────────────────────────────┐
│ File Edit Tools View Help [MenuBar]             │
├─────────────────────────────────────────────────┤
│ [Save] [SaveAs] [Browse] | [Verify] [Run] ...   │ ← FRAMEWORK TOOLBAR
├─────────────────────────────────────────────────┤
│  Canvas Area (70%)     │  Right Panel (30%)     │
│  • Grid                │  [Comp] [Node] Tabs    │
│  • Nodes               │  ├─────────────────┤   │
│  • Connections         │  │ Properties      │   │
│  • Selection rect      │  │ Panel content   │   │
│  • Context menus       │  └─────────────────┘   │
└─────────────────────────────────────────────────┘
```

**Verification Checklist**:
- [ ] **[Save] button** is visible
  - Expected: To the left of [SaveAs]
  - State: Disabled if document not dirty, enabled if dirty
  
- [ ] **[SaveAs] button** is visible
  - Expected: Right of [Save]
  - Action: Click should open Save-As dialog
  
- [ ] **[Browse] button** is visible
  - Expected: Right of [SaveAs]
  - Action: Click should open file browser

- [ ] **Separator line** visible after [Browse]
  - Expected: Visual divider between framework and type-specific buttons

- [ ] **[Verify] button** is visible (Type-Specific)
  - Expected: After separator
  - Purpose: Validate graph structure

- [ ] **[Run] button** is visible (Type-Specific)
  - Expected: Right of [Verify]
  - Purpose: Execute placeholder graph

- [ ] **[✓ Minimap] checkbox** is visible (Type-Specific)
  - Expected: Right side of toolbar
  - State: Checked (minimap visible)

- [ ] **Minimap Size slider** is visible
  - Expected: Right of checkbox
  - Range: Small slider for size adjustment
  - Default: ~15% of canvas size

**Diagnostic Output** (if toolbar missing):
- If you see: `DEBUG: m_toolbar is nullptr - buttons will not appear`
  - Means: m_toolbar not initialized
  - Action: Check Load() initialization at line 55

---

### Step 3: Verify Feature #2 - Rectangle Selection

**What to Test**:

1. **Drag-Select on Canvas**
   - [ ] Click and drag in empty canvas area
   - [ ] Expected: Blue selection rectangle appears while dragging
   - [ ] Release: Nodes inside rectangle should be highlighted

2. **Multi-Node Selection**
   - [ ] Select multiple nodes with one rectangle drag
   - [ ] Expected: All nodes inside glow blue
   - [ ] Count: Should match visual node count

3. **Ctrl+Click Additive Selection**
   - [ ] Hold Ctrl while dragging new rectangle
   - [ ] Expected: Adds to existing selection (doesn't clear)

4. **Visual Feedback**
   - [ ] Selected nodes have blue outline/glow
   - [ ] Properties panel updates to show selected node
   - [ ] Selection persists until clicking elsewhere

---

### Step 4: Verify Minimap Overlay Display

**What to Look For**:

1. **Minimap Visual Appearance**
   - [ ] Small preview window in **top-right corner** of canvas (default position)
   - [ ] Expected size: ~15% of canvas
   - [ ] Minimap shows:
     - Graph background (dark blue/gray)
     - Node positions (small rectangles)
     - Viewport rectangle (yellow/light overlay)

2. **Minimap Controls**
   - [ ] **Checkbox**: Click to toggle visibility
     - When checked: Minimap visible
     - When unchecked: Minimap hidden
   
   - [ ] **Size Slider**: Adjust minimap size
     - Drag left: Smaller minimap (5% of canvas)
     - Drag right: Larger minimap (50% of canvas)
     - Default: 15%

   - [ ] **Position Combo** (if visible):
     - TopLeft, TopRight (default), BottomLeft, BottomRight

3. **Minimap Interaction** (if implemented):
   - [ ] Click on minimap viewport rectangle
   - [ ] Expected: Canvas pans/zooms to that area
   - [ ] Note: May not be fully implemented yet

---

### Step 5: Feature Integration Verification

**End-to-End Test**:

1. [ ] Load Placeholder graph successfully
2. [ ] Toolbar buttons appear and render properly
3. [ ] Rectangle selection works on canvas
4. [ ] Minimap displays in corner with controls
5. [ ] All visual elements positioned correctly
6. [ ] No visual overlaps or glitching
7. [ ] Responsive to user input

---

## 🐛 Common Issues & Diagnostics

### Issue: Toolbar buttons missing entirely

**Possible Causes**:
1. m_toolbar is nullptr
   - Check: Console for "DEBUG: m_toolbar is nullptr" message
   - Fix: Verify Load() initialization at line 55

2. Buttons rendered off-screen
   - Check: Are other toolbar elements visible?
   - Fix: Check ImGui positioning code

3. BeginGroup context issue
   - Check: Are separators visible?
   - Fix: Verify ImGui::BeginGroup nesting

### Issue: Minimap checkbox visible but overlay missing

**Possible Causes**:
1. RenderMinimap() not called
   - Check: Add debug text "Minimap overlay" in Render()
   - Fix: Verify RenderMinimap() is in render chain

2. Minimap data not updated
   - Check: Verify UpdateNodes/UpdateViewport called
   - Fix: Check if PlaceholderCanvas::RenderMinimap() executing

3. Rendering at wrong coordinates
   - Check: Verify canvasScreenPos calculation
   - Fix: Check ImGui coordinate transform

### Issue: Rectangle selection not working

**Possible Causes**:
1. Event handlers not connected
   - Check: OnMouseDown/OnMouseMove/OnMouseUp triggered
   - Fix: Verify PrefabCanvas input handling (not PlaceholderCanvas)

2. AABB test incorrect
   - Check: Manual test: drag from (0,0) to (100,100), should select nodes in that rect
   - Fix: Verify SelectNodesInRectangle() logic

3. Visual feedback missing
   - Check: Look for blue glow on selected nodes
   - Fix: Verify RenderSelectionRectangle() called

---

## 📊 Expected Test Results

### Successful Run:
```
Console Output:
[PlaceholderGraphRenderer] Loading: graph.json
[PlaceholderGraphRenderer] Loaded successfully

Visual:
✅ Toolbar: [Save] [SaveAs] [Browse] | [Verify] [Run] [✓Minimap] [size]
✅ Canvas: Grid + Nodes + Connections rendered
✅ Minimap: Small preview visible in top-right corner
✅ Selection: Drag-select works, nodes highlight blue
```

### Build-Only Success (Compilation OK):
```
If build succeeds but features don't appear:
1. Add ImGui::Text() diagnostics (line 127 has example)
2. Check console for initialization traces
3. Verify m_toolbar/m_canvas not nullptr
4. Test individual components
```

---

## 🔍 Code Reference

### Files Modified This Session

**PlaceholderGraphRenderer.cpp**:
- Line 117-127: Toolbar rendering with null check diagnostic

**PlaceholderCanvas.h**:
- Line 55: SetSize() method (was SetSizeRatio)
- Line 60: SetPosition() method (with MinimapPosition enum)

**PlaceholderCanvas.cpp**:
- Line 454: UpdateNodes() with tuple conversion
- Line 480: UpdateViewport() with 8 parameters
- Line 483: RenderCustom() with screen coordinates

### API Contracts Fixed

```cpp
// ✅ CORRECT IMPLEMENTATIONS NOW

// SetSize: NOT SetSizeRatio()
minimap.SetSize(0.15f);  // 0.05 - 0.5 ratio

// SetPosition: Takes MinimapPosition enum
minimap.SetPosition(MinimapPosition::TopRight);

// UpdateNodes: Vector of tuples
std::vector<std::tuple<int, float, float, float, float>> nodes;
for (auto& node : graphNodes)
    nodes.push_back(make_tuple(node.id, node.x, node.y, node.w, node.h));
minimap.UpdateNodes(nodes, minX, maxX, minY, maxY);

// UpdateViewport: 8 parameters required
minimap.UpdateViewport(viewMinX, viewMaxX, viewMinY, viewMaxY,
                       graphMinX, graphMaxX, graphMinY, graphMaxY);

// RenderCustom: Screen position + size
minimap.RenderCustom(screenPos, screenSize);  // NOT (minPos, maxPos)
```

---

## ✅ Verification Status

| Feature | Code | Build | Runtime |
|---------|------|-------|---------|
| **Toolbar Buttons** | ✅ | ✅ | ⏳ Pending |
| **Rectangle Selection** | ✅ | ✅ | ⏳ Pending |
| **Minimap Overlay** | ✅ | ✅ | ⏳ Pending |
| **Minimap Controls** | ✅ | ✅ | ⏳ Pending |

---

## 🎯 Next Steps After Verification

1. **If all features work**:
   - Document final success
   - Remove diagnostic output (line 127)
   - Mark Phase 4 Step 5 complete

2. **If toolbar still missing**:
   - Check console for "DEBUG: m_toolbar is nullptr"
   - Trace initialization path (Load vs InitializeCanvasEditor)
   - Verify CanvasToolbarRenderer::Render() implementation

3. **If minimap doesn't appear**:
   - Verify RenderMinimap() in PlaceholderCanvas::Render()
   - Check minimap checkbox state (should be checked)
   - Verify canvas bounds calculation

4. **If rectangle selection fails**:
   - Note: Rectangle selection code inherited from Phase 31
   - May need separate placeholder canvas input handling
   - Check OnMouseDown/OnMouseMove event connections

---

**Status**: 🟢 **READY FOR RUNTIME TESTING**  
**Build Quality**: ✅ **0 errors, 0 warnings**  
**Documentation**: ✅ **Complete verification guide provided**  
**Next Action**: Launch editor and test features against checklist above.
