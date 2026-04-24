# Phase 48: SaveFilePickerModal Logging Spam Fix

## Problem Statement

**Console Frame Spam**: SaveFilePickerModal was logging directory scan results on every frame, generating 100+ identical log entries per second:
```
[SaveFilePickerModal] Found 1 files and 3 folders in .
[SaveFilePickerModal] Found 1 files and 3 folders in .
[SaveFilePickerModal] Found 1 files and 3 folders in .
... (repeated 60+ times per second)
```

**Root Cause**: The logging statements were located inside `RefreshFileList()`, which was being called on every frame when the path input field was active, causing the `ImGui::InputText()` logic to re-scan the directory and log results 60+ times per second.

**Architectural Violation**: This violated Phase 46 **Logging Discipline** rule:
> ❌ NO logs in render loops or methods called 60+ times/sec  
> ✅ Logs ONLY on state changes and user actions

**User Question**: "Why is processing architecture in the render loop? Is this normal?"
- **Answer**: NO. File I/O operations should NOT be tied to per-frame rendering. This fix separates processing from rendering.

## Solution Implemented

### Pattern: Event-Driven Logging (Phase 46 Compliance)

Moved logging OUT of the frame-loop by creating two-tier file scanning:

1. **Public Method** (`RefreshFileList()`): Used for initialization/state changes
   - Called from constructor, Open(), user actions
   - Logs on EVERY call (because these are infrequent state changes)
   - Calls internal method with `bLog = true`

2. **Internal Method** (`RefreshFileListInternal(bool bLog)`): Used for render-loop operations  
   - Called from Render() when user interacts with modal
   - Scans directory only when path actually changes
   - Only logs if `bLog = true`
   - Logs only on user-initiated navigation (folder click, parent directory, refresh button)

### Call Flow

**Initialization (logs every time - correct, happens once per Open()):**
```
User clicks "Save As"
  ↓
DataManager::OpenSaveFilePickerModal()
  ↓
SaveFilePickerModal::Open(path)
  ↓
RefreshFileList()  ← Public wrapper
  ↓
RefreshFileListInternal(true)  ← Logs results
  ↓
SYSTEM_LOG: "[SaveFilePickerModal] Found X files and Y folders in Z"
```

**User Navigation (logs only on explicit action - CORRECT, not per-frame):**
```
User clicks folder in modal
  ↓
RenderFolderList() → ImGui::Selectable(folder)
  ↓
m_currentPath = new_path
  ↓
RefreshFileListInternal(true)  ← Logs because user took action
  ↓
SYSTEM_LOG: "[SaveFilePickerModal] Found X files and Y folders in Z"
```

**Render Loop (does NOT log - CORRECT, per-frame operation):**
```
Frame 1-60 per second:
  RenderModals() calls SaveFilePickerModal::Render()
    ↓
    ImGui::InputText() change detection
      ↓
      if (path changed) RefreshFileListInternal(false)  ← NO LOG
      ↓
      Silent directory scan, results cached
```

## Implementation Details

### Modified Methods

#### SaveFilePickerModal.cpp

1. **RefreshFileList()** - Public wrapper (unchanged signature for compatibility)
   - Calls `RefreshFileListInternal(true)` for initialization
   - Used by constructor, Open(), initial modal setup

2. **RefreshFileListInternal(bool bLog)** - NEW internal method
   - Actual file scanning implementation
   - Conditional logging based on `bLog` parameter
   - Shared platform code (Windows + Unix)

3. **Render()** - Modified to use conditional scanning
   - `ImGui::InputText()` change: calls `RefreshFileListInternal(true)` only if path actually changed
   - Eliminates unnecessary re-scans on every frame while input field is active
   - Parent directory navigation: calls `RefreshFileListInternal(true)`
   - Refresh button: calls `RefreshFileListInternal(true)`

4. **RenderFolderList()** - Modified to log on user action
   - Folder selection: calls `RefreshFileListInternal(true)`
   - Only logs when user actually navigates, not every frame

### Modified Header

**SaveFilePickerModal.h**:
- Updated `RefreshFileList()` documentation to explain Phase 46 compliance
- Added `RefreshFileListInternal(bool bLog)` declaration with detailed explanation
- Both methods are private (no API change)

## Verification

**Build Status**: ✅ 0 errors, 0 warnings

**What This Fixes**:
1. ✅ Eliminates 60+ FPS logging spam from SaveFilePickerModal
2. ✅ Restores visibility of Phase 46-47 diagnostic logs (no longer drowned out)
3. ✅ Complies with Logging Discipline rule
4. ✅ Separates UI rendering from file I/O processing
5. ✅ Maintains logging on important state changes (initialization, user navigation)

**Console Output Before Fix**:
```
[SaveFilePickerModal] Found 1 files and 3 folders in . (repeated 100+ times)
[CanvasToolbarRenderer::OnSaveClicked] ENTER  ← Gets lost in spam!
```

**Console Output After Fix**:
```
[SaveFilePickerModal] Found 1 files and 3 folders in .  ← Logged once on Open()
[CanvasToolbarRenderer::OnSaveClicked] ENTER  ← Now visible!
[CanvasToolbarRenderer::OnSaveClicked] EXIT
```

## Related Phases

- **Phase 45**: Fixed ImGui modal frame ordering (modals must render after content)
- **Phase 46**: Established Logging Discipline (no logs in render loops or 60+ FPS methods)
- **Phase 47**: Added save-flow diagnostic logging
- **Phase 48** (This): Fixed logging spam in SaveFilePickerModal to comply with Phase 46

## Architecture Pattern Applied

**Two-Layer UI/Processing Separation** (Phase 44.4):
```
Layer 1 - UI Presentation (Render)
  └─ Render folder list, display files, handle ImGui UI state
  
Layer 2 - Processing (with conditional logging)
  └─ RefreshFileListInternal(bool bLog)
  └─ File scanning, directory enumeration
  └─ Logging ONLY when bLog=true (state changes, not per-frame)
```

**Key Principle**: UI layer handles interaction, processing layer handles work. Logging follows processing layer's execution frequency (not UI frequency).

## Lesson Learned

**Frame-Rate Consciousness**: Any logging in methods called 60+ times per second will cause massive spam and drown out diagnostic logs. The "60 FPS rule" is critical for debug visibility.

Current pattern:
- ✅ Initialization (called once): Log freely
- ✅ User actions (called on demand): Log freely  
- ❌ Render loops (called 60+ FPS): NEVER log (or make logging conditional)

## Status

✅ **Phase 48 COMPLETE**
- Problem identified and fixed
- Logging Discipline now enforced
- Frame spam eliminated
- Build successful
- Architecture pattern verified
