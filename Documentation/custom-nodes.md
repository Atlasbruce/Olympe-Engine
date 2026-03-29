---
id: bt-custom-nodes
title: Creating Custom Behavior Tree Nodes
sidebar_label: Custom Nodes
sidebar_position: 2
---

# Creating Custom Behavior Tree Nodes

**Version:** 2.0  
**Last Updated:** 2025-02-13

## Overview

Olympe Engine's behavior tree system is designed to be extensible. While the engine provides a comprehensive set of built-in nodes, you can create custom condition and action nodes to implement game-specific AI behaviors.

This guide covers:
- **Adding custom condition types** for game-specific checks
- **Adding custom action types** for game-specific behaviors
- **Extending the behavior tree system** without modifying core code
- **Best practices** for maintainable custom nodes

## Architecture Overview

### Core Components

```cpp
// Source/AI/BehaviorTree.h
enum class BTConditionType : uint8_t { /* Built-in conditions */ };
enum class BTActionType : uint8_t { /* Built-in actions */ };

BTStatus ExecuteBTCondition(BTConditionType condType, float param, 
                            EntityID entity, const AIBlackboard_data& blackboard);
BTStatus ExecuteBTAction(BTActionType actionType, float param1, float param2, 
                         EntityID entity, AIBlackboard_data& blackboard);
```

### Execution Flow

```
JSON Behavior Tree
        ↓
BehaviorTreeManager::LoadTreeFromFile()
        ↓
BTNode structures created
        ↓
ExecuteBTNode() → ExecuteBTCondition() / ExecuteBTAction()
        ↓
Your custom logic
```

---

## Adding Custom Condition Types

Custom conditions allow you to check game-specific state that isn't covered by built-in conditions.

### Step 1: Extend BTConditionType Enum

**File:** `Source/AI/BehaviorTree.h`

```cpp
enum class BTConditionType : uint8_t
{
    // ... existing built-in conditions ...
    
    // YOUR CUSTOM CONDITIONS (add at end to preserve compatibility)
    HasInventoryItem = 50,      // Check if entity has item
    IsInSafeZone,                // Check if in safe zone
    HasEnoughMana,               // Check mana level
    IsNearAlly,                  // Check for nearby allies
    IsQuestActive,               // Check quest state
};
```

:::tip Enum Values
Start your custom conditions at 50+ to avoid conflicts with future built-in conditions.
:::

### Step 2: Implement Condition Logic

**File:** `Source/AI/BehaviorTree.cpp`

Add your cases to `ExecuteBTCondition()`:

