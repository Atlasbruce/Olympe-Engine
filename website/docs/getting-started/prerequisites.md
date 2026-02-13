---
id: prerequisites
title: Prerequisites
sidebar_label: Prerequisites
sidebar_position: 1
---

# Prerequisites

Before installing Olympe Engine, ensure your development environment meets the following requirements.

## System Requirements

### Minimum Hardware
- **CPU**: Dual-core processor (2 GHz or higher)
- **RAM**: 4 GB minimum, 8 GB recommended
- **Storage**: 500 MB for engine and tools
- **Graphics**: OpenGL 3.3 compatible GPU

### Recommended Hardware
- **CPU**: Quad-core processor (3 GHz or higher)
- **RAM**: 16 GB
- **Storage**: SSD with 2 GB free space
- **Graphics**: Dedicated GPU with OpenGL 4.5+ support

## Development Tools

### C++ Compiler
Olympe Engine requires a C++17 compatible compiler:

#### Linux
- **GCC** 7.0 or higher
- **Clang** 5.0 or higher

```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# Fedora
sudo dnf install gcc-c++ make
```

#### Windows
- **Visual Studio** 2017 or higher (Community Edition is free)
- **MinGW-w64** as an alternative

Download from: [Visual Studio Downloads](https://visualstudio.microsoft.com/downloads/)

#### macOS
- **Xcode Command Line Tools** (includes Clang)

```bash
xcode-select --install
```

### CMake

CMake 3.15 or higher is required for building the engine.

```bash
# Ubuntu/Debian
sudo apt-get install cmake

# Windows
# Download from https://cmake.org/download/

# macOS
brew install cmake
```

### Git

Git is needed for cloning the repository and version control.

```bash
# Ubuntu/Debian
sudo apt-get install git

# Windows
# Download from https://git-scm.com/download/win

# macOS
brew install git
```

## SDL3 Libraries

Olympe Engine uses SDL3 for cross-platform rendering, input, and audio.

### Building SDL3 from Source

SDL3 is still in development. Build from source for the latest features:

```bash
# Clone SDL3 repository
git clone https://github.com/libsdl-org/SDL
cd SDL
git checkout main

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build (Linux/macOS)
make -j$(nproc)
sudo make install

# Build (Windows)
cmake --build . --config Release
cmake --install . --prefix "C:/SDL3"
```

### SDL3_image

For image loading support:

```bash
# Clone SDL3_image
git clone https://github.com/libsdl-org/SDL_image
cd SDL_image
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)
sudo make install
```

## Optional Tools

### Tiled Map Editor

The Tiled Map Editor is used for creating game levels.

**Download:** [Tiled Map Editor](https://www.mapeditor.org/)

```bash
# Ubuntu/Debian
sudo apt-get install tiled

# macOS
brew install --cask tiled

# Windows
# Download installer from https://www.mapeditor.org/
```

### Doxygen (for API documentation)

If you plan to generate API documentation:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows
# Download from https://www.doxygen.nl/download.html
```

### Python 3.7+ (for scripts)

Some utility scripts require Python:

```bash
# Ubuntu/Debian
sudo apt-get install python3 python3-pip

# macOS
brew install python3

# Windows
# Download from https://www.python.org/downloads/
```

## Verification

Verify your tools are installed correctly:

```bash
# Check C++ compiler
g++ --version    # Linux/macOS
cl              # Windows (Visual Studio)

# Check CMake
cmake --version

# Check Git
git --version

# Check SDL3 (after installation)
pkg-config --modversion sdl3  # Linux/macOS
```

## Next Steps

Once all prerequisites are installed, proceed to the [Installation Guide](./installation.md) to build Olympe Engine.

## Troubleshooting

### SDL3 Not Found

If CMake cannot find SDL3:

1. Set `SDL3_DIR` environment variable:
   ```bash
   export SDL3_DIR=/usr/local/lib/cmake/SDL3
   ```

2. Or specify in CMake:
   ```bash
   cmake -DSDL3_DIR=/path/to/SDL3/lib/cmake/SDL3 ..
   ```

### Compiler Version Too Old

Update your compiler:

```bash
# Ubuntu - Add toolchain PPA for newer GCC
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-11 g++-11
```

### CMake Too Old

Download the latest CMake from [cmake.org](https://cmake.org/download/) or use a package manager.

## Platform-Specific Notes

### Linux
- Ensure development headers are installed (`-dev` packages)
- May need to add library paths to `LD_LIBRARY_PATH`

### Windows
- Use Visual Studio Developer Command Prompt for building
- Add SDL3 bin directory to `PATH` environment variable

### macOS
- May need to allow unsigned binaries in System Preferences
- Use `brew` for easier dependency management
