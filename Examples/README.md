# AI System Examples

This directory contains example code demonstrating how to use the Olympe Engine AI system.

## Files

- `AI_Example.cpp` - Practical examples of creating NPCs with AI behavior

## Quick Start

### 1. Initialize AI System

At game startup, load behavior tree assets:

```cpp
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_patrol.json", 2);
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_combat.json", 3);
```

### 2. Create NPC with AI

```cpp
EntityID guard = CreateGuardNPC(Vector(200.0f, 200.0f, 0.0f));
```

### 3. Emit Events

```cpp
// Explosion at position
AIEvents::EmitExplosion(Vector(500.0f, 300.0f, 0.0f), 200.0f);

// Noise/sound
AIEvents::EmitNoise(Vector(400.0f, 250.0f, 0.0f), 1.0f);

// Damage
AIEvents::EmitDamageDealt(victimEntity, attackerEntity, 25.0f);
```

## See Also

- Full Documentation: `Documentation/AI/AI_ARCHITECTURE.md`
- Example Behavior Trees: `Blueprints/AI/`
- AI Components: `Source/ECS_Components_AI.h`
- AI Systems: `Source/ECS_Systems_AI.h`

---

## Technical Prerequisites Test Programs

### imgui_imnodes_test.cpp
**ImGui & ImNodes Integration Test** - Validates that ImGui and ImNodes libraries are properly integrated.

**Build:**
```bash
g++ -std=c++17 \
    -I../Source/third_party \
    -I../Source/third_party/imgui \
    -I../Source/third_party/imnodes \
    imgui_imnodes_test.cpp \
    ../Source/third_party/imgui/imgui.cpp \
    ../Source/third_party/imgui/imgui_demo.cpp \
    ../Source/third_party/imgui/imgui_draw.cpp \
    ../Source/third_party/imgui/imgui_tables.cpp \
    ../Source/third_party/imgui/imgui_widgets.cpp \
    ../Source/third_party/imnodes/imnodes.cpp \
    -o imgui_imnodes_test
```

### asset_loader_test.cpp
**Asset Loader API Test** - Tests the unified asset loading API for Entity Blueprints, Prefabs, Behavior Trees, HFSM, and Templates.

**Build:**
```bash
g++ -std=c++17 \
    -I../Source \
    -I../Source/third_party \
    asset_loader_test.cpp \
    -o asset_loader_test
```

See [TECHNICAL_PREREQUISITES.md](../TECHNICAL_PREREQUISITES.md) for complete documentation.