```cpp
BTStatus ExecuteBTCondition(BTConditionType condType, float param, 
                            EntityID entity, const AIBlackboard_data& blackboard)
{
    switch (condType)
    {
        // ... existing built-in conditions ...
        
        // YOUR CUSTOM CONDITIONS
        case BTConditionType::HasInventoryItem:
        {
            // param is item ID to check
            int itemId = static_cast<int>(param);
            
            if (World::Get().HasComponent<Inventory_data>(entity))
            {
                const Inventory_data& inventory = World::Get().GetComponent<Inventory_data>(entity);
                
                // Check if inventory contains item
                for (const InventorySlot& slot : inventory.slots)
                {
                    if (slot.itemId == itemId && slot.quantity > 0)
                    {
                        return BTStatus::Success;
                    }
                }
            }
            return BTStatus::Failure;
        }
        
        case BTConditionType::IsInSafeZone:
        {
            if (World::Get().HasComponent<Position_data>(entity))
            {
                const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                
                // Check if position is in any safe zone (using collision map custom flags)
                int gridX, gridY;
                CollisionMap::Get().WorldToGrid(pos.position.x, pos.position.y, gridX, gridY);
                
                if (CollisionMap::Get().IsValidGridPosition(gridX, gridY))
                {
                    const TileProperties& tile = CollisionMap::Get().GetTileProperties(gridX, gridY);
                    
                    // Custom flag bit 0 = safe zone
                    bool isSafeZone = (tile.customFlags & 0x01) != 0;
                    return isSafeZone ? BTStatus::Success : BTStatus::Failure;
                }
            }
            return BTStatus::Failure;
        }
        
        case BTConditionType::HasEnoughMana:
        {
            // param is minimum mana percentage (0.0 to 1.0)
            if (World::Get().HasComponent<Mana_data>(entity))
            {
                const Mana_data& mana = World::Get().GetComponent<Mana_data>(entity);
                float manaPercent = static_cast<float>(mana.current) / 
                                   static_cast<float>(mana.maximum);
                return (manaPercent >= param) ? BTStatus::Success : BTStatus::Failure;
            }
            return BTStatus::Failure;
        }
        
        case BTConditionType::IsNearAlly:
        {
            // param is search radius
            if (!World::Get().HasComponent<Position_data>(entity))
                return BTStatus::Failure;
            
            const Position_data& myPos = World::Get().GetComponent<Position_data>(entity);
            
            // Query all entities with AIController and Position
            auto entities = World::Get().GetAllEntitiesWithComponents<AIController_data, Position_data>();
            
            for (EntityID other : entities)
            {
                if (other == entity) continue; // Skip self
                
                // Check if same faction (assuming Faction_data component)
                if (World::Get().HasComponent<Faction_data>(entity) && 
                    World::Get().HasComponent<Faction_data>(other))
                {
                    const Faction_data& myFaction = World::Get().GetComponent<Faction_data>(entity);
                    const Faction_data& otherFaction = World::Get().GetComponent<Faction_data>(other);
                    
                    if (myFaction.factionId == otherFaction.factionId)
                    {
                        // Same faction - check distance
                        const Position_data& otherPos = World::Get().GetComponent<Position_data>(other);
                        float dist = (myPos.position - otherPos.position).Magnitude();
                        
                        if (dist <= param)
                        {
                            return BTStatus::Success;
                        }
                    }
                }
            }
            return BTStatus::Failure;
        }
        
        case BTConditionType::IsQuestActive:
        {
            // param is quest ID
            int questId = static_cast<int>(param);
            
            // Access quest system (assuming singleton)
            QuestManager& questMgr = QuestManager::Get();
            return questMgr.IsQuestActive(questId) ? BTStatus::Success : BTStatus::Failure;
        }
    }
    
    return BTStatus::Failure;
}
```

### Step 3: Add JSON Support

**File:** `Source/AI/BehaviorTree.cpp` (in `LoadTreeFromFile()`)

```cpp
// Parse condition type
if (node.type == BTNodeType::Condition && nodeJson.contains("conditionType"))
{
    std::string condStr = JsonHelper::GetString(nodeJson, "conditionType", "");
    
    // ... existing built-in mappings ...
    
    // YOUR CUSTOM MAPPINGS
    else if (condStr == "HasInventoryItem")
        node.conditionType = BTConditionType::HasInventoryItem;
    else if (condStr == "IsInSafeZone")
        node.conditionType = BTConditionType::IsInSafeZone;
    else if (condStr == "HasEnoughMana")
        node.conditionType = BTConditionType::HasEnoughMana;
    else if (condStr == "IsNearAlly")
        node.conditionType = BTConditionType::IsNearAlly;
    else if (condStr == "IsQuestActive")
        node.conditionType = BTConditionType::IsQuestActive;
    
    // Handle parameters (v2 format)
    if (isV2 && nodeJson.contains("parameters") && nodeJson["parameters"].is_object())
    {
        const json& params = nodeJson["parameters"];
        node.conditionParam = JsonHelper::GetFloat(params, "param", 0.0f);
    }
    else
    {
        // v1 format (flat)
        node.conditionParam = JsonHelper::GetFloat(nodeJson, "param", 0.0f);
    }
}
```

### Step 4: Use in Behavior Tree JSON

