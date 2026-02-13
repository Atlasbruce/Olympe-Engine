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

## 🚀 Quick Start

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- SDL3 development libraries

### Building

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
