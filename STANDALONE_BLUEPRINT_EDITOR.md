# Olympe Blueprint Editor Standalone

## Overview

The **Olympe Blueprint Editor Standalone** is a dedicated WYSIWYG blueprint editor executable that operates independently from the game runtime. It provides full CRUD (Create, Read, Update, Delete) operations on blueprint JSON files without requiring the game engine to be running.

## Two Editor Modes

### 1. Runtime Editor (Integrated)
- **Executable**: `OlympeEngine.exe`
- **Activation**: Press F2 in-game
- **Purpose**: Runtime debugging, visualization of behavior trees, and animation graphs during gameplay
- **Mode**: Read-mostly with limited editing capabilities
- **Use Cases**:
  - Debug entity behaviors during runtime
  - Visualize active behavior trees
  - Inspect runtime entity states
  - Test blueprints in-game

### 2. Blueprint Editor Standalone (NEW)
- **Executable**: `OlympeBlueprintEditor.exe`
- **Activation**: Always active on launch
- **Purpose**: Full blueprint creation, editing, and management (CRUD operations)
- **Mode**: Full editing capabilities
- **Use Cases**:
  - Create new blueprints from scratch
  - Edit existing blueprints
  - Organize blueprint library
  - Template management
  - Blueprint validation and migration

## Architecture

### Code Sharing (100%)

Both executables share **all** Blueprint Editor code from `Source/BlueprintEditor/`:
- `blueprinteditor.cpp/.h` - Backend business logic
- `BlueprintEditorGUI.cpp/.h` - Frontend ImGui interface
- `AssetBrowser.cpp/.h` - Asset management
- `NodeGraphManager.cpp/.h` - Node graph editing
- All editor panels and plugins

**No code duplication** - maintaining one codebase benefits both executables.

### Entry Points

1. **Runtime Entry**: `Source/OlympeEngine.cpp`
   ```cpp
   Olympe::BlueprintEditor::Get().Initialize();
   Olympe::BlueprintEditor::Get().InitializeRuntimeEditor();
   ```

2. **Standalone Entry**: `Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp`
   ```cpp
   Olympe::BlueprintEditor::Get().Initialize();
   Olympe::BlueprintEditor::Get().InitializeStandaloneEditor();
   ```

### Editor Context System

The `EditorContext` singleton manages capabilities based on mode:

```cpp
// Runtime mode (read-only)
EditorCapabilities::Runtime()
- canCreate: false
- canEdit: false
- canDelete: false
- canSave: false
- showEntityContext: true

// Standalone mode (full CRUD)
EditorCapabilities::Standalone()
- canCreate: true
- canEdit: true
- canDelete: true
- canSave: true
- showEntityContext: false
```

## Building

### Visual Studio

#### Build Both Executables
1. Open `Olympe Engine.sln`
2. Select desired configuration (Debug/Release)
3. Select platform (x64/Win32)
4. Build Solution (Ctrl+Shift+B)
   - Builds `OlympeEngine.exe` (Runtime)
   - Builds `OlympeBlueprintEditor.exe` (Standalone)

#### Build Standalone Only
1. Open `OlympeBlueprintEditor.sln`
2. Select desired configuration (Debug/Release)
3. Select platform (x64/Win32)
4. Build Solution

### CMake (Cross-Platform)

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build both executables
cmake --build build

# Build only runtime engine
cmake -B build -DBUILD_RUNTIME_ENGINE=ON -DBUILD_BLUEPRINT_EDITOR_STANDALONE=OFF
cmake --build build

# Build only standalone editor
cmake -B build -DBUILD_RUNTIME_ENGINE=OFF -DBUILD_BLUEPRINT_EDITOR_STANDALONE=ON
cmake --build build

