---
id: bt-debugger
title: "Behavior Tree Runtime Debugger"
sidebar_label: "BT Debugger"
---

# Behavior Tree Runtime Debugger

The Behavior Tree Debugger is a powerful real-time visualization tool for inspecting and debugging AI behavior during gameplay. It runs in a separate window and provides complete visibility into decision-making processes.

## Overview

The debugger provides:
- **Real-time visualization** of tree execution
- **Entity filtering** to focus on specific AI
- **Execution history** to track decision patterns
- **Blackboard inspection** to view runtime state
- **Performance metrics** for optimization

## Opening the Debugger

### During Gameplay

**Method 1: Keyboard Shortcut**
- Press `Ctrl+Shift+B` (Windows/Linux)
- Press `Cmd+Shift+B` (macOS)

**Method 2: Menu**
1. Click **Debug** in the main menu
2. Select **Behavior Tree Debugger**

**Result:** A separate window opens with the debugger interface.

### Standalone Mode

You can also launch the debugger before starting the game:
1. Start Olympe Engine
2. Open debugger (it remains hidden)
3. Launch level
4. Debugger automatically populates with entities

---

## Interface Layout

The debugger window consists of three main panels:

```
┌─────────────────────────────────────────────────┐
│  [Entity List]  │  [Node Graph]  │ [Inspector] │
│                 │                 │             │
│  - Guard 1      │   ┌─Selector─┐ │ Runtime Info│
│  - Guard 2      │   │  Combat  │ │ Blackboard  │
│  - Zombie 1     │   └──────────┘ │ Exec Log    │
│                 │                 │             │
└─────────────────────────────────────────────────┘
```

### 1. Entity List Panel (Left)

Displays all entities with behavior trees currently active.

**Features:**
- Entity name and ID
- Tree name
- Current execution status
- Last update time
- Filter and sort controls

**Interactions:**
- Click entity to select and view its tree
- Double-click to center in game view
- Right-click for context menu

### 2. Node Graph Panel (Center)

Visual representation of the behavior tree for the selected entity.

**Features:**
- Node hierarchy display
- Execution status highlighting
- Connection visualization
- Pan and zoom controls
- Layout options (vertical/horizontal)

**Status Indicators:**
- 🔵 **Blue highlight** - Currently Running
- ✅ **Green highlight** - Succeeded (last execution)
- ❌ **Red highlight** - Failed (last execution)
- ⚪ **Gray** - Idle (not executed recently)

### 3. Inspector Panel (Right)

Detailed information about the selected entity's AI state.

**Sections:**
- **Runtime Info** - Current node, status, execution time
- **Blackboard** - All runtime variables
- **Execution Log** - Recent decision history
- **Performance** - CPU time, evaluation count

---

## Using the Entity List

### Viewing Entities

The list shows all entities with `AIBehaviorTree_data` components.

**Columns:**
- **Name** - Entity name (e.g., "Guard", "Zombie")
- **Tree** - Behavior tree name
- **Status** - Current execution status
- **Mode** - AI mode (Patrol, Combat, etc.)

### Filtering Entities

**Text Filter:**
1. Click the filter box (or press `Ctrl+F`)
2. Type entity name or tree name
3. List updates in real-time

**Filter Options:**
- ☑️ **Active Only** - Show only entities currently executing
- ☑️ **Has Target** - Show only entities with targets

**Example:**
- Type `Guard` → Shows all guards
- Type `Patrol` → Shows all using patrol tree
- Enable "Active Only" → Shows only moving/attacking entities

### Sorting

Click column headers to sort:
- **Name** - Alphabetical order
- **Tree Name** - Group by behavior tree
- **Last Update** - Most recently active first
- **AI Mode** - Group by current behavior mode

---

## Node Graph Visualization

### Reading the Graph

**Node Layout:**
```
        ┌─────────────┐
        │  Selector   │ Root node (top)
        └──────┬──────┘
               │
       ┌───────┴───────┐
       │               │
  ┌────┴────┐    ┌────┴────┐
  │ Combat  │    │ Patrol  │ Children (below)
  └─────────┘    └─────────┘
```

**Vertical Layout (Default):**
- Root at top
- Children below
- Execution flows downward

**Horizontal Layout:**
- Root at left
- Children to the right
- Execution flows left-to-right

### Node Colors by Type

| Node Type | Color |
|-----------|-------|
| Selector | Purple |
| Sequence | Blue |
| Condition | Yellow |
| Action | Green |
| Inverter | Orange |
| Repeater | Teal |

### Execution Highlights

Nodes show their current execution state:

**Running (Blue Pulse):**
```
┌─────────────┐
│ Move to Goal│ ← Animated blue glow
└─────────────┘
```
This node is currently executing.

**Success (Green Flash):**
```
┌─────────────┐
│ Target Check│ ← Green tint
└─────────────┘
```
This node succeeded in the last evaluation.

**Failure (Red Flash):**
```
┌─────────────┐
│ Can Attack? │ ← Red tint
└─────────────┘
```
This node failed in the last evaluation.

