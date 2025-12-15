# Olympe Engine

> **Modern 2D/3D Game Engine with ECS Architecture**  
> Version 2.0 - 2025

---

## 🎮 Features

- ✅ **Entity Component System (ECS)** - High-performance data-oriented architecture
- ✅ **Modern Input System** - Multi-player local support with hotplug
- ✅ **Blueprint System** - JSON-based entity templates
- ✅ **SDL3 Integration** - Cross-platform hardware abstraction
- ✅ **Visual Blueprint Editor** - Create entities without coding

---

## 📚 Documentation

### Core Systems

- **[Input System](Documentation/Input/INPUT_SYSTEM_GUIDE.md)** - Complete input management documentation
  - [Quick Start Guide](Documentation/Input/INPUT_QUICK_START.md) - Get started in 5 minutes
  - [API Reference](Documentation/Input/INPUT_API_REFERENCE.md) - Detailed API documentation
  - [Examples](Documentation/Input/INPUT_EXAMPLES.md) - Practical code examples
  - [Architecture](Documentation/Input/INPUT_ARCHITECTURE.md) - System design details
  - [Migration Guide](Documentation/Input/INPUT_MIGRATION.md) - Migrate from old code
  - [Troubleshooting](Documentation/Input/INPUT_TROUBLESHOOTING.md) - Problem solving

- **[Architecture](ARCHITECTURE.md)** - Overall engine architecture
- **[Blueprint System](Blueprints/README.md)** - Entity blueprint documentation
- **[French Guide](GUIDE_FRANCAIS.md)** - Guide en français

### Tools

- **[Blueprint Editor](OlympeBlueprintEditor/README.md)** - Visual entity editor

---

## 🚀 Quick Start

### Input System Example

Create a controllable player entity:

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

// Bind keyboard to player 0 (-1 = keyboard)
InputsManager::Get().BindControllerToPlayer(0, -1);
```

See [Input Quick Start](Documentation/Input/INPUT_QUICK_START.md) for more details.

---

## 🏗️ Architecture

Olympe Engine uses a modern **Entity Component System (ECS)** architecture:

```
┌─────────────────────────────────────────┐
│           Game Application              │
└──────────────┬──────────────────────────┘
               │
       ┌───────▼────────┐
       │   ECS World    │
       │                │
       │  - Entities    │
       │  - Components  │
       │  - Systems     │
       └───────┬────────┘
               │
    ┌──────────┴───────────┐
    ▼                      ▼
┌─────────┐          ┌──────────┐
│Input Sys│          │Other Sys │
│- Player │          │- Physics │
│  Control│          │- Render  │
│- Mapping│          │- AI      │
└─────────┘          └──────────┘
```

---

## 🎯 Key Features

### Input System

- **Multi-player Support** - Up to 8 players (keyboard + gamepads)
- **Hotplug** - Automatic device connection/disconnection handling
- **Pull & Push APIs** - Both polling and event-driven input
- **ECS Integration** - Components for bindings, controllers, and actions
- **Thread-safe** - Manager operations are protected

### Blueprint System

- **JSON-based** - Human-readable entity definitions
- **Visual Editor** - Create and edit entities visually
- **Component Library** - Pre-built component templates
- **Hot-reload** - Runtime entity reloading (planned)

---

## 🛠️ Building

### Prerequisites

- C++17 compatible compiler
- SDL3 library
- CMake or Visual Studio

### Windows (Visual Studio)

```bash
# Open solution
start "Olympe Engine.sln"

# Build in Visual Studio (Ctrl+Shift+B)
```

### Blueprint Editor

```bash
cd OlympeBlueprintEditor
make
./OlympeBlueprintEditor
```

---

## 📖 More Documentation

- [Development Plan](DEVELOPMENT_PLAN.md) - Roadmap and phases
- [Phase 1 Summary](PHASE1_SUMMARY.md) - Blueprint system phase 1
- [ClickUp Tasks](CLICKUP_TASKS.md) - Task tracking

---

## 🤝 Contributing

Contributions are welcome! Please read the documentation before submitting PRs.

---

## 📝 License

See LICENSE file in the repository.

---

## 👨‍💻 Author

**Nicolas Chereau**  
📧 nchereau@gmail.com

**Olympe Engine Team - 2025**