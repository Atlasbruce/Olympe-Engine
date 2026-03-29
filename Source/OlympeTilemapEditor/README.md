# Olympe Tilemap Editor

**Version:** 1.0.0 - Phase 1 (Foundation)  
**Author:** Atlasbruce  
**License:** Olympe Engine Project

## Overview

The Olympe Tilemap Editor is a standalone visual editor for creating and editing tilemap-based levels for the Olympe Engine. This tool provides a user-friendly interface for placing entities, painting tiles, and defining collision data.

### Phase 1 Features

- **Level Management**: Load, save, and create new levels in LevelDefinition JSON format
- **Entity Editing**: Place, move, delete, and configure entities with prefab references
- **Tile Painting**: Set tile IDs on a 2D grid
- **Collision Editing**: Define collision masks for tiles
- **Undo/Redo System**: Full command history with unlimited undo/redo (limited to 100 steps)
- **ImGui-based UI**: Modern, dockable interface with multiple panels
- **JSON Compatibility**: Fully compatible with Olympe Engine's existing JSON formats

## Architecture

The editor is built with the following core components:

### LevelManager
- Manages level data (entities, tiles, collisions)
- Handles JSON serialization/deserialization
- Provides CRUD operations for entities and tiles
- Location: `Source/OlympeTilemapEditor/include/LevelManager.h`

### EditorState
- Implements Command pattern for undo/redo
- Manages command history
- Provides concrete commands: PlaceEntity, MoveEntity, DeleteEntity, SetTile, SetCollision
- Location: `Source/OlympeTilemapEditor/include/EditorState.h`

### TilemapEditorApp
- Main application class
- ImGui UI rendering (menu bar, toolbar, viewport, entity list, properties, history)
- File dialogs for new/open/save operations
- Location: `Source/OlympeTilemapEditor/include/TilemapEditorApp.h`

## Build Instructions

⚠️ **IMPORTANT:** This project is a **standalone executable**. Do not build from `Olympe Engine.sln`!

**Quick Start:**
```powershell
# Open the dedicated solution
start OlympeTilemapEditor.sln

# In Visual Studio: Build → Build Solution (Ctrl+Shift+B)
```

For detailed build instructions and troubleshooting, see: **[BUILD_TILEMAP_EDITOR.md](../../../BUILD_TILEMAP_EDITOR.md)** in the repository root.

### Prerequisites

- **Visual Studio 2022** (or compatible)
- **Windows SDK 10.0**
- **SDL3** (already included in project)
- **ImGui** (already included in project)

### Building on Windows

1. Open `OlympeTilemapEditor.sln` in Visual Studio (the dedicated solution file, **NOT** "Olympe Engine.sln")
2. Select your desired configuration:
   - **Debug|x64** (recommended for development)
   - **Release|x64** (for production builds)
3. Build the solution: `Build ? Build Solution` or press `Ctrl+Shift+B`
4. The executable will be generated in:
   - Debug: `bin/TilemapEditor/Debug_x64/OlympeTilemapEditor_d.exe`
   - Release: `bin/TilemapEditor/Release_x64/OlympeTilemapEditor.exe`

### Building on Linux/macOS (Future Support)

CMake support is planned for future releases to enable cross-platform builds.

## Usage

### Starting the Editor

Run the executable:
```bash
# Debug build
./OlympeTilemapEditor_d.exe

# Release build
./OlympeTilemapEditor.exe
```

### Keyboard Shortcuts

- **Ctrl+N**: New Level
- **Ctrl+O**: Open Level
- **Ctrl+S**: Save Level
- **Ctrl+Shift+S**: Save Level As
- **Ctrl+Z**: Undo
- **Ctrl+Y**: Redo
- **Ctrl+Q**: Quit
- **ESC**: Cancel/Deselect

### Creating a New Level

1. Launch the editor
2. Press `Ctrl+N` or go to `File ? New Level`
3. Enter a level name
4. Click "Create"

### Opening an Existing Level

1. Press `Ctrl+O` or go to `File ? Open Level`
2. Enter the path to a LevelDefinition JSON file (e.g., `Levels/my_level.json`)
3. Click "Open"

### Saving a Level

- **Quick Save** (Ctrl+S): Saves to the current file path
- **Save As** (Ctrl+Shift+S): Prompts for a new file path

### Working with Entities

