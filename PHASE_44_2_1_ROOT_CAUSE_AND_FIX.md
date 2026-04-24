# Phase 44.2.1 - ROOT CAUSE ANALYSIS & FIX PLAN

## THE ACTUAL PROBLEM (Root Cause #1)

### CanvasModalRenderer - Confirmation Flag Never Resets

**File**: `Source/BlueprintEditor/Framework/CanvasModalRenderer.cpp`  
**Lines**: 151-190

```cpp
void CanvasModalRenderer::OpenSaveFilePickerModal(...)
{
    m_saveFileModal.Open(initialPath, suggestedName);
    m_saveConfirmed = false;              // ← Reset when opening
    m_cachedSaveSelection = "";
}

void CanvasModalRenderer::RenderSaveFilePickerModal()
{
    m_saveFileModal.Render();
    
    if (!m_saveConfirmed && m_saveFileModal.IsConfirmed())
    {
        m_saveConfirmed = true;           // ← Set when user confirms
        m_cachedSaveSelection = m_saveFileModal.GetSelectedFile();
    }
    // ✅ PROBLEM: Flag NEVER reset to false!
}

bool CanvasModalRenderer::IsSaveFileModalConfirmed() const
{
    return m_saveConfirmed;               // ← Flag stays true forever
}
```

**The Bug**: 
1. User confirms save → m_saveConfirmed = true
2. Toolbar reads flag → calls OnSaveAsComplete()
3. Flag is NEVER reset
4. Next frame: Flag still true → toolbar might call handler again or flag behavior is confused

### CanvasToolbarRenderer - After Save, Doesn't Update UI

**File**: `Source/BlueprintEditor/Framework/CanvasToolbarRenderer.cpp`  
**Lines**: 367-388

```cpp
void CanvasToolbarRenderer::OnSaveAsComplete(const std::string& filePath)
{
    if (!m_document || filePath.empty())
        return;

    SYSTEM_LOG << "[CanvasToolbarRenderer] SaveAs selected: " << filePath << "\n";

    if (ExecuteSave(filePath))           // ← Calls document.Save()
    {
        m_selectedFilePath = filePath;
        SYSTEM_LOG << "[CanvasToolbarRenderer] Successfully saved to: " << filePath << "\n";
        if (m_onSaveComplete)
            m_onSaveComplete(filePath);
    }
    // ❌ PROBLEM: Doesn't update TabManager with new path!
}
```

**The Issue**:
- ExecuteSave() calls document.Save(filePath) ✓
- document.Save() updates m_filePath internally ✓
- BUT: TabManager doesn't know the path changed
- Result: Tab title still shows "Untitled-1" even though file was saved

---

## COMPLETE FIX STRATEGY

### Fix 1: Reset Confirmation Flag After Reading
**Location**: `CanvasToolbarRenderer::RenderModals()` lines 120-126
**Action**: Reset flag after reading it

### Fix 2: Notify TabManager After Save
**Location**: `CanvasToolbarRenderer::OnSaveAsComplete()` line 382
**Action**: Call callback or notify TabManager to update display

### Fix 3: Verify Modal is Actually Closed
**Location**: `SaveFilePickerModal` internal state
**Action**: Ensure IsOpen() returns false when user closes modal

---

## DETAILED FIXES

### Fix 1: CanvasModalRenderer - Reset Confirmation Flag

**Current Code** (CanvasModalRenderer.cpp, lines 176-179):
```cpp
bool CanvasModalRenderer::IsSaveFileModalConfirmed() const
{
    return m_saveConfirmed;
}
```

**Fixed Code**:
```cpp
bool CanvasModalRenderer::IsSaveFileModalConfirmed()  // Remove const
{
    bool result = m_saveConfirmed;
    if (m_saveConfirmed)
    {
        m_saveConfirmed = false;  // ← RESET after reading
    }
    return result;
}
```

**OR alternatively** in CanvasToolbarRenderer (cleaner):

**Current Code** (CanvasToolbarRenderer.cpp, lines 120-126):
```cpp
if (CanvasModalRenderer::Get().IsSaveFileModalConfirmed())
{
    std::string selectedPath = CanvasModalRenderer::Get().GetSelectedSaveFilePath();
    OnSaveAsComplete(selectedPath);
    m_showSaveAsModal = false;
    CanvasModalRenderer::Get().CloseSaveFileModal();
}
```