### Execution Token

A **token** (small circle) animates through the tree showing the current execution path:

```
Selector
 ├─ Combat ➜ ● ← Token here (failed)
 └─ Patrol ● ← Token moves here
```

---

## Camera Controls

### Zoom

**Mouse Wheel:**
- Scroll up → Zoom in
- Scroll down → Zoom out
- Range: 30% to 300%

**Keyboard:**
- `+` or `=` → Zoom in (120%)
- `-` → Zoom out (80%)
- `0` → Reset to 100%

**Current zoom:** Displayed in top-right corner (e.g., "Zoom: 150%")

### Pan

**Middle Mouse Button:**
- Click and drag to pan view

**Space + Drag:**
- Hold `Space`, then drag with left mouse

**Keyboard (when node selected):**
- Arrow keys → Pan in direction

### Framing

**Fit to View:**
- Press `Home` or click **Fit to View** button
- Centers entire tree in viewport

**Center on Selected:**
- Press `F` or double-click entity in list
- Centers on selected entity's tree

**Reset Camera:**
- Click **Reset Camera** button
- Resets zoom to 100% and centers tree

---

## Layout Options

### Switching Layouts

**Vertical (Top-to-Bottom):**
- Click **Vertical** button
- Or press `Ctrl+1`
- Best for: Deep trees, many levels

**Horizontal (Left-to-Right):**
- Click **Horizontal** button
- Or press `Ctrl+2`
- Best for: Wide trees, many children

### Spacing Controls

**Adjust Node Spacing:**
1. Find spacing sliders in toolbar
2. **Horizontal Spacing**: 150-500px (default: 250px)
3. **Vertical Spacing**: 100-400px (default: 180px)
4. Drag sliders to adjust

**Reset to Defaults:**
- Click **Reset Spacing** button
- Restores 250×180px spacing

**Auto Layout:**
- Click **Auto Layout** button
- Automatically arranges nodes for best visibility

---

## Inspector Panel

### Runtime Info Section

Displays current execution state:

**Fields:**
- **Current Node**: Name and ID of executing node
- **Status**: Running/Success/Failure
- **Tree ID**: Behavior tree asset ID
- **Tree Name**: Loaded tree file name
- **Execution Time**: Total time spent in tree this frame (ms)

**Example:**
```
Current Node: Move to Goal (#12)
Status: Running
Tree: guard_ai.json (ID: 42)
Execution Time: 0.8ms
```

### Blackboard Section

Shows all runtime variables stored in `AIBlackboard_data`.

**Common Variables:**

| Variable | Type | Description |
|----------|------|-------------|
| `targetEntity` | EntityID | Current target (0 = none) |
| `lastKnownTargetPosition` | Vector | Last seen target location |
| `moveGoal` | Vector | Current movement destination |
| `waitTimer` | float | Timer for wait actions |
| `heardNoise` | bool | Noise detection flag |
| `noisePosition` | Vector | Location of noise |
| `reachedDestination` | bool | Arrived at goal? |

**Example Display:**
```
Blackboard:
  targetEntity: 12
  moveGoal: (450, 320)
  waitTimer: 0.0
  lastKnownTargetPosition: (500, 300)
```

**Features:**
- Values update in real-time
- Click to inspect details
- Editable in debug mode (advanced)

### Execution Log

Chronological list of recent node executions.

**Log Entry Format:**
```
[Time] Entity | Node Name | Status
```

**Example Log:**
```
[0.0s] Guard #5 | Guard Decision | Running
[0.0s] Guard #5 | Combat | Failure
[0.1s] Guard #5 | Patrol | Running
[0.1s] Guard #5 | Pick Next Waypoint | Success
[0.2s] Guard #5 | Move to Waypoint | Running
[3.5s] Guard #5 | Move to Waypoint | Success
[3.6s] Guard #5 | Patrol | Success
```

**Features:**
- Auto-scrolls to latest entry
- Max 100 entries (circular buffer)
- Color-coded by status
- Filterable by entity

**Controls:**
- **Clear Log**: Removes all entries
- **Pause**: Stops logging new entries
- **Copy**: Copy log to clipboard
- **Save**: Export log to file

---

## Debugging Workflows

### Workflow 1: Why isn't my AI working?

**Steps:**
1. Open debugger (`Ctrl+Shift+B`)
2. Find entity in Entity List
3. Select entity to view tree
4. Watch Node Graph:
   - Which nodes are Running?
   - Which nodes are Failing?
5. Check Inspector:
   - Is `targetEntity` set correctly?
   - Are movement goals valid?
6. Review Execution Log:
   - See decision pattern over time
   - Identify stuck nodes

**Common Issues:**
- **Node always Running**: Action never completes (check logic)
- **Node always Failing**: Condition never met (check parameters)
- **Tree stuck on one branch**: Selector/Sequence not re-evaluating

---

### Workflow 2: Why did the AI make that decision?

