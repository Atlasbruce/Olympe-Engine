# BT Editor Save Functionality - Implementation Summary

## Overview
This document describes the implementation of save functionality for the Behavior Tree Editor (Phase 2), which allows users to save modified behavior trees to JSON files in the proper schema v2 format.

## Key Features Implemented

### 1. Dirty Flag Tracking
**Purpose:** Track unsaved modifications to warn users before data loss

**Implementation:**
- Added `m_IsDirty` boolean flag to `NodeGraph` class
- All modification methods mark the graph dirty:
  - `CreateNode()` - Creating new nodes
  - `DeleteNode()` - Removing nodes
  - `LinkNodes()` / `UnlinkNodes()` - Connecting/disconnecting nodes
  - `SetNodeParameter()` - Changing node parameters
  - Node position updates - When nodes are dragged in the editor
  - Node editing modal - When node properties are changed

**API:**
```cpp
bool IsDirty() const;      // Check if graph has unsaved changes
void MarkDirty();          // Mark graph as modified
void ClearDirty();         // Clear dirty flag (called after save/load)
```

### 2. Filepath Management
**Purpose:** Track where the graph should be saved

**Implementation:**
- Added `m_Filepath` string to `NodeGraph` class
- Set automatically on `LoadGraph()` and `SaveGraph()`
- Used to enable/disable Save button

**API:**
```cpp
const std::string& GetFilepath() const;
void SetFilepath(const std::string& filepath);
bool HasFilepath() const;  // Returns true if save location is known
```

### 3. Save Functionality

#### Save Button
- Located in NodeGraphPanel toolbar
- **Disabled** when no filepath is set (with tooltip explaining why)
- **Validates** graph before saving
- Shows **validation error** modal if graph is invalid
- Updates **lastModified** timestamp on save
- Clears dirty flag on successful save

#### Save As Button
- Opens popup with filepath input
- Automatically adds `.json` extension if missing
- **Validates** graph before saving
- Sets filepath and clears dirty flag on successful save

#### Validation
Graph validation checks:
- All child node references are valid (node IDs exist)
- Decorator child references are valid
- Returns error message describing the problem

**Error Handling:**
```
Cannot save: Graph validation failed!
Node 5 has invalid child 10
```

### 4. Schema v2 Format Compliance

The `NodeGraph::ToJson()` method outputs the complete schema v2 format:

```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "name": "TreeName",
  "description": "",
  "metadata": {
    "author": "User",
    "created": "",
    "lastModified": "2026-02-15T19:45:00",
    "tags": []
  },
  "editorState": {
    "zoom": 1.0,
    "scrollOffset": {
      "x": 0,
      "y": 0
    }
  },
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "Selector",
        "name": "Root Selector",
        "position": {
          "x": 200.0,
          "y": 100.0
        },
        "children": [2, 3],
        "parameters": {},
        "actionType": "",
        "conditionType": ""
      }
    ]
  }
}
```

**Key Points:**
- Full v2 wrapper with all required sections
- Nodes nested under `data` section
- Position as `{x, y}` object (not separate fields)
- Parameters as nested object (even if empty)
- Timestamp updated on each save

### 5. Unsaved Changes Protection

#### Tab Close Buttons
- Close button (X) enabled on all graph tabs
- Clicking X triggers confirmation if graph is dirty

#### Confirmation Dialog
Shown when closing a tab with unsaved changes:

**Options:**
1. **Save and Close** - Validates, saves (if filepath known), then closes
2. **Close Without Saving** - Discards changes and closes
3. **Cancel** - Cancels the close operation

**Special Cases:**
- If no filepath is set, "Save and Close" prompts for Save As
- Validation errors prevent saving but allow closing without saving

### 6. UI Indicators

#### Dirty Indicator (*)
- Shown in tab names: `"My Tree *"`
- Shown in toolbar next to Save buttons
- Orange color (`ImVec4(1.0f, 0.7f, 0.2f, 1.0f)`)
- Tooltip: "Unsaved changes"

#### Button States
- Save button grayed out when no filepath
- Tooltip on disabled Save: "No filepath set. Use 'Save As...' first."

#### Color Coding
- Errors: Red (`ImVec4(1.0f, 0.3f, 0.3f, 1.0f)`)
- Warnings: Orange (`ImVec4(1.0f, 0.7f, 0.2f, 1.0f)`)
- Success: Green (`ImVec4(0.5f, 1.0f, 0.5f, 1.0f)`)

## Code Structure

### Modified Files

1. **NodeGraphManager.h**
   - Added `IsDirty()`, `MarkDirty()`, `ClearDirty()` to NodeGraph
   - Added `GetFilepath()`, `SetFilepath()`, `HasFilepath()` to NodeGraph
   - Added `IsGraphDirty()`, `HasUnsavedChanges()` to NodeGraphManager