```json
{
  "schema_version": 2,
  "name": "MerchantAI",
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "Selector",
        "name": "Root",
        "children": [2, 5, 8]
      },
      {
        "id": 2,
        "type": "Sequence",
        "name": "Flee to Safety",
        "children": [3, 4]
      },
      {
        "id": 3,
        "type": "Inverter",
        "name": "NOT in Safe Zone",
        "decoratorChildId": 10
      },
      {
        "id": 10,
        "type": "Condition",
        "name": "In Safe Zone?",
        "conditionType": "IsInSafeZone"
      },
      {
        "id": 4,
        "type": "Action",
        "name": "Run to Safe Zone",
        "actionType": "FleeToSafety"
      },
      {
        "id": 5,
        "type": "Sequence",
        "name": "Cast Spell if Mana",
        "children": [6, 7]
      },
      {
        "id": 6,
        "type": "Condition",
        "name": "Has 50% Mana?",
        "conditionType": "HasEnoughMana",
        "parameters": { "param": 0.5 }
      },
      {
        "id": 7,
        "type": "Action",
        "name": "Cast Fireball",
        "actionType": "CastSpell",
        "parameters": { "param1": 1.0 }
      },
      {
        "id": 8,
        "type": "Action",
        "name": "Idle",
        "actionType": "Idle"
      }
    ]
  }
}
```

---

## Adding Custom Action Types

Custom actions allow you to implement game-specific behaviors.

### Step 1: Extend BTActionType Enum

**File:** `Source/AI/BehaviorTree.h`

```cpp
enum class BTActionType : uint8_t
{
    // ... existing built-in actions ...
    
    // YOUR CUSTOM ACTIONS (add at end)
    CastSpell = 50,              // Cast spell by ID
    UseItem,                     // Use item from inventory
    FleeToSafety,                // Run to nearest safe zone
    CallForHelp,                 // Alert nearby allies
    PlayAnimation,               // Trigger animation
    EmitSound,                   // Play sound effect
};
```

### Step 2: Implement Action Logic

**File:** `Source/AI/BehaviorTree.cpp`

Add your cases to `ExecuteBTAction()`:

