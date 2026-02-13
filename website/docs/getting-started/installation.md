---
id: installation
title: Installation Guide
sidebar_label: Installation
---

# Installation Guide

This guide will help you set up Olympe Engine on your development machine.

## Prerequisites

Before you begin, ensure you have the following installed:

### Required Tools

- **C++ Compiler**: C++14 compatible compiler
  - **Linux**: GCC 7+ or Clang 5+
  - **Windows**: MSVC 2017+ or MinGW-w64
  - **macOS**: Xcode Command Line Tools (Clang)
- **CMake**: Version 3.14 or higher
- **Git**: For cloning the repository
- **SDL3**: Simple DirectMedia Layer 3 (see below)

### Installing SDL3

SDL3 is required for rendering, input handling, and audio.

#### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake git

# Clone and build SDL3
git clone https://github.com/libsdl-org/SDL.git
cd SDL
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

#### Windows

1. Download SDL3 pre-built binaries from [libsdl.org](https://www.libsdl.org/)
2. Extract to a known location (e.g., `C:\SDL3`)
3. Add SDL3 to your system PATH or set `SDL3_DIR` environment variable

#### macOS

```bash
# Using Homebrew
brew install sdl3

# Or build from source
git clone https://github.com/libsdl-org/SDL.git
cd SDL
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

## Cloning the Repository

Clone the Olympe Engine repository from GitHub:

```bash
git clone https://github.com/Atlasbruce/Olympe-Engine.git
cd Olympe-Engine
```

## Building with CMake

Olympe Engine uses CMake as its build system.

### Basic Build

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build .
```

### Build Options

You can customize the build with these options:

```bash
# Build only the runtime engine
cmake -DBUILD_RUNTIME_ENGINE=ON -DBUILD_BLUEPRINT_EDITOR_STANDALONE=OFF ..

# Build only the blueprint editor
cmake -DBUILD_RUNTIME_ENGINE=OFF -DBUILD_BLUEPRINT_EDITOR_STANDALONE=ON ..

# Build both (default)
cmake -DBUILD_RUNTIME_ENGINE=ON -DBUILD_BLUEPRINT_EDITOR_STANDALONE=ON ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### Build Targets

Olympe Engine produces two executables:

1. **OlympeEngine**: The main runtime engine for playing games
2. **OlympeBlueprintEditor**: Standalone editor for creating/editing entity blueprints

## Running the Engine

After building, you can run the engine:

```bash
# From the build directory
./OlympeEngine

# Or from the project root (if binaries are in build/)
./build/OlympeEngine
```

### Required Files

Make sure these directories are accessible from your working directory:

- `Blueprints/` - Entity blueprint definitions
- `Gamedata/` - Game resources (maps, sprites, audio)
- `Resources/` - Engine resources
- `Config/` - Configuration files

## Verifying the Installation

To verify your installation is working correctly:

1. **Run the engine**:
   ```bash
   ./OlympeEngine
   ```

2. **Check for errors**: The console should show initialization messages without errors

3. **Load a test map**:
   ```cpp
   // In your code
   World::Get().LoadLevel("Gamedata/Levels/test_map.tmj");
   ```

4. **Test the Blueprint Editor**:
   ```bash
   ./OlympeBlueprintEditor
   ```

## Common Issues

### SDL3 Not Found

**Error**: `Could not find SDL3`

**Solution**:
```bash
# Set SDL3_DIR environment variable
export SDL3_DIR=/path/to/SDL3/install

# Or specify in CMake command
cmake -DSDL3_DIR=/path/to/SDL3/install ..
```

### Compiler Version Too Old

**Error**: `C++14 features not supported`

**Solution**:
- Update your compiler to GCC 7+, Clang 5+, or MSVC 2017+
- On Ubuntu: `sudo apt-get install gcc-7 g++-7`

### Missing Dependencies

**Error**: Build fails with missing headers

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libsdl3-dev libsdl3-image-dev

# Make sure all submodules are initialized
git submodule update --init --recursive
```

### CMake Version Too Old

**Error**: `CMake 3.14 or higher is required`

**Solution**:
```bash
# Ubuntu/Debian - install from Kitware repository
sudo apt-get install software-properties-common
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install cmake

# Or download from cmake.org
```

### Windows DLL Not Found

**Error**: `SDL3.dll not found` when running

**Solution**:
- Copy `SDL3.dll` to the same directory as `OlympeEngine.exe`
- Or add SDL3's bin directory to your system PATH
- The CMake build should do this automatically on Windows

## IDE Setup

### Visual Studio Code

1. Install C++ extension
2. Install CMake Tools extension
3. Open the project folder
4. Select a kit (compiler)
5. Press F7 to build or Ctrl+F5 to run

### Visual Studio

1. Open the project folder (not the .sln file)
2. Visual Studio will detect CMakeLists.txt
3. Select build configuration
4. Build → Build All
5. Debug → Start Without Debugging

### CLion

1. Open the project folder
2. CLion will automatically load CMake configuration
3. Select build configuration
4. Build → Build Project
5. Run → Run 'OlympeEngine'

## Project Structure

After building, your directory structure should look like:

```
Olympe-Engine/
├── build/                  # Build output (created by you)
│   ├── OlympeEngine       # Main executable
│   └── OlympeBlueprintEditor
├── Blueprints/            # Entity blueprints
├── Config/                # Configuration files
├── Documentation/         # Additional documentation
├── Docs/                  # Technical guides
├── Gamedata/              # Game content
│   └── Levels/           # Tiled maps
├── Resources/             # Engine resources
├── Source/                # C++ source code
│   ├── AI/               # AI and behavior trees
│   ├── BlueprintEditor/  # Blueprint editor
│   ├── TiledLevelLoader/ # Tiled map loader
│   └── ...
├── CMakeLists.txt         # CMake build configuration
└── website/               # Documentation site (this site!)
```

## Next Steps

Now that you have Olympe Engine installed:

1. [Quick Start Guide](./quick-start.md) - Create your first project
2. [Project Structure](./project-structure.md) - Understand the codebase
3. [ECS Overview](../technical-reference/architecture/ecs-overview.md) - Learn the architecture

## Updating Olympe Engine

To update to the latest version:

```bash
# Pull latest changes
git pull origin master

# Update submodules
git submodule update --recursive

# Rebuild
cd build
cmake ..
cmake --build .
```

## Need Help?

If you encounter issues not covered here:

1. Check the [Troubleshooting Guide](../technical-reference/troubleshooting.md)
2. Search [existing issues](https://github.com/Atlasbruce/Olympe-Engine/issues)
3. Ask in [Discussions](https://github.com/Atlasbruce/Olympe-Engine/discussions)
4. [Open a new issue](https://github.com/Atlasbruce/Olympe-Engine/issues/new)
