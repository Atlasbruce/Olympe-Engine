# Olympe Engine

> **Modern 2D/3D Game Engine with ECS Architecture**  
> Version 2.0 - 2025

---

## 🎮 Features

- ✅ **Entity Component System (ECS)** - High-performance data-oriented architecture
- ✅ **Modern Input System** - Multi-player local support with hotplug
- ✅ **Blueprint System** - JSON-based entity templates
- ✅ **SDL3 Integration** - Cross-platform hardware abstraction
- ✅ **Interactive Blueprint Editor** - Full-featured visual editor with ImGui
- ✅ **Runtime Entity Inspector** - Real-time entity viewing and editing
- ✅ **Asset Browser** - Navigate assets and runtime entities (SDL3)
- ✅ **Synchronized Panels** - All editor panels auto-sync on selection

---

## 🎨 Blueprint Editor (NEW!)

The Blueprint Editor is now fully interactive and production-ready!

### Quick Access
- Press **F2** in-game to toggle the editor
- See **[QUICKSTART.md](QUICKSTART.md)** for usage guide
- See **[TESTING_GUIDE.md](TESTING_GUIDE.md)** for testing procedures
- See **[ARCHITECTURE.md](ARCHITECTURE.md)** for technical details

### Features
- ✅ **Interactive Panels** - Drag, resize, dock all panels freely
- ✅ **Runtime Entities** - View all active entities from World in Asset Browser
- ✅ **Panel Synchronization** - Select entity once, all panels update
- ✅ **Full Menu System** - File, Edit, View, Help with keyboard shortcuts
- ✅ **Component Inspector** - View and edit component properties in real-time
- ✅ **Persistent Layout** - Panel positions saved between sessions

### Documentation
- **[QUICKSTART.md](QUICKSTART.md)** - User guide and workflows
- **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - Technical details
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Architecture diagrams
- **[TESTING_GUIDE.md](TESTING_GUIDE.md)** - Testing procedures

---

## 📚 Documentation

**Complete documentation is now organized in themed sections:**

### 🚀 Start Here
- **[Documentation Hub](Documentation/README.md)** - Central documentation portal
- **[Getting Started](Documentation/01-Getting-Started/README.md)** - Your first steps
- **[Installation Guide](Documentation/01-Getting-Started/INSTALLATION.md)** - Setup instructions
- **[Guide Français](Documentation/01-Getting-Started/GUIDE_FRANCAIS.md)** - Documentation en français

### 📖 Documentation Sections

| Section | Description | Link |
|---------|-------------|------|
| **01 - Getting Started** | Quick start, installation, basics | [→](Documentation/01-Getting-Started/) |
| **02 - Architecture** | System design and structure | [→](Documentation/02-Architecture/) |
| **03 - Core Systems** | Input, Blueprint, ECS systems | [→](Documentation/03-Core-Systems/) |
| **04 - AI Systems** | Behavior Trees, NPC AI | [→](Documentation/04-AI-Systems/) |
| **05 - Tools** | Blueprint Editor, Asset Browser | [→](Documentation/05-Tools/) |
| **06 - Development** | Contributing, roadmap | [→](Documentation/06-Development/) |
| **07 - Reference** | API reference, credits | [→](Documentation/07-Reference/) |

### 🎯 Quick Links by Topic

- **Input System** - [Guide](Documentation/03-Core-Systems/Input/INPUT_SYSTEM_GUIDE.md) | [Quick Start](Documentation/03-Core-Systems/Input/INPUT_QUICK_START.md) | [Examples](Documentation/03-Core-Systems/Input/INPUT_EXAMPLES.md)
- **Blueprint System** - [Guide](Documentation/03-Core-Systems/Blueprint/README.md) | [Editor](Documentation/05-Tools/BLUEPRINT_EDITOR.md)
- **AI Systems** - [Overview](Documentation/04-AI-Systems/README.md) | [Behavior Trees](Documentation/04-AI-Systems/BEHAVIOR_TREES.md)
- **ECS** - [Guide](Documentation/03-Core-Systems/ECS/README.md) | [Architecture](Documentation/02-Architecture/README.md)

---

## 🚀 Quick Start

### Blueprint Editor

The Blueprint Editor is integrated into the game engine and can be toggled at runtime:

1. **Launch the game engine**
2. **Press F2** to open/close the Blueprint Editor
3. **Use File menu** to create, load, or save blueprints
4. **Edit components** visually through the node editor
5. **Browse assets** using the integrated Asset Browser

The editor features:
- **Backend (BlueprintEditor)**: Singleton managing all data, state, and business logic
  - Recursive asset scanning across all subdirectories
  - Asset type detection (EntityBlueprint, BehaviorTree, etc.)
  - Search and filtering API
  - Metadata extraction and caching
  - Robust error handling
- **Frontend (BlueprintEditorGUI)**: ImGui-based visual interface
  - Asset Browser with tree view
  - Asset Info Panel with type-specific displays
  - Search by name and filter by type
  - No direct file access - all through backend API
- Complete separation ensures scalability and maintainability

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
- **Visual Editor** - Create and edit entities visually (SDL3)
- **Asset Browser** - Navigate blueprints, AI trees, and all assets
- **Search & Filter** - Quick asset discovery by name or type
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

### Blueprint Editor (SDL3)

```bash
cd OlympeBlueprintEditor

# Setup dependencies (first time only)
./setup_phase2.sh

# Build GUI editor with Asset Browser
make -f Makefile.gui

# Run from project root
cd ..
./OlympeBlueprintEditor/OlympeBlueprintEditorGUI
```

---

## 📖 More Information

- **[Complete Documentation](Documentation/README.md)** - Full documentation hub
- **[Development Plan](Documentation/06-Development/DEVELOPMENT_PLAN.md)** - Roadmap and phases
- **[Contributing Guide](Documentation/06-Development/CONTRIBUTING.md)** - How to contribute
- **[Changelog](Documentation/06-Development/CHANGELOG.md)** - Version history
- **[Credits](Documentation/07-Reference/CREDITS.md)** - Contributors and acknowledgments

---

## 🤝 Contributing

Contributions are welcome! 

- 📖 Read the [Contributing Guide](Documentation/06-Development/CONTRIBUTING.md)
- 📋 Check the [Development Plan](Documentation/06-Development/DEVELOPMENT_PLAN.md)
- 🏗️ Study the [Architecture](Documentation/02-Architecture/README.md)
- 💬 Join discussions on GitHub

---

## 📝 License

See LICENSE file in the repository.

---

## 👨‍💻 Credits

**Nicolas Chereau** - Engine architect & lead developer  
📧 nchereau@gmail.com

See [full credits](Documentation/07-Reference/CREDITS.md) for all contributors and acknowledgments.

**Olympe Engine Team - 2025**