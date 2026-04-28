# SubGraph Modal Architecture Guide

## Overview

This document describes the architecture of SubGraph file selection modals across the Olympe Blueprint Editor ecosystem.

**Current Version**: Phase 44 (Unified Architecture)
**Status**: ✅ Consolidated to single implementation
**Last Updated**: 2026-04-09

---

## Architecture Diagram

### System-Level View

```
┌─────────────────────────────────────────────────────────────────┐
│                         Three Graph Editors                      │
├──────────────────┬──────────────────┬──────────────────────────┤
│                  │                  │                          │
│  VisualScript    │  BehaviorTree    │  EntityPrefab            │
│  Editor          │  Editor          │  Editor                  │
│                  │                  │                          │
└──────────────────┴──────────────────┴──────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│              Property Panels (All Editors)                       │
│         Click "Browse" button on SubGraph node property          │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                CanvasModalRenderer (Singleton)                   │
│           Centralized Modal Dispatcher (Phase 42)                │
│                                                                   │
│  OpenSubGraphFilePickerModal(path)                              │
│  RenderSubGraphFilePickerModal()  ← Called by TabManager        │
│  IsSubGraphModalConfirmed()                                      │
│  GetSelectedSubGraphFile()                                       │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│           FilePickerModal (Phase 44 - UNIFIED)                   │
│                                                                   │
│     Private Member: m_subGraphModal{FilePickerType::SubGraph}   │
│                                                                   │
│     Features:                                                    │
│     • Folder tree navigation (LEFT panel)                       │
│     • File list with scrolling (RIGHT panel)                    │
│     • Search/filter capability                                  │
│     • Parent directory (..) navigation                          │
│     • .ats file extension filtering                             │
└─────────────────────────────────────────────────────────────────┘
```

### Call Flow: Property Panel to Modal

```
User Action: Click "Browse" button on SubGraph node property
                                ↓
BTNodePropertyPanel::RenderSubGraphControls() [line 182]
    ↓
    CanvasModalRenderer::Get().OpenSubGraphFilePickerModal(directory)
                                ↓
    CanvasModalRenderer::OpenSubGraphFilePickerModal() [CanvasModalRenderer.cpp:43]
                                ↓
    m_subGraphModal.Open(initialPath)  ← FilePickerModal::Open()
                                ↓
TabManager::RenderTabBar() [line 691]
    ↓
    CanvasModalRenderer::Get().RenderSubGraphFilePickerModal()
                                ↓
    CanvasModalRenderer::RenderSubGraphFilePickerModal() [CanvasModalRenderer.cpp:52]
                                ↓
    m_subGraphModal.Render()  ← FilePickerModal::Render()
                                ↓
                    [Modal displays with folder tree UI]
                                ↓
User selects file + clicks "Select"
                                ↓
    m_subGraphModal.IsConfirmed() → true
    m_subGraphModal.GetSelectedFile() → returns path
                                ↓
CanvasModalRenderer::IsSubGraphModalConfirmed()
CanvasModalRenderer::GetSelectedSubGraphFile()
                                ↓
Property panel updates SubGraph path
```

---

## Component Details

### 1. CanvasModalRenderer (Coordinator)

**File**: `Source/BlueprintEditor/Framework/CanvasModalRenderer.h/cpp`

**Purpose**: 
- Single dispatcher for all SubGraph modal operations
- Prevents duplicate rendering
- Centralizes confirmation state management

**Key Members**:
```cpp
FilePickerModal m_subGraphModal{FilePickerType::SubGraph};
bool m_subGraphConfirmed;
std::string m_cachedSelection;
```

**Public Interface**:
```cpp
void OpenSubGraphFilePickerModal(const std::string& initialPath);
void RenderSubGraphFilePickerModal();
bool IsSubGraphModalConfirmed() const;
std::string GetSelectedSubGraphFile() const;
bool IsSubGraphModalOpen() const;
void CloseSubGraphModal();
```

**How It Works**:
1. Property panel calls `OpenSubGraphFilePickerModal(path)` → opens modal
2. TabManager calls `RenderSubGraphFilePickerModal()` each frame → renders modal
3. Property panel queries `IsSubGraphModalConfirmed()` → checks if user selected
4. Property panel calls `GetSelectedSubGraphFile()` → gets selected path
5. State is cached to prevent multiple readings of same selection

