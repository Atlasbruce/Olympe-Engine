---
id: installation
title: Installation
sidebar_label: Installation
sidebar_position: 2
---

# Installation

This guide will help you set up Olympe Engine on your development machine.

## Prerequisites

- **CMake** 3.15 or higher
- **C++17** compatible compiler (GCC, Clang, MSVC)
- **SDL3** development libraries
- **Git** for version control

## Building from Source

### Clone the Repository

```bash
git clone https://github.com/Atlasbruce/Olympe-Engine.git
cd Olympe-Engine
```

### Build with CMake

```bash
mkdir build
cd build
cmake ..
make
```

### Windows (Visual Studio)

Open `Olympe Engine.sln` in Visual Studio and build the solution.

## Verify Installation

Run the engine to verify it's working:

```bash
./OlympeEngine
```

## Next Steps

Now that you have Olympe Engine installed, continue to [Quick Start](quick-start) to create your first project.