**Fixed Code**:
```cpp
if (CanvasModalRenderer::Get().IsSaveFileModalConfirmed())
{
    std::string selectedPath = CanvasModalRenderer::Get().GetSelectedSaveFilePath();
    OnSaveAsComplete(selectedPath);
    m_showSaveAsModal = false;
    CanvasModalRenderer::Get().CloseSaveFileModal();
    CanvasModalRenderer::Get().ResetSaveFileModal();  // ← RESET flag
}
```

---

### Fix 2: TabManager - Update After Save

**Current Issue**: 
- Toolbar saves file but Tab doesn't know the new path
- Tab still shows "Untitled-1 *" even though file exists

**Solution**: CanvasToolbarRenderer needs callback to notify TabManager

**Location**: Add method to TabManager to update tab display

```cpp
// In TabManager.h
void OnGraphSaved(IGraphDocument* document, const std::string& newPath);

// In TabManager.cpp
void TabManager::OnGraphSaved(IGraphDocument* document, const std::string& newPath)
{
    for (auto& tab : m_tabs)
    {
        if (tab.second.document == document)
        {
            tab.second.displayName = DisplayNameFromPath(newPath);
            SYSTEM_LOG << "[TabManager] Updated tab display name: " << tab.second.displayName << "\n";
            break;
        }
    }
}
```

**In CanvasToolbarRenderer::OnSaveAsComplete**:
```cpp
void CanvasToolbarRenderer::OnSaveAsComplete(const std::string& filePath)
{
    // ... existing code ...
    
    if (ExecuteSave(filePath))
    {
        m_selectedFilePath = filePath;
        SYSTEM_LOG << "[CanvasToolbarRenderer] Successfully saved to: " << filePath << "\n";
        if (m_onSaveComplete)
            m_onSaveComplete(filePath);
        
        // ← NEW: Notify TabManager to update display
        TabManager::Get().OnGraphSaved(m_document, filePath);
    }
}
```

---

### Fix 3: Ensure Modal Closes Properly

**Check SaveFilePickerModal::IsOpen()** to ensure it returns false when:
1. User clicks "Save" button
2. User clicks "Cancel" button
3. User clicks X (close) button

If IsOpen() doesn't return false, the toolbar will keep the modal open forever.

---

## VERIFICATION CHECKLIST

After applying fixes:

- [ ] CanvasModalRenderer resets confirmation flag after read
- [ ] CanvasToolbarRenderer calls ExecuteSave(filePath) 
- [ ] ExecuteSave calls m_document->Save(filePath)
- [ ] document->Save updates internal path
- [ ] TabManager gets notified of save completion
- [ ] Tab display name updates to new filename
- [ ] Dirty flag (* indicator) clears
- [ ] Modal actually closes after save
- [ ] Next frame doesn't trigger duplicate save

---

## COMPLETE WORKFLOW AFTER FIX

```
1. User clicks "Save" button
   ↓
2. Toolbar: OnSaveClicked() or OnSaveAsClicked()
   ↓
3. Toolbar: m_showSaveAsModal = true
   ↓
4. Toolbar: RenderModals() opens and renders modal
   ↓
5. User enters filename and clicks "Save"
   ↓
6. Modal: Sets internal confirmed flag
   ↓
7. Toolbar: IsSaveFileModalConfirmed() returns true
   ↓
8. Toolbar: GetSelectedSaveFilePath() returns path
   ↓
9. Toolbar: OnSaveAsComplete(path) called
   ↓
10. Toolbar: ExecuteSave(path) called
    ↓
11. Document: Save(path) called
    ↓
12. Document: Serializes and writes JSON to disk
    ↓
13. Document: Sets m_isDirty = false, m_filePath = path
    ↓
14. Toolbar: Calls TabManager::OnGraphSaved(document, path)
    ↓
15. TabManager: Updates tab title to filename
    ↓
16. Toolbar: Resets modal confirmation flag
    ↓
17. Modal: Closes and returns to idle state
    ↓
18. **RESULT: File saved, tab updated, UI consistent** ✅
```

---

## NEXT ACTIONS

1. Apply Fix 1: Reset confirmation flag
2. Add Fix 2: TabManager notification
3. Apply Fix 3: Verify modal closure
4. Test all three editors
5. Verify tab titles update after save
6. Verify dirty flags clear
7. Verify files persist on disk