**Phase 44 Change**:
```cpp
// Before: SubGraphFilePickerModal m_subGraphModal;
// After:  FilePickerModal m_subGraphModal{FilePickerType::SubGraph};
```

### 2. FilePickerModal (Implementation)

**File**: `Source/Editor/Modals/FilePickerModal.h/cpp`

**Purpose**:
- General-purpose file picker with folder navigation
- Supports multiple file types via FilePickerType enum
- Reusable component for all file selection needs

**Supported FilePickerTypes**:
```cpp
enum class FilePickerType
{
    BehaviorTree,  // .bt.json in ./Gamedata
    SubGraph,      // .ats in Blueprints
    Audio,         // .ogg, .wav
    Tileset        // .tsj
};
```

**Key Members**:
```cpp
FilePickerType m_fileType;
std::vector<std::string> m_fileList;      // Files in current directory
std::vector<std::string> m_folderList;    // Subdirectories
std::string m_currentPath;                // Current browsing directory
int m_selectedIndex;                      // Currently selected file
```

**UI Layout** (for SubGraph):
```
┌─────────────────────────────────────────────────────┐
│ Select SubGraph File (.ats) to use as SubGraph      │
├─────────────────────────────────────────────────────┤
│ Path: [Blueprints/                            ]    │
│ Refresh                          Search: [    ]     │
├───────────────┬──────────────────────────────────┤
│ Folders:      │ Available Files:                 │
│ ..            │ ├─ boss_combat.ats               │
│ ├─ AI         │ ├─ guard_behavior.ats            │
│ ├─ Bosses     │ ├─ npc_simple.ats                │
│ └─ Common     │ └─ patrol_route.ats              │
├───────────────┴──────────────────────────────────┤
│ Selected: (none)                                   │
│                                                    │
│ [Select] [Cancel]                                │
└─────────────────────────────────────────────────────┘
```

**Folder Navigation**:
- Click ".." to go to parent directory
- Click any folder name to enter that directory
- Automatically refreshes file list when directory changes

**File Filtering**:
- Automatically filters by extension based on FilePickerType
- SubGraph type shows only .ats files
- Search filter applies case-insensitive substring matching

**Public Interface**:
```cpp
explicit FilePickerModal(FilePickerType fileType);
void Open(const std::string& currentPath = "");
void Close();
void Render();
bool IsOpen() const;
bool IsConfirmed() const;
const std::string& GetSelectedFile() const;
FilePickerType GetFileType() const;
```

### 3. Calling Code (Property Panels)

#### BTNodePropertyPanel
**File**: `Source/BlueprintEditor/BTNodePropertyPanel.cpp`
**Method**: `RenderSubGraphControls()` [line 158-200]

```cpp
if (ImGui::Button("Browse...##subgraphPath", ImVec2(-1, 0)))
{
    // Extract directory from current path
    std::string directory = "Blueprints/";
    if (!currentPath.empty())
    {
        // Parse path...
    }
    CanvasModalRenderer::Get().OpenSubGraphFilePickerModal(directory);
}

// Handle result
if (CanvasModalRenderer::Get().IsSubGraphModalConfirmed())
{
    std::string selectedFile = CanvasModalRenderer::Get().GetSelectedSubGraphFile();
    node->parameters["subgraphPath"] = selectedFile;
    // Update UI...
}
```

#### VisualScriptEditorPanel
**File**: `Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp`
**Method**: `RenderSubGraphNodeProperties()` [line 720-800]

```cpp
if (ImGui::Button("Browse##subgraph_browse", ImVec2(75, 0)))
{
    CanvasModalRenderer::Get().OpenSubGraphFilePickerModal("Blueprints/");
}

// Handle result
if (CanvasModalRenderer::Get().IsSubGraphModalConfirmed())
{
    std::string selectedFile = CanvasModalRenderer::Get().GetSelectedSubGraphFile();
    
    // Extract relative path (remove "Blueprints/" prefix)
    std::string relativePath = selectedFile;
    if (relativePath.find("Blueprints/") == 0)
        relativePath = relativePath.substr(11);
    
    nodePtr->SubGraphPath = relativePath;
}
```

