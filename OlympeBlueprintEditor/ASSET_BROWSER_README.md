# Asset Browser - SDL3 Implementation

## Overview

The Asset Browser is a new feature integrated into the Olympe Blueprint Editor that provides a comprehensive file explorer for navigating and managing game assets.

## Features

### 1. **Recursive Directory Scanning**
- Automatically scans the `Blueprints/` directory and all subdirectories
- Displays assets in a tree structure for easy navigation
- Supports multiple asset types (Entities, Behavior Trees, Prefabs, etc.)

### 2. **Search and Filtering**
- **Search by name**: Type to filter assets by filename
- **Filter by type**: Dropdown to show only specific asset types
  - EntityBlueprint
  - BehaviorTree
  - Prefab
  - Trigger
  - FX
  - Sound
  - All (no filter)

### 3. **Asset Info Panel**
- Displays detailed metadata for selected assets
- **Entity Blueprints**: Shows component count and component types
- **Behavior Trees**: Shows node count and node structure
- **Generic Assets**: Basic information display

### 4. **Asset Loading**
- Single-click to select an asset and view its info
- Double-click to open/load an asset in the editor
- Seamless integration with existing blueprint loading system

### 5. **Error Handling**
- Clear error messages for corrupted or malformed JSON files
- Graceful handling of missing directories
- Visual indication of errors in the Asset Info panel

## SDL3 Compatibility

This implementation is **SDL3 only** and includes:
- Updated `main_gui.cpp` with SDL3 API calls
- SDL3 event handling (SDL_EVENT_* constants)
- SDL3 window and renderer creation
- No SDL2 dependencies

## Usage

### Navigation
1. Launch the editor
2. The Asset Browser panel appears on the left side
3. Click folders to expand/collapse them
4. Click files to select and view info
5. Double-click files to load them in the editor

### Search
1. Type in the search box to filter by filename
2. Use the Type Filter dropdown to show only specific asset types
3. Click "Refresh" to rescan the directory

### Asset Info
The Asset Info panel automatically updates when you select an asset, showing:
- Asset name and type
- Description (if available)
- Component/node counts
- Detailed lists of components or nodes (expandable)

## File Structure

```
OlympeBlueprintEditor/
├── include/
│   ├── AssetBrowser.h        - Asset tree navigation
│   ├── AssetInfoPanel.h      - Asset metadata display
│   └── BlueprintEditorGUI.h  - Updated with asset panels
├── src/
│   ├── AssetBrowser.cpp      - Implementation
│   ├── AssetInfoPanel.cpp    - Implementation
│   ├── BlueprintEditorGUI.cpp - Updated integration
│   └── main_gui.cpp          - SDL3 entry point
└── Makefile.gui              - Updated for SDL3
```

## Building

1. Setup dependencies (if not already done):
   ```bash
   cd OlympeBlueprintEditor
   ./setup_phase2.sh
   ```

2. Build the editor:
   ```bash
   make -f Makefile.gui
   ```

3. Run from project root:
   ```bash
   cd ..
   ./OlympeBlueprintEditor/OlympeBlueprintEditorGUI
   ```

## Supported Asset Types

The Asset Browser automatically detects:
- **EntityBlueprint**: JSON files with `"type": "EntityBlueprint"` or `"components"` array
- **BehaviorTree**: JSON files with `"rootNodeId"` and `"nodes"` array
- **Generic**: Any other valid JSON file

## Technical Details

### Asset Detection
Assets are identified by analyzing JSON structure:
1. Check for explicit `"type"` field
2. Check for Behavior Tree indicators (rootNodeId, nodes)
3. Check for Entity Blueprint indicators (components)
4. Default to "Generic" for other JSON files

### Performance
- Directory scanning is done once at startup
- Refresh can be triggered manually with the "Refresh" button
- Filtering is performed in-memory for fast response

### Cross-Platform
- Uses C++17 `<filesystem>` for directory operations
- Compatible with Linux and Windows
- SDL3 provides cross-platform window management

## Future Enhancements

Potential additions for future versions:
- Drag-and-drop asset placement
- Asset thumbnails/previews
- Asset creation wizards
- Recent files list
- Favorites/bookmarks
- Multi-asset selection
- Asset validation on selection

## Notes

- This is **SDL3 only** - no SDL2 code or dependencies
- ImGui and ImNodes libraries must be downloaded via `setup_phase2.sh`
- The Asset Browser integrates seamlessly with the existing blueprint editing workflow