```cpp
BTStatus ExecuteBTAction(BTActionType actionType, float param1, float param2, 
                         EntityID entity, AIBlackboard_data& blackboard)
{
    switch (actionType)
    {
        // ... existing built-in actions ...
        
        // YOUR CUSTOM ACTIONS
        case BTActionType::CastSpell:
        {
            // param1 = spell ID
            int spellId = static_cast<int>(param1);
            
            if (!World::Get().HasComponent<Mana_data>(entity))
                return BTStatus::Failure;
            
            Mana_data& mana = World::Get().GetComponent<Mana_data>(entity);
            
            // Get spell data from spell system
            SpellData* spell = SpellDatabase::Get().GetSpell(spellId);
            if (!spell)
                return BTStatus::Failure;
            
            // Check mana cost
            if (mana.current < spell->manaCost)
                return BTStatus::Failure;
            
            // Cast spell (assuming SpellCaster component)
            if (World::Get().HasComponent<SpellCaster_data>(entity))
            {
                SpellCaster_data& caster = World::Get().GetComponent<SpellCaster_data>(entity);
                caster.queuedSpellId = spellId;
                caster.hasCastIntent = true;
                
                // Deduct mana
                mana.current -= spell->manaCost;
                
                return BTStatus::Success;
            }
            
            return BTStatus::Failure;
        }
        
        case BTActionType::UseItem:
        {
            // param1 = item ID
            int itemId = static_cast<int>(param1);
            
            if (!World::Get().HasComponent<Inventory_data>(entity))
                return BTStatus::Failure;
            
            Inventory_data& inventory = World::Get().GetComponent<Inventory_data>(entity);
            
            // Find item in inventory
            for (InventorySlot& slot : inventory.slots)
            {
                if (slot.itemId == itemId && slot.quantity > 0)
                {
                    // Use item (trigger item effect)
                    ItemDatabase::Get().UseItem(itemId, entity);
                    
                    // Consume item
                    slot.quantity--;
                    
                    return BTStatus::Success;
                }
            }
            
            return BTStatus::Failure;
        }
        
        case BTActionType::FleeToSafety:
        {
            if (!World::Get().HasComponent<Position_data>(entity))
                return BTStatus::Failure;
            
            const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            
            // Find nearest safe zone tile
            float nearestDist = std::numeric_limits<float>::max();
            Vector nearestSafePos;
            bool foundSafeZone = false;
            
            int searchRadius = 20; // Search 20 tiles around
            int centerX, centerY;
            CollisionMap::Get().WorldToGrid(pos.position.x, pos.position.y, centerX, centerY);
            
            for (int dy = -searchRadius; dy <= searchRadius; ++dy)
            {
                for (int dx = -searchRadius; dx <= searchRadius; ++dx)
                {
                    int tx = centerX + dx;
                    int ty = centerY + dy;
                    
                    if (CollisionMap::Get().IsValidGridPosition(tx, ty))
                    {
                        const TileProperties& tile = CollisionMap::Get().GetTileProperties(tx, ty);
                        
                        // Check if safe zone (custom flag bit 0)
                        if ((tile.customFlags & 0x01) != 0)
                        {
                            float worldX, worldY;
                            CollisionMap::Get().GridToWorld(tx, ty, worldX, worldY);
                            
                            float dist = (Vector(worldX, worldY, 0.0f) - pos.position).Magnitude();
                            if (dist < nearestDist)
                            {
                                nearestDist = dist;
                                nearestSafePos = Vector(worldX, worldY, 0.0f);
                                foundSafeZone = true;
                            }
                        }
                    }
                }
            }
            
            if (foundSafeZone)
            {
                // Set move goal to safe zone
                blackboard.moveGoal = nearestSafePos;
                blackboard.hasMoveGoal = true;
                
                // Set move intent
                if (World::Get().HasComponent<MoveIntent_data>(entity))
                {
                    MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
                    intent.targetPosition = nearestSafePos;
                    intent.desiredSpeed = 2.0f; // Run fast
                    intent.hasIntent = true;
                    intent.usePathfinding = true;
                }
                
                return BTStatus::Success;
            }
            
            return BTStatus::Failure;
        }
        
        case BTActionType::CallForHelp:
        {
            // param1 = alert radius
            float radius = (param1 > 0.0f) ? param1 : 200.0f;
            
            if (!World::Get().HasComponent<Position_data>(entity))
                return BTStatus::Failure;
            
            const Position_data& myPos = World::Get().GetComponent<Position_data>(entity);
            
            // Alert all allies in radius
            auto entities = World::Get().GetAllEntitiesWithComponents<AIController_data, Position_data>();
            
            int alliesAlerted = 0;
            for (EntityID other : entities)
            {
                if (other == entity) continue;
                
                // Check if same faction
                if (World::Get().HasComponent<Faction_data>(entity) && 
                    World::Get().HasComponent<Faction_data>(other))
                {
                    const Faction_data& myFaction = World::Get().GetComponent<Faction_data>(entity);
                    const Faction_data& otherFaction = World::Get().GetComponent<Faction_data>(other);
                    
                    if (myFaction.factionId == otherFaction.factionId)
                    {
                        const Position_data& otherPos = World::Get().GetComponent<Position_data>(other);
                        float dist = (myPos.position - otherPos.position).Magnitude();
                        
                        if (dist <= radius)
                        {
                            // Alert ally by setting their target
                            if (World::Get().HasComponent<AIBlackboard_data>(other))
                            {
                                AIBlackboard_data& otherBB = World::Get().GetComponent<AIBlackboard_data>(other);
                                
                                // Copy my target to ally
                                if (blackboard.hasTarget)
                                {
                                    otherBB.targetEntity = blackboard.targetEntity;
                                    otherBB.hasTarget = true;
                                    otherBB.targetVisible = true;
                                    alliesAlerted++;
                                }
                            }
                        }
                    }
                }
            }
            
            return (alliesAlerted > 0) ? BTStatus::Success : BTStatus::Failure;
        }
        
        case BTActionType::PlayAnimation:
        {
            // param1 = animation ID
            int animId = static_cast<int>(param1);
            
            if (World::Get().HasComponent<Animation_data>(entity))
            {
                Animation_data& anim = World::Get().GetComponent<Animation_data>(entity);
                anim.currentAnimationId = animId;
                anim.frameIndex = 0;
                return BTStatus::Success;
            }
            
            return BTStatus::Failure;
        }
        
        case BTActionType::EmitSound:
        {
            // param1 = sound ID
            int soundId = static_cast<int>(param1);
            
            if (World::Get().HasComponent<Position_data>(entity))
            {
                const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                
                // Play sound at position (assuming audio system)
                AudioSystem::Get().PlaySoundAtPosition(soundId, pos.position.x, pos.position.y);
                
                return BTStatus::Success;
            }
            
            return BTStatus::Failure;
        }
    }
    
    return BTStatus::Failure;
}
```