#### EntityPrefabRenderer
**File**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp`
**Method**: Property panel modal handling (via CanvasModalRenderer)

Automatically uses same CanvasModalRenderer pattern for SubGraph nodes.

---

## Data Flow: Selection to Update

### Step 1: User Selects File

```
Modal displays:
  └─ User clicks "Blueprints/AI/boss_combat.ats"
     └─ m_selectedIndex = 0
     └─ User clicks "Select" button
        └─ m_confirmed = true
        └─ m_selectedFile = "Blueprints/AI/boss_combat.ats"
```

### Step 2: Confirmation Propagation

```
CanvasModalRenderer::RenderSubGraphFilePickerModal() detects confirmation:
  └─ m_subGraphModal.IsConfirmed() → true
     └─ CanvasModalRenderer caches: m_subGraphConfirmed = true
     └─ CanvasModalRenderer caches: m_cachedSelection = "Blueprints/AI/boss_combat.ats"
```

### Step 3: Property Panel Reads Selection

```
Property panel checks: CanvasModalRenderer::Get().IsSubGraphModalConfirmed()
  └─ Returns: true
  └─ Property panel calls: CanvasModalRenderer::Get().GetSelectedSubGraphFile()
     └─ Returns: "Blueprints/AI/boss_combat.ats"
     └─ Property panel updates node property: subgraphPath = ...
```

### Step 4: Node Graph Updated

```
Property panel notifies graph:
  └─ BTNodePropertyPanel: Updates GraphNode->parameters["subgraphPath"]
  └─ VisualScriptEditorPanel: Updates node->SubGraphPath
  └─ EntityPrefabRenderer: Updates node->properties["subgraph_path"]
```

---

## File Organization

### Source Tree

```
Source/
├── BlueprintEditor/
│   ├── Framework/
│   │   ├── CanvasModalRenderer.h        ← Dispatcher
│   │   ├── CanvasModalRenderer.cpp      ← Dispatcher implementation
│   │   ├── CanvasToolbarRenderer.h      ← Toolbar buttons
│   │   └── CanvasToolbarRenderer.cpp
│   ├── BTNodePropertyPanel.h            ← Uses CanvasModalRenderer
│   ├── BTNodePropertyPanel.cpp
│   ├── VisualScriptEditorPanel_Properties.cpp  ← Uses CanvasModalRenderer
│   └── EntityPrefabEditor/
│       ├── EntityPrefabRenderer.h       ← Uses CanvasModalRenderer
│       └── EntityPrefabRenderer.cpp
├── Editor/
│   └── Modals/
│       ├── FilePickerModal.h            ← UNIFIED modal (Phase 44)
│       ├── FilePickerModal.cpp
│       ├── SubGraphFilePickerModal.h    ← DEPRECATED (not used)
│       └── SubGraphFilePickerModal.cpp  ← DEPRECATED (not used)
└── ...
```

### Include Dependencies

```
Property Panels (all three editors):
  ↓ include
CanvasModalRenderer.h
  ↓ include
FilePickerModal.h
  ↓ include
SubGraphFilePickerType enum
```

---

## Design Patterns

### 1. Singleton Pattern (CanvasModalRenderer)

**Purpose**: Ensure single rendering point per frame

```cpp
class CanvasModalRenderer {
public:
    static CanvasModalRenderer& Get()  // Singleton accessor
    {
        static CanvasModalRenderer instance;
        return instance;
    }
private:
    CanvasModalRenderer();  // Private constructor
    CanvasModalRenderer(const CanvasModalRenderer&) = delete;  // No copy
    CanvasModalRenderer& operator=(const CanvasModalRenderer&) = delete;  // No assign
};
```

**Benefits**:
- Global access without parameter passing
- Single state management
- Prevents duplicate modal rendering
- Fixes BehaviorTree duplicate UI zones (Phase 42)

### 2. Adapter Pattern (FilePickerType)

**Purpose**: Support multiple file types with single modal implementation

```cpp
enum class FilePickerType
{
    BehaviorTree,
    SubGraph,
    Audio,
    Tileset
};

class FilePickerModal
{
private:
    std::string GetDefaultDirectory() const
    {
        switch (m_fileType)
        {
            case FilePickerType::BehaviorTree: return "./Gamedata";
            case FilePickerType::SubGraph: return "Blueprints";
            // ...
        }
    }
};
```

**Benefits**:
- Reusable modal component
- Consistent UI across file types
- Easy to add new file types
- Single maintenance point

### 3. State Caching (CanvasModalRenderer)

**Purpose**: Prevent multiple readings of same modal result

```cpp
void CanvasModalRenderer::RenderSubGraphFilePickerModal()
{
    m_subGraphModal.Render();
    
    // Cache confirmation state (one-time)
    if (!m_subGraphConfirmed && m_subGraphModal.IsConfirmed())
    {
        m_subGraphConfirmed = true;
        m_cachedSelection = m_subGraphModal.GetSelectedFile();
    }
}

