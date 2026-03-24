# Behavior Tree Runtime Debugger - UI Layout Reference

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│ Behavior Tree Runtime Debugger                                                      [×]     │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│ View | Actions                                                                               │
│  └─ Auto Refresh (0.5s)                      └─ Refresh Now (F5)                            │
│  └─ Entity List Width (250px)                └─ Clear Execution Log                         │
│  └─ Inspector Width (350px)                                                                  │
│  └─ Node Spacing X/Y (250px/180px)           └─ Reset Spacing to Defaults                   │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                       │                                               │                      │
│  Entity List          │         Node Graph (ImNodes)                 │     Inspector        │
│  (Left Panel)         │         (Center Panel)                       │     (Right Panel)    │
│  250px                │         ~900px                               │     350px            │
│                       │                                               │                      │
├───────────────────────┤  Layout: ● Vertical  ○ Horizontal            ├──────────────────────┤
│ Entities with BT      │  ─────────────────────────────────           │ Inspector            │
│ ────────────────      │              ┌────────────┐                  │ ─────────            │
│ Search: [_______]     │              │ ? RootSel  │ ◄── Blue         │                      │
│ ☑ Active Only         │              │  (0)       │                  │ ▼ Runtime Info       │
│ ☑ Has Target          │                    │                         │  Tree ID: 42         │
│                       │         ┌──────────┴──────────┐              │  Tree: Patrol_BT     │
│ Sort: [Name ▼] [Asc]  │         │                     │              │  Node ID: 7          │
│ ─────────────────     │    ┌────▼─────┐        ┌─────▼────┐         │  Node: MoveToPoint   │
│ Entities: 5/10        │    │ → Patrol │        │ → Combat │         │  Status: Success ✓   │
│                       │    │   Seq    │ Green  │   Seq    │ Green   │  Active: Yes         │
│ ● ✓ Guard_01          │    │   (3)    │        │   (4)    │         │  AI Mode: Patrol     │
│     Patrol_BT         │    └────┬─────┘        └─────┬────┘         │  Time: 12.3s         │
│                       │         │                    │               │                      │
│ ● ▶ Enemy_02 ◄────────┼─────────┤              ┌─────┴────┐         │ ▼ Blackboard         │
│     Combat_BT   SELECTED         │              │ ◆ Target │ Orange  │  ▶ Target            │
│                       │    ┌─────▼────┐         │   Visible│         │  ▶ Movement          │
│ ○ ✗ NPC_03            │    │ ► Move   │ Rose    │   (5)    │         │  ▶ Patrol            │
│     Wander_BT         │    │   ToPoint│ PULSING └──────────┘         │  ▼ Combat            │
│                       │    │   (7)    │ YELLOW                        │   Can Attack: Yes    │
│ ● ✓ Soldier_04        │    └──────────┘                              │   Cooldown: 1.5s     │
│     Guard_BT          │                                               │   Last Attack: 0.8s  │
│                       │                                               │  ▶ Stimuli           │
│ ● ▶ Boss_05           │  [Mouse wheel to zoom]                       │  ▶ Wander            │
│     Boss_BT           │  [Middle mouse to pan]                       │                      │
│                       │  [Hover nodes for details]                   │ ▼ Execution Log      │
│ [Refresh]             │                                               │  [Clear Log]         │
│                       │                                               │  ─────────────       │
│                       │                                               │  [0.1s] ✓ Node 7     │
│                       │                                               │         (MoveToPoint)│
│                       │                                               │  [0.5s] ▶ Node 5     │
│                       │                                               │         (TargetVis)  │
│                       │                                               │  [1.2s] ✓ Node 3     │
│                       │                                               │         (PatrolSeq)  │
│                       │                                               │  [2.0s] ✓ Node 0     │
│                       │                                               │         (RootSel)    │
│                       │                                               │  ...                 │
│                       │                                               │                      │
└───────────────────────┴───────────────────────────────────────────────┴──────────────────────┘

