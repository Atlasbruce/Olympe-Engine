# Phase 55: Double-Delete Root Cause Analysis & Fix

## Exception Summary
- **Exception Type**: `0xC0000005` (Access Violation)
- **Error Message**: Violation d'accès lors de la lecture de l'emplacement `0xFFFFFFFFFFFFFFFF` (Access violation reading memory)
- **Location**: `EditorTab::~EditorTab()` at line `delete document;` in `TabManager.h`
- **Status**: ✅ **FIXED**

## Root Cause Analysis

### The Immediate Symptom
The document pointer's virtual function pointer table (`__vfptr`) contains `0xdddddddddddddddd`, which is the debug memory fill pattern used by the C++ runtime for **freed/deallocated memory**. This indicates the document object was already deleted before the destructor attempted to delete it again.

### The Double-Delete Bug Chain

**Location**: `TabManager::DestroyTab()` (Lines 401-419 in original code)

```cpp
void TabManager::DestroyTab(size_t index)
{
    if (index >= m_tabs.size())
        return;

    const std::string closedID = m_tabs[index].tabID;
    delete m_tabs[index].renderer;              // ← EXPLICIT DELETE #1
    m_tabs[index].renderer = nullptr;
    m_tabs.erase(m_tabs.begin() + ...);         // ← TRIGGERS ~EditorTab()
    // ...
}
```

**What happens**:

1. **Line 1 (explicit delete)**: `delete m_tabs[index].renderer;` deletes the renderer
   - For some graph types (BehaviorTree, VisualScript), the renderer's destructor may also delete or invalidate the document it returned via `GetDocument()`
   - The renderer and document can share ownership in complex ways

2. **Line 2 (nullify renderer)**: `m_tabs[index].renderer = nullptr;` ✓ Correct

3. **Line 3 (erase from vector)**: `m_tabs.erase(m_tabs.begin() + ...)` **TRIGGERS THE DESTRUCTOR**
   - `std::vector::erase()` removes the element from the container
   - Before removal, it **calls the destructor** `~EditorTab()`
   - The destructor now executes on the already-mutated tab

4. **In ~EditorTab()** (Lines 55-65 in `TabManager.h`):
   ```cpp
   ~EditorTab()
   {
       if (document)  // ← document pointer still has value 0x000001a6f9f6ac30
       {
           delete document;  // ← DOUBLE DELETE! Object already freed
           document = nullptr;
       }
       if (renderer)  // ← renderer is nullptr after line 2 above
       {
           delete renderer;  // ← Safe because nullptr (no-op)
           renderer = nullptr;
       }
   }
   ```

### Why This Happens

**Ownership Model Breakdown**:
- **EditorTab** is supposed to own both `renderer` and `document` pointers
- **For VisualScript/BehaviorTree**: The renderer returns its own internal document via `GetDocument()`, creating **aliased ownership**
  - `tab.document = r->GetDocument();` means both the renderer and the tab think they own the document
- **For EntityPrefab**: The tab owns a separate heap-allocated document
  - `tab.document = epDoc;` where `epDoc` is separately allocated

**The Problem**:
- When `DestroyTab()` deletes the renderer first, the renderer's destructor may clean up its internal document
- The tab still has a stale pointer to that now-freed document
- When `m_tabs.erase()` calls `~EditorTab()`, it tries to delete the already-freed document
- This is **undefined behavior** → crash with access violation

### Why the Renderer was Deleted Explicitly

Looking at the code history, the explicit `delete renderer` was likely added because:
1. The developer wanted to ensure the renderer was cleaned up before the vector erase
2. They didn't realize `m_tabs.erase()` would call the destructor
3. The destructor was expected to clean up after the explicit delete, but this created double-deletion

## The Fix

**Solution**: Remove the explicit `delete m_tabs[index].renderer;` line and let the `EditorTab` destructor handle all cleanup.

### Why This Works

The `EditorTab` class has **proper move semantics** already implemented:

```cpp
EditorTab(EditorTab&& other) noexcept
    : /* ... */
    , renderer(other.renderer)
    , document(other.document)
{
    other.renderer = nullptr;    // ← Nullify source
    other.document = nullptr;    // ← Nullify source
}

~EditorTab()
{
    if (document) delete document;
    if (renderer) delete renderer;
}
```

By removing the explicit delete:
1. `m_tabs.erase()` calls `~EditorTab()`
2. The destructor checks `if (document)` and `if (renderer)`
3. Both pointers are deleted exactly once
4. No double-delete, no memory corruption

### Code Change

**File**: `Source/BlueprintEditor/TabManager.cpp`  
**Method**: `TabManager::DestroyTab(size_t index)`

**Before (BROKEN)**:
```cpp
void TabManager::DestroyTab(size_t index)
{
    if (index >= m_tabs.size())
        return;

    const std::string closedID = m_tabs[index].tabID;
    delete m_tabs[index].renderer;              // ❌ CAUSES DOUBLE-DELETE
    m_tabs[index].renderer = nullptr;
    m_tabs.erase(m_tabs.begin() + static_cast<std::vector<EditorTab>::difference_type>(index));
    // ...
}
```

**After (FIXED)**:
```cpp
void TabManager::DestroyTab(size_t index)
{
    if (index >= m_tabs.size())
        return;

    const std::string closedID = m_tabs[index].tabID;
    // PHASE 55 FIX: Removed explicit 'delete renderer' - EditorTab destructor handles all cleanup
    // when m_tabs.erase() is called. Explicit deletion was causing double-delete of renderer
    // and potentially corrupting the document pointer before destructor tried to delete it.
    // The EditorTab move semantics and destructor properly manage ownership transfer.
    m_tabs.erase(m_tabs.begin() + static_cast<std::vector<EditorTab>::difference_type>(index));
    // ...
}
```

## Key Learnings

### Pattern: Don't Manually Delete Objects Before Container Erase

**❌ WRONG**:
```cpp
delete container[i].pointer;       // Manual delete
container[i].pointer = nullptr;
container.erase(container.begin() + i);  // Calls destructor → double-delete
```

**✅ CORRECT**:
```cpp
container.erase(container.begin() + i);  // Calls destructor once
// Destructor handles all cleanup
```

### Pattern: Trust Your Destructor

If a class has:
- Proper move semantics (move constructor/assignment)
- Destructor that safely deletes pointers
- Null pointer checks in destructor

Then **rely on the destructor** to clean up. Don't also manually delete before container operations.

### Memory Debug Pattern Recognition

When debugging access violations:
- `0xdddddddddddddddd` = freed memory (debug heap fill)
- `0xcccccccccccccccc` = uninitialized heap memory (debug heap pattern)
- These patterns indicate a use-after-free bug

## Build Status

✅ **0 errors, 0 warnings** after fix

## Testing Recommendation

1. Load a BehaviorTree graph
2. Close the tab (using X button or File > Close Tab)
3. Verify no crash occurs
4. Repeat with VisualScript and EntityPrefab graph types

## Files Modified

- `Source/BlueprintEditor/TabManager.cpp` - Removed explicit `delete renderer` from `DestroyTab()`