bool CanvasModalRenderer::IsSubGraphModalConfirmed() const
{
    return m_subGraphConfirmed;
}
```

**Benefits**:
- Single frame processing of confirmation
- Prevents duplicate updates
- Clear intent: "was just confirmed"

---

## Lifecycle: Modal Open → Close → Result

### Frame 1: User Clicks Browse

```
Property Panel:
  if (ImGui::Button("Browse")) {
      CanvasModalRenderer::Get().OpenSubGraphFilePickerModal("Blueprints/");
      // ↓ Sets m_subGraphModal.m_isOpen = true
  }

TabManager:
  CanvasModalRenderer::Get().RenderSubGraphFilePickerModal();
  // ↓ m_subGraphModal.Render() displays modal
```

### Frame N: User Navigates Folders

```
Modal displays:
  User clicks on "AI" folder
  ↓
  m_currentPath = "Blueprints/AI"
  ↓
  RefreshFileList() scans directory for .ats files
  ↓
  m_fileList updated with new files
  ↓
  Next frame: modal re-renders with new files visible
```

### Frame N+M: User Selects File

```
Modal displays "boss_combat.ats"
  User clicks file
  ↓
  m_selectedIndex = 0
  ↓
  User clicks "Select" button
  ↓
  m_confirmed = true
  m_selectedFile = "Blueprints/AI/boss_combat.ats"
  m_isOpen = false  (modal closes)
```

### Frame N+M+1: Result Processing

```
TabManager:
  CanvasModalRenderer::Get().RenderSubGraphFilePickerModal();
  // ↓ Detects m_subGraphModal.IsConfirmed() → true
  // ↓ Sets m_subGraphConfirmed = true (one-time cache)
  // ↓ Sets m_cachedSelection = "Blueprints/AI/boss_combat.ats"

Property Panel:
  if (CanvasModalRenderer::Get().IsSubGraphModalConfirmed()) {
      std::string path = CanvasModalRenderer::Get().GetSelectedSubGraphFile();
      // ↓ path = "Blueprints/AI/boss_combat.ats"
      node->subgraphPath = path;
      // ↓ Node updated
  }
```

---

## Configuration & Customization

### File Type Configuration

To add new file types to FilePickerModal:

1. **Add to enum** (FilePickerModal.h):
```cpp
enum class FilePickerType
{
    BehaviorTree,
    SubGraph,
    Audio,
    Tileset,
    MyNewType  // ← Add here
};
```

2. **Add defaults** (FilePickerModal.cpp):
```cpp
std::string FilePickerModal::GetDefaultDirectory() const
{
    switch (m_fileType)
    {
        // ... existing cases ...
        case FilePickerType::MyNewType:
            return "./Gamedata/MyNewType";
    }
}

std::string FilePickerModal::GetFilePattern() const
{
    switch (m_fileType)
    {
        // ... existing cases ...
        case FilePickerType::MyNewType:
            return ".mytype";
    }
}
```

### UI Customization

Modal UI can be customized by modifying FilePickerModal::Render():

```cpp
void FilePickerModal::Render()
{
    // Modify size constraints
    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 300), ImVec2(1200, 800));
    
    // Modify split panel width
    float folderWidth = 150.0f;  // ← Change folder panel width
    
    // Modify colors
    ImGui::TextColored(customColor, "Folders:");
}
```

---

## Performance Considerations

### Directory Scanning (O(n))

FilePickerModal::RefreshFileList():
```cpp
// Windows: FindFirstFile/FindNextFile API
HANDLE hFind = FindFirstFileA((path + "\\*.*").c_str(), &findData);
while (FindNextFileA(hFind, &findData)) {
    // O(n) where n = files in directory
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        m_folderList.push_back(...)
    else
        m_fileList.push_back(...)
}
```

**Performance Notes**:
- Refreshed only when directory changes
- Typically <10ms for small directories
- Can optimize with caching if needed

### String Matching (O(m*n))

FilePickerModal::GetFilteredFiles():
```cpp
for (const auto& filename : m_blueprintFiles) {
    if (filenameLower.find(searchLower) != std::string::npos)
        filtered.push_back(filename);
}
```

**Performance Notes**:
- Applied only to render frame
- Case conversion and search: O(m*n)
- Typically <1ms for small lists

### Caching Strategy

To optimize repeated selections from same directory:
- Implement directory cache in FilePickerModal
- Store last 5 opened directories
- Only refresh on explicit Refresh button click

---

## Testing Strategy

### Unit Tests

```cpp
// Test FilePickerModal with SubGraph type
TEST(FilePickerModal, SubGraphTypeInitialization) {
    FilePickerModal modal(FilePickerType::SubGraph);
    EXPECT_EQ(modal.GetFileType(), FilePickerType::SubGraph);
    EXPECT_EQ(modal.GetDefaultDirectory(), "Blueprints");
    EXPECT_EQ(modal.GetFilePattern(), ".ats");
}

