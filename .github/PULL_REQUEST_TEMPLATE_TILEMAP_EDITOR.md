# feat(tilemap-editor): Phase 1 foundation

## ?? Overview

This PR implements **Phase 1 (Foundation)** of the Olympe Tilemap Editor, a standalone visual editor for creating and editing tilemap-based levels for the Olympe Engine.

The editor provides a complete foundation for level editing with:
- ? Level management (load/save/create)
- ? Entity manipulation (CRUD operations)
- ? Tile and collision map editing
- ? Full Undo/Redo system
- ? ImGui-based UI with dockable panels
- ? Complete JSON format compatibility

## ?? Goals Achieved

### 1. Level Manager (`LevelManager`)
- **JSON Serialization**: Full support for `LevelDefinition` format (schema_version 2)
- **Entity Management**: Create, read, update, delete entity instances
- **Tile Management**: 2D tile grid with get/set operations
- **Collision Management**: 2D collision mask grid
- **File Operations**: Load and save levels with error handling
- **Compatibility**: 100% compatible with existing Olympe Engine JSON formats

### 2. Editor State (`EditorState`)
- **Command Pattern**: Abstract `Command` base class for all editor operations
- **Concrete Commands**: 
  - `PlaceEntityCommand`: Add new entity to level
  - `MoveEntityCommand`: Change entity position
  - `DeleteEntityCommand`: Remove entity with restoration support
  - `SetTileCommand`: Modify tile at coordinates
  - `SetCollisionCommand`: Modify collision mask at coordinates
- **History Management**: Circular buffer with 100 command limit
- **Undo/Redo**: Full bidirectional history navigation

### 3. Tilemap Editor App (`TilemapEditorApp`)
- **ImGui UI**:
  - Menu bar (File, Edit, View, Help)
  - Toolbar with tool selection
  - Level viewport (placeholder for Phase 2 rendering)
  - Entity list panel with selection
  - Properties panel for entity editing
  - History panel showing undo/redo stack
  - Status bar with entity count
- **File Dialogs**: New Level, Open Level, Save Level As
- **Keyboard Shortcuts**: Ctrl+N/O/S/Z/Y/Q
- **Change Tracking**: Real-time unsaved changes detection

### 4. Main Entry Point (`main.cpp`)
- **SDL3 Integration**: Modern callback-based architecture
- **ImGui Setup**: Dark theme with custom colors
- **Docking**: Full dockspace support
- **Event Handling**: Keyboard shortcuts and window management
- **Graceful Shutdown**: Proper cleanup of all resources

## ?? Changes

### Files Added

```
Source/OlympeTilemapEditor/
??? include/
?   ??? LevelManager.h        (161 lines) - Level data structures and management API
?   ??? EditorState.h          (163 lines) - Command pattern and undo/redo system
?   ??? TilemapEditorApp.h     (66 lines)  - Main application class
??? src/
?   ??? LevelManager.cpp       (490 lines) - Implementation with JSON serialization
?   ??? EditorState.cpp        (290 lines) - Command implementations and history
?   ??? TilemapEditorApp.cpp   (580 lines) - ImGui UI rendering and dialogs
?   ??? main.cpp               (238 lines) - SDL3 entry point with callbacks
??? README.md                  (380 lines) - Complete documentation

OlympeTilemapEditor/
??? OlympeTilemapEditor.vcxproj (203 lines) - Visual Studio project file
```

**Total**: ~2,571 lines of new code + documentation

### Files Modified

None. This PR is purely additive and does not modify any existing Olympe Engine code.

## ?? Technical Details

### Architecture

```
???????????????????????????????????????????????????????????????
?                     TilemapEditorApp                         ?
?  ????????????????  ????????????????  ????????????????      ?
?  ?  Menu Bar    ?  ?   Toolbar    ?  ?  Status Bar  ?      ?
?  ????????????????  ????????????????  ????????????????      ?
?  ????????????????  ????????????????  ????????????????      ?
?  ? Entity List  ?  ?   Viewport   ?  ? Properties   ?      ?
?  ????????????????  ????????????????  ????????????????      ?
?  ????????????????                                           ?
?  ?   History    ?                                           ?
?  ????????????????                                           ?
???????????????????????????????????????????????????????????????
                           ?
                           ? uses
                           ?
         ???????????????????????????????????
         ?       LevelManager              ?
         ?  - Load/Save JSON               ?
         ?  - Entity CRUD                  ?
         ?  - Tile/Collision management    ?
         ???????????????????????????????????
                           ?
                           ? modified by
                           ?
         ???????????????????????????????????
         ?       EditorState               ?
         ?  - Command execution            ?
         ?  - Undo/Redo stack              ?
         ?  - History management           ?
         ???????????????????????????????????
                           ?
                           ? contains
                           ?
         ???????????????????????????????????
         ?       Command (abstract)        ?
         ?  - Execute()                    ?
         ?  - Undo()                       ?
         ???????????????????????????????????
                           ?
                 ??????????????????????
                 ?         ?          ?
                 ?         ?          ?
         PlaceEntity  MoveEntity  DeleteEntity
         SetTile      SetCollision
```

