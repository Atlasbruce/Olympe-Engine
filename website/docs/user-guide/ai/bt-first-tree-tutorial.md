---
id: bt-first-tree-tutorial
title: "Tutorial: Your First Behavior Tree"
sidebar_label: "First BT Tutorial"
---

# Tutorial: Your First Behavior Tree

This step-by-step tutorial will guide you through creating your first behavior tree from scratch. We'll build a simple **Guard AI** that patrols an area and attacks when it sees a target.

**What you'll learn:**
- Creating a new behavior tree
- Adding and connecting nodes
- Configuring node parameters
- Testing with the debugger
- Attaching AI to an entity

**Estimated time:** 15-20 minutes

---

## Prerequisites

Before starting, make sure you have:
- ✅ Olympe Engine installed and running
- ✅ Basic understanding of the editor interface
- ✅ A test level loaded (any level with navigable terrain)

If you're new to the engine, complete the [Getting Started Guide](../../getting-started/quick-start.md) first.

---

## Step 1: Create a New Behavior Tree

### 1.1 Open the Blueprint Editor

1. Launch the Olympe Engine
2. Click **Tools** → **Blueprint Editor**
3. The editor opens in a new window

### 1.2 Create New Behavior Tree

1. Click **File** → **New** (or press `Ctrl+N`)
2. Select **Behavior Tree** from the blueprint type menu
3. Enter name: `GuardAI_Tutorial`
4. Click **Create**

**Result:** A new behavior tree with a default root node appears on the canvas.

---

## Step 2: Plan Your AI Structure

Before adding nodes, let's plan the behavior:

**Guard AI Logic:**
```
IF target visible THEN
    Attack target
ELSE
    Patrol waypoints
```

**As a behavior tree:**
```
Selector (Root) - Try combat first, then patrol
 ├─ Sequence (Combat) - Attack if target visible
 │   ├─ Condition: Target Visible
 │   └─ Action: Attack Melee
 └─ Sequence (Patrol) - Patrol when no target
     ├─ Action: Pick Next Patrol Point
     └─ Action: Move To Goal
```

---

## Step 3: Build the Root Selector

### 3.1 Configure Root Node

The default root is already a Selector - perfect for our needs!

1. **Select the root node** (click on it)
2. In the **Properties Panel** (right side):
   - Set **Name** to: `Guard Decision`
   - Leave **Type** as: `Selector`
3. Press `Enter` to confirm

**Why Selector?** We want to try combat first (high priority), then fall back to patrol (low priority).

---

## Step 4: Add Combat Sequence

### 4.1 Add Combat Sequence Node

1. **Select the root node** `Guard Decision`
2. Press `Shift+Q` (Quick Add Sequence)
   - Or right-click → Add Child → Sequence
3. A new Sequence appears connected to the root

### 4.2 Configure Combat Sequence

1. **Select the new Sequence node**
2. In **Properties Panel**:
   - **Name**: `Combat`
   - **Type**: `Sequence` (already set)

---

## Step 5: Add Combat Condition

### 5.1 Add Target Check

1. **Select the Combat Sequence node**
2. Press `Shift+C` (Quick Add Condition)
   - Or right-click → Add Child → Condition
3. A new Condition appears

### 5.2 Configure Condition

1. **Select the new Condition node**
2. In **Properties Panel**:
   - **Name**: `Target Visible?`
   - **Condition Type**: `TargetVisible` (dropdown)
   - **Parameters**: None needed

**What this does:** Checks if the entity can see a target entity. Returns Success if target exists, Failure if not.

---

## Step 6: Add Attack Action

### 6.1 Add Attack Node

1. **Select the Combat Sequence node**
2. Press `Shift+X` (Quick Add Action)
3. A new Action appears

### 6.2 Configure Attack Action

1. **Select the new Action node**
2. In **Properties Panel**:
   - **Name**: `Attack Target`
   - **Action Type**: `AttackIfClose` (dropdown)
   - **Parameters**: None needed

**What this does:** Performs a melee attack if target is in range. Returns Running during attack, Success when complete.

**Current tree:**
```
Selector "Guard Decision"
 └─ Sequence "Combat"
     ├─ Condition "Target Visible?"
     └─ Action "Attack Target"
```

---

## Step 7: Add Patrol Sequence

### 7.1 Add Patrol Sequence

1. **Select the root Selector** `Guard Decision`
2. Press `Shift+Q` (Quick Add Sequence)
3. A new Sequence appears as second child

### 7.2 Configure Patrol Sequence

1. **Select the new Sequence node**
2. In **Properties Panel**:
   - **Name**: `Patrol`
   - **Type**: `Sequence`

---

## Step 8: Add Patrol Actions

### 8.1 Add Pick Patrol Point

1. **Select Patrol Sequence**
2. Press `Shift+X` (Quick Add Action)
3. Configure the Action:
   - **Name**: `Pick Next Waypoint`
   - **Action Type**: `PatrolPickNextPoint`

