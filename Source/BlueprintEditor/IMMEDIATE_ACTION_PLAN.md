/**
 * @file IMMEDIATE_ACTION_PLAN.md
 * @brief Immediate actions needed to complete Phase 4
 */

# IMMEDIATE ACTION PLAN - Phase 4 Completion

## Current Status

**GOOD NEWS** ✅
- All code changes are implemented and ready
- All fixes for input handling are in place
- Build error is ONLY due to exe being locked (not a code issue)

**BLOCKER** ⏳
- OlympeBlueprintEditor.exe is still running
- Prevents linker from writing output exe
- Need to close exe to unlock file

---

## Step-by-Step Instructions

### 1. CLOSE THE RUNNING EXE
```
In Visual Studio:
- Debug → Stop Debugging (Shift+F5)
OR
- Press Ctrl+Alt+Break
OR
- Manual: Task Manager → Find OlympeBlueprintEditor → End Task
```

### 2. REBUILD THE PROJECT
```
In Visual Studio:
- Build → Rebuild Solution
OR
- Right-click project → Rebuild
OR
- Press Ctrl+Shift+B
```

### 3. WAIT FOR BUILD TO COMPLETE
```
Expected result: ✅ Build succeeded (0 errors, 0 warnings)
Time: ~30-60 seconds
```

### 4. RUN THE EXE
```
- Debug → Start Debugging (F5)
OR
- Press F5
```

### 5. OPEN guard.json
```
In the editor:
- File → Open → navigate to ./Gamedata/EntityPrefab/guard.json
OR
- (Or whichever prefab file you're testing)
```

### 6. PERFORM TESTS
See PHASE_4_SUMMARY_AND_NEXT_STEPS.md for full test plan

---

## What Changed in the Code

### CustomCanvasEditor::UpdateInputState() - FIXED HOVER DETECTION

**Before (BROKEN for child windows)**:
```cpp
void CustomCanvasEditor::UpdateInputState()
{
    ImGuiIO& io = ImGui::GetIO();

    // ❌ PROBLEM: Returns FALSE for child windows
    if (!IsCanvasHovered())
    {
        m_lastMousePos = io.MousePos;
        m_isPanning = false;
        return;  // ← Never reaches input handling!
    }

    HandlePanning();      // ← Never called
    HandleZooming();      // ← Never called
    m_lastMousePos = io.MousePos;
}
```

**After (FIXED)**:
```cpp
void CustomCanvasEditor::UpdateInputState()
{
    ImGuiIO& io = ImGui::GetIO();

    // ✅ BETTER: Point-based detection works for child windows
    bool isPointInCanvasArea = IsPointInCanvas(io.MousePos);
    bool hasWindowFocus = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    bool shouldHandleInput = isPointInCanvasArea && hasWindowFocus;

    if (!shouldHandleInput)
    {
        m_lastMousePos = io.MousePos;
        m_isPanning = false;
        return;
    }

    HandlePanning();      // ← NOW CALLED ✅
    HandleZooming();      // ← NOW CALLED ✅
    m_lastMousePos = io.MousePos;
}
```

**Why This Works**:
- IsPointInCanvas() checks absolute coordinates: `mouse.x > canvasPos.x`
- Works regardless of ImGui window state
- Child windows flag tells ImGui to check child hierarchy
- More robust for complex layouts

---

## Expected Behavior After Build

### Test #1: Zoom
```
Action: Scroll wheel UP
Expected: Zoom increases (1.0x → 1.1x → 1.2x)
Grid becomes denser (smaller squares)
Debug info shows: "Zoom: 1.10"

Action: Scroll wheel DOWN  
Expected: Zoom decreases (1.0x → 0.9x → 0.8x)
Grid becomes sparser (larger squares)
Debug info shows: "Zoom: 0.90"
```

### Test #2: Pan
```
Action: Middle-mouse drag RIGHT
Expected: Canvas pans RIGHT, nodes shift LEFT visually
Debug info shows: "Offset: 100, 0"

Action: Middle-mouse drag UP
Expected: Canvas pans UP, nodes shift DOWN visually
Debug info shows: "Offset: 100, 50"
```

### Test #3: Grid Scaling
```
At Zoom 1.0x: Grid squares ~24px
At Zoom 2.0x: Grid squares ~48px
At Zoom 0.5x: Grid squares ~12px
```

### Test #4: Multi-Select
```
Action: Ctrl+Click drag to select multiple nodes
Drag them while holding left-mouse
Expected: All move together, no offset
Should work same at Zoom 0.5x, 1.0x, 2.0x
```

---

## If Build STILL Fails

**Error**: LNK1168: impossible d'ouvrir ..\OlympeBlueprintEditor_d.exe

**Solution**:
1. Definitely close the exe (check Task Manager)
2. Navigate to output folder: OlympeBlueprintEditor_d.exe location
3. Delete the exe manually if needed (will be recreated)
4. Try build again

**If Still Fails**:
1. Restart Visual Studio
2. Rebuild

---

## If Tests FAIL

### If Zoom Doesn't Work
- Check CustomCanvasEditor::HandleZooming() is being called
- Add debug output: `printf("[ZOOM] io.MouseWheel=%f\n", io.MouseWheel);`
- Verify io.MouseWheel is non-zero when scrolling

### If Pan Doesn't Work
- Check CustomCanvasEditor::HandlePanning() is being called
- Add debug output: `printf("[PAN] Middle-mouse pressed\n");`
- Verify ImGui::IsMouseDown(ImGuiMouseButton_Middle) is true

### If Grid Doesn't Scale
- Zoom should be working (fix that first)
- Check gridConfig.zoom value: `printf("[GRID] zoom=%f\n", gridConfig.zoom);`
- Verify CanvasGridRenderer::RenderGrid() receives correct zoom

### If Multi-Select Still Has Offset
- This needs separate investigation
- Could be HandleNodeDrag() issue
- Could be zoom/pan interaction during drag
- Will need debug logging in HandleNodeDrag()

---

## Files to Check If Needed

1. **Source/BlueprintEditor/Utilities/CustomCanvasEditor.cpp**
   - UpdateInputState() - Line ~216
   - HandlePanning() - Line ~243  
   - HandleZooming() - Line ~266

2. **Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.cpp**
   - RenderGrid() - Line ~560
   - OnMouseScroll() - Line ~256

3. **Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp**
   - RenderLayoutWithTabs() - Line ~31

---

## Success Criteria

✅ **Phase 4 Complete When**:
- Build succeeds without errors
- Scroll wheel changes zoom
- Middle-mouse pans canvas
- Grid scales with zoom
- Multi-select works without unexpected offset
- All functionality works at zoom 0.5x, 1.0x, 2.0x

---

## Estimated Timeline

```
NOW:         Close exe + Start rebuild
+5 min:      Build complete
+10 min:     Exe launched
+15 min:     Load guard.json
+30 min:     Run all 5 tests
+15 min:     Document results
---
45 min total: Phase 4 Step 4-10 effectively complete
```

Then:
- Ready for Phase 5 (VisualScript integration) - much simpler!
- Or: Debug multi-select if needed

---

## Quick Reference Commands

```powershell
# Close exe gracefully from PowerShell
taskkill /IM OlympeBlueprintEditor_d.exe /F

# Delete output exe to force clean link
rm ..\OlympeBlueprintEditor_d.exe -Force

# Rebuild in Visual Studio
Ctrl+Shift+B
```

---

**You're 95% done with Phase 4. Just need to close the exe and rebuild!**

