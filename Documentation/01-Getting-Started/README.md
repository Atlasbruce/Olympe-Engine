# 🚀 Getting Started with Olympe Engine

Welcome to Olympe Engine! This guide will help you get started quickly with the engine.

---

## 📖 Documentation in This Section

| Document | Description | Audience |
|----------|-------------|----------|
| **[Quick Start](#quick-start)** | Get up and running in 5 minutes | All users |
| **[Installation Guide](INSTALLATION.md)** | Complete installation and build instructions | All users |
| **[Guide Français](GUIDE_FRANCAIS.md)** | Documentation complète en français | Utilisateurs francophones |

---

## Quick Start

### Prerequisites

- C++17 compatible compiler (Visual Studio 2022 or g++)
- SDL3 library
- CMake (optional)

### Build and Run

#### Windows (Visual Studio)

1. **Open the solution**
   ```bash
   start "Olympe Engine.sln"
   ```

2. **Build the project**
   - Press `Ctrl+Shift+B` or select Build → Build Solution

3. **Run the engine**
   - Press `F5` or select Debug → Start Debugging

#### Linux

```bash
# Install dependencies
sudo apt-get install libsdl3-dev

# Build
make

# Run
./OlympeEngine
```

---

## 🎮 Your First Entity

Create a simple controllable player entity:

```cpp
#include "World.h"
#include "ECS_Components.h"
#include "InputsManager.h"

// Create player entity
EntityID player = World::Get().CreateEntity();

// Add position
Position_data pos;
pos.position = Vector{100.f, 100.f, 0.f};
World::Get().AddComponent<Position_data>(player, pos);

// Add input components
PlayerBinding_data binding;
binding.playerIndex = 0;
binding.controllerID = -1; // Keyboard
World::Get().AddComponent<PlayerBinding_data>(player, binding);

World::Get().AddComponent<Controller_data>(player);
World::Get().AddComponent<PlayerController_data>(player);

// Bind keyboard to player 0
InputsManager::Get().BindControllerToPlayer(0, -1);
```

See the [Input Quick Start](../03-Core-Systems/Input/INPUT_QUICK_START.md) for more details.

---

## 🛠️ Using the Blueprint Editor

The Blueprint Editor allows you to create entities visually without coding:

1. **Launch the game engine**
2. **Press F2** to open/close the Blueprint Editor
3. **Use File menu** to create, load, or save blueprints
4. **Edit components** visually through the node editor
5. **Browse assets** using the integrated Asset Browser

Learn more in the [Blueprint System Guide](../03-Core-Systems/Blueprint/README.md).

---

## 📚 Next Steps

### For Beginners
1. ✅ Complete the Quick Start above
2. 📖 Read the [Input System Guide](../03-Core-Systems/Input/INPUT_SYSTEM_GUIDE.md)
3. 🎨 Try the [Blueprint Editor](../05-Tools/BLUEPRINT_EDITOR.md)
4. 💡 Explore [Code Examples](../03-Core-Systems/Input/INPUT_EXAMPLES.md)

### For Developers
1. 📊 Understand the [Architecture](../02-Architecture/README.md)
2. 🔍 Review the [API Reference](../03-Core-Systems/Input/INPUT_API_REFERENCE.md)
3. 🏗️ Learn about [ECS Components](../03-Core-Systems/ECS/README.md)
4. 🤖 Explore the [AI Systems](../04-AI-Systems/README.md)

### For Contributors
1. 📋 Check the [Development Plan](../06-Development/DEVELOPMENT_PLAN.md)
2. 🤝 Read [Contributing Guidelines](../06-Development/CONTRIBUTING.md)
3. 🏗️ Study the [Architecture Documentation](../02-Architecture/README.md)

---

## 🆘 Getting Help

### Documentation
- 📚 [Main Documentation Hub](../README.md)
- 🔍 [Troubleshooting Guide](../03-Core-Systems/Input/INPUT_TROUBLESHOOTING.md)
- 💡 [Examples](../03-Core-Systems/Input/INPUT_EXAMPLES.md)

### Support
- 📧 **Email**: nchereau@gmail.com
- 📖 **Documentation**: Check this guide first
- 🐛 **Bug Reports**: Include minimal reproduction code

---

## 🌐 Language Options

- 🇬🇧 **English**: You're reading it!
- 🇫🇷 **Français**: [Guide Français](GUIDE_FRANCAIS.md)

---

**Ready to build amazing games? Let's go! 🎮**

---

[← Back to Documentation Hub](../README.md) | [Next: Installation Guide →](INSTALLATION.md)