### 8.2 Add Move Action

1. **Select Patrol Sequence** (again)
2. Press `Shift+X` (Quick Add Action)
3. Configure the Action:
   - **Name**: `Move to Waypoint`
   - **Action Type**: `MoveToGoal`
   - **Param1** (acceptance radius): `20.0`

**Final tree structure:**
```
Selector "Guard Decision"
 ├─ Sequence "Combat"
 │   ├─ Condition "Target Visible?"
 │   └─ Action "Attack Target"
 └─ Sequence "Patrol"
     ├─ Action "Pick Next Waypoint"
     └─ Action "Move to Waypoint"
```

---

## Step 9: Arrange and Validate

### 9.1 Auto-Layout

1. Press `Ctrl+L` or right-click canvas → **Auto Layout**
2. Nodes arrange automatically for clarity
3. Press `Home` to frame entire tree

### 9.2 Validate Tree

1. Press `Ctrl+Shift+V` or click **Validate** button
2. Check for any errors in the output panel
3. Should see: ✅ "Validation passed"

**Common validation errors:**
- ❌ Disconnected nodes - Connect all nodes to root
- ❌ Decorator without child - Add exactly 1 child
- ❌ Missing parameters - Fill required fields

---

## Step 10: Save Your Tree

1. Press `Ctrl+S` or click **File** → **Save**
2. Save location: `Blueprints/AI/GuardAI_Tutorial.json`
3. Confirm save

**File structure:**
```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "name": "GuardAI_Tutorial",
  "data": {
    "rootNodeId": 1,
    "nodes": [...]
  }
}
```

---

## Step 11: Create Guard Entity Prefab

Now let's create an entity that uses this AI.

### 11.1 Create Prefab File

Create a new file: `Blueprints/Prefabs/GuardEntity.json`

```json
{
  "schema_version": 2,
  "name": "Guard",
  "components": [
    {
      "type": "Transform_data",
      "position": { "x": 400, "y": 300 }
    },
    {
      "type": "VisualSprite_data",
      "texturePath": "Gamedata/Sprites/guard.png",
      "srcRect": { "x": 0, "y": 0, "w": 32, "h": 32 }
    },
    {
      "type": "AIBehaviorTree_data",
      "treeFile": "Blueprints/AI/GuardAI_Tutorial.json"
    },
    {
      "type": "AIBlackboard_data"
    },
    {
      "type": "Patrol_data",
      "waypoints": [
        { "x": 200, "y": 300 },
        { "x": 600, "y": 300 },
        { "x": 600, "y": 500 },
        { "x": 200, "y": 500 }
      ],
      "currentIndex": 0
    },
    {
      "type": "Navigation_data",
      "speed": 100.0
    },
    {
      "type": "Combat_data",
      "attackDamage": 10,
      "attackRange": 50,
      "attackCooldown": 1.0
    },
    {
      "type": "Health_data",
      "current": 100,
      "maximum": 100
    }
  ]
}
```

**Key components:**
- `AIBehaviorTree_data` - Links to your tree file
- `AIBlackboard_data` - Stores runtime AI state
- `Patrol_data` - Defines patrol waypoints
- `Navigation_data` - Enables movement
- `Combat_data` - Enables attacking

---

## Step 12: Test in Runtime Debugger

### 12.1 Launch Game with Guard

1. Add your guard prefab to a test level
2. Launch the game (`F5`)
3. The guard entity spawns and starts patrolling

### 12.2 Open BT Debugger

1. Press `Ctrl+Shift+B` to open BT Debugger
2. Debugger window opens showing:
   - **Entity List** (left) - Your guard entity
   - **Node Graph** (center) - Your tree structure
   - **Inspector** (right) - Runtime data

### 12.3 Observe Patrol Behavior

1. **Select the guard entity** in the Entity List
2. Watch the **Node Graph**:
   - Root Selector: 🔵 Running
   - Combat Sequence: ❌ Failure (no target)
   - Patrol Sequence: 🔵 Running
   - Currently executing nodes are **highlighted**

3. Observe in the **Execution Log**:
   ```
   [0.0s] Guard Decision - Running
   [0.0s] Combat - Failure (no target)
   [0.1s] Patrol - Running
   [0.1s] Pick Next Waypoint - Success
   [0.2s] Move to Waypoint - Running
   [3.5s] Move to Waypoint - Success
   [3.6s] Pick Next Waypoint - Success
   ...
   ```

### 12.4 Test Combat Behavior

To test the combat branch:

**Option A: Use Debug Mode**
1. In the debugger, click the guard entity
2. In **Inspector** → **Blackboard** section
3. Manually set `targetEntity` to player entity ID
4. Watch combat sequence activate

**Option B: Natural Detection**
1. Make sure guard has a detection system
2. Move player close to guard
3. Guard detects player and engages

**Expected behavior:**
- Combat Sequence: ✅ Success (target visible)
- Attack Target: 🔵 Running (attacking)
- Patrol Sequence: ⏸️ Not evaluated (combat wins)