### Step 3: Add JSON Support

**File:** `Source/AI/BehaviorTree.cpp` (in `LoadTreeFromFile()`)

```cpp
// Parse action type
if (node.type == BTNodeType::Action && nodeJson.contains("actionType"))
{
    std::string actStr = JsonHelper::GetString(nodeJson, "actionType", "");
    
    // ... existing built-in mappings ...
    
    // YOUR CUSTOM MAPPINGS
    else if (actStr == "CastSpell")
        node.actionType = BTActionType::CastSpell;
    else if (actStr == "UseItem")
        node.actionType = BTActionType::UseItem;
    else if (actStr == "FleeToSafety")
        node.actionType = BTActionType::FleeToSafety;
    else if (actStr == "CallForHelp")
        node.actionType = BTActionType::CallForHelp;
    else if (actStr == "PlayAnimation")
        node.actionType = BTActionType::PlayAnimation;
    else if (actStr == "EmitSound")
        node.actionType = BTActionType::EmitSound;
    
    // Handle parameters (v2 format)
    if (isV2 && nodeJson.contains("parameters") && nodeJson["parameters"].is_object())
    {
        const json& params = nodeJson["parameters"];
        node.actionParam1 = JsonHelper::GetFloat(params, "param1", 0.0f);
        node.actionParam2 = JsonHelper::GetFloat(params, "param2", 0.0f);
    }
    else
    {
        // v1 format (flat)
        node.actionParam1 = JsonHelper::GetFloat(nodeJson, "param1", 0.0f);
        node.actionParam2 = JsonHelper::GetFloat(nodeJson, "param2", 0.0f);
    }
}
```

---

## Multi-Frame Actions

Some actions need to execute across multiple frames (e.g., playing an animation, channeling a spell).

### Example: Channeling Action

```cpp
case BTActionType::ChannelSpell:
{
    // param1 = spell ID
    // param2 = channel duration
    int spellId = static_cast<int>(param1);
    float channelDuration = (param2 > 0.0f) ? param2 : 3.0f;
    
    // Use blackboard to track channel state
    if (blackboard.channelTimer == 0.0f)
    {
        // First frame: start channeling
        blackboard.channelTimer = channelDuration;
        blackboard.channelingSpellId = spellId;
        
        // Set animation
        if (World::Get().HasComponent<Animation_data>(entity))
        {
            Animation_data& anim = World::Get().GetComponent<Animation_data>(entity);
            anim.currentAnimationId = ANIM_CHANNELING;
        }
        
        return BTStatus::Running;
    }
    else if (blackboard.channelTimer > 0.0f)
    {
        // Continue channeling
        blackboard.channelTimer -= GameEngine::fDt;
        
        // Check if interrupted (e.g., took damage)
        if (blackboard.wasInterrupted)
        {
            blackboard.channelTimer = 0.0f;
            blackboard.channelingSpellId = 0;
            blackboard.wasInterrupted = false;
            return BTStatus::Failure;
        }
        
        // Still channeling
        if (blackboard.channelTimer > 0.0f)
        {
            return BTStatus::Running;
        }
        
        // Finished channeling
        blackboard.channelTimer = 0.0f;
        
        // Cast spell
        SpellDatabase::Get().CastSpell(blackboard.channelingSpellId, entity);
        blackboard.channelingSpellId = 0;
        
        return BTStatus::Success;
    }
    
    return BTStatus::Failure;
}
```

---

## Best Practices

### 1. Use Blackboard for State

Store temporary state in the AI blackboard, not as static variables:

```cpp
// GOOD: Store in blackboard
blackboard.customTimer = 5.0f;
blackboard.customState = CustomState::Waiting;

// BAD: Static or global state
static float s_timer = 5.0f; // Shared across all entities!
```

### 2. Check Component Existence

Always check if required components exist:

```cpp
// GOOD: Safe component access
if (!World::Get().HasComponent<MyComponent_data>(entity))
    return BTStatus::Failure;

const MyComponent_data& comp = World::Get().GetComponent<MyComponent_data>(entity);

// BAD: Assume component exists
const MyComponent_data& comp = World::Get().GetComponent<MyComponent_data>(entity); // May crash!
```

