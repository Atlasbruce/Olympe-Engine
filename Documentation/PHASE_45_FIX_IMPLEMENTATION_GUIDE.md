# Phase 45: Framework Save/SaveAs Button Fix - Implementation Guide

## 🎯 Problem Statement

**User Issue**: Framework Save/SaveAs buttons don't work - modals never appear
**Root Cause**: Modal rendering called at wrong time in ImGui frame cycle
**Expected**: Modal appears when Save As button clicked
**Actual**: Nothing happens; no modal appears

---

## 🔍 Root Cause Analysis

### ImGui Frame Ordering Issue

ImGui is **immediate-mode** - modals must be rendered:
1. ✅ AFTER content (buttons, canvas)
2. ✅ BEFORE frame swap
3. ❌ NOT during content rendering

**Current Broken Code**:
```
Frame cycle:
1. RenderTabBar() [Line 667]
   ↓ Opens modal, renders it (Line 805)
   ↓ Modal closes/processes this frame
2. RenderActiveCanvas() [Line 672]
   ↓ Tries to render modals again
   ↓ But flag already cleared!
3. Result: User sees nothing
```

**Why It Fails**:
- Modals rendered at **Line 805** during `RenderTabBar()`
- Modal rendered **again** at **Line 112** during `RenderActiveCanvas()`
- Both attempt to process same modal in same frame
- Second attempt fails because flag already consumed

---

## ✅ Solution: Move Modal Rendering to Correct Frame Point

### Changes Required

#### File 1: `Source/BlueprintEditor/TabManager.cpp`

**Location**: Lines 800-806

**ACTION**: Delete modal rendering from RenderTabBar()

```cpp
// BEFORE (Lines 800-806):
    // Phase 43: Render framework modals for all graph types
    // Centralized rendering point for Save/SaveAs/Browse toolbar buttons
    EditorTab* activeTab = GetActiveTab();
    if (activeTab && activeTab->renderer)
    {
        activeTab->renderer->RenderFrameworkModals();
    }
}

// AFTER: Just close the function
}
```

**Reason**: Rendering modals during RenderTabBar() is too early in frame cycle

---

#### File 2: `Source/BlueprintEditor/BlueprintEditorGUI.cpp`

**Location**: After Line 672 (after RenderActiveCanvas call)

**ACTION**: Add modal rendering at correct frame point

```cpp
// BEFORE (Lines 671-674):
            // Active graph canvas
            TabManager::Get().RenderActiveCanvas();
        }
        ImGui::EndChild();

// AFTER:
            // Active graph canvas
            TabManager::Get().RenderActiveCanvas();

            // Phase 45: Framework modal rendering at proper ImGui timing
            // Must be after all content rendering for correct ImGui frame ordering
            {
                EditorTab* modalTab = TabManager::Get().GetActiveTab();
                if (modalTab && modalTab->renderer)
                {
                    modalTab->renderer->RenderFrameworkModals();
                }
            }
        }
        ImGui::EndChild();
```

**Reason**: Modals must render AFTER all content in ImGui frame cycle

---

## 📋 Implementation Steps

### Step 1: Remove Old Modal Rendering

**File**: `Source/BlueprintEditor/TabManager.cpp`
**Lines to delete**: 800-806

**Before**:
```cpp
    }

    // Phase 43: Render framework modals for all graph types
    // Centralized rendering point for Save/SaveAs/Browse toolbar buttons
    EditorTab* activeTab = GetActiveTab();
    if (activeTab && activeTab->renderer)
    {
        activeTab->renderer->RenderFrameworkModals();
    }
}
```

**After**:
```cpp
    }
}
```

### Step 2: Add New Modal Rendering Point

**File**: `Source/BlueprintEditor/BlueprintEditorGUI.cpp`
**Location**: After Line 672

**Before**:
```cpp
            // Active graph canvas
            TabManager::Get().RenderActiveCanvas();
        }
        ImGui::EndChild();
```

**After**:
```cpp
            // Active graph canvas
            TabManager::Get().RenderActiveCanvas();

            // Phase 45: Framework modal rendering at proper ImGui timing
            // Must be after all content rendering for correct ImGui frame ordering
            {
                EditorTab* modalTab = TabManager::Get().GetActiveTab();
                if (modalTab && modalTab->renderer)
                {
                    modalTab->renderer->RenderFrameworkModals();
                }
            }
        }
        ImGui::EndChild();
```

---

## 🧪 Verification Steps

After implementing the fix:

### Test 1: Save Button
1. Open BehaviorTree graph
2. Add a node or modify graph (mark dirty)
3. Click **Save** button in toolbar
4. Expected: File saves to current path
5. Check logs: `[CanvasToolbarRenderer::ExecuteSave] ✓ Saved: ...`

### Test 2: Save As Button
1. Open BehaviorTree graph
2. Click **Save As** button in toolbar
3. Expected: Modal appears with filepath picker
4. Select new path and click Save
5. Expected: File saves to new location
6. Check logs: `[CanvasToolbarRenderer::OnSaveAsComplete] Successfully saved to: ...`

### Test 3: Ctrl+S Shortcut
1. Open BehaviorTree graph
2. Modify graph
3. Press **Ctrl+S**
4. Expected: File saves immediately
5. Check logs: Save confirmation

### Test 4: Ctrl+Shift+S Shortcut
1. Open BehaviorTree graph
2. Press **Ctrl+Shift+S**
3. Expected: SaveAs modal appears
4. Complete save operation

### Test 5: Dirty Flag Tracking
1. Open graph
2. Note Save button should be DISABLED (greyed out)
3. Modify graph (add node, delete node)
4. Note Save button should be ENABLED
5. Click Save
6. Note Save button should be DISABLED again
7. Modify again
8. Note Save button should be ENABLED

---

## 🔧 Build Verification

After changes, verify:

```powershell
# 1. Build the project
cmake --build . --config Debug

# 2. Check for errors
# Expected: 0 errors, 0 warnings

# 3. Run the editor
./OlympeBlueprintEditor.exe

# 4. Test Save/SaveAs buttons
# Expected: All operations work, modals appear correctly
```

---

## 📊 Before & After Comparison

| Scenario | Before Fix | After Fix |
|----------|-----------|-----------|
| Click Save | No action | File saves ✅ |
| Click Save As | No modal | Modal appears ✅ |
| Ctrl+S | No save | File saves ✅ |
| Ctrl+Shift+S | No modal | Modal appears ✅ |
| Dirty flag | Shows but no save | Works correctly ✅ |
| Error logs | No save errors | Clean logs ✅ |

---

## 📝 Code Review Checklist

- [ ] Deleted Lines 800-806 from TabManager.cpp
- [ ] Added modal rendering block to BlueprintEditorGUI.cpp (after Line 672)
- [ ] No syntax errors in either file
- [ ] Build succeeds with 0 errors
- [ ] Tested Save button functionality
- [ ] Tested Save As modal
- [ ] Verified Ctrl+S shortcut
- [ ] Verified Ctrl+Shift+S shortcut
- [ ] Checked dirty flag behavior
- [ ] Reviewed logs for errors

---

## 🎯 Expected Result

After fix:
1. **Save button** → File saves immediately
2. **Save As button** → Modal appears with filepath picker
3. **Dirty flag** → Reflects graph modification state correctly
4. **Keyboard shortcuts** → Ctrl+S and Ctrl+Shift+S work properly
5. **Framework buttons** → Fully functional across all document types (BT, VS, EP)

---

## 📌 Notes

- This fix maintains **backward compatibility**
- All existing functionality preserved
- Framework architecture unchanged
- Only frame rendering order corrected
- No breaking changes to API

