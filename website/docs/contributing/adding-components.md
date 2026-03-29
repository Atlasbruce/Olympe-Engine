---
id: adding-components
title: Adding ECS Components
sidebar_label: Adding Components
---

# Adding ECS Components Guide

## Table of Contents
- [Overview](#overview)
- [Complete Workflow](#complete-workflow)
- [Step 1: Define C++ Struct](#step-1-define-c-struct-in-ecs_componentsh)
- [Step 2: Add to ParameterSchemas.json](#step-2-add-to-parameterschemasjson)
- [Step 3: Update Prefabs](#step-3-update-prefabs)
- [Step 4: Add TMJ/TMX Parsing](#step-4-add-tmjtmx-parsing-tiledtoolympecpp)
- [Step 5: Create ECS System](#step-5-create-ecs-system)
- [Step 6: Register System](#step-6-register-system-worldcpp)
- [Step 7: Test and Debug](#step-7-test-and-debug)
- [Common Pitfalls](#common-pitfalls)
- [Complete Examples](#complete-examples)

---

## Overview

Adding a new ECS component to the Olympe Engine involves 7 coordinated steps across multiple files. This guide provides a complete walkthrough with examples.

### **Component Lifecycle**

1. **Definition**: Define C++ struct in `ECS_Components.h`
2. **Schema**: Add serialization schema to `ParameterSchemas.json`
3. **Prefabs**: Create/update entity prefab files (`.json`)
4. **Parsing**: Add Tiled map parsing logic (optional, for level editor)
5. **System**: Create ECS system to process component
6. **Registration**: Register system in `World::Initialize_ECS_Systems()`
7. **Testing**: Verify component behavior in-game

---

## Complete Workflow

```
┌─────────────────────────────────────────────────────────────┐
│  1. ECS_Components.h                                        │
│     Define struct MyComponent_data { ... }                  │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  2. ParameterSchemas.json                                   │
│     Add schema for MyComponent_data                         │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  3. Prefabs/*.json                                          │
│     Add component to entity prefabs                         │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  4. TiledToOlympe.cpp (optional)                            │
│     Parse component from Tiled custom properties            │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  5. ECS_Systems.h/cpp                                       │
│     Create MyComponentSystem : public ECS_System            │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  6. World.cpp                                               │
│     Register system: AddSystem<MyComponentSystem>()         │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  7. Test in-game                                            │
│     Create entities, verify system behavior                 │
└─────────────────────────────────────────────────────────────┘
```

---

## Step 1: Define C++ Struct in ECS_Components.h

**File**: `/Source/ECS_Components.h`

### **Component Naming Convention**

- Component structs end with `_data`
- Use descriptive names: `NavigationAgent_data`, `Health_data`, `Inventory_data`

### **Component Structure Template**

```cpp
// --- Component MyFeature Data ---
struct MyFeature_data
{
    // Fields
    float someValue = 1.0f;
    int someCount = 0;
    bool someFlag = false;
    std::string someText = "";
    Vector somePosition;
    std::vector<int> someArray;
    
    // Constructors (rule of three/five)
    MyFeature_data() = default;
    MyFeature_data(const MyFeature_data&) = default;
    MyFeature_data& operator=(const MyFeature_data&) = default;
};
```

### **Field Type Guidelines**

| C++ Type | Use Case | Default Value |
|----------|----------|---------------|
| `bool` | Flags, toggles | `false` |
| `int` | Counters, IDs | `0` |
| `float` | Physical values | `0.0f` or `1.0f` |
| `std::string` | Names, IDs, paths | `""` |
| `Vector` | Positions, directions | `Vector(0, 0, 0)` |
| `SDL_FRect` | Bounding boxes | `{ 0.f, 0.f, 0.f, 0.f }` |
| `std::vector<T>` | Dynamic arrays | `{}` |
| `uint8_t` | Bitflags, small ints | `0` |
| `EntityID` | Entity references | `INVALID_ENTITY_ID` |

### **Example 1: Simple Component**

```cpp
// --- Component Stamina Data ---
struct Stamina_data
{
    float currentStamina = 100.0f;
    float maxStamina = 100.0f;
    float regenRate = 5.0f;  // Stamina per second
    
    Stamina_data() = default;
    Stamina_data(const Stamina_data&) = default;
    Stamina_data& operator=(const Stamina_data&) = default;
};
```

### **Example 2: Complex Component**

```cpp
// --- Component Inventory Data (already exists, for reference) ---
struct Inventory_data
{
    std::vector<std::string> items; // List of item IDs in inventory
    int maxSlots = 10;
    int gold = 0;
    
    Inventory_data() = default;
    Inventory_data(const Inventory_data&) = default;
    Inventory_data& operator=(const Inventory_data&) = default;
    
    // Helper methods (optional)
    bool AddItem(const std::string& itemId)
    {
        if (items.size() >= maxSlots) return false;
        items.push_back(itemId);
        return true;
    }
    
    bool RemoveItem(const std::string& itemId)
    {
        auto it = std::find(items.begin(), items.end(), itemId);
        if (it == items.end()) return false;
        items.erase(it);
        return true;
    }
};
```

### **Example 3: Component with Enums**

```cpp
// --- Component Weapon Data ---
enum class WeaponType : uint8_t
{
    Melee,
    Ranged,
    Magic
};

struct Weapon_data
{
    WeaponType type = WeaponType::Melee;
    float damage = 10.0f;
    float attackSpeed = 1.0f;  // Attacks per second
    float range = 50.0f;       // Attack range
    int ammo = -1;             // -1 = infinite
    
    Weapon_data() = default;
    Weapon_data(const Weapon_data&) = default;
    Weapon_data& operator=(const Weapon_data&) = default;
};
```

---

## Step 2: Add to ParameterSchemas.json

**File**: `/Gamedata/EntityPrefab/ParameterSchemas.json`

### **Schema Structure**

```json
{
  "schemas": [
    {
      "componentType": "MyComponent_data",
      "parameters": [
        { "name": "fieldName", "type": "Type", "defaultValue": value }
      ]
    }
  ]
}
```

### **Parameter Types**

| C++ Type | JSON Type | Example Default |
|----------|-----------|-----------------|
| `bool` | `"Bool"` | `false` |
| `int` | `"Int"` | `0` |
| `float` | `"Float"` | `0.0` |
| `std::string` | `"String"` | `""` |
| `Vector` (3 floats) | `"Vector3"` | N/A (use 3 floats) |
| `Vector` (2 floats) | `"Vector2"` | `{"x": 0, "y": 0}` |
| `SDL_Color` | `"Color"` | `"#FFFFFF"` |
| `std::vector<T>` | `"Array"` | `[]` |

### **Example 1: Stamina Component Schema**

```json
{
  "componentType": "Stamina_data",
  "parameters": [
    { "name": "currentStamina", "type": "Float", "defaultValue": 100.0 },
    { "name": "maxStamina", "type": "Float", "defaultValue": 100.0 },
    { "name": "regenRate", "type": "Float", "defaultValue": 5.0 }
  ]
}
```

### **Example 2: Weapon Component Schema**

```json
{
  "componentType": "Weapon_data",
  "parameters": [
    { "name": "type", "type": "String", "defaultValue": "Melee" },
    { "name": "damage", "type": "Float", "defaultValue": 10.0 },
    { "name": "attackSpeed", "type": "Float", "defaultValue": 1.0 },
    { "name": "range", "type": "Float", "defaultValue": 50.0 },
    { "name": "ammo", "type": "Int", "defaultValue": -1 }
  ]
}
```

### **Example 3: Complex Schema with Nested Data**

```json
{
  "componentType": "Inventory_data",
  "parameters": [
    { "name": "items", "type": "Array", "defaultValue": [] },
    { "name": "maxSlots", "type": "Int", "defaultValue": 10 },
    { "name": "gold", "type": "Int", "defaultValue": 0 }
  ]
}
```

### **Existing NavigationAgent_data Schema (Reference)**

```json
{
  "componentType": "NavigationAgent_data",
  "parameters": [
    { "name": "agentRadius", "type": "Float", "defaultValue": 16.0 },
    { "name": "maxSpeed", "type": "Float", "defaultValue": 100.0 },
    { "name": "arrivalThreshold", "type": "Float", "defaultValue": 5.0 },
    { "name": "layerMask", "type": "Int", "defaultValue": 1 },
    { "name": "steeringWeight", "type": "Float", "defaultValue": 1.0 },
    { "name": "avoidObstacles", "type": "Bool", "defaultValue": true }
  ]
}
```

---

## Step 3: Update Prefabs

**Directory**: `/Gamedata/EntityPrefab/*.json`

### **Prefab Structure**

```json
{
  "prefabName": "MyEntityPrefab",
  "components": [
    {
      "type": "Identity_data",
      "parameters": {
        "name": "MyEntity",
        "tag": "Enemy",
        "entityType": "Enemy"
      }
    },
    {
      "type": "Position_data",
      "parameters": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
      }
    },
    {
      "type": "MyComponent_data",
      "parameters": {
        "field1": value1,
        "field2": value2
      }
    }
  ]
}
```

### **Example 1: Enemy with Stamina**

**File**: `/Gamedata/EntityPrefab/Enemy_Warrior.json`

```json
{
  "prefabName": "Enemy_Warrior",
  "components": [
    {
      "type": "Identity_data",
      "parameters": {
        "name": "Warrior",
        "tag": "Enemy",
        "entityType": "Enemy"
      }
    },
    {
      "type": "Position_data",
      "parameters": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
      }
    },
    {
      "type": "Health_data",
      "parameters": {
        "maxHealth": 150,
        "currentHealth": 150
      }
    },
    {
      "type": "Stamina_data",
      "parameters": {
        "currentStamina": 100.0,
        "maxStamina": 100.0,
        "regenRate": 5.0
      }
    },
    {
      "type": "Weapon_data",
      "parameters": {
        "type": "Melee",
        "damage": 25.0,
        "attackSpeed": 1.5,
        "range": 50.0,
        "ammo": -1
      }
    },
    {
      "type": "NavigationAgent_data",
      "parameters": {
        "agentRadius": 16.0,
        "maxSpeed": 80.0,
        "arrivalThreshold": 5.0,
        "layerMask": 1,
        "avoidObstacles": true
      }
    }
  ]
}
```

### **Example 2: NPC with Inventory**

**File**: `/Gamedata/EntityPrefab/Merchant_NPC.json`

```json
{
  "prefabName": "Merchant_NPC",
  "components": [
    {
      "type": "Identity_data",
      "parameters": {
        "name": "Merchant",
        "tag": "NPC",
        "entityType": "NPC"
      }
    },
    {
      "type": "Position_data",
      "parameters": {
        "x": 500.0,
        "y": 300.0,
        "z": 0.0
      }
    },
    {
      "type": "Inventory_data",
      "parameters": {
        "items": ["potion_health", "sword_iron", "shield_wooden"],
        "maxSlots": 20,
        "gold": 500
      }
    },
    {
      "type": "VisualSprite_data",
      "parameters": {
        "spritePath": "Resources/merchant.png",
        "width": 32,
        "height": 32
      }
    }
  ]
}
```

---

## Step 4: Add TMJ/TMX Parsing (TiledToOlympe.cpp)

**File**: `/Source/TiledLevelLoader/src/TiledToOlympe.cpp`

### **When to Add Parsing**

Add parsing logic if you want to set component values from Tiled custom properties.

### **Tiled Custom Property Flow**

```
Tiled Object → Custom Property → TiledToOlympe.cpp → ECS Component
```

### **Parsing Template**

```cpp
// In TiledToOlympe::ConvertObjects() or similar method

// Check for custom property
auto it = obj.properties.find("myProperty");
if (it != obj.properties.end())
{
    // Parse property value
    std::string value = it->second.value;
    
    // Set component field
    if (World::Get().HasComponent<MyComponent_data>(entityID))
    {
        MyComponent_data& comp = World::Get().GetComponent<MyComponent_data>(entityID);
        comp.myField = ParseValue(value);
    }
}
```

### **Example 1: Parse Navigation Layer**

```cpp
// Parse "layer" custom property for NavigationAgent_data
auto layerProp = obj.properties.find("layer");
if (layerProp != obj.properties.end())
{
    std::string layerStr = layerProp->second.value;
    
    if (World::Get().HasComponent<NavigationAgent_data>(entityID))
    {
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entityID);
        
        // Convert string to layer mask
        if (layerStr == "ground") {
            agent.layerMask = 0x01;
        } else if (layerStr == "sky") {
            agent.layerMask = 0x02;
        } else if (layerStr == "underground") {
            agent.layerMask = 0x04;
        }
    }
}
```

### **Example 2: Parse Weapon Type**

```cpp
// Parse "weaponType" custom property for Weapon_data
auto weaponProp = obj.properties.find("weaponType");
if (weaponProp != obj.properties.end())
{
    std::string weaponStr = weaponProp->second.value;
    
    if (World::Get().HasComponent<Weapon_data>(entityID))
    {
        Weapon_data& weapon = World::Get().GetComponent<Weapon_data>(entityID);
        
        if (weaponStr == "melee") {
            weapon.type = WeaponType::Melee;
        } else if (weaponStr == "ranged") {
            weapon.type = WeaponType::Ranged;
        } else if (weaponStr == "magic") {
            weapon.type = WeaponType::Magic;
        }
    }
}
```

### **Example 3: Parse Multiple Properties**

```cpp
// Parse all stamina properties
auto currentStaminaProp = obj.properties.find("currentStamina");
auto maxStaminaProp = obj.properties.find("maxStamina");
auto regenRateProp = obj.properties.find("regenRate");

if (World::Get().HasComponent<Stamina_data>(entityID))
{
    Stamina_data& stamina = World::Get().GetComponent<Stamina_data>(entityID);
    
    if (currentStaminaProp != obj.properties.end()) {
        stamina.currentStamina = std::stof(currentStaminaProp->second.value);
    }
    
    if (maxStaminaProp != obj.properties.end()) {
        stamina.maxStamina = std::stof(maxStaminaProp->second.value);
    }
    
    if (regenRateProp != obj.properties.end()) {
        stamina.regenRate = std::stof(regenRateProp->second.value);
    }
}
```

---

## Step 5: Create ECS System

**Files**: `/Source/ECS_Systems.h` and `/Source/ECS_Systems.cpp`

### **System Template**

**ECS_Systems.h**:
```cpp
// MyComponentSystem: processes entities with MyComponent_data
class MyComponentSystem : public ECS_System
{
public:
    MyComponentSystem();
    virtual void Process() override;
    
private:
    // Helper methods
    void ProcessEntity(EntityID entity, float deltaTime);
};
```

**ECS_Systems.cpp**:
```cpp
MyComponentSystem::MyComponentSystem()
{
    // Define required signature
    requiredSignature.SetBit(World::Get().GetComponentID<MyComponent_data>());
    // Add other required components
    // requiredSignature.SetBit(World::Get().GetComponentID<Position_data>());
}

void MyComponentSystem::Process()
{
    float deltaTime = GameEngine::Get().GetDeltaTime();
    
    for (EntityID entity : m_entities)
    {
        ProcessEntity(entity, deltaTime);
    }
}

void MyComponentSystem::ProcessEntity(EntityID entity, float deltaTime)
{
    MyComponent_data& comp = World::Get().GetComponent<MyComponent_data>(entity);
    
    // System logic here
}
```

### **Example 1: Stamina System**

**ECS_Systems.h**:
```cpp
class StaminaSystem : public ECS_System
{
public:
    StaminaSystem();
    virtual void Process() override;
};
```

**ECS_Systems.cpp**:
```cpp
StaminaSystem::StaminaSystem()
{
    requiredSignature.SetBit(World::Get().GetComponentID<Stamina_data>());
}

void StaminaSystem::Process()
{
    float deltaTime = GameEngine::Get().GetDeltaTime();
    
    for (EntityID entity : m_entities)
    {
        Stamina_data& stamina = World::Get().GetComponent<Stamina_data>(entity);
        
        // Regenerate stamina over time
        if (stamina.currentStamina < stamina.maxStamina)
        {
            stamina.currentStamina += stamina.regenRate * deltaTime;
            
            // Clamp to max
            if (stamina.currentStamina > stamina.maxStamina) {
                stamina.currentStamina = stamina.maxStamina;
            }
        }
    }
}
```

### **Example 2: Weapon System**

**ECS_Systems.h**:
```cpp
class WeaponSystem : public ECS_System
{
public:
    WeaponSystem();
    virtual void Process() override;
    
private:
    void TryAttack(EntityID entity);
    bool IsInRange(const Vector& attackerPos, const Vector& targetPos, float range);
    void ApplyDamage(EntityID target, float damage);
};
```

**ECS_Systems.cpp**:
```cpp
WeaponSystem::WeaponSystem()
{
    requiredSignature.SetBit(World::Get().GetComponentID<Weapon_data>());
    requiredSignature.SetBit(World::Get().GetComponentID<Position_data>());
    requiredSignature.SetBit(World::Get().GetComponentID<AttackIntent_data>());
}

void WeaponSystem::Process()
{
    for (EntityID entity : m_entities)
    {
        AttackIntent_data& intent = World::Get().GetComponent<AttackIntent_data>(entity);
        
        if (intent.hasIntent && intent.isAttacking)
        {
            TryAttack(entity);
            intent.hasIntent = false; // Consume intent
        }
    }
}

void WeaponSystem::TryAttack(EntityID entity)
{
    Weapon_data& weapon = World::Get().GetComponent<Weapon_data>(entity);
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    
    // Find nearest target
    EntityID target = FindNearestEnemy(entity);
    
    if (target == INVALID_ENTITY_ID) return;
    
    Position_data& targetPos = World::Get().GetComponent<Position_data>(target);
    
    // Check range
    if (!IsInRange(position.position, targetPos.position, weapon.range)) {
        return; // Out of range
    }
    
    // Apply damage
    ApplyDamage(target, weapon.damage);
    
    // Consume ammo (if applicable)
    if (weapon.ammo > 0) {
        --weapon.ammo;
    }
}

bool WeaponSystem::IsInRange(const Vector& attackerPos, const Vector& targetPos, float range)
{
    float distance = (targetPos - attackerPos).Length();
    return distance <= range;
}

void WeaponSystem::ApplyDamage(EntityID target, float damage)
{
    if (World::Get().HasComponent<Health_data>(target))
    {
        Health_data& health = World::Get().GetComponent<Health_data>(target);
        health.currentHealth -= static_cast<int>(damage);
        
        if (health.currentHealth <= 0) {
            // Entity died
            OnEntityDeath(target);
        }
    }
}
```

### **Example 3: Inventory System**

**ECS_Systems.h**:
```cpp
class InventorySystem : public ECS_System
{
public:
    InventorySystem();
    virtual void Process() override;
    
    // Public API for adding/removing items
    bool AddItem(EntityID entity, const std::string& itemId);
    bool RemoveItem(EntityID entity, const std::string& itemId);
    bool HasItem(EntityID entity, const std::string& itemId);
};
```

**ECS_Systems.cpp**:
```cpp
InventorySystem::InventorySystem()
{
    requiredSignature.SetBit(World::Get().GetComponentID<Inventory_data>());
}

void InventorySystem::Process()
{
    // Inventory system is mostly event-driven, not frame-based
    // Process() can be empty or handle periodic updates (e.g., item decay)
}

bool InventorySystem::AddItem(EntityID entity, const std::string& itemId)
{
    if (!World::Get().HasComponent<Inventory_data>(entity)) {
        return false;
    }
    
    Inventory_data& inventory = World::Get().GetComponent<Inventory_data>(entity);
    
    if (inventory.items.size() >= inventory.maxSlots) {
        // Inventory full
        return false;
    }
    
    inventory.items.push_back(itemId);
    return true;
}

bool InventorySystem::RemoveItem(EntityID entity, const std::string& itemId)
{
    if (!World::Get().HasComponent<Inventory_data>(entity)) {
        return false;
    }
    
    Inventory_data& inventory = World::Get().GetComponent<Inventory_data>(entity);
    
    auto it = std::find(inventory.items.begin(), inventory.items.end(), itemId);
    if (it == inventory.items.end()) {
        return false; // Item not found
    }
    
    inventory.items.erase(it);
    return true;
}

bool InventorySystem::HasItem(EntityID entity, const std::string& itemId)
{
    if (!World::Get().HasComponent<Inventory_data>(entity)) {
        return false;
    }
    
    Inventory_data& inventory = World::Get().GetComponent<Inventory_data>(entity);
    
    return std::find(inventory.items.begin(), inventory.items.end(), itemId) 
           != inventory.items.end();
}
```

---

## Step 6: Register System (World.cpp)

**File**: `/Source/World.cpp`

### **Registration Location**

In `World::Initialize_ECS_Systems()`, add your system in the appropriate processing order.

### **System Order Matters!**

```cpp
void World::Initialize_ECS_Systems()
{
    // 1. Input systems (consume events)
    AddSystem<InputEventConsumeSystem>();
    AddSystem<GameEventConsumeSystem>();
    AddSystem<UIEventConsumeSystem>();
    AddSystem<CameraEventConsumeSystem>();
    
    // 2. Input processing
    AddSystem<InputSystem>();
    AddSystem<InputMappingSystem>();
    AddSystem<PlayerControlSystem>();
    
    // 3. AI systems
    AddSystem<AIStimuliSystem>();
    AddSystem<AIPerceptionSystem>();
    AddSystem<AIStateTransitionSystem>();
    AddSystem<BehaviorTreeSystem>();
    
    // 4. Movement intent systems
    AddSystem<NavigationSystem>();
    AddSystem<MovementIntentSystem>();
    
    // 5. Physics and collision
    AddSystem<PhysicsSystem>();
    AddSystem<CollisionSystem>();
    
    // 6. Gameplay systems
    AddSystem<StaminaSystem>();      // ← Add here
    AddSystem<WeaponSystem>();       // ← Add here
    AddSystem<InventorySystem>();    // ← Add here
    
    // 7. Camera
    AddSystem<CameraSystem>();
    
    // 8. Rendering systems (separate pass)
    AddSystem<RenderingSystem>();
    AddSystem<GridSystem>();
    AddSystem<RenderingEditorSystem>();
    AddSystem<UIRenderingSystem>();
}
```

### **Example Registration**

```cpp
void World::Initialize_ECS_Systems()
{
    // ... existing systems ...
    
    // Add custom systems
    AddSystem<StaminaSystem>();
    AddSystem<WeaponSystem>();
    AddSystem<InventorySystem>();
    
    // ... remaining systems ...
}
```

---

## Step 7: Test and Debug

### **Testing Checklist**

- [ ] Component appears in prefab editor
- [ ] Component loads from prefab JSON
- [ ] Component serializes/deserializes correctly
- [ ] System processes entities with component
- [ ] Component updates correctly in-game
- [ ] No crashes or memory leaks
- [ ] Component interacts correctly with other systems

### **Debugging Tools**

#### **1. Print Component State**

```cpp
void DebugPrintStamina(EntityID entity)
{
    if (World::Get().HasComponent<Stamina_data>(entity))
    {
        Stamina_data& stamina = World::Get().GetComponent<Stamina_data>(entity);
        
        std::cout << "Entity " << entity << " Stamina: "
                  << stamina.currentStamina << "/" << stamina.maxStamina
                  << " (regen: " << stamina.regenRate << "/s)\n";
    }
}
```

#### **2. Verify Component Signature**

```cpp
void DebugCheckSignature(EntityID entity)
{
    ComponentSignature sig = World::Get().GetEntitySignature(entity);
    
    std::cout << "Entity " << entity << " has components:\n";
    
    if (sig.HasBit(World::Get().GetComponentID<Position_data>())) {
        std::cout << "  - Position_data\n";
    }
    if (sig.HasBit(World::Get().GetComponentID<Stamina_data>())) {
        std::cout << "  - Stamina_data\n";
    }
    // ... check other components
}
```

#### **3. System Entity Count**

```cpp
void DebugSystemEntityCount()
{
    StaminaSystem* staminaSystem = World::Get().GetSystem<StaminaSystem>();
    
    std::cout << "StaminaSystem processing " 
              << staminaSystem->m_entities.size() << " entities\n";
}
```

#### **4. Validate Prefab Loading**

```cpp
void DebugValidatePrefab(const std::string& prefabPath)
{
    EntityID entity = PrefabFactory::Get().CreateFromFile(prefabPath);
    
    std::cout << "Loaded prefab: " << prefabPath << "\n";
    std::cout << "Entity ID: " << entity << "\n";
    
    DebugCheckSignature(entity);
    
    // Cleanup
    World::Get().DestroyEntity(entity);
}
```

---

## Common Pitfalls

### **Pitfall 1: Forgot to Add Schema**

**Symptom**: Component loads with default values, ignoring prefab JSON

**Solution**: Add schema to `ParameterSchemas.json`

```json
{
  "componentType": "MyComponent_data",
  "parameters": [
    { "name": "myField", "type": "Float", "defaultValue": 1.0 }
  ]
}
```

---

### **Pitfall 2: Forgot to Register System**

**Symptom**: Component exists but doesn't update

**Solution**: Register system in `World::Initialize_ECS_Systems()`

```cpp
AddSystem<MyComponentSystem>();
```

---

### **Pitfall 3: Wrong Component Signature**

**Symptom**: System doesn't process any entities

**Solution**: Verify signature in system constructor

```cpp
MySystem::MySystem()
{
    requiredSignature.SetBit(World::Get().GetComponentID<MyComponent_data>());
    // Add all required components
}
```

---

### **Pitfall 4: Component Not in Prefab**

**Symptom**: Component missing on spawned entities

**Solution**: Add component to prefab JSON

```json
{
  "type": "MyComponent_data",
  "parameters": { ... }
}
```

---

### **Pitfall 5: Type Mismatch in Schema**

**Symptom**: Parsing errors or wrong values

**Solution**: Match C++ type to JSON type

| C++ | JSON Type |
|-----|-----------|
| `float` | `"Float"` |
| `int` | `"Int"` |
| `bool` | `"Bool"` |
| `std::string` | `"String"` |

---

### **Pitfall 6: System Order Issues**

**Symptom**: Component updates in wrong order (e.g., movement before physics)

**Solution**: Register systems in correct order in `World::Initialize_ECS_Systems()`

---

### **Pitfall 7: Missing Constructors**

**Symptom**: Compiler errors when copying component

**Solution**: Add rule-of-three constructors

```cpp
MyComponent_data() = default;
MyComponent_data(const MyComponent_data&) = default;
MyComponent_data& operator=(const MyComponent_data&) = default;
```

---

### **Pitfall 8: Forgetting to Check HasComponent**

**Symptom**: Crashes when accessing component

**Solution**: Always check before accessing

```cpp
if (World::Get().HasComponent<MyComponent_data>(entity))
{
    MyComponent_data& comp = World::Get().GetComponent<MyComponent_data>(entity);
    // Safe to use
}
```

---

## Complete Examples

### **Example 1: Shield Component (Simple)**

#### **1. ECS_Components.h**
```cpp
// --- Component Shield Data ---
struct Shield_data
{
    float currentShield = 50.0f;
    float maxShield = 50.0f;
    float regenRate = 2.0f;      // Shield per second
    float regenDelay = 3.0f;     // Seconds before regen starts
    float timeSinceHit = 0.0f;   // Time since last damage
    
    Shield_data() = default;
    Shield_data(const Shield_data&) = default;
    Shield_data& operator=(const Shield_data&) = default;
};
```

#### **2. ParameterSchemas.json**
```json
{
  "componentType": "Shield_data",
  "parameters": [
    { "name": "currentShield", "type": "Float", "defaultValue": 50.0 },
    { "name": "maxShield", "type": "Float", "defaultValue": 50.0 },
    { "name": "regenRate", "type": "Float", "defaultValue": 2.0 },
    { "name": "regenDelay", "type": "Float", "defaultValue": 3.0 }
  ]
}
```

#### **3. Prefab: Enemy_Shielded.json**
```json
{
  "prefabName": "Enemy_Shielded",
  "components": [
    {
      "type": "Identity_data",
      "parameters": {
        "name": "Shielded Enemy",
        "tag": "Enemy",
        "entityType": "Enemy"
      }
    },
    {
      "type": "Position_data",
      "parameters": { "x": 0.0, "y": 0.0, "z": 0.0 }
    },
    {
      "type": "Health_data",
      "parameters": {
        "maxHealth": 100,
        "currentHealth": 100
      }
    },
    {
      "type": "Shield_data",
      "parameters": {
        "currentShield": 50.0,
        "maxShield": 50.0,
        "regenRate": 2.0,
        "regenDelay": 3.0
      }
    }
  ]
}
```

#### **4. ECS_Systems.h**
```cpp
class ShieldSystem : public ECS_System
{
public:
    ShieldSystem();
    virtual void Process() override;
    
    void DamageShield(EntityID entity, float damage);
};
```

#### **5. ECS_Systems.cpp**
```cpp
ShieldSystem::ShieldSystem()
{
    requiredSignature.SetBit(World::Get().GetComponentID<Shield_data>());
}

void ShieldSystem::Process()
{
    float deltaTime = GameEngine::Get().GetDeltaTime();
    
    for (EntityID entity : m_entities)
    {
        Shield_data& shield = World::Get().GetComponent<Shield_data>(entity);
        
        // Update timer
        shield.timeSinceHit += deltaTime;
        
        // Regenerate shield after delay
        if (shield.timeSinceHit >= shield.regenDelay)
        {
            if (shield.currentShield < shield.maxShield)
            {
                shield.currentShield += shield.regenRate * deltaTime;
                
                if (shield.currentShield > shield.maxShield) {
                    shield.currentShield = shield.maxShield;
                }
            }
        }
    }
}

void ShieldSystem::DamageShield(EntityID entity, float damage)
{
    if (!World::Get().HasComponent<Shield_data>(entity)) return;
    
    Shield_data& shield = World::Get().GetComponent<Shield_data>(entity);
    
    shield.currentShield -= damage;
    shield.timeSinceHit = 0.0f; // Reset regen timer
    
    if (shield.currentShield < 0.0f)
    {
        // Shield depleted - overflow damage goes to health
        float overflow = -shield.currentShield;
        shield.currentShield = 0.0f;
        
        if (World::Get().HasComponent<Health_data>(entity))
        {
            Health_data& health = World::Get().GetComponent<Health_data>(entity);
            health.currentHealth -= static_cast<int>(overflow);
        }
    }
}
```

#### **6. World.cpp**
```cpp
void World::Initialize_ECS_Systems()
{
    // ... existing systems ...
    AddSystem<ShieldSystem>();
    // ... remaining systems ...
}
```

---

### **Example 2: Quest Component (Complex)**

#### **1. ECS_Components.h**
```cpp
// --- Component Quest Data ---
enum class QuestStatus : uint8_t
{
    NotStarted,
    InProgress,
    Completed,
    Failed
};

struct Quest_data
{
    std::string questId = "";
    std::string questName = "";
    QuestStatus status = QuestStatus::NotStarted;
    std::vector<std::string> objectives;
    std::vector<bool> objectivesCompleted;
    int rewardGold = 0;
    std::vector<std::string> rewardItems;
    
    Quest_data() = default;
    Quest_data(const Quest_data&) = default;
    Quest_data& operator=(const Quest_data&) = default;
    
    bool IsComplete() const
    {
        if (objectives.size() != objectivesCompleted.size()) return false;
        
        for (bool completed : objectivesCompleted) {
            if (!completed) return false;
        }
        
        return true;
    }
};
```

#### **2. ParameterSchemas.json**
```json
{
  "componentType": "Quest_data",
  "parameters": [
    { "name": "questId", "type": "String", "defaultValue": "" },
    { "name": "questName", "type": "String", "defaultValue": "" },
    { "name": "status", "type": "String", "defaultValue": "NotStarted" },
    { "name": "objectives", "type": "Array", "defaultValue": [] },
    { "name": "rewardGold", "type": "Int", "defaultValue": 0 },
    { "name": "rewardItems", "type": "Array", "defaultValue": [] }
  ]
}
```

#### **3. Prefab: Quest_Rescue.json**
```json
{
  "prefabName": "Quest_Rescue",
  "components": [
    {
      "type": "Quest_data",
      "parameters": {
        "questId": "rescue_villager",
        "questName": "Rescue the Villager",
        "status": "NotStarted",
        "objectives": [
          "Find the villager",
          "Defeat the bandits",
          "Return to town"
        ],
        "rewardGold": 100,
        "rewardItems": ["potion_health", "key_silver"]
      }
    }
  ]
}
```

#### **4-6. System, Registration, Testing**
(Similar pattern to Shield example)

---

## Summary

This guide covered the complete workflow for adding ECS components:

✅ **Step 1**: Define C++ struct in `ECS_Components.h`  
✅ **Step 2**: Add schema to `ParameterSchemas.json`  
✅ **Step 3**: Update prefab JSON files  
✅ **Step 4**: Add Tiled parsing (optional)  
✅ **Step 5**: Create ECS system  
✅ **Step 6**: Register system in `World.cpp`  
✅ **Step 7**: Test and debug  

**Common pitfalls** and solutions provided.

For more details, see:
- [Navigation System Reference](Navigation_System_Reference.md)
- [Pathfinding Integration Guide](Pathfinding_Integration_Guide.md)
- [Collision Types Reference](Collision_Types_Reference.md)