### 3. Provide Default Parameters

Use default values when parameters are zero or invalid:

```cpp
float radius = (param1 > 0.0f) ? param1 : 100.0f; // Default: 100
int attempts = (param2 > 0.0f) ? static_cast<int>(param2) : 10; // Default: 10
```

### 4. Return Appropriate Status

- Return `Running` for multi-frame actions
- Return `Success` for instant actions that complete
- Return `Failure` for actions that can't execute

```cpp
// Instant action
return BTStatus::Success;

// Multi-frame action (still executing)
if (stillExecuting)
    return BTStatus::Running;
else
    return BTStatus::Success;

// Can't execute
if (!canExecute)
    return BTStatus::Failure;
```

### 5. Log Errors

Log errors for debugging:

```cpp
if (!World::Get().HasComponent<RequiredComponent_data>(entity))
{
    SYSTEM_LOG << "ERROR: Custom action requires RequiredComponent_data\n";
    return BTStatus::Failure;
}
```

### 6. Document Parameters

Document what each parameter does:

```cpp
case BTActionType::MyCustomAction:
{
    // param1: Search radius (default: 100.0)
    // param2: Max attempts (default: 5)
    float radius = (param1 > 0.0f) ? param1 : 100.0f;
    int maxAttempts = (param2 > 0.0f) ? static_cast<int>(param2) : 5;
    
    // ... implementation ...
}
```

---

## Advanced: Custom Composite Nodes

You can also add custom composite node types for specialized control flow.

### Example: Random Selector

```cpp
// In BehaviorTree.h
enum class BTNodeType : uint8_t
{
    // ... existing types ...
    RandomSelector = 10, // Executes random child
};

// In BehaviorTree.cpp
case BTNodeType::RandomSelector:
{
    if (node.childIds.empty())
        return BTStatus::Failure;
    
    // Pick random child
    int randomIndex = rand() % node.childIds.size();
    uint32_t childId = node.childIds[randomIndex];
    
    const BTNode* child = tree.GetNode(childId);
    if (!child)
        return BTStatus::Failure;
    
    return ExecuteBTNode(*child, entity, blackboard, tree);
}
```

---

## Testing Custom Nodes

### Unit Test Example

```cpp
void TestCustomCondition()
{
    // Create test entity
    EntityID testEntity = World::Get().CreateEntity();
    
    // Add required components
    Mana_data mana;
    mana.current = 50;
    mana.maximum = 100;
    World::Get().AddComponent(testEntity, mana);
    
    // Create blackboard
    AIBlackboard_data blackboard;
    
    // Test HasEnoughMana condition with 50% threshold
    BTStatus result = ExecuteBTCondition(BTConditionType::HasEnoughMana, 0.5f, testEntity, blackboard);
    
    // Should succeed (50/100 = 0.5, exactly at threshold)
    assert(result == BTStatus::Success);
    
    // Test with 60% threshold
    result = ExecuteBTCondition(BTConditionType::HasEnoughMana, 0.6f, testEntity, blackboard);
    
    // Should fail (50/100 = 0.5, below threshold)
    assert(result == BTStatus::Failure);
    
    // Cleanup
    World::Get().DestroyEntity(testEntity);
}
```

### Integration Test

Create a test behavior tree JSON and verify execution:

```json
{
  "schema_version": 2,
  "name": "TestCustomNodes",
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "Sequence",
        "name": "Test Sequence",
        "children": [2, 3]
      },
      {
        "id": 2,
        "type": "Condition",
        "name": "Test Custom Condition",
        "conditionType": "HasEnoughMana",
        "parameters": { "param": 0.3 }
      },
      {
        "id": 3,
        "type": "Action",
        "name": "Test Custom Action",
        "actionType": "CastSpell",
        "parameters": { "param1": 5.0 }
      }
    ]
  }
}
```

---

## Debugging Custom Nodes

### Add Debug Logging

```cpp
case BTActionType::MyCustomAction:
{
    SYSTEM_LOG << "[BT] MyCustomAction executing for entity " << entity << "\n";
    SYSTEM_LOG << "[BT]   param1=" << param1 << ", param2=" << param2 << "\n";
    
    // ... implementation ...
    
    SYSTEM_LOG << "[BT] MyCustomAction result: " << (result == BTStatus::Success ? "Success" : "Failure") << "\n";
    return result;
}
```

