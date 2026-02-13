# 🔍 Behavior Tree Runtime Debugger

## Overview

The Behavior Tree Runtime Debugger is a comprehensive visual debugging tool for inspecting and understanding AI decision-making in real-time. It provides an interactive interface to view behavior tree execution, entity states, and blackboard variables without interrupting gameplay.

## Features

### 🎯 Real-Time Visualization
- **Live execution tracking**: See which nodes are currently executing with a pulsing yellow highlight
- **Interactive node graph**: Zoom, pan, and inspect behavior tree structure
- **Execution history**: Review the last 100 node executions with timestamps

### 📊 Entity Management
- **Entity list**: View all entities with active behavior trees
- **Filtering options**:
  - Text search by entity name
  - Show only active entities
  - Show only entities with targets
- **Sorting options**:
  - By name (alphabetical)
  - By tree name
  - By last update time
  - By AI mode (Idle/Patrol/Combat/etc.)

### 🔬 Detailed Inspection
- **Runtime information**: Current tree, node, and execution status
- **Blackboard viewer**: Inspect all AI variables (targets, movement, patrol, combat, stimuli)
- **AI state monitoring**: Track current AI mode and time in mode

## Activation

### Keyboard Shortcut
Press **F10** to toggle the debugger window on/off.

### From Code
The debugger is automatically initialized when the engine starts. You can access it via:
```cpp
extern Olympe::BehaviorTreeDebugWindow* g_btDebugWindow;
if (g_btDebugWindow) {
    g_btDebugWindow->ToggleVisibility();
}
```

## User Interface

### Left Panel: Entity List
The left panel displays all entities with behavior trees:

```
Entities with Behavior Trees
─────────────────────────────
Search: [____________]
☑ Active Only  ☑ Has Target

Sort by: [Name ▼] [Asc]
─────────────────────────────
Entities: 5 / 10

● ✓ Guard_01
    Patrol_BT
● ▶ Enemy_02
    Combat_BT
○ ✗ NPC_03
    Wander_BT
```

**Status Icons:**
- `●` = Active / `○` = Inactive
- `✓` = Last execution succeeded
- `✗` = Last execution failed
- `▶` = Currently running

### Center Panel: Node Graph
The center panel shows the behavior tree as an interactive node graph:

**Node Colors:**
- **Blue** (Selector): OR logic - succeeds if any child succeeds
- **Green** (Sequence): AND logic - succeeds if all children succeed
- **Orange** (Condition): Checks a condition (has target, health below, etc.)
- **Rose** (Action): Performs an action (move, attack, patrol, etc.)
- **Purple** (Inverter): Inverts child result
- **Light Blue** (Repeater): Repeats child N times

**Node Icons:**
- `?` Selector
- `→` Sequence
- `◆` Condition
- `►` Action
- `!` Inverter
- `↻` Repeater

**Execution Token:**
The currently executing node is highlighted with a **pulsing yellow border**.

**Interaction:**
- **Mouse wheel**: Zoom in/out
- **Middle mouse button**: Pan the graph
- **Hover**: View node details

### Right Panel: Inspector
The right panel provides detailed information about the selected entity:

#### Runtime Info
```
Tree ID: 42
Tree Name: Patrol_BT
Current Node ID: 7
Node Name: MoveToPatrolPoint
Last Status: Success
Active: Yes
AI Mode: Patrol
Time in Mode: 12.34 s
```

#### Blackboard
Collapsible sections for different blackboard variables:

**Target**
- Has Target: Yes/No
- Target Entity: 123
- Target Visible: Yes/No
- Distance: 145.2
- Time Since Seen: 2.5 s
- Last Known Pos: (100.0, 200.0)

**Movement**
- Has Move Goal: Yes/No
- Goal Position: (150.0, 300.0)

**Patrol**
- Has Patrol Path: Yes/No
- Current Point: 2
- Point Count: 4

**Combat**
- Can Attack: Yes/No
- Attack Cooldown: 1.5 s
- Last Attack: 0.8 s ago

**Stimuli**
- Heard Noise: Yes/No
- Last Damage: 25.0

**Wander**
- Has Destination: Yes/No
- Destination: (200.0, 150.0)
- Wait Timer: 2.1 / 3.0 s

#### Execution Log
Shows the last 100 node executions for the selected entity:

```
[0.1s ago] ✓ Node 7 (MoveToPatrolPoint)
[0.5s ago] ▶ Node 5 (HasReachedDestination)
[1.2s ago] ✓ Node 3 (PatrolSequence)
[2.0s ago] ✓ Node 1 (RootSelector)
```

**Color coding:**
- **Yellow** (`▶`): Running
- **Green** (`✓`): Success
- **Red** (`✗`): Failure

**Actions:**
- **Clear Log** button: Clears the execution history

## Configuration

### View Menu Options
Access these options via the menu bar (View menu):