1. Click "Add Entity" in the Entity List panel
2. Select an entity from the list
3. Edit properties in the Properties panel:
   - Name
   - Position (X, Y)
4. Right-click an entity to delete it

### Undo/Redo

- View command history in the History panel
- Use Undo/Redo buttons or keyboard shortcuts
- History is limited to 100 commands by default

## JSON Format Compatibility

The editor maintains full compatibility with Olympe Engine's LevelDefinition format:

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
    "scrollOffset": {
      "x": 0,
      "y": 0
    }
  },
  "data": {
    "levelName": "MyLevel",
    "worldSize": {
      "x": 1024,
      "y": 768
    },
    "backgroundMusic": "",
    "ambientColor": "#000000",
    "entities": [
      {
        "id": "entity_1",
        "prefabPath": "Blueprints/Player.json",
        "name": "Player",
        "position": {
          "x": 100,
          "y": 100
        },
        "overrides": {}
      }
    ],
    "tileMap": [
      [0, 0, 0, ...],
      [0, 1, 0, ...],
      ...
    ],
    "collisionMap": [
      [0, 0, 0, ...],
      [0, 1, 0, ...],
      ...
    ]
  }
}
```

## Project Structure

```
Source/OlympeTilemapEditor/
??? include/
?   ??? LevelManager.h        # Level data management
?   ??? EditorState.h          # Command pattern & undo/redo
?   ??? TilemapEditorApp.h     # Main application
??? src/
?   ??? LevelManager.cpp
?   ??? EditorState.cpp
?   ??? TilemapEditorApp.cpp
?   ??? main.cpp               # Entry point (SDL3 callbacks)
??? README.md

OlympeTilemapEditor/
??? OlympeTilemapEditor.vcxproj
```

## Technical Details

### Dependencies

- **SDL3**: Windowing and rendering backend
- **ImGui**: Immediate mode GUI framework
- **nlohmann/json**: Custom minimal JSON library (already in project)

### Code Standards

- **Language**: C++14
- **Namespace**: `Olympe::Editor`
- **Naming Convention**:
  - Classes: PascalCase
  - Members: m_camelCase (private)
  - Functions: PascalCase
- **Memory Management**: Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- **Error Handling**: Console logging with `std::cout` / `std::cerr`

## Known Limitations (Phase 1)

- No visual rendering of tiles/entities in viewport (placeholder canvas only)
- File dialogs use text input (no native file browser integration)
- No tileset/texture loading
- No grid snapping or visual gizmos
- Entity prefabs are referenced by path only (no validation)

These features are planned for Phase 2 and beyond.

## Troubleshooting

### Build Errors

**Error**: Cannot find SDL3.lib
- **Solution**: Ensure SDL3 libraries are in `SDL/lib/` directory

**Error**: Cannot open include file 'SDL3/SDL.h'
- **Solution**: Verify SDL3 headers are in `SDL/include/SDL3/` directory

**Error**: C++14 features not available
- **Solution**: Check project settings ? C/C++ ? Language ? C++ Language Standard is set to `ISO C++14`

### Runtime Issues

**Editor crashes on startup**
- Check console output for SDL initialization errors
- Ensure SDL3.dll is in the same directory as the executable or in system PATH

**Cannot load level**
- Verify the JSON file path is correct
- Check JSON format matches schema_version 2
- Look for parsing errors in the console

**Undo/Redo not working**
- Ensure changes are made through the UI (not direct file editing)
- Check History panel to see if commands are being recorded

## Future Roadmap

### Phase 2: Visual Editing
- Tile rendering with tileset support
- Entity sprite rendering
- Grid overlay and snapping
- Visual gizmos for entity manipulation
- Camera pan and zoom controls

### Phase 3: Advanced Features
- Multi-layer tile support
- Collision shape editor
- Entity component inspector
- Blueprint hot-reloading
- Asset browser integration
- Prefab validation

### Phase 4: Polish
- Native file dialogs (Windows/Linux/macOS)
- Keyboard shortcuts customization
- UI themes and preferences
- Performance optimizations
- Automated testing

## Contributing

This is part of the Olympe Engine project. For contributions:

1. Follow the existing code style (see Technical Details)
2. Test changes on Windows x64
3. Ensure JSON compatibility with existing levels
4. Update README.md for new features

## Support

For issues, questions, or feature requests, please refer to the main Olympe Engine repository.

---

**Built with ?? for Olympe Engine**
