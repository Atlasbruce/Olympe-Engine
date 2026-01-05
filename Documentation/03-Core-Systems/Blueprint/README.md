# 📋 Blueprint System

The Olympe Blueprint System allows you to define entities using JSON files, making it easy for designers to create game content without writing C++ code.

---

## 📖 Documentation in This Section

| Document | Description | Audience |
|----------|-------------|----------|
| **[Blueprint System Guide](BLUEPRINT_SYSTEM.md)** | Complete blueprint format and component reference | All users |
| **[Blueprint Editor](../../05-Tools/BLUEPRINT_EDITOR.md)** | Visual editor for creating blueprints | Designers |
| **[Blueprint Examples](#quick-examples)** | Common blueprint patterns | Beginners |

---

## 🎯 What are Blueprints?

Blueprints are JSON files that define entities with their components and properties. They allow you to:

- ✅ Create entities without coding
- ✅ Modify game content in a text editor
- ✅ Version control your entities
- ✅ Share entity templates
- ✅ Rapid prototyping

---

## 🚀 Quick Start

### Creating Your First Blueprint

Create a file `player.json` in the `Blueprints/` directory:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "Player",
    "description": "Main player character",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 100.0, "y": 100.0, "z": 0.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/player.png",
                "srcRect": {"x": 0.0, "y": 0.0, "w": 32.0, "h": 32.0},
                "hotSpot": {"x": 16.0, "y": 16.0}
            }
        },
        {
            "type": "Health",
            "properties": {
                "currentHealth": 100,
                "maxHealth": 100
            }
        }
    ]
}
```

### Loading in Code

```cpp
#include "EntityBlueprint.h"

// Load blueprint
EntityBlueprint player = EntityBlueprint::LoadFromFile("Blueprints/player.json");

// Use it to create entities in your game
// (Integration with ECS World coming in Phase 2)
```

---

## 📦 Quick Examples

### Simple Enemy

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "BasicEnemy",
    "description": "Simple patrol enemy",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 200.0, "y": 150.0, "z": 0.0}
            }
        },
        {
            "type": "Health",
            "properties": {
                "currentHealth": 50,
                "maxHealth": 50
            }
        },
        {
            "type": "AIBehavior",
            "properties": {
                "behaviorType": "patrol"
            }
        }
    ]
}
```

### Collectible Item

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "HealthPotion",
    "description": "Restores 25 health",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 300.0, "y": 200.0, "z": 0.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/potion.png",
                "srcRect": {"x": 0.0, "y": 0.0, "w": 16.0, "h": 16.0},
                "hotSpot": {"x": 8.0, "y": 8.0}
            }
        },
        {
            "type": "TriggerZone",
            "properties": {
                "radius": 20.0,
                "triggered": false
            }
        }
    ]
}
```

### NPC Vendor

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "ShopKeeper",
    "description": "Item vendor NPC",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 400.0, "y": 300.0, "z": 0.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/npc_vendor.png",
                "srcRect": {"x": 0.0, "y": 0.0, "w": 32.0, "h": 48.0},
                "hotSpot": {"x": 16.0, "y": 40.0}
            }
        },
        {
            "type": "NPC",
            "properties": {
                "npcType": "vendor"
            }
        },
        {
            "type": "Inventory",
            "properties": {
                "items": ["potion_health", "potion_mana", "scroll_fireball"]
            }
        }
    ]
}
```

---

## 🧩 Available Components

### Core Components
- **Position** - Entity location in 2D/3D space
- **BoundingBox** - Collision rectangle
- **VisualSprite** - Sprite rendering

### Physics Components
- **Movement** - Direction and velocity
- **PhysicsBody** - Mass and speed

### Gameplay Components
- **Health** - Hit points
- **AIBehavior** - AI behavior type (idle, patrol, chase, flee, wander)
- **TriggerZone** - Detection zone
- **Inventory** - Item storage

### Visual/Audio Components
- **Animation** - Sprite animation control
- **AudioSource** - Sound effects
- **FX** - Visual effects

### Input Components
- **Controller** - Gamepad input
- **PlayerController** - Player input states

### Other Components
- **Camera** - Camera control
- **NPC** - NPC type data

See [Blueprint System Guide](BLUEPRINT_SYSTEM.md) for detailed component properties.

---

## 🛠️ Using the Blueprint Editor

The visual Blueprint Editor makes creating blueprints even easier:

1. **Press F2** in-game to open the editor
2. **Browse existing blueprints** in the Asset Browser
3. **Create new blueprints** with the File menu
4. **Edit components visually** with the node editor
5. **Save** and test immediately

See: [Blueprint Editor Guide](../../05-Tools/BLUEPRINT_EDITOR.md)

---

## 📖 Complete Documentation

- **[Blueprint System Guide](BLUEPRINT_SYSTEM.md)** - Full component reference and file format
- **[Blueprint Architecture](../../02-Architecture/BLUEPRINT_ARCHITECTURE.md)** - System design and internals
- **[Blueprint Editor](../../05-Tools/BLUEPRINT_EDITOR.md)** - Visual editor documentation

---

## 🔄 Blueprint Workflow

```
Design Phase                Creation Phase              Runtime Phase
────────────                ──────────────              ─────────────

  Concept      ────►     JSON Blueprint    ────►     ECS Entity
   Idea                       or                      in World
                         Visual Editor
                         
    │                          │                          │
    │                          │                          │
    ▼                          ▼                          ▼
                                                          
 Game Design         Blueprint Editor           Game Engine
 Document            (F2 in-game)               CreateEntity()
                     Text Editor                AddComponents()
```

---

## 🎓 Learning Path

### For Beginners
1. ✅ Read this README
2. 📝 Create a simple blueprint (copy examples above)
3. 🎨 Try the Blueprint Editor (F2 in-game)
4. 📖 Read the [Blueprint System Guide](BLUEPRINT_SYSTEM.md)

### For Advanced Users
1. 📊 Study [Blueprint Architecture](../../02-Architecture/BLUEPRINT_ARCHITECTURE.md)
2. 🔧 Create custom component types (requires C++)
3. 🏗️ Integrate with game logic
4. 🚀 Build a blueprint library

---

## 💡 Best Practices

### Organization
- ✅ Group blueprints by type: `Blueprints/Enemies/`, `Blueprints/Items/`
- ✅ Use descriptive names: `enemy_goblin_warrior.json` not `enemy1.json`
- ✅ Add detailed descriptions in the blueprint

### Component Usage
- ✅ Only add components you need
- ✅ Use appropriate default values
- ✅ Keep properties organized and readable

### Version Control
- ✅ Commit blueprints to source control
- ✅ Use meaningful commit messages
- ✅ Review blueprint changes in PRs

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../../README.md)
- 🏗️ [Architecture Overview](../../02-Architecture/README.md)
- 🤖 [AI Systems](../../04-AI-Systems/README.md) - For AI behavior blueprints
- 🛠️ [Tools](../../05-Tools/README.md)

---

[← Back to Core Systems](../README.md) | [Next: Blueprint System Guide →](BLUEPRINT_SYSTEM.md)
