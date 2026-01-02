# Olympe Engine - Build Instructions

## Prerequisites

### System Requirements
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Git (for submodules)
- SDL3 library
- Make or CMake (Linux/Unix)
- Visual Studio 2017+ (Windows)

### Third-Party Dependencies

The following libraries are included as git submodules:

- **ImGui** - Immediate Mode GUI library for visual editors
- **ImNodes** - Node graph editor extension for ImGui
- **nlohmann/json** - JSON parser (included in source)

## Cloning the Repository

To clone the repository with all submodules:

```bash
git clone --recurse-submodules https://github.com/Atlasbruce/Olympe-Engine.git
cd Olympe-Engine
```

If you've already cloned the repository without submodules:

```bash
git submodule update --init --recursive
```

## Building on Linux/Unix

### OlympeBlueprintEditor

The Blueprint Editor is a standalone tool for editing entity blueprints:

```bash
cd OlympeBlueprintEditor
make
./OlympeBlueprintEditor
```

### Main Engine

```bash
# Build commands will be added here
# The main engine uses Visual Studio on Windows
```

### Testing ImGui/ImNodes Integration

A test program is provided to verify ImGui and ImNodes are properly integrated:

```bash
cd Examples
g++ -std=c++17 \
    -I../Source/third_party \
    -I../Source/third_party/imgui \
    -I../Source/third_party/imnodes \
    imgui_imnodes_test.cpp \
    ../Source/third_party/imgui/imgui.cpp \
    ../Source/third_party/imgui/imgui_demo.cpp \
    ../Source/third_party/imgui/imgui_draw.cpp \
    ../Source/third_party/imgui/imgui_tables.cpp \
    ../Source/third_party/imgui/imgui_widgets.cpp \
    ../Source/third_party/imnodes/imnodes.cpp \
    -o imgui_imnodes_test

./imgui_imnodes_test
```

Expected output:
```
=== Olympe Engine - ImGui & ImNodes Integration Test ===

[ImGui Test]
  Creating ImGui context...
  ✓ ImGui context created successfully!
  ✓ ImGui version: 1.92.6 WIP
  ✓ ImGuiIO initialized
  ✓ Font atlas built (512x128)
  ✓ ImGui frame rendered (simulated)

[ImNodes Test]
  Creating ImNodes context...
  ✓ ImNodes context created successfully!
  ✓ ImNodes node created (simulated)
  ✓ ImNodes context destroyed

[Cleanup]
  ✓ ImGui context destroyed

=== All Tests Passed! ===
```

## Building on Windows

### Visual Studio

1. Open `Olympe Engine.sln` in Visual Studio
2. Select your build configuration (Debug/Release)
3. Build the solution (Ctrl+Shift+B)

### OlympeBlueprintEditor on Windows

1. Open `OlympeBlueprintEditor/OlympeBlueprintEditor.sln` in Visual Studio
2. Build and run the project

## Include Paths for ImGui/ImNodes

When using ImGui and ImNodes in your code, use the following include paths:

```cpp
#include "third_party/imgui/imgui.h"
#include "third_party/imnodes/imnodes.h"
```

### Compiler Flags

Add these include directories to your compiler:
- `-I../Source/third_party`
- `-I../Source/third_party/imgui`
- `-I../Source/third_party/imnodes`

### Link the following source files:
- `imgui.cpp`
- `imgui_demo.cpp` (optional, for demo windows)
- `imgui_draw.cpp`
- `imgui_tables.cpp`
- `imgui_widgets.cpp`
- `imnodes.cpp`

And the appropriate backend files from `imgui/backends/` (e.g., `imgui_impl_sdl3.cpp`, `imgui_impl_sdlrenderer3.cpp`)

## Troubleshooting

### Submodule Issues

If you encounter issues with submodules:

```bash
# Update all submodules to latest commit
git submodule update --remote

# Reset submodules if corrupted
git submodule deinit -f .
git submodule update --init --recursive
```

### ImGui/ImNodes Not Found

Make sure you have initialized the submodules:

```bash
git submodule status
# Should show commits for Source/third_party/imgui and Source/third_party/imnodes
```

### Compilation Errors

- Ensure you're using a C++17 compatible compiler
- Check that all include paths are correctly set
- Verify SDL3 is properly installed and accessible

## Additional Resources

- [ImGui Documentation](https://github.com/ocornut/imgui)
- [ImNodes Documentation](https://github.com/Nelarius/imnodes)
- [SDL3 Documentation](https://wiki.libsdl.org/SDL3)
- [Olympe Engine Architecture](ARCHITECTURE.md)
- [Blueprint System Documentation](Blueprints/README.md)

---

**Olympe Engine V2 - 2025**
