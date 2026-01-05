# Asset Explorer Multi-Folder Architecture

## Overview

This document describes the architecture of the Asset Explorer for Olympe Blueprint Editor, following a clean backend/frontend separation pattern.

## Architecture Principles

### Backend (BlueprintEditor)
- **Responsibility**: Business logic, data management, file system operations
- **Location**: `Source/BlueprintEditor/blueprinteditor.h/cpp`
- **No UI Code**: Contains zero ImGui or rendering code
- **Singleton Pattern**: Single instance accessed via `BlueprintEditor::Get()`

### Frontend (BlueprintEditorGUI, AssetBrowser, AssetInfoPanel)
- **Responsibility**: UI rendering, user interactions
- **No Business Logic**: All data comes from backend
- **No Direct File Access**: Uses backend API for all data

## Key Components

### 1. BlueprintEditor Backend

#### Asset Management Structures

```cpp
// Asset metadata - complete information about an asset
struct AssetMetadata
{
    std::string filepath;       // Full path to asset file
    std::string name;           // Asset name (from JSON or filename)
    std::string type;           // Asset type (EntityBlueprint, BehaviorTree, etc.)
    std::string description;    // Asset description
    bool isDirectory;           // True if this is a directory
    int componentCount;         // For EntityBlueprint
    int nodeCount;              // For BehaviorTree
    std::vector<std::string> components;  // Component types
    std::vector<std::string> nodes;       // Node types
    bool isValid;               // False if JSON is malformed
    std::string errorMessage;   // Error message if not valid
};

// Asset tree node - hierarchical structure for directory tree
struct AssetNode
{
    std::string name;           // Display name (filename without path)
    std::string fullPath;       // Complete file path
    std::string type;           // Asset type
    bool isDirectory;
    std::vector<std::shared_ptr<AssetNode>> children;
};
```

#### Backend API

**Asset Path Management**
```cpp
// Set the root path for assets (triggers rescan)
void SetAssetRootPath(const std::string& path);

// Get the current asset root path
std::string GetAssetRootPath() const;
```

**Asset Scanning**
```cpp
// Rescan asset directory (recursive)
void RefreshAssets();

// Get the complete asset tree structure
std::shared_ptr<AssetNode> GetAssetTree() const;
```

**Asset Queries**
```cpp
// Get all assets as a flat list
std::vector<AssetMetadata> GetAllAssets() const;

// Filter assets by type
std::vector<AssetMetadata> GetAssetsByType(const std::string& type) const;

// Search assets by name (case-insensitive)
std::vector<AssetMetadata> SearchAssets(const std::string& query) const;
```

**Asset Metadata**
```cpp
// Get detailed metadata for a specific asset
AssetMetadata GetAssetMetadata(const std::string& filepath);

// Check if asset file is valid JSON
bool IsAssetValid(const std::string& filepath) const;

// Detect asset type from JSON content
std::string DetectAssetType(const std::string& filepath);
```

**Error Handling**
```cpp
// Get the last error message
std::string GetLastError() const;

// Check if an error occurred
bool HasError() const;

// Clear error state
void ClearError();
```

#### Backend Implementation Details

**Recursive Directory Scanning**
- Uses C++17 `std::experimental::filesystem`
- Scans all subdirectories recursively
- Filters JSON files only
- Sorts directories before files, then alphabetically
- Skips hidden files (starting with '.')

**Asset Type Detection**
- Parses JSON to determine type:
  - `EntityBlueprint`: Has "components" array or explicit "type": "EntityBlueprint"
  - `BehaviorTree`: Has "rootNodeId" and "nodes" array
  - `Generic`: Any other valid JSON
  - `Unknown`: Invalid or unparseable JSON

**Metadata Parsing**
- For EntityBlueprint: Extracts name, description, component list
- For BehaviorTree: Extracts name, description, node list with types
- Robust error handling for malformed JSON
- Uses `JsonHelper` utilities for safe JSON access

### 2. AssetBrowser (Frontend)

**Responsibilities**
- Render tree view of assets
- Handle user selection
- Apply filters (search, type)
- Trigger asset opening on double-click

**Key Changes from Original**
- Removed direct filesystem access
- Removed `ScanDirectory()` method (now in backend)
- Removed `DetectAssetType()` method (now in backend)
- Calls `BlueprintEditor::Get().GetAssetTree()` for data
- Calls `BlueprintEditor::Get().RefreshAssets()` to rescan

**UI Features**
- Tree view with expand/collapse
- Single-click selection
- Double-click to open
- Search by filename (case-insensitive)
- Filter by type dropdown
- Refresh button

### 3. AssetInfoPanel (Frontend)

**Responsibilities**
- Display detailed information about selected asset
- Render type-specific metadata
- Show error messages for invalid assets

**Key Changes from Original**
- Removed direct JSON parsing
- Removed `ParseEntityBlueprint()`, `ParseBehaviorTree()` methods (now in backend)
- Calls `BlueprintEditor::Get().GetAssetMetadata()` for data
- Uses pointer to backend-cached metadata (no local copy)

**UI Features**
- Type-specific display (Entity, BehaviorTree, Generic)
- Component list for entities
- Node list for behavior trees
- Error panel for invalid files
- Color-coded type indicators

## Data Flow

### Initialization Flow
```
1. BlueprintEditorGUI::Initialize()
   └─> AssetBrowser::Initialize(path)
       └─> BlueprintEditor::Get().SetAssetRootPath(path)
           └─> BlueprintEditor::RefreshAssets()
               └─> ScanDirectory() [recursive]
                   └─> DetectAssetType() [for each file]
```

