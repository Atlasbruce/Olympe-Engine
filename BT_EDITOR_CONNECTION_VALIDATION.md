# BT Editor - Connection Rules & Validation System

## Overview

This document describes the connection rules and validation system implemented for the Behavior Tree Editor in Olympe Engine. The system ensures that behavior trees follow proper hierarchical structure and prevents common errors during graph editing.

## Architecture

### Core Components

1. **BTConnectionValidator** (`Source/BlueprintEditor/BTConnectionValidator.h/cpp`)
   - Core validation logic for connection rules
   - Implements cycle detection algorithm
   - Provides parent/child counting and validation

2. **BlueprintValidator** (`Source/BlueprintEditor/BlueprintValidator.h/cpp`)
   - Graph-level validation
   - Integrates BTConnectionValidator for connection validation
   - Validates node types, parameters, and structure

3. **LinkNodesCommand** (`Source/BlueprintEditor/CommandSystem.h/cpp`)
   - Validates connections before execution
   - Provides error messages for invalid connections
   - Prevents invalid links from being created

4. **NodeGraphPanel** (`Source/BlueprintEditor/NodeGraphPanel.cpp`)
   - Real-time visual feedback during link creation
   - Shows tooltips for invalid connections
   - Integrates with validation system

5. **BlueprintEditor Save** (`Source/BlueprintEditor/blueprinteditor.cpp`)
   - Blocks save on critical errors
   - Shows warning dialog for non-critical issues
   - Ensures graph integrity before saving

## Connection Rules

### Node Types and Their Rules

#### Composite Nodes (Selector, Sequence)
- **Max Parents:** 1
- **Max Children:** Unlimited
- **Min Children:** 1 (recommended, warning if 0)
- **Description:** OR/AND logic nodes that execute children in order

#### Decorator Nodes (Repeater, Inverter)
- **Max Parents:** 1
- **Max Children:** 1 (mandatory)
- **Min Children:** 1 (error if 0)
- **Description:** Modifier nodes that alter child behavior

#### Leaf Nodes (Action, Condition)
- **Max Parents:** 1
- **Max Children:** 0 (cannot have children)
- **Description:** Terminal nodes that perform actions or check conditions

#### Root Node
- **Max Parents:** 0 (cannot have parent)
- **Max Children:** At least 1 (recommended)
- **Description:** Entry point of the behavior tree

### Graph-Level Rules

1. **Single Parent Rule**
   - Each node can have at most one parent
   - Enforced during link creation
   - Validated at graph level

2. **No Cycles**
   - A node cannot reach itself through child connections
   - Uses depth-first search to detect cycles
   - Prevents infinite loops in tree execution

3. **Single Root**
   - Graph must have exactly one root node
   - Root node is explicitly marked in graph data
   - Orphan nodes (no parent, not root) are detected as warnings

4. **Valid References**
   - All child IDs must reference existing nodes
   - Decorator child must exist if specified
   - Prevents dangling references

## Validation Levels

### Error Severities

1. **Critical** (Red)
   - Blocks save operation
   - Must be fixed before blueprint can be saved
   - Examples: cycles, missing nodes, root has parent

2. **Error** (Orange)
   - Blocks save operation
   - Indicates structural problems
   - Examples: decorator with no child, leaf with children, multiple parents

3. **Warning** (Yellow)
   - Allows save with user confirmation
   - Should be addressed but not blocking
   - Examples: composite with no children, orphan nodes

4. **Info** (Blue)
   - Informational messages
   - Does not block save
   - Examples: node count, structural hints

## Real-Time Validation

### During Link Creation

When a user drags a connection between nodes:

1. **Pre-validation**: System checks if connection is valid
   - Verifies parent can accept child
   - Verifies child can accept parent
   - Checks for cycle creation
   - Validates node types

2. **Visual Feedback**:
   - Invalid connection shows error tooltip
   - Connection is not created if invalid
   - User sees error message explaining why

3. **Link Execution**:
   - Only valid connections are executed
   - Invalid attempts are logged to console
   - No undo entry created for failed attempts

### During Editing

The ValidationPanel continuously monitors the graph:

1. **Auto-Validation** (every 2 seconds by default)
   - Checks entire graph structure
   - Updates error list automatically
   - Can be disabled if needed

2. **Manual Validation** (on-demand)
   - Triggered by "Validate Now" button
   - Useful for checking after multiple changes
   - Shows immediate feedback

### Before Save

When saving a blueprint:

1. **Critical Error Check**:
   - Save is blocked if critical/error severity issues exist
   - Error popup shows reason for block
   - User must fix issues before saving

2. **Warning Check**:
   - Dialog shown if warnings exist
   - User can choose to save anyway or cancel
   - Provides opportunity to review issues

## API Reference

### BTConnectionValidator

```cpp
// Check if a connection can be created
ConnectionValidationResult CanCreateConnection(
    const NodeGraph* graph, 
    int parentId, 
    int childId) const;

// Check for cycles
bool WouldCreateCycle(
    const NodeGraph* graph, 
    int parentId, 
    int childId) const;

// Get parent node
int GetParentNode(const NodeGraph* graph, int nodeId) const;

// Get orphan nodes
std::set<int> GetOrphanNodes(const NodeGraph* graph) const;

// Get root nodes
std::set<int> GetRootNodes(const NodeGraph* graph) const;
```

### BlueprintValidator