2. **NodeGraphManager.cpp**
   - Updated all modification methods to call `MarkDirty()`
   - Updated `ToJson()` to output schema v2 format
   - Updated `SaveGraph()` to update timestamp, set filepath, clear dirty
   - Updated `LoadGraph()` to set filepath, clear dirty
   - Added timestamp generation code

3. **NodeGraphPanel.cpp**
   - Added Save and Save As buttons to toolbar
   - Added dirty indicator display
   - Added validation error popup
   - Added confirmation dialog for closing unsaved graphs
   - Updated tab rendering to show dirty indicator
   - Added close button support to tabs
   - Updated node editing to mark dirty (removed auto-save)
   - Added position change detection to mark dirty

## Testing

### Validation Script
A Python validation script (`test_bt_save_format.py`) is provided to verify JSON format compliance:

```bash
python3 test_bt_save_format.py Blueprints/AI/guard_ai.json
```

**Validates:**
- Schema version is 2
- All required top-level fields present
- Metadata structure correct
- EditorState structure correct
- Data section with rootNodeId and nodes
- Node structure with position, parameters, etc.

### Manual Testing Checklist
- [ ] Create new BT, add nodes, save (should work)
- [ ] Load existing BT, modify, save (should work)
- [ ] Load BT, modify, close tab (should show confirmation)
- [ ] Save with invalid graph (should show error)
- [ ] Save button disabled without filepath (tooltip shows)
- [ ] Save As adds .json extension automatically
- [ ] Dirty indicator appears in tabs and toolbar
- [ ] Timestamp updates on each save
- [ ] Saved JSON matches schema v2 format
- [ ] Runtime loader can load saved files

## Compatibility

### Runtime Loader
The saved format is fully compatible with:
- `BehaviorTree.cpp` loader (lines 27-277)
- Auto-detects v1 vs v2 format
- Parses v2 schema correctly

### Existing Files
All existing BT files validated:
- `guard_ai.json` ✅
- `guard_patrol.json` ✅
- `zombie_ai.json` ✅
- `npc_wander_ai.json` ✅

## Future Enhancements

### Potential Improvements (from code review)

**High Priority:**
1. **File Dialog Integration** - Replace text input with native file picker
2. **Instance Member Variables** - Move static popup state (saveAsPopupOpen, confirmationOpen, graphToClose) to NodeGraphPanel members for better multi-instance support
3. **Reduce Code Duplication** - Refactor timestamp generation to declare std::tm once and only branch on system call

**Medium Priority:**
4. **Auto-save** - Optional periodic auto-save to temp file
5. **Undo/Redo Integration** - Clear dirty flag only on save, not undo/redo
6. **Creation Timestamp** - Track when graph was first created
7. **Author Configuration** - Allow user to set author name in preferences
8. **Multiple Save Locations** - Support "Export to..." for different paths
9. **Backup Files** - Create .bak files before overwriting
10. **Save All** - Button to save all dirty graphs at once

### Known Limitations
1. Save As popup uses simple text input (no file browser)
2. Author field hardcoded to "User"
3. Created timestamp not tracked (empty string)
4. No conflict resolution for concurrent edits
5. No file locking mechanism
6. Static variables for popup state (designed for single panel instance)

## Error Messages

### Validation Errors
```
Cannot save: Graph validation failed!
Node 5 has invalid child 10
```

### File Errors
```
[NodeGraphPanel] Failed to save graph!
```

### User Actions
All errors logged to console for debugging.

## API Reference

### NodeGraph
```cpp
// Dirty flag management
bool IsDirty() const;
void MarkDirty();
void ClearDirty();

// Filepath management
const std::string& GetFilepath() const;
void SetFilepath(const std::string& filepath);
bool HasFilepath() const;

// Validation
bool ValidateGraph(std::string& errorMsg) const;

// Serialization
nlohmann::json ToJson() const;
static NodeGraph FromJson(const nlohmann::json& j);
```

### NodeGraphManager
```cpp
// File operations
bool SaveGraph(int graphId, const std::string& filepath);
int LoadGraph(const std::string& filepath);

// State queries
bool IsGraphDirty(int graphId) const;
bool HasUnsavedChanges() const;  // True if any graph is dirty
```

## Conclusion

The BT Editor save functionality is now complete with:
- ✅ Full dirty flag tracking
- ✅ Save and Save As operations
- ✅ Graph validation before save
- ✅ Schema v2 format compliance
- ✅ Unsaved changes confirmation
- ✅ Clear UI indicators
- ✅ Compatible with runtime loader

The implementation follows best practices for editor software with proper validation, user warnings, and data integrity checks.
