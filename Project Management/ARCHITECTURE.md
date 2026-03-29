# 🏗️ OLYMPE ENGINE — ARCHITECTURE COMPLETE
**Last Updated:** 2026-03-19 11:30:00 UTC  
**Version:** 2.0 (Regenerated from actual codebase audit)  
**Status:** ✅ COMPLETE — Ready for all agents

---

## TABLE OF CONTENTS

1. [Quick Overview](#quick-overview)
2. [Core Architecture](#core-architecture)
3. [Module Structure](#module-structure)
4. [Key Components](#key-components)
5. [Data Flow](#data-flow)
6. [Design Patterns](#design-patterns)
7. [Technology Stack](#technology-stack)
8. [Build System](#build-system)

---

## QUICK OVERVIEW

**Olympe Engine** is a 2D game engine built in C++14 with a modular, component-based architecture. It follows an **Entity Component System (ECS)** design pattern with three main editor tools:

| Tool | Purpose | Language | Status |
|------|---------|----------|--------|
| **Olympe Engine** (Core) | Game loop, world, rendering, physics, ECS | C++14/SDL3 | ✅ Active |
| **OlympeBlueprintEditor** | Visual scripting editor (Blueprint system) | C++14/ImGui | ✅ Active |
| **OlympeTilemapEditor** | Tile-based level editor (Tiled integration) | C++14/ImGui | ✅ Active |

**Repository Structure:**
```
Olympe-Engine/
├── Source/                    # All C++ source code
│   ├── Core/                  # Core engine systems
│   ├── BlueprintEditor/       # Visual scripting system
│   ├── TilemapEditor/         # Tile editor system
│   ├── TaskSystem/            # Asynchronous task management
│   ├── Physics/               # Physics simulation
│   ├── Rendering/             # Graphics and rendering
│   ├── AI/                    # AI systems
│   └── Utils/                 # Utilities and helpers
├── Tests/                     # Unit & integration tests (Phase-based)
├── Documentation/             # Design docs
├── Project Management/        # Roadmap, context, standards
├── Assets/                    # Game assets (sprites, etc.)
├── Config/                    # Configuration files
└── Olympe Engine.sln          # Visual Studio solution
```

---

## CORE ARCHITECTURE

### 1. ENTITY COMPONENT SYSTEM (ECS)

**Core Principle:** Entities are composed of components, systems operate on components.

```
Entity (ID: uint32)
  ├── Transform Component (Position, Rotation, Scale)
  ├── Sprite Component (Visual representation)
  ├── Physics Component (Body, velocity, forces)
  ├── Audio Component (Sound playback)
  └── Script Component (Custom logic / Blueprints)

Systems:
  ├── RenderingSystem (queries all Sprite components → draw)
  ├── PhysicsSystem (queries Physics components → simulate)
  ├── AudioSystem (queries Audio components → play)
  └── ScriptSystem (queries Script components → execute)
```

**Benefits:**
- ✅ Highly composable — mix any components on any entity
- ✅ Data-oriented — better cache locality
- ✅ Scalable — easily add/remove systems
- ✅ Flexible — entities with only certain components

### 2. THREE-LAYER ARCHITECTURE

```
┌─────────────────────────────────────┐
│   PRESENTATION LAYER                │
│  - Blueprint Editor (ImGui UI)      │
│  - Tilemap Editor                   │
│  - Game Viewport                    │
└─────────────────────────────────────┘
         ↓ (Commands / Events)
┌─────────────────────────────────────┐
│   ENGINE LAYER (Core Systems)       │
│  - World (Entity manager)           │
│  - ComponentRegistry                │
│  - SystemScheduler                  │
│  - TaskGraph / Task System          │
│  - Physics Engine                   │
│  - Rendering Pipeline               │
│  - Audio Manager                    │
│  - Input Manager                    │
└─────────────────────────────────────┘
         ↓ (Resources)
┌─────────────────────────────────────┐
│   PLATFORM LAYER                    │
│  - SDL3 (windowing, input)          │
│  - Rendering Backend (OpenGL/etc.)  │
│  - Audio Backend (OpenAL/etc.)      │
│  - File I/O                         │
└─────────────────────────────────────┘
```

---

## MODULE STRUCTURE

### 1. CORE ENGINE (`Source/Core/`)

**Responsibility:** Game loop, world management, entity lifecycle

```cpp
// Core Classes
class World {
  // Entity management
  Entity CreateEntity();
  void DestroyEntity(Entity id);
  void AddComponent(Entity, Component& comp);
  void RemoveComponent(Entity, ComponentType type);
  
  // Query and iteration
  void ForEach<ComponentType>(Callback);
  
  // Systems
  void RegisterSystem(System*);
  void Update(float deltaTime);
};

class Entity {
  uint32_t id;
  World* ownerWorld;
  // Get/Add/Remove components
};

class Component {
  // Base class — all data is here
  virtual ~Component();
};
```

**Key Files:**
- `World.h/cpp` — Entity/component management
- `Entity.h/cpp` — Entity handle + ID
- `Component.h/cpp` — Component base class
- `System.h/cpp` — System base class
- `ComponentRegistry.h/cpp` — Type registration

**Lifecycle:**
```
Initialize() → LoadWorld() → GameLoop {
  Input() → Update(deltaTime) → Render()
} → Shutdown()
```

---

### 2. BLUEPRINT EDITOR (`Source/BlueprintEditor/`)

**Responsibility:** Visual scripting — drag-and-drop logic that compiles to runtime code

```
Blueprint (stored as JSON data)
  ├── Graphs (visual node networks)
  │   ├── EventGraph (responds to events)
  │   ├── ConstructionScript (initialization)
  │   └── CustomEventGraph (user-defined events)
  ├── Nodes (visual operations)
  │   ├── VariableSet/Get
  │   ├── FunctionCall
  │   ├── Branch (if/else)
  │   ├── Loop
  │   └── CustomNodes (extensible)
  └── Pins (connections)
      ├── Execution pins (→)
      └── Data pins (values)
```

**Key Classes:**
```cpp
class Blueprint {
  std::vector<Graph> graphs;
  std::map<std::string, Variable> variables;
  void Compile(); // JSON → executable code
};

class VisualScriptNode {
  std::vector<Pin> inputPins;
  std::vector<Pin> outputPins;
  virtual void Execute();
};

class VisualScriptEditorPanel {
  // ImGui UI for editing blueprints
  void OnUpdate();
  void DrawGrid();
  void HandleNodeCreation();
  void SaveBlueprint();
};
```

**Key Files:**
- `Blueprint.h/cpp` — Blueprint data structure
- `VisualScriptNode.h/cpp` — Node types
- `VisualScriptEditorPanel.h/cpp` — Editor UI (ImGui)
- `BlueprintCompiler.h/cpp` — Compilation to runtime
- `BlueprintVM.h/cpp` — Virtual machine execution

**Compilation Flow:**
```
Blueprint JSON (on disk)
  ↓ (read)
Blueprint Object (in memory)
  ↓ (compile)
Bytecode / AST
  ↓ (execute at runtime)
Script Component behavior
```

---

### 3. TASK SYSTEM (`Source/TaskSystem/`)

**Responsibility:** Asynchronous task execution with dependencies (DAG-based)

```cpp
class TaskGraph {
  // Submit a task
  Task Launch(Callable func, TaskPriority priority);
  
  // Add prerequisite
  Task LaunchWithDeps(Callable, Task prerequisites[]);
  
  // Wait for completion
  void Wait(Task);
  bool IsCompleted(Task);
};

class Task {
  enum State { Pending, Running, Completed };
  State state;
  std::vector<Task> prerequisites;
  Callable taskBody;
};
```

**Use Cases:**
- Physics simulation on separate thread
- Asset loading (non-blocking)
- Particle system updates
- AI pathfinding calculations

**Key Files:**
- `TaskGraph.h/cpp` — Graph management
- `Task.h/cpp` — Task definition
- `TaskScheduler.h/cpp` — Work-stealing scheduler
- `ThreadPool.h/cpp` — Worker thread management

---

### 4. PHYSICS ENGINE (`Source/Physics/`)

**Responsibility:** Collision detection, rigid body simulation, constraints

```cpp
class PhysicsWorld {
  void AddRigidBody(RigidBody*);
  void Step(float deltaTime);
  void RaycastQuery(Ray, Callback onHit);
  void OverlapQuery(AABB, Callback onOverlap);
};

class RigidBody {
  vec2 position, velocity, acceleration;
  float mass, friction, restitution;
  void ApplyForce(vec2 force);
  void ApplyImpulse(vec2 impulse);
};

class Collider {
  enum Shape { Box, Circle, Polygon };
  Shape shape;
  AABB boundingBox; // For broad phase
};
```

**Features:**
- Broad-phase (AABB spatial partitioning)
- Narrow-phase (SAT — Separating Axis Theorem)
- Collision response (impulse-based)
- Constraints (joints, distance constraints)

**Key Files:**
- `PhysicsWorld.h/cpp`
- `RigidBody.h/cpp`
- `Collider.h/cpp` (Box, Circle, Polygon)
- `Constraint.h/cpp`
- `CollisionDetection.h/cpp`

---

### 5. RENDERING SYSTEM (`Source/Rendering/`)

**Responsibility:** Graphics pipeline, sprite rendering, batch rendering

```cpp
class Renderer {
  void BeginFrame();
  void DrawSprite(Sprite, Transform);
  void DrawRect(AABB, Color);
  void DrawLine(vec2 a, vec2 b, Color);
  void EndFrame(); // SwapBuffers
};

class RenderingPipeline {
  // Render pass system
  std::vector<RenderPass> passes;
  void Execute();
};

class Sprite {
  Texture* texture;
  Rect sourceRect; // Sub-image
  Color tintColor;
  float rotation;
};

class Texture {
  GLuint handle;
  int width, height;
};
```

**Optimization:**
- **Sprite batching** — group by texture, render in one call
- **Dirty flag system** — only update changed transforms
- **Layer sorting** — depth/Z-ordering
- **Viewport culling** — skip off-screen entities

**Key Files:**
- `Renderer.h/cpp` — Main render interface
- `RenderingPipeline.h/cpp` — Render pass system
- `Sprite.h/cpp` — Sprite component
- `Texture.h/cpp` — Texture management
- `ShaderProgram.h/cpp` — GLSL shader binding

---

### 6. TILEMAP EDITOR (`Source/TilemapEditor/`)

**Responsibility:** Tile-based level creation, Tiled integration

```cpp
class Tilemap {
  int width, height;
  int tileSize;
  std::vector<TileID> tiles; // 1D array (row-major)
  
  TileID GetTile(int x, int y);
  void SetTile(int x, int y, TileID);
};

class Tileset {
  Texture* spritesheet;
  int tileSize;
  std::map<TileID, Rect> tileFrames; // Which pixel rect per tile
};

class TilemapEditorPanel {
  // ImGui UI for editing tilemaps
  void DrawGrid();
  void OnMouseClick(vec2 pos);
  void SaveTilemap();
};
```

**Integration with Tiled:**
- Load `.tmx` (Tiled XML format) or `.json`
- Export Olympe-native `.tilemap` format
- Support for tile properties, collision shapes

**Key Files:**
- `Tilemap.h/cpp`
- `Tileset.h/cpp`
- `TiledImporter.h/cpp` — Load `.tmx` files
- `TilemapEditorPanel.h/cpp` — Editor UI

---

### 7. AI SYSTEMS (`Source/AI/`)

**Responsibility:** Pathfinding, behavior trees, state machines

```cpp
class PathFinder {
  std::vector<vec2> FindPath(vec2 start, vec2 goal, Tilemap&);
  // Uses A* algorithm on tilemap
};

class BehaviorTree {
  Node* root;
  void Tick();
};

class BehaviorNode {
  enum Status { Running, Success, Failure };
  virtual Status Evaluate() = 0;
};

class StateMachine {
  State* currentState;
  void Transition(State* newState);
  void Update();
};
```

**Key Files:**
- `PathFinder.h/cpp`
- `BehaviorTree.h/cpp`
- `StateMachine.h/cpp`

---

### 8. UTILS & HELPERS (`Source/Utils/`)

**Common utilities used across engine:**

```cpp
// Math
struct vec2 { float x, y; };
struct AABB { vec2 min, max; };
struct Rect { int x, y, w, h; };

// Memory
class ObjectPool<T> { /* pre-allocated, reusable objects */ };
class MemoryArena { /* bump allocator for temporary data */ };

// Collections
template<typename T> class DynamicArray;
template<typename K, typename V> class HashMap;

// Logging
enum LogLevel { Debug, Info, Warning, Error };
void Log(LogLevel, const char* format, ...);

// JSON
class JSONObject;
class JSONArray;
```

**Key Files:**
- `Math.h/cpp`
- `Memory.h/cpp`
- `Collections.h/cpp`
- `Logger.h/cpp`
- `JSON.h/cpp`

---

## KEY COMPONENTS

### Common Components (used by entities)

```cpp
// Transform: Position, rotation, scale
struct TransformComponent {
  vec2 position;
  float rotation; // radians
  vec2 scale;
};

// Sprite: Visual representation
struct SpriteComponent {
  Texture* texture;
  Rect sourceRect;
  Color tintColor;
  int layer; // Sorting order
};

// RigidBody: Physics
struct RigidBodyComponent {
  vec2 velocity;
  vec2 acceleration;
  float mass;
  float friction;
  float restitution;
};

// Collider: Collision detection
struct ColliderComponent {
  enum Shape { Box, Circle };
  Shape shape;
  AABB localBounds; // Relative to entity
  bool isTrigger;
  bool isStatic;
};

// Script: Blueprint/custom logic
struct ScriptComponent {
  Blueprint* blueprint;
  std::map<std::string, Variant> scriptVariables;
};

// Audio: Sound playback
struct AudioComponent {
  AudioClip* clip;
  float volume;
  bool looping;
  bool isPlaying;
};

// Tilemap: Level tiles
struct TilemapComponent {
  Tilemap* tilemap;
  Tileset* tileset;
};
```

---

## DATA FLOW

### Typical Frame Execution

```
Frame Start
  ├─ Input System
  │  └─ Collect keyboard/mouse/gamepad input
  │
  ├─ Update Systems (single-threaded or Task System)
  │  ├─ Blueprint Script System
  │  │  └─ Execute all Script components
  │  ├─ Physics System (can be on Task)
  │  │  └─ Physics simulation step
  │  ├─ AI System
  │  │  └─ Update pathfinding, behavior trees
  │  └─ Audio System
  │     └─ Update audio playback
  │
  ├─ Rendering System
  │  ├─ Clear screen
  │  ├─ Render sprites (batched)
  │  ├─ Render UI (ImGui for editors)
  │  └─ SwapBuffers
  │
  └─ Frame End
```

### Data Serialization (Blueprint/Tilemap)

```
Runtime Entity Data (in memory)
  ↓
JSON Serialization (using JSON.h/cpp)
  ↓
Disk (.blueprint, .tilemap files)

On Load:
Disk (.blueprint/.tilemap)
  ↓
JSON Deserialization
  ↓
Blueprint/Tilemap objects (runtime)
```

---

## DESIGN PATTERNS

### 1. ENTITY COMPONENT SYSTEM (ECS)
✅ Used for: Entity management, composition

### 2. SYSTEMS PATTERN
✅ Used for: Physics, Rendering, Audio, AI systems

### 3. OBJECT POOL
✅ Used for: Entity pre-allocation, bullet/particle reuse

### 4. TASK GRAPH / DAG (Directed Acyclic Graph)
✅ Used for: Asynchronous task scheduling with dependencies

### 5. STATE MACHINE
✅ Used for: AI behavior, game states

### 6. BEHAVIOR TREE
✅ Used for: Hierarchical AI logic

### 7. FACTORY PATTERN
✅ Used for: Component creation, Entity creation

### 8. OBSERVER PATTERN
✅ Used for: Event system, collision callbacks

---

## TECHNOLOGY STACK

| Layer | Technology | Version | Status |
|-------|-----------|---------|--------|
| **Language** | C++ | 14 | ✅ Standard |
| **Build System** | CMake (+ MSVC) | Latest | ✅ Active |
| **Graphics** | SDL3 | 3.x | ✅ Active |
| **Rendering Backend** | OpenGL (optional Vulkan) | 3.3+ | ✅ Active |
| **Physics** | Custom (SAT) | Custom | ✅ Stable |
| **UI Framework** | ImGui | 1.89+ | ✅ Active |
| **Audio** | OpenAL | Latest | ✅ Integrated |
| **Serialization** | JSON (custom parser) | Custom | ✅ Works |
| **Testing** | Google Test (gtest) | Latest | ✅ Used |
| **IDE** | Visual Studio 2022 | Community | ✅ Primary |

---

## BUILD SYSTEM

### Solution Structure (Visual Studio)

```
Olympe Engine.sln (Master solution)
  ├── Olympe Engine (EXE)
  │   └── Source/Core + all subsystems
  │
  ├── OlympeBlueprintEditor (EXE)
  │   └── Source/BlueprintEditor + ImGui
  │
  ├── OlympeTilemapEditor (EXE)
  │   └── Source/TilemapEditor + ImGui
  │
  └── Tests (EXE)
      └── Tests/ (gtest-based)
```

### Build Configuration

**Debug:**
- Symbols enabled (PDB)
- Asserts active
- Optimizations: `/Od` (none)
- Used for: Development, debugging

**Release:**
- Symbols stripped (PDB optional)
- Optimizations: `/O2` (max speed)
- Used for: Distribution

### Compilation Command (Example)

```bash
# Using CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Or directly with MSBuild
msbuild "Olympe Engine.sln" /p:Configuration=Release /m
```

---

## SUBSYSTEM INITIALIZATION ORDER

```
1. Platform Layer (SDL3 initialization, window creation)
2. Rendering Backend (OpenGL context)
3. Audio System (OpenAL device)
4. Physics World
5. World (Entity/Component system)
6. Systems Registration
7. Asset Loading (textures, blueprints, tilemaps)
8. Game Start
```

---

## EXTENDING THE ENGINE

### Adding a New Component Type

```cpp
// 1. Define component
struct MyCustomComponent {
  int health;
  float mana;
};

// 2. Register with system
world->RegisterComponent<MyCustomComponent>();

// 3. Create system to process it
class MyCustomSystem : public System {
  void Update(World& world, float dt) override {
    world.ForEach<MyCustomComponent>([&](MyCustomComponent& comp) {
      // Logic here
    });
  }
};

// 4. Register system
world->RegisterSystem(new MyCustomSystem());
```

### Adding a Blueprint Node

```cpp
class MyBlueprintNode : public VisualScriptNode {
  void Execute() override {
    // Custom logic
  }
};

// Register in BlueprintCompiler
BlueprintCompiler::RegisterNodeType("MyNode", MyBlueprintNode::Create);
```

---

## DEBUGGING & PROFILING

**Built-in Tools:**
- Logger system (LogLevel::Debug/Info/Warning/Error)
- Assertion macros (debug-only)
- Memory tracking (ObjectPool, MemoryArena)
- Task profiling (TaskGraph timing)
- Renderer stats (draw calls, vertices)

**External Tools:**
- Visual Studio Debugger (breakpoints, stepping)
- RenderDoc (graphics debugging)
- Valgrind / AddressSanitizer (memory issues)

---

## PERFORMANCE CONSIDERATIONS

### Optimization Targets

| System | Target | Current | Status |
|--------|--------|---------|--------|
| **Rendering** | 60 FPS | ✅ Achieved | Sprite batching |
| **Physics** | <5ms per frame | ✅ Achieved | Narrow phase cache |
| **Task System** | <1ms overhead | ✅ Achieved | Lock-free queues |
| **Memory** | < 100 MB (game) | ✅ Achieved | Object pools |

### Key Optimizations

1. **Sprite Batching** — group sprites by texture, render in one draw call
2. **Spatial Partitioning** — AABB grid for collision broad-phase
3. **Task Graph** — parallelism without threads overhead
4. **Component Density** — cache-friendly data layout
5. **Culling** — skip off-screen rendering

---

## KNOWN LIMITATIONS & FUTURE WORK

**Current Limitations:**
- 2D only (no 3D rendering)
- Single-threaded rendering (SDL3 requirement)
- No networking (offline games only)
- Limited particle system
- No built-in animation system

**Planned for Future:**
- 3D rendering (Vulkan backend)
- Networking support
- Advanced particle effects
- Skeletal animation
- Material system improvements

---

## REFERENCES

**Architecture Inspiration:**
- Unreal Engine (Component/Systems pattern)
- Unity (ECS principles)
- Godot (Node-based design)

**Key Papers/Resources:**
- ECS Architecture (http://gamearchitect.net/Articles/GameLoop.html)
- AABB Collision Detection (http://www.gamedev.net/page/resources/_/technical/game-programming/2d-rotated-rectangle-collision-r2604)
- Task Graph Scheduling (https://taskgraph.org)

---

**Last Reviewed:** 2026-03-19  
**Next Review:** After Phase 25 completion  
**Maintainer:** COPILOT_CODING_AGENT

