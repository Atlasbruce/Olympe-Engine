# Olympe Tilemap Editor

A standalone tilemap editor for creating and editing game levels for the Olympe Engine. Built with SDL3 and ImGui, this editor provides a user-friendly interface for designing 2D tile-based game worlds.

## Overview

The Olympe Tilemap Editor is a dedicated tool for creating, editing, and managing tilemaps used in Olympe Engine games. It features a comprehensive set of tools for tile placement, layer management, and tileset organization.

## Features

- **Intuitive UI**: ImGui-based interface with dockable panels
- **Multi-Layer Support**: Create and manage multiple layers with individual visibility and locking
- **Tileset Management**: Load, organize, and manage multiple tilesets
- **Drawing Tools**: Brush, eraser, fill, rectangle, eyedropper, and selection tools
- **Tile Groups**: Organize tiles into logical groups for easier access
- **Undo/Redo**: Full history support with configurable stack size
- **Grid Display**: Toggleable grid with snap-to-grid functionality
- **Level Metadata**: Store and edit level information (name, author, description, etc.)
- **JSON Export**: Save levels in a clean, readable JSON format
- **Cross-Platform**: Runs on Windows, macOS, and Linux

## Architecture

### Core Components

```
TilemapEditorApp (Main Application)
├── LevelManager      - Level loading, saving, and metadata
├── EditorState       - Current tool, selections, undo/redo
├── AssetManager      - Texture and asset caching
└── TilesetManager    - Tileset data and tile properties
```

### Class Overview

- **TilemapEditorApp**: Main application class that manages SDL3 window, ImGui integration, and main loop
- **LevelManager**: Handles level file I/O, validation, and metadata management
- **EditorState**: Maintains editor state including selected tools, tiles, layers, and command history
- **AssetManager**: Manages asset loading, caching, and hot-reload support
- **TilesetManager**: Handles tileset loading, tile properties, and auto-tiling rules

### UI Layout

The editor features a modern, dockable UI with the following panels:

- **Main Menu Bar**: File, Edit, View, Tools, Help menus
- **Toolbar**: Quick access to drawing tools
- **Tileset Panel**: Display and select tilesets
- **Tile Groups Panel**: Organize tiles into groups
- **Tile List Panel**: View and select individual tiles
- **Canvas**: Main editing area for tile placement
- **Layer Panel**: Manage layers (visibility, locking, reordering)
- **Properties Panel**: View and edit tile and level properties
- **Status Bar**: Display FPS, current tool, coordinates, etc.

## Dependencies

The Olympe Tilemap Editor requires the following libraries:

- **SDL3**: Window management, rendering, and input handling
- **ImGui**: Immediate mode GUI framework
- **nlohmann/json**: JSON parsing and serialization
- **fmt**: String formatting and logging (optional, falls back to iostream)

### Included Dependencies

The following dependencies are already included in the Olympe Engine repository:

- ImGui (in `Source/third_party/imgui/`)
- nlohmann/json (in `Source/third_party/nlohmann/`)

### External Dependencies

You need to install SDL3 on your system:

#### Windows

