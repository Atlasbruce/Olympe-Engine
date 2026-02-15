# BT Editor Validation - Quick Reference

## Quick Start

### Basic Usage

1. **Open Validation Panel**: View → Validation
2. **Create Nodes**: Drag from palette or right-click
3. **Connect Nodes**: Drag from output (Out) to input (In) pin
4. **Check Errors**: See Validation panel for issues
5. **Fix Errors**: Critical errors must be fixed before save
6. **Save**: File → Save (Ctrl+S)

## Connection Rules Quick Reference

| Node Type | Max Parents | Min Children | Max Children | Notes |
|-----------|-------------|--------------|--------------|-------|
| Selector  | 1           | 1 (warn)     | Unlimited    | OR logic |
| Sequence  | 1           | 1 (warn)     | Unlimited    | AND logic |
| Repeater  | 1           | 1 (error)    | 1            | Decorator |
| Inverter  | 1           | 1 (error)    | 1            | Decorator |
| Action    | 1           | 0            | 0            | Leaf node |
| Condition | 1           | 0            | 0            | Leaf node |
| Root      | 0           | 1+ (warn)    | Unlimited    | Entry point |

## Common Errors

### "Cannot connect node to itself"
❌ **Wrong**: Node → Node  
✅ **Right**: Node A → Node B

### "Would create a cycle"
❌ **Wrong**: A → B → C → A  
✅ **Right**: A → B → C

### "Node already has a parent"
❌ **Wrong**: 
```
Root → A
    ↘
B   → A
```
✅ **Right**: Each node has only one parent

### "Leaf node cannot have children"
❌ **Wrong**: Action → Sequence  
✅ **Right**: Sequence → Action

### "Decorator must have exactly 1 child"
❌ **Wrong**: Repeater (no children)  
✅ **Right**: Repeater → Action

### "Root node cannot have parent"
❌ **Wrong**: Sequence → Root  
✅ **Right**: Root is always at top

## Validation Severity Levels

| Level | Color | Blocks Save? | Description |
|-------|-------|--------------|-------------|
| Critical | 🔴 Red | YES | Structural failure, must fix |
| Error | 🟠 Orange | YES | Rule violation, must fix |
| Warning | 🟡 Yellow | NO (dialog) | Should fix, can save |
| Info | 🔵 Blue | NO | Informational only |

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Save | Ctrl+S |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y |
| Delete Node | Delete |
| Validate | F5 |

## Tips & Tricks

### 💡 Tip 1: Use Validation Panel
Keep the Validation panel open while editing to see errors in real-time.

### 💡 Tip 2: Fix Errors Top-Down
Start with critical errors, then errors, then warnings.

### 💡 Tip 3: Check Before Save
Run manual validation (F5) before attempting to save.

### 💡 Tip 4: Understand Node Types
Learn which nodes can have children:
- **Can have children**: Selector, Sequence, Repeater, Inverter, Root
- **Cannot have children**: Action, Condition

### 💡 Tip 5: Root Node Setup
Always start with a root node (usually Selector or Sequence) at the top level.

## Troubleshooting

### "I can't connect these nodes!"
1. Check tooltip that appears when you try to connect
2. Verify parent can have children
3. Verify child doesn't already have parent
4. Check for cycle creation

### "Save is blocked!"
1. Open Validation panel
2. Look for red/orange errors
3. Fix each error
4. Try save again

### "Too many warnings!"
1. Warnings don't block save
2. Click "Save Anyway" in dialog
3. Or fix warnings for cleaner tree

### "Where's the error?"
1. Check Validation panel
2. Note the node ID in error message
3. Find node in graph (future: click to navigate)

## Best Practices

### ✅ Do This
- Start with root Selector/Sequence
- Build tree top-down
- Keep tree shallow (3-5 levels)
- Use meaningful node names
- Validate frequently
- Fix errors immediately

### ❌ Avoid This
- Creating cycles
- Orphan nodes
- Deep nesting (>10 levels)
- Multiple roots
- Leaf nodes with children
- Decorators without children

## Examples

### Simple Patrol Tree
```
Root (Selector)
├─ Condition: HasTarget
│  └─ Sequence: Attack
│     ├─ Action: MoveToTarget
│     └─ Action: AttackMelee
└─ Sequence: Patrol
   ├─ Action: PickPatrolPoint
   └─ Action: MoveToGoal
```

### Valid Structure
✅ All nodes have at most one parent  
✅ No cycles  
✅ Decorators have exactly one child  
✅ Leaf nodes have no children  
✅ One root node  

## API Quick Reference

### Check Connection Validity
```cpp
BTConnectionValidator validator;
auto result = validator.CanCreateConnection(graph, parentId, childId);
if (!result.isValid) {
    std::cerr << result.errorMessage << std::endl;
}
```

### Validate Graph
```cpp
BlueprintValidator validator;
auto errors = validator.ValidateGraph(graph);
for (const auto& error : errors) {
    std::cerr << error.message << std::endl;
}
```

### Create Validated Link
```cpp
auto cmd = std::make_unique<LinkNodesCommand>(graphId, parentId, childId);
if (cmd->IsValid()) {
    commandStack->ExecuteCommand(std::move(cmd));
}
```

## Further Reading

- Full Documentation: `BT_EDITOR_CONNECTION_VALIDATION.md`
- Behavior Tree Guide: `website/docs/user-guide/behavior-trees/`
- API Reference: `website/docs/api-reference/`
