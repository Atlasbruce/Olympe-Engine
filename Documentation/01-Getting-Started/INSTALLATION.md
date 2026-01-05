# 📦 Olympe Engine - Installation Guide

Complete installation and build instructions for Olympe Engine.

---

## Table of Contents

- [System Requirements](#system-requirements)
- [Windows Installation](#windows-installation)
- [Linux Installation](#linux-installation)
- [SDL3 Setup](#sdl3-setup)
- [Verifying Installation](#verifying-installation)
- [Troubleshooting](#troubleshooting)

---

## System Requirements

### Minimum Requirements

- **OS**: Windows 10/11 or Linux (Ubuntu 20.04+)
- **Compiler**: 
  - Windows: Visual Studio 2022 (Community Edition or higher)
  - Linux: g++ 9.0+ with C++17 support
- **RAM**: 4 GB minimum, 8 GB recommended
- **Storage**: 2 GB free space
- **Graphics**: OpenGL 3.3+ compatible GPU

### Required Libraries

- **SDL3** - Simple DirectMedia Layer 3
- **OpenGL 3.3+** - Graphics rendering
- **Standard C++17 libraries**

---

## Windows Installation

### Step 1: Install Visual Studio 2022

1. Download [Visual Studio 2022 Community](https://visualstudio.microsoft.com/)
2. During installation, select:
   - ✅ Desktop development with C++
   - ✅ C++ CMake tools for Windows
   - ✅ Windows 10/11 SDK

### Step 2: Install SDL3

The engine includes SDL3 in the `SDL/` directory. Follow these steps:

1. **Add SDL3 to your project** (already configured in the solution)
   - SDL3 headers are in `SDL/include/`
   - SDL3 libraries are in `SDL/lib/`

2. **Copy SDL3.dll to project directory**
   ```bash
   # The DLL should already be in the root directory
   # If not, copy from SDL/lib/x64/SDL3.dll
   ```

See [SDL Setup Guide](../07-Reference/SDL_SETUP.md) for detailed SDL3 configuration.

### Step 3: Clone the Repository

```bash
git clone https://github.com/Atlasbruce/Olympe-Engine.git
cd Olympe-Engine
```

### Step 4: Open and Build

1. **Open the solution file**
   ```bash
   start "Olympe Engine.sln"
   ```

2. **Select build configuration**
   - Debug (for development with debugging symbols)
   - Release (optimized for performance)

3. **Build the solution**
   - Press `Ctrl+Shift+B`
   - Or: Build → Build Solution

4. **Run the engine**
   - Press `F5` to run with debugger
   - Or: `Ctrl+F5` to run without debugger

---

## Linux Installation

### Step 1: Install Build Tools

```bash
# Update package lists
sudo apt-get update

# Install build essentials
sudo apt-get install -y build-essential cmake git

# Install C++17 compiler
sudo apt-get install -y g++-9 # or newer
```

### Step 2: Install SDL3

SDL3 must be built from source on Linux:

```bash
# Install SDL3 dependencies
sudo apt-get install -y \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libasound2-dev \
    libpulse-dev \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libxinerama-dev \
    libxxf86vm-dev \
    libxss-dev

# Clone SDL3
git clone https://github.com/libsdl-org/SDL.git -b SDL3
cd SDL

# Build and install
mkdir build
cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Step 3: Build Olympe Engine

```bash
# Navigate to project directory
cd /path/to/Olympe-Engine

# Build using make (if Makefile exists)
make

# Or build with cmake
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Step 4: Run the Engine

```bash
./OlympeEngine
```

---

## SDL3 Setup

SDL3 is required for Olympe Engine. The engine uses SDL3 for:

- Window management
- Input handling (keyboard, mouse, gamepad)
- Graphics context creation
- Audio playback
- File I/O

### Windows SDL3 Configuration

For detailed Windows SDL3 setup, see [SDL Setup Guide](../07-Reference/SDL_SETUP.md).

**Quick setup for Visual Studio:**

1. **Include Directories**
   - Add `$(ProjectDir)SDL\include` to VC++ Directories → Include Directories

2. **Library Directories**
   - Add `$(ProjectDir)SDL\lib\x64` to VC++ Directories → Library Directories

3. **Linker Input**
   - Add `SDL3.lib` to Linker → Input → Additional Dependencies

4. **Copy DLL**
   - Ensure `SDL3.dll` is in the same directory as the executable

### Linux SDL3 Configuration

SDL3 is typically installed system-wide:

```bash
# Verify SDL3 installation
pkg-config --modversion sdl3

# Check SDL3 libraries location
pkg-config --libs sdl3

# Check SDL3 headers location
pkg-config --cflags sdl3
```

---

## Verifying Installation

### Test the Build

1. **Run the engine**
   - The main window should appear
   - No error messages in console

2. **Test the Blueprint Editor**
   - Press `F2` to toggle the editor
   - The editor UI should appear

3. **Test input**
   - Move mouse in the window
   - Press keyboard keys
   - Connect a gamepad and test buttons

### Common Success Indicators

✅ Main window opens without errors  
✅ Console shows "Olympe Engine initialized"  
✅ Blueprint Editor opens with F2  
✅ Input responds to keyboard/mouse/gamepad  
✅ No missing DLL errors

---

## Troubleshooting

### Windows Issues

#### "SDL3.dll not found"

**Solution**: Copy SDL3.dll to the executable directory
```bash
copy SDL\lib\x64\SDL3.dll .
```

#### "Cannot open include file 'SDL3/SDL.h'"

**Solution**: Check Visual Studio project properties
- Verify Include Directories contains `SDL\include`
- Rebuild the project

#### "Unresolved external symbol SDL_*"

**Solution**: Link SDL3 library
- Verify Library Directories contains `SDL\lib\x64`
- Verify Additional Dependencies contains `SDL3.lib`
- Clean and rebuild

### Linux Issues

#### "SDL.h: No such file or directory"

**Solution**: Install SDL3 development files
```bash
sudo apt-get install libsdl3-dev
# Or build from source (see Step 2 above)
```

#### "error while loading shared libraries: libSDL3.so"

**Solution**: Update library cache
```bash
sudo ldconfig
```

#### Build errors with C++17 features

**Solution**: Ensure g++ version is 9.0 or newer
```bash
g++ --version
# If older, install newer version
sudo apt-get install g++-11
# Update alternatives
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
```

### General Issues

#### Black screen on startup

**Possible causes**:
- Graphics drivers outdated
- OpenGL version too old

**Solution**: Update graphics drivers to latest version

#### Crashes on startup

**Possible causes**:
- Missing DLLs/shared libraries
- Corrupted build

**Solution**: 
1. Clean build directory
2. Rebuild from scratch
3. Verify all dependencies installed

---

## Next Steps

After successful installation:

1. 🚀 **Try the Quick Start**: [Getting Started Guide](README.md)
2. 🎮 **Create your first entity**: [Input Quick Start](../03-Core-Systems/Input/INPUT_QUICK_START.md)
3. 🎨 **Use the Blueprint Editor**: [Blueprint Editor Guide](../05-Tools/BLUEPRINT_EDITOR.md)
4. 📚 **Explore the documentation**: [Documentation Hub](../README.md)

---

## Additional Resources

- **[SDL3 Documentation](https://wiki.libsdl.org/SDL3)** - Official SDL3 reference
- **[SDL Setup Guide](../07-Reference/SDL_SETUP.md)** - Detailed SDL configuration
- **[Troubleshooting Guide](../03-Core-Systems/Input/INPUT_TROUBLESHOOTING.md)** - Common problems and solutions

---

[← Back to Getting Started](README.md) | [Next: Architecture Overview →](../02-Architecture/README.md)