**Steps:**
1. Select entity after unexpected behavior
2. Check **Execution Log**:
   - See exact sequence of decisions
   - Identify which condition triggered behavior
3. Review **Blackboard**:
   - What values caused the condition to pass?
4. Replay in slow motion:
   - Pause game
   - Step frame-by-frame (`N` key)
   - Watch decision tree evaluate

**Example:**
"Why did the guard attack instead of patrol?"
```
Log shows:
[10.2s] Combat | Running
[10.2s] Target Visible? | Success ← Here's why!

Blackboard shows:
  targetEntity: 7 ← Player was detected
```

---

### Workflow 3: Performance optimization

**Steps:**
1. Enable performance metrics
2. Sort entities by **Last Update**
3. Identify slow evaluating entities
4. Select entity to view tree
5. Check **Execution Time** in Inspector
6. Identify expensive nodes (>1ms)
7. Optimize:
   - Add conditions to early-exit
   - Cache expensive calculations in blackboard
   - Simplify tree structure

**Performance Tips:**
- Trees should evaluate in &lt;0.5ms
- Use conditions to skip expensive branches
- Limit tree depth (2-4 levels ideal)

---

## Advanced Features

### Minimap

**Toggle:** Press `Ctrl+M` or click **Minimap** button

A small overview of the entire tree appears in corner:
```
┌─────────┐
│ ┌─┐     │
│ │ │ ←●  │ ← You are here
│ └─┘     │
└─────────┘
```

**Usage:**
- See where you are in large trees
- Click to jump to location
- Useful for trees with 20+ nodes

### Grid Display

**Toggle:** Press `Ctrl+G` or click **Grid** button

Displays alignment grid for node positioning:
- Helps align nodes visually
- Shows spacing increments
- Useful for manual layout

### Playback Control (Experimental)

Some debugger builds support playback control:

**Features:**
- **Pause** (`Space`) - Pause AI execution
- **Step Frame** (`N`) - Advance one frame
- **Step Node** (`Shift+N`) - Execute one node

**Note:** This feature may not be available in all builds.

---

## Troubleshooting

### Debugger window blank

**Causes:**
- No entities with behavior trees in scene
- Trees not loaded successfully
- Debugger not initialized

**Solutions:**
- ✅ Spawn entity with `AIBehaviorTree_data`
- ✅ Check console for tree load errors
- ✅ Restart debugger window

---

### Can't see entity in list

**Causes:**
- Entity doesn't have behavior tree
- Filter is active
- Entity just spawned (refresh pending)

**Solutions:**
- ✅ Clear filter (`Escape`)
- ✅ Check entity has `AIBehaviorTree_data`
- ✅ Wait for auto-refresh (0.5s interval)

---

### Nodes not highlighting

**Causes:**
- Tree not executing (entity disabled)
- Debugger out of sync

**Solutions:**
- ✅ Check entity is active in game
- ✅ Close and reopen debugger
- ✅ Select different entity, then re-select

---

## Keyboard Shortcuts Quick Reference

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+B` | Toggle debugger window |
| `Ctrl+F` | Filter entities |
| `Mouse Wheel` | Zoom |
| `+` / `-` | Zoom in/out |
| `0` | Reset zoom |
| `Middle Mouse Drag` | Pan |
| `Home` | Fit to view |
| `F` | Center on selection |
| `Ctrl+1` | Vertical layout |
| `Ctrl+2` | Horizontal layout |
| `Ctrl+M` | Toggle minimap |
| `Ctrl+G` | Toggle grid |
| `Up`/`Down` | Navigate entity list |
| `Enter` | Center on entity tree |

---

## Best Practices

### 1. Keep Debugger Open During Development
- Always have debugger visible when testing AI
- Catch issues immediately
- Understand AI decision patterns

### 2. Use Filters Effectively
- Focus on specific entity type
- Filter by active entities only
- Reduce visual clutter

### 3. Monitor Execution Log
- Watch for unexpected patterns
- Identify infinite loops (same node repeating)
- Track state transitions

### 4. Check Blackboard Regularly
- Verify variables are set correctly
- Ensure conditions have correct data
- Debug logic errors early

### 5. Learn Your Trees' Normal Behavior
- Know what successful execution looks like
- Spot anomalies quickly
- Understand performance baseline

---

## Next Steps

Now that you understand the debugger:

- **[First BT Tutorial](bt-first-tree-tutorial.md)** - Build and debug a tree
- **[Node Types](bt-node-types.md)** - Understand what you're debugging
- **[Keyboard Shortcuts](bt-keyboard-shortcuts.md)** - Work more efficiently

## Additional Resources

- [BT Overview](bt-editor-overview.md) - Behavior tree concepts
- [Technical Reference](../../technical-reference/behavior-trees/behavior-trees-overview.md) - Architecture details
- [Behavior Tree Basics](../../tutorials/behavior-tree-basics.md) - Complete tutorial

---

**Pro Tip:** Always run the debugger on a second monitor if available. This lets you see both the game and AI visualization simultaneously for maximum insight!