### Use Behavior Tree Inspector

Enable behavior tree debugging in your game:

```cpp
// Show currently executing node
if (DebugMode::Get().showBehaviorTrees)
{
    const BTNode* currentNode = GetCurrentlyExecutingNode(entity);
    if (currentNode)
    {
        DrawDebugText(entity, currentNode->name);
    }
}
```

---

## Migration Guide

### Upgrading from Built-in Nodes

If you've been using placeholder code, migrate to custom nodes:

**Before (hardcoded in game logic):**
```cpp
// In some game system
if (entity.hasMana() && entity.manaPercent > 0.5f)
{
    CastSpell(entity, SPELL_FIREBALL);
}
```

**After (behavior tree):**
```json
{
  "type": "Sequence",
  "children": [
    {
      "type": "Condition",
      "conditionType": "HasEnoughMana",
      "parameters": { "param": 0.5 }
    },
    {
      "type": "Action",
      "actionType": "CastSpell",
      "parameters": { "param1": 1.0 }
    }
  ]
}
```

---

## Complete Example: Wizard AI

### Custom Nodes

```cpp
// Conditions
case BTConditionType::HasEnoughMana: // ... (see above)
case BTConditionType::IsInSafeZone:  // ... (see above)

// Actions
case BTActionType::CastSpell:        // ... (see above)
case BTActionType::FleeToSafety:     // ... (see above)
case BTActionType::CallForHelp:      // ... (see above)
```

### Behavior Tree

```json
{
  "schema_version": 2,
  "name": "WizardAI",
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "Selector",
        "name": "Wizard Root",
        "children": [2, 10, 15, 18]
      },
      {
        "id": 2,
        "type": "Sequence",
        "name": "Flee if Endangered",
        "children": [3, 4, 5]
      },
      {
        "id": 3,
        "type": "Condition",
        "name": "Health Below 30%?",
        "conditionType": "HealthBelow",
        "parameters": { "param": 0.3 }
      },
      {
        "id": 4,
        "type": "Action",
        "name": "Call for Help",
        "actionType": "CallForHelp",
        "parameters": { "param1": 300.0 }
      },
      {
        "id": 5,
        "type": "Action",
        "name": "Flee to Safety",
        "actionType": "FleeToSafety"
      },
      {
        "id": 10,
        "type": "Sequence",
        "name": "Attack with Magic",
        "children": [11, 12, 13]
      },
      {
        "id": 11,
        "type": "Condition",
        "name": "Has Target?",
        "conditionType": "HasTarget"
      },
      {
        "id": 12,
        "type": "Condition",
        "name": "Has 40% Mana?",
        "conditionType": "HasEnoughMana",
        "parameters": { "param": 0.4 }
      },
      {
        "id": 13,
        "type": "Action",
        "name": "Cast Fireball",
        "actionType": "CastSpell",
        "parameters": { "param1": 1.0 }
      },
      {
        "id": 15,
        "type": "Sequence",
        "name": "Move to Safe Position",
        "children": [16, 17]
      },
      {
        "id": 16,
        "type": "Inverter",
        "name": "NOT in Safe Zone",
        "decoratorChildId": 20
      },
      {
        "id": 20,
        "type": "Condition",
        "name": "In Safe Zone?",
        "conditionType": "IsInSafeZone"
      },
      {
        "id": 17,
        "type": "Action",
        "name": "Move to Safety",
        "actionType": "FleeToSafety"
      },
      {
        "id": 18,
        "type": "Action",
        "name": "Idle",
        "actionType": "Idle"
      }
    ]
  }
}
```

**Behavior:**
1. If low health → call for help and flee
2. Else if has target and enough mana → cast spell
3. Else if not in safe zone → move to safety
4. Else → idle

---

## See Also

- [Behavior Tree Nodes Reference](./nodes.md) - Built-in node types
- [AI Components](../architecture/ai-components.md) - ECS components for AI
- [Navigation System](../navigation/pathfinding.md) - Pathfinding integration
- [ECS Systems](../architecture/ecs-systems.md) - System architecture
