# 🏗️ Architecture Overview

Olympe Engine's architecture documentation provides comprehensive insights into the engine's design, structure, and core systems.

---

## 📖 Documentation in This Section

| Document | Description | Level |
|----------|-------------|-------|
| **[General Architecture](ARCHITECTURE_GENERALE.md)** | Complete engine architecture and ECS design | Intermediate |
| **[Blueprint Architecture](BLUEPRINT_ARCHITECTURE.md)** | Blueprint system architecture and visual editor | Intermediate |
| **[ECS Architecture](#ecs-architecture)** | Entity Component System design patterns | Advanced |

---

## 🎯 Architecture Principles

Olympe Engine is built on these core principles:

### 1. **Data-Oriented Design**
- ECS (Entity Component System) architecture
- Cache-friendly data layouts
- Separation of data and logic

### 2. **Modularity**
- Independent, reusable systems
- Clear system boundaries
- Plugin-friendly architecture

### 3. **Performance First**
- Zero-cost abstractions where possible
- Memory-efficient component storage
- Multi-threaded system execution (planned)

### 4. **Designer-Friendly**
- JSON-based entity blueprints
- Visual Blueprint Editor
- No code required for content creation

---

## 🔧 High-Level Architecture

```
┌──────────────────────────────────────────────────────────┐
│                   Olympe Engine V2                        │
└──────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌───────────────┐   ┌───────────────┐   ┌──────────────┐
│  Game Layer   │   │  Engine Core  │   │  Tools Layer │
│               │   │               │   │              │
│ - Game Logic  │   │ - ECS World   │   │ - Blueprint  │
│ - Blueprints  │   │ - Systems     │   │   Editor     │
│ - AI Trees    │   │ - Components  │   │ - Asset      │
│               │   │               │   │   Browser    │
└───────────────┘   └───────────────┘   └──────────────┘
        │                   │                   │
        └───────────────────┼───────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌───────────────┐   ┌───────────────┐   ┌──────────────┐
│  Input System │   │ Render System │   │ Physics Sys  │
│  (SDL3)       │   │ (OpenGL)      │   │              │
└───────────────┘   └───────────────┘   └──────────────┘
        │                   │                   │
        └───────────────────┼───────────────────┘
                            │
                            ▼
                    ┌───────────────┐
                    │  Platform     │
                    │  (SDL3/OS)    │
                    └───────────────┘
```

---

## 🧩 ECS Architecture

### Entity Component System Overview

Olympe Engine uses a pure ECS architecture:

```
┌─────────────────────────────────────────────┐
│              ECS World                       │
├─────────────────────────────────────────────┤
│                                              │
│  ┌──────────┐     ┌──────────────┐         │
│  │ Entities │────►│  Components  │         │
│  │          │     │              │         │
│  │  ID: 1   │     │  Position    │         │
│  │  ID: 2   │     │  Velocity    │         │
│  │  ID: 3   │     │  Health      │         │
│  │  ...     │     │  ...         │         │
│  └──────────┘     └──────────────┘         │
│                           │                 │
│                           │                 │
│                           ▼                 │
│                   ┌──────────────┐         │
│                   │   Systems    │         │
│                   │              │         │
│                   │  Movement    │         │
│                   │  Rendering   │         │
│                   │  AI          │         │
│                   │  Input       │         │
│                   └──────────────┘         │
│                                              │
└─────────────────────────────────────────────┘
```

### Key ECS Concepts

#### Entities
- Simple integer IDs (EntityID)
- Just a handle to a collection of components
- No data or logic in entities themselves

#### Components
- Pure data structures (POD - Plain Old Data)
- No logic, only state
- Examples: `Position_data`, `Health_data`, `AI_data`

#### Systems
- Pure logic, no state
- Operate on entities with specific component combinations
- Examples: `MovementSystem`, `AISystem`, `RenderSystem`

---

## 📦 Core Systems

### 1. Input System
- Multi-player local support (up to 8 players)
- Hotplug device detection
- ECS-integrated components
- See: [Input System Guide](../03-Core-Systems/Input/INPUT_SYSTEM_GUIDE.md)

### 2. Blueprint System
- JSON-based entity definitions
- Visual editor for entity creation
- Component library
- See: [Blueprint System](../03-Core-Systems/Blueprint/README.md)

### 3. AI System
- Behavior Tree engine
- NPC AI components
- State machines
- See: [AI Systems](../04-AI-Systems/README.md)

### 4. Rendering System
- OpenGL-based renderer
- Sprite rendering
- Camera system
- See: General Architecture documentation

### 5. Physics System
- Simple collision detection
- Movement integration
- Trigger zones
- See: General Architecture documentation

---

## 🎮 System Execution Order

Systems execute in a specific order each frame:

```
Frame Start
    │
    ├─► 1. Input System (collect input events)
    │
    ├─► 2. AI Systems
    │   ├─► AI Perception (detect targets)
    │   ├─► Behavior Trees (decide actions)
    │   └─► AI Motion (generate movement intents)
    │
    ├─► 3. Physics Systems
    │   ├─► Movement System (apply velocities)
    │   ├─► Collision System (detect collisions)
    │   └─► Trigger System (handle triggers)
    │
    ├─► 4. Animation System (update animations)
    │
    ├─► 5. Camera System (update camera)
    │
    ├─► 6. Rendering System (draw everything)
    │
    └─► 7. Event System (process events)
    │
Frame End
```

---

## 💾 Data Flow

### Blueprint to Runtime

```
JSON Blueprint File
        │
        ▼
┌──────────────────┐
│ EntityBlueprint  │  Load & Parse
│     Class        │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ Blueprint Loader │  Validate
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  ECS World       │  Instantiate
│  CreateEntity()  │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ Runtime Entity   │  Execute
│ with Components  │
└──────────────────┘
```

---

## 🔍 Detailed Architecture Documents

### Core Architecture
- **[General Architecture](ARCHITECTURE_GENERALE.md)** - Complete system overview
  - ECS implementation details
  - Component reference
  - System descriptions
  - Event system

### Blueprint Architecture
- **[Blueprint Architecture](BLUEPRINT_ARCHITECTURE.md)** - Blueprint system design
  - JSON schema
  - Component serialization
  - Visual editor architecture
  - Integration patterns

### System-Specific Architecture
- **[Input Architecture](../03-Core-Systems/Input/INPUT_ARCHITECTURE.md)** - Input system design
- **[AI Architecture](../04-AI-Systems/AI_ARCHITECTURE.md)** - AI systems design
- **[Behavior Tree Architecture](../04-AI-Systems/BT_VISUAL_ARCHITECTURE.md)** - BT engine design

---

## 📊 Performance Characteristics

### Memory Layout

Components are stored in contiguous arrays for cache efficiency:

```cpp
// Cache-friendly component storage
std::vector<Position_data> positions;     // All positions together
std::vector<Velocity_data> velocities;    // All velocities together
std::vector<Health_data> healths;         // All healths together
```

### System Performance

| System | Frequency | Performance Target |
|--------|-----------|-------------------|
| Input | Every frame | < 0.1ms |
| AI Perception | 5 Hz (default) | < 1ms |
| Behavior Trees | 10 Hz (default) | < 2ms |
| Movement | Every frame | < 0.5ms |
| Rendering | Every frame | < 16ms (60 FPS) |

---

## 🚀 Future Architecture Plans

### Planned Improvements

1. **Multi-threading**
   - Parallel system execution
   - Job system for AI/Physics

2. **Networking**
   - Client-server architecture
   - Component replication

3. **Asset Streaming**
   - Asynchronous asset loading
   - Memory-efficient resource management

4. **Advanced Rendering**
   - Deferred rendering pipeline
   - Post-processing effects

See: [Development Plan](../06-Development/DEVELOPMENT_PLAN.md)

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../README.md)
- 🎮 [Core Systems](../03-Core-Systems/)
- 🤖 [AI Systems](../04-AI-Systems/)
- 🛠️ [Tools](../05-Tools/)

---

[← Back to Documentation Hub](../README.md) | [Next: Core Systems →](../03-Core-Systems/)