- **Auto Refresh**: Set refresh interval (0.1 - 5.0 seconds, default: 0.5s)
- **Entity List Width**: Adjust left panel width (150 - 400 pixels)
- **Inspector Width**: Adjust right panel width (250 - 500 pixels)
- **Node Spacing X**: Horizontal spacing between nodes (100 - 500 pixels)
- **Node Spacing Y**: Vertical spacing between layers (80 - 300 pixels)

### Actions Menu
- **Refresh Now (F5)**: Manually refresh the entity list
- **Clear Execution Log**: Clear all logged executions

## Performance

### Resource Usage
- **Layout calculation**: < 5ms for trees with 50 nodes
- **Render cost**: < 1ms for 10 active entities
- **Memory**: ~100 execution log entries per entity (circular buffer)

### Auto-Refresh
The debugger automatically refreshes every 0.5 seconds by default. You can adjust this in the View menu to balance between responsiveness and performance.

### Performance Tips
1. Close the debugger window (F10) when not in use to save resources
2. Increase auto-refresh interval if monitoring many entities
3. Use filtering to reduce the number of displayed entities
4. Clear execution log regularly to minimize memory usage

## Graph Layout Algorithm

The debugger uses a **5-phase Sugiyama algorithm** for clean, readable node layouts:

1. **Layering**: Assigns nodes to hierarchical layers via BFS from root
2. **Initial Ordering**: Orders nodes within each layer
3. **Crossing Reduction**: Minimizes edge crossings using barycenter heuristic (10 passes)
4. **X-Coordinate Assignment**: Positions nodes horizontally with parent centering
5. **Collision Resolution**: Resolves overlaps with dynamic spacing

**Result**: Professional-looking graphs without overlapping nodes or tangled edges.

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| **F10** | Toggle Behavior Tree Runtime Debugger |
| **F5** | Refresh entity list (when debugger is open) |
| **ESC** | Close debugger window |

## Integration with AI Systems

### Execution Capture
The debugger automatically captures node executions from `BehaviorTreeSystem::Process()`:

```cpp
// In ECS_Systems_AI.cpp
BTStatus status = ExecuteBTNode(*node, entity, blackboard, *tree);
btRuntime.lastStatus = static_cast<uint8_t>(status);

// Notify debugger if active
if (g_btDebugWindow && g_btDebugWindow->IsVisible())
{
    g_btDebugWindow->AddExecutionEntry(entity, node->id, node->name, status);
}
```

### Entity Queries
The debugger queries entities directly from the ECS World:

```cpp
// Query all entities with BehaviorTreeRuntime_data
const auto& allEntities = World::Get().GetAllEntities();
for (EntityID entity : allEntities)
{
    if (World::Get().HasComponent<BehaviorTreeRuntime_data>(entity))
    {
        // Process entity...
    }
}
```

### Blackboard Access
Reads blackboard variables directly from entity components:

```cpp
const auto& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
// Access blackboard.targetEntity, blackboard.hasTarget, etc.
```

## Troubleshooting

### Window doesn't open
- Press F10 again to toggle
- Check console logs for initialization errors
- Verify ImGui and ImNodes are properly initialized

### No entities shown
- Ensure entities have `BehaviorTreeRuntime_data` component
- Check if filters are hiding entities
- Click "Refresh Now" (F5) to force refresh

### Graph not displaying
- Select an entity from the list
- Verify the entity's behavior tree is loaded
- Check that the tree has valid nodes

### Performance issues
- Increase auto-refresh interval (View > Auto Refresh)
- Close debugger when not in use (F10)
- Reduce number of entities with behavior trees
- Clear execution log regularly

## Technical Details

### File Structure
```
Source/AI/
├── BehaviorTreeDebugWindow.h         # Main debugger class
├── BehaviorTreeDebugWindow.cpp       # Implementation
├── BTGraphLayoutEngine.h             # Layout algorithm
└── BTGraphLayoutEngine.cpp           # Implementation
```

### Dependencies
- **ImGui**: UI framework (already in project)
- **ImNodes**: Node graph visualization (already in project)
- **SDL3**: Input/event handling (already in project)

### Memory Management
- Debugger uses circular buffer for execution log (max 100 entries)
- Layout data is computed on-demand and cached
- Entity list refreshes periodically (configurable)

### Thread Safety
The debugger is designed for single-threaded use in the main game loop. All queries and updates happen on the main thread during the render phase.

## Future Enhancements

Potential additions for future versions:
- Breakpoint support (pause execution at specific nodes)
- Node execution statistics (call count, average duration)
- Blackboard variable editing
- Tree comparison (before/after changes)
- Export execution trace to file
- Multi-entity comparison view

## See Also

- [AI Systems Documentation](../technical-reference/ai/behavior-trees.md)
- [Behavior Tree JSON Format](../technical-reference/ai/bt-json-format.md)
- [ECS Architecture](../technical-reference/architecture/ecs-overview.md)
- [Component Reference](../api-reference/components.md)
