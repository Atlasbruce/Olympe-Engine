# 📚 Reference

Quick reference documentation and supplementary materials.

---

## 📖 Documentation in This Section

| Document | Description |
|----------|-------------|
| **[Credits](CREDITS.md)** | Contributors and acknowledgments |
| **[SDL Setup](SDL_SETUP.md)** | SDL3 installation and configuration |

---

## 🎯 Quick Links

### External Resources

#### SDL3 Documentation
- **Website**: https://libsdl.org/
- **API Reference**: https://wiki.libsdl.org/SDL3
- **Examples**: https://examples.libsdl.org/SDL3
- **Discord**: https://discord.com/invite/BwpFGBWsv8

#### ImGui Documentation
- **Repository**: https://github.com/ocornut/imgui
- **Wiki**: https://github.com/ocornut/imgui/wiki
- **Demo**: Included in `Source/third_party/imgui/`

#### C++ Resources
- **C++ Reference**: https://en.cppreference.com/
- **C++ Core Guidelines**: https://isocpp.github.io/CppCoreGuidelines/
- **Modern C++**: https://github.com/AnthonyCalandra/modern-cpp-features

---

## 📋 Component Reference

Quick reference for all ECS components:

### Core Components
- **Position_data** - Entity position (x, y, z)
- **BoundingBox_data** - Collision rectangle
- **VisualSprite_data** - Sprite rendering

### Physics Components
- **Movement_data** - Velocity and direction
- **PhysicsBody_data** - Mass and speed

### Input Components
- **Controller_data** - Gamepad state
- **PlayerBinding_data** - Player-controller binding
- **PlayerController_data** - Player input states

### AI Components
- **AIBlackboard_data** - AI reasoning data
- **AISenses_data** - Perception parameters
- **AIState_data** - AI state machine
- **BehaviorTreeRuntime_data** - BT execution state
- **MoveIntent_data** - Movement intent
- **AttackIntent_data** - Attack intent

### Gameplay Components
- **Health_data** - Hit points
- **Inventory_data** - Item storage
- **TriggerZone_data** - Detection zone

See: [ECS System Documentation](../03-Core-Systems/ECS/README.md) for detailed component information.

---

## ⚙️ System Reference

Quick reference for all ECS systems:

### Input Systems
- **InputSystem** - Processes input events
- **PlayerControllerSystem** - Updates player states

### AI Systems
- **AIStimuliSystem** - Processes AI events
- **AIPerceptionSystem** - Target detection (5 Hz)
- **AIStateTransitionSystem** - State machine
- **BehaviorTreeSystem** - BT execution (10 Hz)
- **AIMotionSystem** - Intent to movement

### Physics Systems
- **MovementSystem** - Position updates
- **CollisionSystem** - Collision detection
- **TriggerSystem** - Trigger zones

### Rendering Systems
- **RenderSystem** - Sprite rendering
- **CameraSystem** - Camera updates

See: [ECS System Documentation](../03-Core-Systems/ECS/README.md) for detailed system information.

---

## 🎮 Keyboard Shortcuts

### General
- **F2** - Toggle Blueprint Editor
- **Esc** - Close dialogs/menus

### Blueprint Editor
- **Ctrl+N** - New blueprint
- **Ctrl+O** - Open blueprint
- **Ctrl+S** - Save blueprint
- **Ctrl+Shift+S** - Save As
- **F** - Frame all nodes
- **Del** - Delete selected component

### Asset Browser
- **Ctrl+F** - Focus search
- **Enter** - Select asset
- **Esc** - Clear search

---

## 📊 Configuration Values

### Performance Settings

```cpp
// AI System Update Frequencies
const float AI_PERCEPTION_UPDATE_INTERVAL = 0.2f;  // 5 Hz
const float BEHAVIOR_TREE_UPDATE_INTERVAL = 0.1f;  // 10 Hz

// Input System
const int MAX_PLAYERS = 8;
const int MAX_CONTROLLERS = 8;

// ECS Limits
const int MAX_ENTITIES = 100000;
const int INITIAL_COMPONENT_CAPACITY = 1000;
```

### File Paths

```cpp
// Default Directories
const std::string BLUEPRINTS_DIR = "Blueprints/";
const std::string AI_BLUEPRINTS_DIR = "Blueprints/AI/";
const std::string RESOURCES_DIR = "Resources/";
const std::string SPRITES_DIR = "Resources/Sprites/";
const std::string AUDIO_DIR = "Resources/Audio/";
```

---

## 🔧 Build Configurations

### Windows (Visual Studio)

| Configuration | Use Case | Optimizations |
|--------------|----------|---------------|
| Debug | Development | None, Full debugging |
| Release | Production | Maximum, No debugging |
| RelWithDebInfo | Profiling | Optimized with debug info |

### Linux (Make)

```bash
# Debug build
make DEBUG=1

# Release build  
make

# With profiling
make PROFILE=1
```

---

## 📐 Engine Constants

### Physics
- **GRAVITY**: 9.81 m/s²
- **FIXED_TIMESTEP**: 1/60 seconds
- **MAX_VELOCITY**: 1000 units/second

### Rendering
- **TARGET_FPS**: 60
- **VSYNC**: Enabled by default
- **RESOLUTION**: 1920x1080 (configurable)

### AI
- **VISION_RANGE**: 150 units (default)
- **HEARING_RANGE**: 200 units (default)
- **ATTACK_RANGE**: 50 units (default)

---

## 🗂️ File Formats

### Blueprint JSON Schema
```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "string",
    "description": "string",
    "components": []
}
```

### Behavior Tree JSON Schema
```json
{
    "tree_id": 1,
    "name": "string",
    "root_node_id": 1,
    "nodes": []
}
```

See: [Blueprint System](../03-Core-Systems/Blueprint/BLUEPRINT_SYSTEM.md) for complete schemas.

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../README.md)
- 🏗️ [Architecture Overview](../02-Architecture/README.md)
- 🧩 [ECS System](../03-Core-Systems/ECS/README.md)
- 🤖 [AI Systems](../04-AI-Systems/README.md)

---

[← Back to Documentation Hub](../README.md)