```cpp
// Validate entire graph
std::vector<ValidationError> ValidateGraph(const NodeGraph* graph);

// Validate single node
std::vector<ValidationError> ValidateNode(const NodeGraph* graph, int nodeId);

// Check if graph is valid
bool IsGraphValid(const NodeGraph* graph);
```

### LinkNodesCommand

```cpp
// Constructor validates connection
LinkNodesCommand(const std::string& graphId, int parentId, int childId);

// Check if link is valid
bool IsValid() const;

// Get validation error message
std::string GetValidationError() const;
```

## Usage Examples

### Example 1: Detecting Cycles

```cpp
BTConnectionValidator validator;
NodeGraph* graph = /* ... */;

// Check if connecting parent->child would create cycle
if (validator.WouldCreateCycle(graph, parentId, childId)) {
    std::cerr << "Cannot create connection: would create cycle" << std::endl;
    return false;
}
```

### Example 2: Validating Before Save

```cpp
BlueprintValidator validator;
NodeGraph* graph = /* ... */;

auto errors = validator.ValidateGraph(graph);
bool hasCriticalErrors = false;

for (const auto& error : errors) {
    if (error.severity == ErrorSeverity::Critical || 
        error.severity == ErrorSeverity::Error) {
        hasCriticalErrors = true;
        break;
    }
}

if (hasCriticalErrors) {
    // Block save
    return false;
}
```

### Example 3: Creating Validated Connection

```cpp
// Create link command (automatically validates)
auto cmd = std::make_unique<LinkNodesCommand>(graphId, parentId, childId);

if (cmd->IsValid()) {
    // Execute command
    commandStack->ExecuteCommand(std::move(cmd));
} else {
    // Show error
    std::cerr << "Invalid connection: " << cmd->GetValidationError() << std::endl;
}
```

## Validation Panel

The ValidationPanel provides a centralized view of all validation issues:

- **Error Summary**: Shows count by severity
- **Error List**: Detailed list with node IDs and messages
- **Auto-Validate Toggle**: Enable/disable automatic validation
- **Manual Validate Button**: Trigger validation on demand
- **Click to Navigate**: Click error to focus on problematic node (future)

## Testing Guidelines

### Manual Testing Checklist

1. **Connection Rules**:
   - [ ] Try to connect leaf node as parent (should fail)
   - [ ] Try to connect node to itself (should fail)
   - [ ] Try to create cycle A→B→C→A (should fail)
   - [ ] Try to give node multiple parents (should fail)
   - [ ] Try to give decorator multiple children (should fail)

2. **Validation Panel**:
   - [ ] Create tree with errors, check panel shows them
   - [ ] Fix errors, verify they disappear from panel
   - [ ] Test auto-validation toggle
   - [ ] Test manual validation button

3. **Save Protection**:
   - [ ] Try to save with critical errors (should block)
   - [ ] Try to save with warnings (should show dialog)
   - [ ] Save valid tree (should succeed)

4. **Visual Feedback**:
   - [ ] Drag invalid connection, check tooltip appears
   - [ ] Verify error message is clear and helpful

### Automated Testing

For automated testing, consider:

1. Unit tests for BTConnectionValidator
2. Integration tests for validation during save
3. UI tests for validation panel behavior

## Future Enhancements

### Planned Features

1. **Visual Pin Highlighting**
   - Green highlight for valid connection targets
   - Red highlight for invalid targets
   - Requires ImNodes API enhancements

2. **Connection Path Visualization**
   - Show connection path being created
   - Animate invalid connections
   - Visual cycle detection feedback

3. **Smart Auto-Fix**
   - Suggest fixes for common errors
   - One-click fix for simple issues
   - Batch fix for similar errors

4. **Custom Validation Rules**
   - User-defined connection constraints
   - Plugin system for validators
   - Project-specific rules

5. **Performance Optimization**
   - Cache validation results
   - Incremental validation
   - Validation scheduling

## Known Limitations

1. **ImNodes API**: Current version of ImNodes doesn't support pin color customization during drag
2. **Performance**: Large trees (>1000 nodes) may experience slow validation
3. **Visual Feedback**: Limited to tooltips and error panel, no inline highlighting yet

## Troubleshooting

### Common Issues

**Q: Why can't I connect my nodes?**
A: Check the error tooltip when attempting connection. Common reasons:
- Node already has maximum children
- Would create a cycle
- Child already has a parent
- Node types incompatible (e.g., leaf as parent)

**Q: Why can't I save my blueprint?**
A: Check the Validation panel for errors. Save is blocked if critical errors exist.

**Q: How do I fix orphan nodes?**
A: Either delete the orphan node, or connect it to the main tree, or make it the root node.

**Q: What's the difference between Error and Warning?**
A: Errors block save and must be fixed. Warnings allow save with confirmation.

## References

- **Behavior Tree Implementation**: `Source/AI/BehaviorTree.h`
- **Node Types**: `Source/BlueprintEditor/NodeGraphManager.h`
- **Validation Errors**: `Source/BlueprintEditor/BlueprintValidator.h`
- **Command System**: `Source/BlueprintEditor/CommandSystem.h`

## Changelog

### Version 1.0 (2026-02-15)
- Initial implementation of connection validation
- Real-time validation during link creation
- Save protection with critical error blocking
- Warning dialog for non-critical issues
- Comprehensive graph-level validation
- Cycle detection algorithm
- Orphan node detection
- Multiple root detection
- Parent/child count validation