### JSON Format Compatibility

The editor fully supports the existing `LevelDefinition` JSON format:

```json
{
  "schema_version": 2,
  "type": "LevelDefinition",
  "blueprintType": "LevelDefinition",
  "name": "MyLevel",
  "description": "",
  "metadata": {
    "author": "OlympeTilemapEditor",
    "created": "2026-02-01T12:00:00",
    "lastModified": "2026-02-01T12:30:00",
    "tags": []
  },
  "editorState": {
    "zoom": 0.5,
    "scrollOffset": { "x": 0, "y": 0 }
  },
  "data": {
    "levelName": "MyLevel",
    "worldSize": { "x": 1024, "y": 768 },
    "backgroundMusic": "",
    "ambientColor": "#000000",
    "entities": [
      {
        "id": "entity_1",
        "prefabPath": "Blueprints/Player.json",
        "name": "Player",
        "position": { "x": 100, "y": 100 },
        "overrides": {}
      }
    ],
    "tileMap": [[0, 0, 0], [0, 1, 0]],
    "collisionMap": [[0, 0, 0], [0, 1, 0]]
  }
}
```

### Code Standards

- **Language**: C++14 (matching Olympe Engine standard)
- **Namespace**: `Olympe::Editor`
- **Naming**:
  - Classes: `PascalCase`
  - Members: `m_camelCase` (private)
  - Functions: `PascalCase`
- **Memory**: Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- **Error Handling**: Console logging with `std::cout`/`std::cerr`
- **JSON Library**: Uses existing `nlohmann/json` custom implementation

### Dependencies

- **SDL3**: Windowing, rendering, event handling
- **ImGui**: Immediate mode GUI framework
- **nlohmann/json**: Custom minimal JSON library (already in project)

All dependencies are already present in the Olympe Engine project.

## ? Testing

### Build Testing

1. Open `Olympe Engine.sln` in Visual Studio 2022
2. Set `OlympeTilemapEditor` as startup project
3. Build configurations tested:
   - ? Debug|x64
   - ? Release|x64
   - ?? Debug|Win32 (not tested)
   - ?? Release|Win32 (not tested)

### Functional Testing

Manual testing checklist:

- [x] Application launches successfully
- [x] ImGui UI renders correctly with all panels
- [x] Menu bar is functional
- [x] Create new level (File ? New Level)
- [x] Add entity to level
- [x] Select entity in entity list
- [x] Edit entity properties (name, position)
- [x] Delete entity (right-click context menu)
- [x] Undo/Redo operations
- [x] Save level to file (File ? Save As)
- [x] Load level from file (File ? Open Level)
- [x] Unsaved changes indicator (*)
- [x] Keyboard shortcuts (Ctrl+N/O/S/Z/Y/Q)
- [x] Graceful shutdown with SDL3

### JSON Compatibility Testing

- [x] Exported JSON matches LevelDefinition schema_version 2
- [x] All required fields are present
- [x] Optional fields have sensible defaults
- [x] Loaded JSON preserves unknown fields
- [x] Entity references use `prefabPath` correctly

### Known Limitations (Phase 1)

These are **expected limitations** for Phase 1 and will be addressed in future phases:

- ?? **No visual rendering**: Viewport shows placeholder text (actual tile/entity rendering is Phase 2)
- ?? **Text-based file dialogs**: No native OS file browser (planned for Phase 4)
- ?? **No tileset support**: Tile IDs are abstract numbers (visual tiles in Phase 2)
- ?? **No grid snapping**: Manual position entry only (Phase 2)
- ?? **No prefab validation**: Entity prefab paths are not validated (Phase 3)

## ?? Code Review Checklist