---

## Step 13: Understanding Execution Flow

### How the Tree Executes

**Frame 1:**
```
Selector "Guard Decision" - Evaluates children
  ├─ Sequence "Combat"
  │   └─ Condition "Target Visible?" → Failure (no target)
  └─ (Combat fails, try next child...)
```

**Frame 1 (continued):**
```
Selector "Guard Decision" - Still evaluating
  ├─ Sequence "Combat" - Failed
  └─ Sequence "Patrol" - Tries this
      ├─ Action "Pick Next Waypoint" → Success (waypoint selected)
      └─ Action "Move to Waypoint" → Running (started moving)
```

**Frame 2-N:**
```
Selector "Guard Decision" - Re-evaluates (tree is reactive!)
  ├─ Sequence "Combat"
  │   └─ Condition "Target Visible?" → Failure (still no target)
  └─ Sequence "Patrol"
      └─ Action "Move to Waypoint" → Running (continues from last frame)
```

**When target appears:**
```
Selector "Guard Decision"
  ├─ Sequence "Combat"
  │   ├─ Condition "Target Visible?" → Success (target detected!)
  │   └─ Action "Attack Target" → Running (starts attacking)
  └─ (Patrol not evaluated - combat succeeded)
```

**Key insight:** The tree is **reactive** - it re-evaluates from the root every frame, so the guard can switch from patrol to combat instantly when a target appears.

---

## Step 14: Improving the Tree (Optional)

### Enhancement 1: Add Investigation

What if the guard should investigate where it last saw the target?

**Add between Combat and Patrol:**
```
Selector "Guard Decision"
 ├─ Sequence "Combat" (existing)
 ├─ Sequence "Investigate" (new!)
 │   ├─ Condition "Has Last Known Position"
 │   └─ Action "Move to Last Known Position"
 └─ Sequence "Patrol" (existing)
```

### Enhancement 2: Add Attack Range Check

Prevent attacking when too far away:

```
Sequence "Combat"
 ├─ Condition "Target Visible?"
 ├─ Condition "Target in Range" (new! param: 100.0)
 └─ Action "Attack Target"
```

### Enhancement 3: Add Health Check for Flee

Guard flees when health is low:

```
Selector "Guard Decision"
 ├─ Sequence "Flee" (new - highest priority!)
 │   ├─ Condition "Health Below" (param: 25.0)
 │   └─ Action "Move to Safe Point"
 ├─ Sequence "Combat"
 ├─ Sequence "Patrol"
```

---

## Step 15: Common Issues and Solutions

### Issue: Guard doesn't patrol

**Check:**
- ✅ Patrol_data component exists with waypoints
- ✅ Navigation_data component exists
- ✅ Tree file path correct in AIBehaviorTree_data
- ✅ Tree loaded successfully (check console)

**Solution:**
- Add missing components to prefab
- Verify file paths
- Check console for load errors

---

### Issue: Guard doesn't attack

**Check:**
- ✅ Combat_data component exists
- ✅ Target detection system working
- ✅ targetEntity set in blackboard

**Solution:**
- Add Combat_data component
- Implement detection (e.g., vision cone)
- Use debugger to verify target is set

---

### Issue: Nodes always fail

**Check:**
- ✅ Required components exist for actions
- ✅ Parameters are valid (positive ranges, etc.)
- ✅ Conditions are reachable (not blocked by failing parent)

**Solution:**
- Use debugger to identify which node fails
- Check node parameters
- Verify entity has required components

---

## Next Steps

Congratulations! You've created your first behavior tree. 🎉

### Continue Learning

- **[BT Debugger Guide](bt-debugger.md)** - Master runtime debugging
- **[Node Types](bt-node-types.md)** - Explore all available nodes
- **[Advanced Tutorial](../../tutorials/behavior-tree-basics.md)** - Build complex AI

### Try These Challenges

1. **Add an idle state**: Guard waits a few seconds before moving to next waypoint
2. **Create a boss AI**: Multi-phase behavior with health thresholds
3. **Build a merchant**: NPC that wanders but stops to talk to player

### Additional Resources

- [Connection Rules](bt-connection-rules.md) - Understand node connections
- [Keyboard Shortcuts](bt-keyboard-shortcuts.md) - Work more efficiently
- [Custom Nodes](../../technical-reference/behavior-trees/custom-nodes.md) - Extend with code

---

## Summary

You learned how to:
- ✅ Create a new behavior tree
- ✅ Add and connect nodes (Selector, Sequence, Condition, Action)
- ✅ Configure node parameters
- ✅ Validate and save trees
- ✅ Attach AI to entities with prefabs
- ✅ Test and debug at runtime
- ✅ Understand execution flow

**Next tutorial:** [Advanced Behavior Trees](../../tutorials/behavior-tree-basics.md) - Learn decorators, repeaters, and complex patterns!