// Test CanvasModalRenderer dispatcher
TEST(CanvasModalRenderer, OpenAndRenderModal) {
    CanvasModalRenderer& renderer = CanvasModalRenderer::Get();
    renderer.OpenSubGraphFilePickerModal("Blueprints/");
    EXPECT_TRUE(renderer.IsSubGraphModalOpen());
}
```

### Integration Tests

1. **VisualScript**: Open file, select SubGraph node, click Browse, select file, verify path updated
2. **BehaviorTree**: Open file, select BT_SubGraph node, click Browse, navigate folders, select file
3. **EntityPrefab**: Open file, select SubGraph node, click Browse, verify folder tree visible

### UI Tests

1. Folder navigation works in all contexts
2. File selection updates properties
3. Modal closes after selection
4. Search/filter works correctly
5. ".." button navigates to parent
6. Path input field updates when folders clicked

---

## Future Enhancements

### Planned Features

1. **Recent Files List**
   - Track last 10 opened SubGraphs
   - Quick access button for common paths

2. **Favorites/Bookmarks**
   - User can bookmark frequently used directories
   - Quick navigation buttons

3. **File Preview**
   - Show graph thumbnail on hover
   - Display file metadata

4. **Multi-File Selection**
   - Extend FilePickerModal to support Ctrl+Click selection
   - Useful for bulk operations

5. **Drag-Drop Support**
   - Drag files from file explorer into modal
   - Alternative selection method

### Performance Optimizations

1. **Directory Caching**
   - Cache directory contents
   - Reduce filesystem calls

2. **Async Loading**
   - Load large directories in background
   - Prevent UI freezing

3. **Lazy UI Rendering**
   - Virtualize file list
   - Render only visible items

---

## Troubleshooting

### Modal Doesn't Appear

**Check**:
1. Is TabManager calling `RenderSubGraphFilePickerModal()`? (line 691)
2. Is property panel calling `OpenSubGraphFilePickerModal()`?
3. Is CanvasModalRenderer singleton initialized?

### File Not Visible in Modal

**Check**:
1. Correct file extension? (.ats for SubGraph)
2. Correct directory? (Blueprints/ for SubGraph)
3. File permissions? (readable by process)
4. Search filter? (check if filter is hiding file)

### Selection Not Updating Node Property

**Check**:
1. Is `IsSubGraphModalConfirmed()` called after modal closes?
2. Is `GetSelectedSubGraphFile()` called while confirmed flag set?
3. Is property panel updating correct node?
4. Is node marked dirty for save?

---

## References

- **Phase 42**: CanvasModalRenderer unified modal dispatcher
- **Phase 40**: FilePickerModal general-purpose file browser
- **Phase 44**: Consolidation of SubGraphFilePickerModal → FilePickerModal
- **ImGui**: Modal dialog system (BeginPopupModal, IsConfirmed, etc.)

---

## Version History

| Phase | Changes |
|-------|---------|
| Phase 26 | SubGraphFilePickerModal created (simple UI) |
| Phase 40 | FilePickerModal created (full folder browser) |
| Phase 42 | CanvasModalRenderer unifies modal rendering |
| Phase 44 | Consolidation: CanvasModalRenderer uses FilePickerModal |

---

**Last Updated**: 2026-04-09
**Status**: ✅ Current (Phase 44)
**Compatibility**: C++14+
**Platform**: Windows + Unix (with platform-specific file APIs)