Download SDL3 development libraries from [libsdl.org](https://www.libsdl.org/) or install via vcpkg:

```bash
vcpkg install sdl3
```

#### macOS

Install via Homebrew:

```bash
brew install sdl3
```

#### Linux

Install via package manager:

```bash
# Ubuntu/Debian
sudo apt-get install libsdl3-dev

# Fedora
sudo dnf install SDL3-devel

# Arch Linux
sudo pacman -S sdl3
```

## Building

### Prerequisites

- C++14 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15 or higher
- SDL3 development libraries

### Build Instructions

#### Windows (Visual Studio)

```bash
# Navigate to repository root
cd Olympe-Engine

# Create build directory
mkdir build
cd build

# Generate Visual Studio project
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run
bin/Release/OlympeTilemapEditor.exe
```

#### Windows (MinGW)

```bash
cd Olympe-Engine
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
bin/OlympeTilemapEditor.exe
```

#### macOS

```bash
cd Olympe-Engine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
./bin/OlympeTilemapEditor
```

#### Linux

```bash
cd Olympe-Engine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./bin/OlympeTilemapEditor
```

### CMake Options

You can customize the build with these CMake options:

```bash
# Build in Debug mode
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Specify SDL3 location
cmake .. -DSDL3_DIR=/path/to/sdl3

# Build with verbose output
cmake --build . --verbose
```

## Usage

### Getting Started

1. **Launch the Editor**: Run the `OlympeTilemapEditor` executable
2. **Create a New Level**: File → New Level (or Ctrl+N)
3. **Load a Tileset**: Click "Load Tileset" in the Tileset panel
4. **Select a Tool**: Choose a tool from the toolbar (Brush, Eraser, Fill, etc.)
5. **Paint Tiles**: Click and drag on the canvas to place tiles
6. **Save Your Work**: File → Save Level (or Ctrl+S)

### Keyboard Shortcuts

- **Ctrl+N**: New Level
- **Ctrl+O**: Open Level
- **Ctrl+S**: Save Level
- **Ctrl+Z**: Undo
- **Ctrl+Y**: Redo
- **Ctrl+C**: Copy selection
- **Ctrl+V**: Paste
- **Space**: Pan camera (hold and drag)
- **Mouse Wheel**: Zoom in/out
- **B**: Brush tool
- **E**: Eraser tool
- **F**: Fill tool
- **R**: Rectangle tool
- **I**: Eyedropper tool
- **S**: Select tool
- **G**: Toggle grid
- **Alt+F4**: Exit

### File Formats

#### Level Format (.json)

Levels are saved in JSON format with the following structure:

```json
{
  "version": "1.0.0",
  "name": "Level 1",
  "width": 32,
  "height": 24,
  "tile_size": 32,
  "author": "Your Name",
  "description": "Level description",
  "created_date": "2024-01-13",
  "modified_date": "2024-01-13",
  "tilesets": [
    {
      "name": "tileset_name",
      "path": "path/to/tileset.json"
    }
  ],
  "layers": [
    {
      "name": "Ground",
      "visible": true,
      "locked": false,
      "opacity": 1.0,
      "z_index": 0,
      "tiles": [0, 1, 2, ...]
    }
  ]
}
```

#### Tileset Format (.json)

Tilesets are defined in JSON format:

```json
{
  "name": "Terrain",
  "texture": "path/to/texture.png",
  "tile_width": 32,
  "tile_height": 32,
  "spacing": 0,
  "margin": 0,
  "columns": 8,
  "rows": 8,
  "tile_count": 64,
  "tile_properties": {
    "0": {
      "name": "Grass",
      "category": "Ground",
      "is_collidable": false
    }
  }
}
```

## Project Structure

```
Source/OlympeTilemapEditor/
├── include/
│   ├── LevelManager.h       - Level file management
│   ├── EditorState.h        - Editor state management
│   ├── AssetManager.h       - Asset loading and caching
│   ├── TilesetManager.h     - Tileset management
│   └── TilemapEditorApp.h   - Main application class
├── src/
│   ├── LevelManager.cpp
│   ├── EditorState.cpp
│   ├── AssetManager.cpp
│   ├── TilesetManager.cpp
│   ├── TilemapEditorApp.cpp
│   └── main.cpp             - Entry point
├── CMakeLists.txt           - Build configuration
└── README.md                - This file
```

## Development

### Code Style

The project follows Olympe Engine coding standards:

- **Namespace**: `Olympe::Editor`
- **Classes**: PascalCase (e.g., `LevelManager`)
- **Functions/Variables**: snake_case (e.g., `load_level()`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_TILES`)
- **C++ Standard**: C++14

### Adding Features

1. **Header Files**: Add declarations to appropriate header in `include/`
2. **Implementation**: Add implementation in corresponding `.cpp` file in `src/`
3. **Testing**: Build and test your changes
4. **Documentation**: Update this README if needed

### Error Handling

- Use exceptions for critical errors
- Return error codes (bool/int) for recoverable errors
- Log errors to console using `std::cout` and `std::cerr`

## Troubleshooting

### Common Issues

**Issue**: SDL3 not found during build

**Solution**: Make sure SDL3 is installed and CMake can find it:

```bash
cmake .. -DSDL3_DIR=/path/to/sdl3/cmake
```

**Issue**: ImGui headers not found

**Solution**: The ImGui sources should be in `Source/third_party/imgui/`. Verify the repository is complete.

**Issue**: Application crashes on startup

**Solution**: Check console output for error messages. Verify SDL3 DLLs are in the executable directory (Windows).

**Issue**: Cannot load textures

**Solution**: Ensure SDL_image is installed (optional dependency) or use PNG/BMP formats supported by SDL3 natively.

## Contributing

Contributions are welcome! Please follow the Olympe Engine contribution guidelines.

## License

This project is part of the Olympe Engine and follows the same license.

## Credits

- **SDL3**: Simple DirectMedia Layer - [libsdl.org](https://www.libsdl.org/)
- **ImGui**: Dear ImGui - [github.com/ocornut/imgui](https://github.com/ocornut/imgui)
- **nlohmann/json**: JSON for Modern C++ - [github.com/nlohmann/json](https://github.com/nlohmann/json)

## Contact

For questions or support, please refer to the main Olympe Engine repository.
