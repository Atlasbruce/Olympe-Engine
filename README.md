# Olympe Engine

A powerful 2D game engine built with Entity Component System (ECS) architecture.

## 🎮 Features

- **ECS Architecture** - Efficient and flexible entity component system
- **Advanced AI** - Behavior trees, pathfinding, and navigation systems
- **Tiled Integration** - Full support for Tiled map editor (orthogonal, isometric, hexagonal)
- **Prefab System** - Blueprint-based entity creation for rapid development
- **Input Management** - Multi-device input handling
- **Camera System** - Advanced camera controls with zoom and pan
- **Collision & Navigation** - Built-in collision detection and pathfinding

## 📚 Documentation

- **[User Guide & Tutorials](https://atlasbruce.github.io/Olympe-Engine/)** - Complete documentation
- **[API Reference](https://atlasbruce.github.io/Olympe-Engine/api/html/)** - Auto-generated from code

### Building Documentation Locally

```bash
# Generate API docs with Doxygen
doxygen Doxyfile

# Build Docusaurus site
cd website
npm install
npm run build

# Serve locally
npm run serve
```
![Documentation](https://img.shields.io/badge/docs-online-blue?style=flat-square)
![C++14](https://img.shields.io/badge/C%2B%2B-14-00599C?style=flat-square&logo=cplusplus)
![SDL3](https://img.shields.io/badge/SDL-3-blue?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)

**A powerful 2D game engine with Entity Component System architecture**

Olympe Engine is a modern 2D game engine built with C++14, designed for creating complex games with advanced AI, navigation systems, and flexible entity management.

## ✨ Features

- 🎯 **Entity Component System (ECS)** - Clean, modular architecture for managing game entities
- 🎨 **Blueprint System** - JSON-based entity definitions for designers
- 🗺️ **Tiled Map Integration** - First-class support for Tiled MapEditor (orthogonal, isometric, hexagonal)
- 🤖 **AI & Behavior Trees** - Built-in AI system for complex NPC behaviors
- 🧭 **Navigation System** - Advanced pathfinding with collision detection
- 🌍 **Multi-Language Ready** - Internationalization support

## 📚 Documentation

**[View Full Documentation →](https://atlasbruce.github.io/Olympe-Engine/)**

The documentation includes:
- [Getting Started Guide](https://atlasbruce.github.io/Olympe-Engine/getting-started/installation) - Installation and quick start
- [User Guide](https://atlasbruce.github.io/Olympe-Engine/user-guide/tiled-editor/introduction) - Tiled Editor, Blueprints, Input System
- [Technical Reference](https://atlasbruce.github.io/Olympe-Engine/technical-reference/architecture/ecs-overview) - Deep dive into architecture
- [API Reference](https://atlasbruce.github.io/Olympe-Engine/api-reference/introduction) - Complete API documentation
- [Contributing Guide](https://atlasbruce.github.io/Olympe-Engine/contributing/testing-guide) - How to contribute

**Note:** Documentation is automatically built and deployed to GitHub Pages on every push to master.

## 🚀 Quick Start

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- SDL3 development libraries

### Building
- C++14 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14+
- SDL3

### Build

```bash
# Clone the repository
git clone https://github.com/Atlasbruce/Olympe-Engine.git
cd Olympe-Engine

# Build with CMake
mkdir build && cd build
cmake ..
make

# Run
./OlympeEngine
```

### Windows (Visual Studio)

Open `Olympe Engine.sln` in Visual Studio and build the solution.

## 📖 Quick Example

Create a simple entity using blueprints:

```json
{
  "components": [
    {
      "type": "Identity_data",
      "name": "Player"
    },
    {
      "type": "Transform_data",
      "position": [0, 0]
    },
    {
      "type": "Sprite_data",
      "texturePath": "player.png"
    }
  ]
}
```

Load it in code:

```cpp
EntityID player = PrefabFactory::Get().CreateEntityFromPrefabName("Player");
```

## 🗂️ Project Structure

- `Source/` - Engine source code
- `Blueprints/` - Entity blueprint definitions
- `Gamedata/` - Game assets and data
- `Docs/` - Documentation guides
- `Examples/` - Example projects
- `website/` - Docusaurus documentation site

## 🤝 Contributing

Contributions are welcome! Please read our [Code Style Guide](https://atlasbruce.github.io/Olympe-Engine/docs/contributing/code-style) before submitting pull requests.

### Documentation Standards

When contributing code, please add Doxygen comments:

```cpp
/**
 * @brief Brief description of the function
 * @param paramName Description of parameter
 * @return Description of return value
 */
ReturnType FunctionName(ParamType paramName);
```

See [Code Documentation Guide](Docs/CODE_DOCUMENTATION_GUIDE.md) for details.

## 📝 License

See the repository for license details.

## 🔗 Links

- [GitHub Repository](https://github.com/Atlasbruce/Olympe-Engine)
- [Documentation](https://atlasbruce.github.io/Olympe-Engine/)
- [API Reference](https://atlasbruce.github.io/Olympe-Engine/api/html/)

## 🛠️ Built With

- C++17
- SDL3
- ImGui
- Tiled Map Editor
- CMake
cmake --build .

# Run the engine
./OlympeEngine
```

### Your First Game

```cpp
#include "World.h"
#include "PrefabFactory.h"

int main() {
    // Load a Tiled map
    World::Get().LoadLevel("Gamedata/Levels/dungeon.tmj");
    
    // Create a player from blueprint
    EntityID player = PrefabFactory::Get().CreateFromBlueprint(
        "Blueprints/EntityPrefab/player.json"
    );
    
    // Game loop...
    return 0;
}
```

## 🏗️ Architecture

Olympe Engine follows a modular Entity Component System architecture:

- **Core Engine** - Game loop, world management, rendering
- **ECS Systems** - Rendering, AI, Physics, Camera, Input processing
- **Prefab Factory** - Blueprint-based entity creation
- **Tiled Loader** - TMJ/TMX map loading with automatic navigation
- **Input System** - Multi-device input with action mapping
- **Navigation** - Collision maps and pathfinding
- **Behavior Trees** - AI decision trees for NPCs

## 📖 System Requirements

- **Compiler**: C++14 compatible
- **CMake**: 3.14 or higher
- **SDL3**: Required for rendering and input
- **Operating Systems**: Windows, Linux, macOS

## 🤝 Contributing

Contributions are welcome! Please see our [Contributing Guide](https://atlasbruce.github.io/Olympe-Engine/contributing/testing-guide) for:

- Testing guidelines
- Adding ECS components
- Code style conventions
- Pull request process

## 📝 Documentation Development

The documentation is built using [Docusaurus](https://docusaurus.io/) and automatically deployed to GitHub Pages.

### Build Documentation Locally

```bash
cd website
npm install
npm start
```

This will start a local development server at `http://localhost:3000`.

### Build for Production

```bash
cd website
npm run build
```

## 🗂️ Project Structure

```
Olympe-Engine/
├── Source/              # C++ source code
│   ├── AI/             # Behavior trees and AI systems
│   ├── BlueprintEditor/# Blueprint editor tools
│   ├── TiledLevelLoader/# Tiled map loader
│   └── ...
├── Blueprints/          # Entity blueprint definitions
├── Gamedata/            # Game content (maps, sprites)
├── Resources/           # Engine resources
├── Config/              # Configuration files
├── Documentation/       # Additional documentation
├── website/             # Docusaurus documentation site
└── CMakeLists.txt       # CMake build configuration
```

## 📜 License

[View license details in the repository]

## 🔗 Links

- **Documentation**: https://atlasbruce.github.io/Olympe-Engine/
- **GitHub Repository**: https://github.com/Atlasbruce/Olympe-Engine
- **Issues**: https://github.com/Atlasbruce/Olympe-Engine/issues
- **Discussions**: https://github.com/Atlasbruce/Olympe-Engine/discussions

---

Built with ❤️ using C++14 and SDL3
