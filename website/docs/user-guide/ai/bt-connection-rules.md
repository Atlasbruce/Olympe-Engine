---
id: bt-connection-rules
title: "Behavior Tree Connection Rules"
sidebar_label: "Connection Rules"
---

# Behavior Tree Connection Rules

Understanding how to connect nodes correctly is essential for building valid behavior trees. This guide explains the connection rules enforced by the editor.

## Basic Connection Principles

### Parent-Child Relationships

Behavior trees are **hierarchical structures** where:
- **Parent nodes** control the execution of their children
- **Child nodes** are executed by their parents
- Connections flow **downward** (from root to leaves)
- Each node has **exactly one parent** (except the root)

### Visual Representation

In the editor, connections are shown as:
- **Lines** connecting parent output to child input
- **Arrows** indicating execution flow direction
- **Pins** on nodes (output pins on bottom, input pins on top)

```
┌──────────┐
│ Selector │ ← Parent (Output pin)
└────┬─────┘
     │ Connection (flow downward)
     │
┌────┴─────┐
│ Sequence │ ← Child (Input pin)
└──────────┘
```

---

## Node Connection Types

### Pin Types

Each node has specific connection capabilities:

| Node Type | Input Pins | Output Pins | Max Children |
|-----------|------------|-------------|--------------|
| **Selector** | 1 | Multiple | Unlimited |
| **Sequence** | 1 | Multiple | Unlimited |
| **Inverter** | 1 | 1 | 1 |
| **Repeater** | 1 | 1 | 1 |
| **Condition** | 1 | 0 | 0 |
| **Action** | 1 | 0 | 0 |

### Root Node

- **Every tree must have exactly one root node**
- The root node has **no input pin** (no parent)
- Typically a **Selector** or **Sequence**
- Execution starts here

**Valid root nodes:**
- ✅ Selector
- ✅ Sequence
- ⚠️ Inverter (unusual but valid)
- ⚠️ Repeater (for infinite loops)
- ❌ Condition (cannot have children)
- ❌ Action (cannot have children)

---

## Connection Rules by Category

### Composite Nodes (Selector, Sequence)

**Can connect to:**
- ✅ Any node type (Composite, Decorator, Condition, Action)
- ✅ Multiple children (no limit)

**Cannot be connected to:**
- ❌ Another child if already at root level (one root only)

**Example:**
```
Selector (Root)
 ├─ Sequence (child 1) ✅
 ├─ Condition (child 2) ✅
 ├─ Action (child 3) ✅
 └─ Inverter (child 4) ✅
```

**Best Practices:**
- Limit children to 2-5 for readability
- Order children by priority (Selector) or sequence (Sequence)
- Use sub-Sequences/Selectors to group related logic

---

### Decorator Nodes (Inverter, Repeater)

**Can connect to:**
- ✅ Exactly one child (any type)

**Cannot connect to:**
- ❌ Multiple children (limit: 1)
- ❌ No children (must have exactly 1)

**Example:**
```
✅ Valid:
Inverter
 └─ Condition "Target Visible"

❌ Invalid:
Inverter
 ├─ Condition "Target Visible"
 └─ Action "Idle"  ❌ Too many children!

❌ Invalid:
Inverter
 (no children) ❌ Must have 1 child!
```

**Best Practices:**
- Keep decorator chains short (max 2 decorators stacked)
- Use descriptive names when stacking decorators

---

### Leaf Nodes (Condition, Action)

**Can connect to:**
- ❌ Cannot have children

**Can be connected from:**
- ✅ Any parent node (Composite, Decorator)

**Example:**
```
✅ Valid:
Selector
 └─ Condition "Target Visible"

❌ Invalid:
Condition "Target Visible"
 └─ Action "Attack" ❌ Conditions cannot have children!
```

**Best Practices:**
- Always connect Conditions and Actions as leaves
- Never try to add children to leaf nodes

---

## Common Connection Patterns

### Pattern 1: Guarded Action (Sequence)

**Purpose:** Execute action only if condition passes

```
Sequence "Attack if Close"
 ├─ Condition "Target in Range"
 └─ Action "Attack Melee"
```

**Why it works:**
- Sequence checks condition first
- If condition fails, action is skipped
- If condition succeeds, action executes

---

### Pattern 2: Priority Selection (Selector)

**Purpose:** Try behaviors in order of priority

```
Selector "AI Decision"
 ├─ Sequence "Combat" (high priority)
 ├─ Sequence "Investigate" (medium priority)
 └─ Sequence "Patrol" (low priority)
```

**Why it works:**
- Selector tries children left-to-right
- First success wins
- Falls back to next option if previous fails

---

### Pattern 3: Inverted Condition (Inverter)

**Purpose:** Execute action when condition is false

```
Sequence "Patrol When No Target"
 ├─ Inverter "NOT Has Target"
 │   └─ Condition "Target Visible"
 └─ Action "Patrol"
```

**Why it works:**
- Inverter flips Success/Failure
- Patrol only runs if target NOT visible
- Clear "NOT" logic

---

### Pattern 4: Repeated Action (Repeater)

**Purpose:** Execute action multiple times

```
Sequence "Fire 3 Shots"
 ├─ Condition "Can Attack"
 └─ Repeater (count=3)
     └─ Action "Fire Projectile"
```