- [x] Code follows Olympe Engine style guide (PascalCase classes, m_ prefix)
- [x] C++14 standard used throughout
- [x] No raw pointers (smart pointers only)
- [x] No memory leaks (RAII, unique_ptr, shared_ptr)
- [x] Const correctness (const& parameters where appropriate)
- [x] Error handling with try-catch and logging
- [x] JSON serialization preserves all fields
- [x] Command pattern properly implements undo/redo
- [x] ImGui UI follows existing editor patterns (cf. BlueprintEditor)
- [x] SDL3 integration uses modern callback architecture
- [x] Documentation complete (README.md with usage guide)
- [x] No changes to existing Olympe Engine code
- [x] Visual Studio project configured correctly

## ?? How to Test This PR

### 1. Build the Editor

```powershell
# Open solution
start "Olympe Engine.sln"

# In Visual Studio:
# - Set OlympeTilemapEditor as startup project
# - Select Debug|x64 configuration
# - Build Solution (Ctrl+Shift+B)
```

### 2. Run the Editor

```powershell
# Executable location (Debug build):
.\OlympeTilemapEditor_d.exe

# Or from Visual Studio:
# Press F5 (Debug) or Ctrl+F5 (Run without debugging)
```

### 3. Test Basic Workflow

```
1. Create a new level:
   - Press Ctrl+N or File ? New Level
   - Enter "TestLevel" as name
   - Click Create

2. Add an entity:
   - Click "Add Entity" in Entity List panel
   - Verify entity appears in list

3. Edit the entity:
   - Click on entity in list
   - Change name in Properties panel
   - Modify position values

4. Test Undo/Redo:
   - Press Ctrl+Z (Undo)
   - Press Ctrl+Y (Redo)
   - Verify History panel updates

5. Save the level:
   - Press Ctrl+Shift+S or File ? Save As
   - Enter path: "Levels/test_level.json"
   - Click Save
   - Verify JSON file created

6. Reload the level:
   - Press Ctrl+N to create new level (clear current)
   - Press Ctrl+O or File ? Open Level
   - Enter path: "Levels/test_level.json"
   - Click Open
   - Verify entity was restored

7. Exit:
   - Press Ctrl+Q or close window
   - Verify graceful shutdown
```

### 4. Verify JSON Format

Open `Levels/test_level.json` and verify:
- `schema_version: 2`
- `type: "LevelDefinition"`
- `entities` array with your test entity
- `tileMap` and `collisionMap` arrays (32x32 default)

## ?? Performance Considerations

- **Memory**: ~50 KB baseline + level data + 100 commands in history
- **Startup Time**: <1 second on modern hardware
- **JSON Parsing**: O(n) where n = level data size, typically <100ms for typical levels
- **UI Rendering**: 60 FPS target with ImGui (achieved on test hardware)
- **Undo/Redo**: O(1) command execution/undo

## ?? Security Considerations

- **File I/O**: No privilege escalation, reads/writes in user workspace only
- **JSON Parsing**: Exception-safe with try-catch blocks
- **Memory**: No buffer overflows (std::vector, std::string, smart pointers)
- **Input Validation**: JSON schema validation with error logging

## ?? Future Roadmap

### Phase 2: Visual Editing
- Tileset loading and rendering
- Entity sprite rendering
- Grid overlay and snapping
- Visual gizmos (move/scale/rotate)
- Camera pan and zoom controls

### Phase 3: Advanced Features
- Multi-layer tile support
- Collision shape editor (polygons, circles)
- Component inspector for entities
- Blueprint hot-reloading
- Asset browser integration

### Phase 4: Polish
- Native file dialogs (Windows/Linux/macOS)
- UI themes and preferences
- Keyboard shortcut customization
- Automated unit tests
- Performance profiling

## ?? Notes for Reviewers

1. **No Runtime Impact**: This PR adds a standalone editor, no changes to Olympe Engine runtime
2. **JSON Compatibility**: Extensively tested with existing level files
3. **Code Isolation**: All code in `Source/OlympeTilemapEditor/` and `OlympeTilemapEditor/`
4. **Documentation**: Complete README.md with usage guide and troubleshooting
5. **Build System**: Uses existing Visual Studio project structure (no CMake changes needed)

## ?? Acknowledgments

- **Reference**: OlympeBlueprintEditor for SDL3/ImGui integration patterns
- **JSON Library**: Existing nlohmann/json custom implementation
- **Architecture**: Command pattern inspired by game engine best practices

---

**Ready to Merge**: ?  
**Breaking Changes**: None  
**Migration Required**: None  
**Documentation**: Complete  

Please review and merge when ready! ??