# Install
cmake --install build --prefix install
```

## Running

### Standalone Blueprint Editor

**Windows**:
```
OlympeBlueprintEditor.exe
```

**Linux/Mac** (via CMake build):
```
./OlympeBlueprintEditor
```

### Controls
- **Ctrl+Q**: Quit application
- **ESC**: Quit application (if no unsaved changes)
- **Ctrl+N**: New blueprint
- **Ctrl+O**: Open blueprint
- **Ctrl+S**: Save blueprint
- **Ctrl+Shift+S**: Save blueprint as...

## Output Directories

### Visual Studio Build
```
bin/BlueprintEditorStandalone/
  ├── Debug_x64/
  │   ├── OlympeBlueprintEditor_d.exe
  │   ├── SDL3.dll
  │   └── Blueprints/
  └── Release_x64/
      ├── OlympeBlueprintEditor.exe
      ├── SDL3.dll
      └── Blueprints/
```

### CMake Build
```
build/
  ├── OlympeBlueprintEditor
  ├── OlympeEngine
  └── Blueprints/
```

## Technical Details

### Dependencies

**Core Dependencies**:
- SDL3 (windowing, rendering, input)
- ImGui (UI framework)
- ImNodes (node graph editing)
- nlohmann/json (JSON parsing)

**Shared Engine Components** (compiled but minimally used in standalone):
- GameEngine (singleton infrastructure)
- World/ECS (entity-component system)
- DataManager (resource loading)

### Preprocessor Definitions

```cpp
// Both executables
OLYMPE_BLUEPRINT_EDITOR_ENABLED

// Standalone only
OLYMPE_BLUEPRINT_EDITOR_STANDALONE_MODE

// Runtime only
OLYMPE_RUNTIME_MODE
```

### C++ Standard

Both executables use **C++14** standard for compatibility.

## File Formats

Both editors read and write the same blueprint JSON format defined in `BLUEPRINT_JSON_SCHEMA.md`.

**Example Blueprint**:
```json
{
  "name": "MyEntityBlueprint",
  "description": "Custom entity blueprint",
  "type": "EntityBlueprint",
  "version": "2.0",
  "components": [
    {
      "type": "Transform",
      "position": [0.0, 0.0],
      "rotation": 0.0,
      "scale": [1.0, 1.0]
    },
    {
      "type": "Sprite",
      "texture": "character.png"
    }
  ]
}
```

## Interoperability

Both editors:
- Read/write the same JSON format
- Share the same asset directory structure
- Use the same plugin system
- Maintain compatibility with all blueprint types:
  - EntityBlueprint
  - BehaviorTree
  - HFSM (Hierarchical Finite State Machine)

Changes made in the standalone editor are immediately available in the runtime editor and vice versa.

## Roadmap

### Completed
- [x] Standalone executable with dedicated entry point
- [x] Shared codebase (100% code reuse)
- [x] EditorContext capability system
- [x] Visual Studio and CMake build support
- [x] Independent SDL3 window and ImGui context

### Future Enhancements
- [ ] Real-time preview of blueprints
- [ ] Blueprint diffing and merging
- [ ] Multi-blueprint editing (tabs)
- [ ] Blueprint search and replace
- [ ] Performance profiling integration
- [ ] Cloud blueprint storage
- [ ] Collaborative editing

## Troubleshooting

### Missing SDL3.dll
Copy `SDL3.dll` from the `SDL/` directory to the executable directory.

### Blueprints Directory Not Found
The Blueprints directory should be in the same folder as the executable. CMake build automatically copies it during post-build.

### Unsaved Changes Warning
The standalone editor tracks modifications. Use Ctrl+S to save or confirm quit when prompted.

## Documentation

- [Architecture Overview](ARCHITECTURE.md)
- [Blueprint JSON Schema](BLUEPRINT_JSON_SCHEMA.md)
- [User Guide](BLUEPRINT_EDITOR_USER_GUIDE.md)
- [Plugin Development](BLUEPRINT_EDITOR_V2_ARCHITECTURE.txt)

## License

Same license as Olympe Engine V2.

## Credits

Developed by Nicolas Chereau (nchereau@gmail.com)
Standalone editor implementation: 2025
