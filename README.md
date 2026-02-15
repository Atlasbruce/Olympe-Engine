# Olympe Engine

![Documentation](https://img.shields.io/badge/docs-online-blue?style=flat-square)
![C++14](https://img.shields.io/badge/C%2B%2B-14-00599C?style=flat-square&logo=cplusplus)
![SDL3](https://img.shields.io/badge/SDL-3-blue?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)

**A powerful 2D game engine with Entity Component System architecture**

Olympe Engine is a modern 2D game engine built with C++14, designed for creating complex games with advanced AI, navigation systems, and flexible entity management.

## ✨ Features

- 🎯 **Entity Component System (ECS)** - Clean, modular architecture for managing game entities
- 🎨 **Blueprint System** - JSON-based entity definitions for designers
- 🎬 **2D Sprite Animation System** - Spritesheet support with FSM state machines and frame-by-frame control
- 🗺️ **Tiled Map Integration** - First-class support for Tiled MapEditor (orthogonal, isometric, hexagonal)
- 🤖 **AI & Behavior Trees** - Built-in AI system for complex NPC behaviors
- 🧭 **Navigation System** - Advanced pathfinding with collision detection
- 🌍 **Multi-Language Ready** - Internationalization support

## 📚 Documentation

**[View Full Documentation →](https://atlasbruce.github.io/Olympe-Engine/)**

The documentation includes:
- [Getting Started Guide](https://atlasbruce.github.io/Olympe-Engine/getting-started/installation) - Installation and quick start
- [User Guide](https://atlasbruce.github.io/Olympe-Engine/user-guide/tiled-editor/tiled-introduction) - Tiled Editor, Blueprints, Input System
- [Technical Reference](https://atlasbruce.github.io/Olympe-Engine/technical-reference/architecture/ecs-overview) - Deep dive into architecture
- [API Reference](https://atlasbruce.github.io/Olympe-Engine/api-reference/api-index) - Complete API documentation
- [Contributing Guide](https://atlasbruce.github.io/Olympe-Engine/contributing/testing-guide) - How to contribute

**Note:** Documentation is automatically built and deployed to GitHub Pages on every push to master.

## 🚀 Quick Start

### Prerequisites

- C++14 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15 or higher
- SDL3 development libraries

### Building

```bash
# Clone the repository
git clone https://github.com/Atlasbruce/Olympe-Engine.git
cd Olympe-Engine

# Build with CMake
mkdir build && cd build
cmake ..
cmake --build .

# Run the engine
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
├── website/             # Docusaurus documentation site
└── CMakeLists.txt       # CMake build configuration
```

## 🤝 Contributing

Contributions are welcome! Please see our [Contributing Guide](https://atlasbruce.github.io/Olympe-Engine/contributing/testing-guide) for:

- Testing guidelines
- Adding ECS components
- Code style conventions
- Pull request process

**For AI-assisted development:** Please review [**Copilot Instructions**](.github/COPILOT_INSTRUCTIONS.md) before generating code. This ensures compatibility with our C++14 standard, ImGui/ImNodes versions, and existing code patterns.

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

## 📝 Documentation Development

The documentation is built using [Docusaurus](https://docusaurus.io/) and automatically deployed to GitHub Pages.

### Build Documentation Locally

```bash
# Quick build using helper script
./scripts/setup-docs.sh full

# Or manually:
cd website
npm install
npm start  # Development server at http://localhost:3000
npm run build  # Production build
```

### Generate API Documentation

```bash
# Generate Doxygen API docs
doxygen Doxyfile
# Output: website/static/api/html/
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