**Why it works:**
- Repeater loops its child N times
- Guard with condition to prevent invalid repeats
- Returns success after N executions

---

### Pattern 5: Sub-Tree Organization

**Purpose:** Group related behaviors

```
Selector "Main AI"
 ├─ Sequence "Combat Logic"
 │   ├─ Condition "Has Target"
 │   └─ Selector "Combat Options"
 │       ├─ Sequence "Melee Attack"
 │       └─ Sequence "Ranged Attack"
 └─ Sequence "Patrol Logic"
```

**Why it works:**
- Clear logical grouping
- Easier to understand
- Reusable sub-trees

---

## Validation Rules

The editor enforces these validation rules:

### Rule 1: Root Node Required
❌ **Error:** "No root node defined"
- Every tree must have a root node
- Root node has no parent

### Rule 2: No Orphaned Nodes
❌ **Warning:** "Node X is not connected to root"
- All nodes must be reachable from root
- Disconnected nodes won't execute

### Rule 3: Decorator Child Count
❌ **Error:** "Decorator must have exactly 1 child"
- Inverter/Repeater must have 1 child
- Cannot have 0 or 2+ children

### Rule 4: Leaf Node Children
❌ **Error:** "Leaf nodes cannot have children"
- Condition/Action nodes cannot have children
- Remove invalid connections

### Rule 5: Circular References
❌ **Error:** "Circular reference detected"
- Node cannot be ancestor of itself
- Prevents infinite loops in graph structure

### Rule 6: Duplicate Node IDs
❌ **Error:** "Node ID X appears multiple times"
- Each node must have unique ID
- Editor auto-generates IDs

---

## Connection Editor Controls

### Creating Connections

**Mouse:**
1. Click and drag from **parent's output pin**
2. Release on **child's input pin**
3. Connection created if valid

**Keyboard:**
1. Select parent node
2. Press `C` (Connect)
3. Select child node
4. Press `Enter` to confirm

### Removing Connections

**Mouse:**
- Right-click on connection line → "Delete Connection"

**Keyboard:**
- Select connection → Press `Delete`

### Reordering Children

**Mouse:**
- Drag child node left/right (Selector/Sequence)
- Order matters for execution

**Keyboard:**
- Select child → `Ctrl+Up` / `Ctrl+Down`

---

## Visual Feedback

### Valid Connection Indicators

✅ **Green highlight** - Valid drop target
- Parent can accept this child
- Connection allowed

### Invalid Connection Indicators

❌ **Red highlight** - Invalid drop target
- Connection not allowed (see error)
- Release to cancel

🟡 **Yellow warning** - Unusual but valid
- Leaf node as root (works but uncommon)
- Very deep nesting (works but hard to read)

---

## Troubleshooting Connections

### "Cannot connect nodes"

**Problem:** Trying to connect incompatible nodes

**Solutions:**
- ✅ Check node types (leaf nodes cannot have children)
- ✅ Check decorator limit (must have exactly 1 child)
- ✅ Verify connection direction (parent → child)

---

### "Node not executing"

**Problem:** Node appears in tree but doesn't run

**Solutions:**
- ✅ Verify node connected to root (no orphans)
- ✅ Check parent node logic (Selector/Sequence success/failure)
- ✅ Use debugger to trace execution path

---

### "Circular reference error"

**Problem:** Tree has cycle (A → B → A)

**Solutions:**
- ✅ Remove connection creating cycle
- ✅ Redesign tree structure
- ✅ Use separate sub-trees instead

---

## Best Practices

### ✅ DO

1. **Keep connections simple**
   - Prefer shallow hierarchies (2-4 levels)
   - Group related logic in sub-trees

2. **Use meaningful structure**
   - Selector for priorities (first success)
   - Sequence for steps (all must succeed)

3. **Test connections incrementally**
   - Add one connection at a time
   - Test in debugger after each change

4. **Follow visual flow**
   - Top-down or left-right execution
   - Consistent ordering across tree

### ❌ DON'T

1. **Don't create orphaned nodes**
   - Every node must connect to root
   - Remove unused nodes

2. **Don't over-nest**
   - Avoid 5+ levels of depth
   - Split into multiple trees if needed

3. **Don't ignore validation errors**
   - Fix errors before saving
   - Invalid trees won't load at runtime

4. **Don't connect randomly**
   - Think about execution flow
   - Plan tree structure before connecting

---

## Connection Checklist

Before finalizing your tree, verify:

- [ ] Exactly one root node exists
- [ ] All nodes connected to root (no orphans)
- [ ] No circular references
- [ ] Decorators have exactly 1 child
- [ ] Leaf nodes have 0 children
- [ ] Execution order makes sense
- [ ] Tree depth is reasonable (2-4 levels)
- [ ] Validation passes (no errors)

---

## Next Steps

Now that you understand connection rules:

- **[Keyboard Shortcuts](bt-keyboard-shortcuts.md)** - Work faster with hotkeys
- **[First BT Tutorial](bt-first-tree-tutorial.md)** - Build a complete tree
- **[Node Types](bt-node-types.md)** - Review available nodes

## Additional Resources

- [BT Debugger](bt-debugger.md) - Visualize connections at runtime
- [Behavior Tree Basics](../../tutorials/behavior-tree-basics.md) - Complete tutorial
- [Technical Reference](../../technical-reference/behavior-trees/behavior-trees-overview.md) - Architecture details