Window Size: 1400x900 pixels (default)
Position: 100, 100 (default, first use)
```

## Color Legend

### Node Types (in graph)
- **Blue** (IM_COL32(100, 150, 255, 255)) - Selector (?) - OR logic
- **Green** (IM_COL32(100, 255, 150, 255)) - Sequence (→) - AND logic
- **Orange** (IM_COL32(255, 200, 100, 255)) - Condition (◆) - Checks
- **Rose** (IM_COL32(255, 100, 150, 255)) - Action (►) - Executes
- **Purple** (IM_COL32(200, 100, 255, 255)) - Inverter (!) - Inverts result
- **Light Blue** (IM_COL32(150, 150, 255, 255)) - Repeater (↻) - Repeats child

### Status Icons (in entity list and log)
- **●** Active entity / **○** Inactive entity
- **✓** Success (green)
- **✗** Failure (red)
- **▶** Running (yellow)

### Current Execution
- **YELLOW PULSING BORDER** - The node currently being executed
  - Pulses between 50% and 100% opacity at 1 Hz
  - Only visible when entity is active and executing

## Interaction Guide

### Entity List Panel
1. **Click** entity to select and view its tree
2. **Type** in search box to filter by name
3. **Check** "Active Only" to hide inactive entities
4. **Check** "Has Target" to show only entities with targets
5. **Select** sort mode from dropdown
6. **Click** Asc/Desc button to toggle sort order
7. **Click** Refresh button for manual refresh (or press F5)

### Node Graph Panel
1. **Toggle Layout** - Click "Vertical" or "Horizontal" radio buttons to switch layout
2. **Mouse Wheel** to zoom in/out
3. **Middle Mouse Button + Drag** to pan
4. **Hover** over nodes to see details
5. **Watch** yellow pulsing highlight on current node
6. **Follow** execution flow through tree visually

**Layout Modes:**
- **Vertical** (default): Traditional top-to-bottom tree - best for deep trees or balanced structures
- **Horizontal**: Left-to-right flowchart - best for wide trees with many siblings

### Inspector Panel
1. **Click** section headers to expand/collapse
2. **Read** runtime info (tree, node, status)
3. **Monitor** blackboard variables in real-time
4. **Review** execution history in log
5. **Click** "Clear Log" to reset execution history

### Keyboard Shortcuts
- **F10** - Toggle debugger window (open/close)
- **F5** - Manual refresh (when window is open)
- **ESC** - Close debugger window

## Usage Scenarios

### Scenario 1: Debug Patrol Behavior
1. Press F10 to open debugger
2. Search for patrol NPCs
3. Select an NPC from list
4. Watch nodes execute in real-time
5. Check blackboard patrol points
6. Verify movement goals are set correctly

### Scenario 2: Investigate Combat Issues
1. Filter entities with "Has Target"
2. Select combat entity
3. Watch condition checks (TargetVisible, TargetInRange)
4. Verify combat cooldowns in blackboard
5. Check execution log for failure patterns
6. Identify which nodes are failing

### Scenario 3: Performance Analysis
1. Open debugger on multiple entities
2. Monitor execution log accumulation rate
3. Check if trees are executing too frequently
4. Adjust thinkHz if needed
5. Verify layout performance with large trees

### Scenario 4: Tree Design Validation
1. Select entity with new tree
2. Verify visual layout is clean (no overlaps)
3. Check all nodes are connected correctly
4. Follow execution flow makes logical sense
5. Confirm node types match intent (colors/icons)

## Tips and Tricks

1. **Keep window closed when not debugging** - Saves performance
2. **Increase auto-refresh interval** - If monitoring many entities
3. **Use filtering aggressively** - Reduces visual clutter
4. **Clear log frequently** - Keeps history relevant
5. **Adjust spacing** - If graph too cramped or sparse
6. **Toggle layout direction** - Use horizontal for wide trees, vertical for deep trees
7. **Watch pulsing highlight** - Best way to track execution
8. **Check blackboard first** - Often reveals root cause
9. **Use execution log** - To find patterns over time

## Layout Direction Feature

The debugger supports two layout orientations, selectable via radio buttons in the node graph panel:

### Vertical Layout (TopToBottom)
```
        [Root]
          |
    +-----------+
    |           |
 [Child1]   [Child2]
    |           |
 [Leaf1]    [Leaf2]
```
**Characteristics:**
- Root at top, layers progress downward
- Siblings spread horizontally
- Traditional hierarchical tree view
- **Best for**: Deep trees, balanced structures

### Horizontal Layout (LeftToRight)
```
                [Child1] → [Leaf1]
               /
[Root] -------+
               \
                [Child2] → [Leaf2]
```
**Characteristics:**
- Root at left, layers progress rightward
- Siblings spread vertically
- Flowchart-style visualization
- **Best for**: Wide trees with many siblings, limited vertical space

### When to Use Each Layout

**Use Vertical:**
- Tree has many layers (deep)
- Limited horizontal screen space
- Prefer traditional tree diagrams
- Tree is roughly balanced

**Use Horizontal:**
- Tree has many children per node (wide)
- Limited vertical screen space
- Prefer flowchart-style visualization
- Tree is shallow but wide

### Implementation Details
- Layout changes are instant (no recompilation needed)
- Uses 90° clockwise coordinate rotation
- Preserves relative node positions
- No performance difference between modes

## Performance Considerations

- **Layout Calculation**: ~5ms for 50-node tree (acceptable)
- **Render Cost**: ~1ms for 10 entities (minimal)
- **Memory**: ~100 log entries × N entities (bounded)
- **Refresh Rate**: Default 0.5s (2 Hz) - balanced
- **When Closed**: Zero overhead (not rendering)

## Common Issues

**Issue**: Window doesn't open
**Solution**: Press F10 again, check console for errors

**Issue**: No entities shown
**Solution**: Ensure entities have BehaviorTreeRuntime_data, check filters

**Issue**: Graph not rendering
**Solution**: Select entity from list, verify tree is loaded

**Issue**: Performance lag
**Solution**: Close window when not using, increase refresh interval

**Issue**: Execution log not updating
**Solution**: Ensure entity is active, verify behavior tree is executing

---

This ASCII art diagram shows the approximate layout and component arrangement. The actual rendered UI will use ImGui styling and be fully interactive with smooth animations and proper fonts.