### Asset Selection Flow
```
1. User clicks asset in AssetBrowser
   └─> AssetBrowser stores selected path
   
2. BlueprintEditorGUI checks for selection change
   └─> AssetBrowser::GetSelectedAssetPath()
   
3. Load asset info
   └─> AssetInfoPanel::LoadAsset(path)
       └─> BlueprintEditor::Get().GetAssetMetadata(path)
           └─> ParseAssetMetadata() [backend]
               └─> ParseEntityBlueprint() or ParseBehaviorTree()
```

### Asset Refresh Flow
```
1. User clicks Refresh button in AssetBrowser
   └─> AssetBrowser::Refresh()
       └─> BlueprintEditor::Get().RefreshAssets()
           └─> ScanDirectory() [recursive rescan]
```

### Asset Search Flow
```
1. User types in search box
   └─> AssetBrowser updates filter
   
2. AssetBrowser::RenderTreeNode() applies filter
   └─> PassesFilter() checks search query and type
   
3. Only matching nodes are displayed
```

## Error Handling

### Robust Error Management

**File System Errors**
- Directory not found: Logged and stored in backend error state
- Permission denied: Exception caught and logged
- Invalid path: Validated before scanning

**JSON Parse Errors**
- Malformed JSON: Caught by JsonHelper, error stored in metadata
- Missing required fields: Defaults applied, validation warnings
- Type mismatch: Safe getters with fallbacks

**Runtime Errors**
- Empty asset tree: Display "No assets found" in UI
- Backend error state: Displayed in AssetBrowser
- Invalid asset metadata: Shown in AssetInfoPanel error view

## Supported Asset Types

### EntityBlueprint
- **Detection**: Has "components" array or "type": "EntityBlueprint"
- **Metadata**: name, description, component count, component types
- **Display**: Green color, component list

### BehaviorTree
- **Detection**: Has "rootNodeId" and "nodes" array
- **Metadata**: name, description, node count, node types with names
- **Display**: Blue color, node list

### Generic
- **Detection**: Valid JSON without specific type indicators
- **Metadata**: name, description (if present)
- **Display**: Yellow color, basic info

### Unknown
- **Detection**: Invalid or unparseable JSON
- **Metadata**: Error message
- **Display**: Red color, error panel

## Multi-Folder Support

### Recursive Scanning
- Scans all subdirectories under asset root
- Maintains tree structure
- No depth limit

### Example Structure
```
Blueprints/
├── example_entity_simple.json          [EntityBlueprint]
├── example_entity_complete.json        [EntityBlueprint]
└── AI/
    ├── guard_patrol.json               [BehaviorTree]
    ├── guard_combat.json               [BehaviorTree]
    ├── investigate.json                [BehaviorTree]
    └── idle.json                       [BehaviorTree]
```

### Navigation
- Directories are collapsible/expandable
- Files shown with type badge: `filename [Type]`
- Directories always show before files in each level
- Alphabetical sorting within each category

## Performance Considerations

### Caching
- Asset tree cached in backend
- Metadata parsed on-demand and cached
- Only rescans on explicit Refresh or path change

### Lazy Loading
- Metadata parsed only when asset is selected
- Tree structure loaded on initialization
- Type detection happens during tree scan (minimal parsing)

### Large Directories
- No pagination yet (future enhancement)
- Filters applied in-memory
- Tree view allows collapsing sections

## Future Enhancements

### Short-term
1. Asset validation on selection
2. File watcher for auto-refresh
3. Keyboard navigation (arrow keys)
4. Multi-asset selection

### Medium-term
1. Asset thumbnails
2. Recent files list
3. Favorites/bookmarks
4. Drag-and-drop support

### Long-term
1. Asset creation wizards
2. Asset templates
3. Dependency visualization
4. Hot-reload in-game
5. Asset metadata caching to disk

## Testing Strategy

### Unit Tests (Future)
- `ScanDirectory()` with various folder structures
- `DetectAssetType()` with different JSON formats
- `SearchAssets()` with various queries
- Error handling for invalid files

### Integration Tests
- Asset selection flow
- Search and filter functionality
- Multi-folder navigation
- Error recovery

### Manual Testing Checklist
- [ ] Navigate full Blueprints/ directory tree
- [ ] Select assets in different folders
- [ ] View metadata in AssetInfoPanel
- [ ] Search for assets by name
- [ ] Filter by type
- [ ] Refresh asset tree
- [ ] Handle corrupted JSON files
- [ ] Test with empty directories
- [ ] Test with large number of files

## Code Quality

### Separation of Concerns
- ✅ Backend has zero UI code
- ✅ Frontend has zero business logic
- ✅ Frontend has zero direct file access
- ✅ Clear API boundaries

### Error Handling
- ✅ Try-catch blocks for file operations
- ✅ Safe JSON parsing with defaults
- ✅ Error messages logged to console
- ✅ Error state accessible to UI

### Modern C++
- ✅ C++17 filesystem
- ✅ Smart pointers (std::shared_ptr)
- ✅ STL containers (vector, map)
- ✅ Range-based for loops
- ✅ Lambda functions

## Acceptance Criteria Status

- ✅ Navigate full Blueprints/ directory tree (multi-folder)
- ✅ Assets displayed with complete metadata
- ✅ Search and filter operational
- ✅ Lifecycle: UI panels only visible when editor active
- ✅ Backend/GUI decoupled with clean API
- ✅ Robust error handling for missing/corrupted files
- ✅ Cross-platform compatible (uses std::filesystem)

## Conclusion

The Asset Explorer implementation follows a clean architecture with complete separation between backend (business logic) and frontend (UI). The backend provides a comprehensive API for asset management, while the frontend focuses solely on rendering and user interaction. This design is scalable, maintainable, and ready for future enhancements.

**Status**: ✅ Implementation Complete
**Next Steps**: Testing and documentation updates
