# Blueprint Editor - User Guide

## 3-Panel Architecture Overview

The Blueprint Editor features a streamlined, professional 3-panel layout designed for maximum productivity and minimal confusion.

---

## 🎯 The Three Panels

### 1. Asset Browser (Left Panel) 📁

The Asset Browser is your navigation hub with two tabs:

#### **Blueprint Files Tab**
- Browse all blueprint assets in a tree view
- **File Types Supported:**
  - 🌳 BehaviorTree - AI behavior definitions
  - 🔄 HFSM - Hierarchical Finite State Machines
  - 🧩 EntityBlueprint - Entity templates with components
  - 🔧 Prefab - Reusable entity prefabs
  - ⚡ Trigger - Event triggers
  - ✨ FX - Visual effects
  - 🔊 Sound - Audio assets

**Features:**
- 🔍 **Search** - Filter by filename
- 🎯 **Type Filter** - Show only specific asset types
- 🔄 **Refresh** - Rescan asset directory
- 🖱️ **Double-click** - Open BT/HFSM in Node Graph Editor
- 💡 **Tooltips** - Hover for quick info

**Workflow:**
```
1. Select tab "Blueprint Files"
2. Browse or search for asset
3. Single-click to select (Inspector shows metadata)
4. Double-click BT/HFSM to edit in Node Graph Editor
```

#### **Runtime Entities Tab**
- View all active entities from the game world
- See entity names and component counts
- Select entities to inspect in real-time

**Features:**
- 📊 **Entity Count** - Shows total active entities
- 🎯 **Selection** - Click to select entity
- 📝 **Component Badge** - Shows how many components each entity has
- 💡 **Tooltips** - Hover to see component list

**Workflow:**
```
1. Select tab "Runtime Entities"
2. Click an entity to select it
3. Inspector (right panel) shows entity components
4. Edit component properties in Inspector
```

---

### 2. Node Graph Editor (Center Panel) 🎨

The primary workspace for visual graph editing.

**Supported Graph Types:**
- 🌳 **Behavior Trees** - AI decision trees with composites, actions, conditions
- 🔄 **HFSM** - Hierarchical Finite State Machines with states and transitions

**Features:**
- 📑 **Multi-Tab Support** - Open multiple graphs simultaneously
- 🖱️ **Visual Editing** - Drag, connect, and arrange nodes
- 🎨 **Node Creation** - Right-click context menu to add nodes
- 💾 **Auto-Save** - Changes tracked per graph
- 🔍 **Zoom/Pan** - Navigate large graphs easily

**Workflow:**
```
1. Double-click BT/HFSM file in Asset Browser
2. Graph opens in new tab
3. Edit nodes visually
4. Switch between tabs to work on multiple graphs
5. Save via File menu (Ctrl+S)
```

**Node Types (Behavior Tree):**
- **Composite Nodes:**
  - Sequence - Execute children in order until one fails
  - Selector - Execute children until one succeeds
- **Leaf Nodes:**
  - Action - Perform specific behavior (MoveTo, Attack, etc.)
  - Condition - Check state (IsEnemyNear, HasAmmo, etc.)
- **Decorator Nodes:**
  - Invert - Reverse child result
  - Repeat - Loop child execution
  - Force Success/Failure - Override result

**Controls:**
- Left-click: Select node
- Drag: Move node
- Right-click: Open context menu
- Mouse wheel: Zoom in/out
- Middle mouse: Pan view

---

### 3. Inspector (Right Panel) 🔍

Context-sensitive properties panel that adapts to your selection.

#### **Context 1: Entity Selected**

When you select a Runtime Entity from Asset Browser:

**Displays:**
- Entity name and ID
- List of all components
- Expandable component properties
- Real-time editable values

**Component Types:**
- Transform - Position, rotation, scale
- Sprite - Texture, color, layer
- BehaviorTree - Active AI tree
- Collider - Physics bounds
- Health - HP, max HP, etc.

**Editing:**
```
1. Select entity in Asset Browser → Runtime Entities tab
2. Inspector shows entity info
3. Expand a component (click header)
4. Edit values:
   - Float: Drag slider
   - Int: Drag or type
   - Bool: Checkbox
   - String: Text input
5. Changes apply immediately to runtime entity
```

#### **Context 2: Asset File Selected**

When you select a Blueprint File from Asset Browser:

**Displays:**
- Asset filename
- Asset type (BehaviorTree, HFSM, EntityBlueprint, etc.)
- Name and description (if available)
- Type-specific metadata:
  - **BehaviorTree/HFSM**: Node count, node list
  - **EntityBlueprint**: Component count, component list

**Special Features:**
- 🚀 **"Open in Node Graph Editor" button** (for BT/HFSM)
  - Quick way to open graph without double-clicking in Asset Browser

**Workflow:**
```
1. Single-click asset file in Asset Browser
2. Inspector shows metadata
3. Browse node/component list
4. Click "Open in Node Graph Editor" to edit
```

#### **Context 3: Nothing Selected**

When no entity or asset is selected:

**Displays:**
- Helper text: "Select an entity or asset file to inspect its properties."
- Guides you to make a selection

---

## 🚀 Common Workflows

### Workflow 1: Editing a Behavior Tree
```
1. Press F2 to open Blueprint Editor
2. Asset Browser → Blueprint Files tab
3. Navigate to Blueprints/AI/
4. Double-click "guard_combat.json"
5. Graph opens in Node Graph Editor
6. Edit nodes, connections
7. File → Save (Ctrl+S)
```

### Workflow 2: Inspecting Runtime Entity
```
1. Press F2 to open Blueprint Editor
2. Asset Browser → Runtime Entities tab
3. Click an entity (e.g., "Guard_01")
4. Inspector shows components
5. Expand "Transform" component
6. Edit position X/Y/Z
7. Changes apply immediately
```

### Workflow 3: Browsing Assets
```
1. Asset Browser → Blueprint Files tab
2. Type filter: Select "BehaviorTree"
3. Search: Type "guard"
4. Single-click "guard_patrol.json"
5. Inspector shows BT metadata
6. View node list in Inspector
7. Click "Open in Node Graph Editor" if you want to edit
```

### Workflow 4: Multi-Graph Editing
```
1. Open first graph: guard_combat.json
2. Open second graph: guard_patrol.json
3. Both appear as tabs in Node Graph Editor
4. Click tabs to switch between graphs
5. Each graph remembers its viewport (zoom, scroll)
6. Save each graph independently
```

---

## ⌨️ Keyboard Shortcuts

### Global
- **F2** - Toggle Blueprint Editor on/off
- **F11** - Toggle fullscreen (mouse coords work correctly now!)

### File Operations
- **Ctrl+N** - New Blueprint
- **Ctrl+O** - Open Blueprint
- **Ctrl+S** - Save current graph
- **Ctrl+Shift+S** - Save As
- **Ctrl+Shift+T** - Save as Template

### Edit Operations
- **Ctrl+Z** - Undo last operation
- **Ctrl+Y** or **Ctrl+Shift+Z** - Redo last undone operation
- **Ctrl+D** - Duplicate selected node
- **Delete** - Delete selected node or link
- **Insert** - Add Component (when editing entity)
- **Double-click node** - Open edit modal for node parameters

### View
- Use View menu to toggle panels on/off

---

## 💡 Tips & Best Practices

### Asset Organization
- Keep BehaviorTrees in `Blueprints/AI/`
- Keep EntityBlueprints in `Blueprints/Entities/`
- Use descriptive names: `guard_combat.json`, not `bt1.json`

### Graph Editing
- Start with a root node (Sequence or Selector)
- Use Comments to annotate complex logic
- Keep trees shallow (prefer wide over deep)
- Test frequently with runtime entities

### Inspector Usage
- Always check component values before editing
- Use Inspector metadata to understand BT structure before opening
- Watch Runtime Entities tab during gameplay to debug

### Performance
- Close unused graph tabs to save memory
- Use Asset Browser filters to reduce clutter
- Refresh assets after external changes (File → Reload Assets)

---

## 🐛 Troubleshooting

### Problem: Double-click doesn't open graph
**Solution:** 
- Ensure file type is BehaviorTree or HFSM (check label in Asset Browser)
- EntityBlueprints cannot be opened in Node Graph Editor (use Inspector instead)

### Problem: Mouse clicks wrong location in fullscreen
**Solution:**
- This is fixed in this version! If still occurring, please restart editor.

### Problem: Inspector shows nothing
**Solution:**
- Select an entity or asset file first
- Check that Asset Browser has a selection (highlighted item)

### Problem: Can't see Runtime Entities
**Solution:**
- Switch to Asset Browser → Runtime Entities tab
- Ensure game world has created entities (entities created at runtime will appear)
- Count shown at top of tab: "Active Entities: N"

### Problem: Panel disappeared
**Solution:**
- View menu → Check panel is enabled
- View menu → Reset Layout (restores all 3 main panels)

---

## 🎓 Learning Path

### Beginner
1. Open existing BehaviorTree (double-click in Asset Browser)
2. Browse nodes in Node Graph Editor
3. Select Runtime Entity and view components in Inspector
4. Make small edits to component values

### Intermediate
1. Create new BehaviorTree from scratch
2. Add composite nodes (Sequence, Selector)
3. Add leaf nodes (Actions, Conditions)
4. Connect nodes to form logic
5. Test with Runtime Entity

### Advanced
1. Design complex multi-state AI with HFSM
2. Use decorators to control node execution
3. Create reusable templates from good designs
4. Debug AI by monitoring Runtime Entities during gameplay
5. Optimize graphs for performance

---

## 📚 Additional Resources

- **QUICKSTART.md** - Quick start guide
- **TESTING_GUIDE.md** - Testing procedures
- **ARCHITECTURE.md** - Technical architecture details
- **CHANGELOG_BLUEPRINT_UI_OPTIMIZATION.md** - Recent changes

---

## ✅ Quick Reference

### Panel Purposes
| Panel | Purpose | Primary Use |
|-------|---------|-------------|
| **Asset Browser** | Navigation | Browse files, select entities |
| **Node Graph Editor** | Editing | Create/modify BT/HFSM graphs |
| **Inspector** | Properties | View/edit metadata and components |

### Context Behavior
| Selection | Inspector Shows |
|-----------|-----------------|
| Runtime Entity | Entity components + properties |
| Blueprint File | Asset metadata + node/component list |
| Nothing | Helper text |

### File Types
| Type | Icon | Can Edit in Node Graph? | Double-Click Action |
|------|------|-------------------------|---------------------|
| BehaviorTree | 🌳 | ✅ Yes | Opens in Node Graph Editor |
| HFSM | 🔄 | ✅ Yes | Opens in Node Graph Editor |
| EntityBlueprint | 🧩 | ❌ No | No action (view in Inspector) |
| Prefab | 🔧 | ❌ No | No action (view in Inspector) |

---

## 🎨 Advanced Node Graph Editing

### Complete CRUD Operations

The Node Graph Editor now supports full Create, Read, Update, Delete operations with undo/redo.

#### Creating Nodes

**Method 1: Context Menu**
```
1. Right-click on empty canvas
2. Select node type from menu:
   - Composite → Sequence / Selector
   - Action → Choose from catalog
   - Condition → Choose from catalog
   - Decorator → Choose from catalog
3. Node appears at cursor position
```

**Method 2: Node Palette (Drag & Drop)**
```
1. Open Asset Browser
2. Switch to "Nodes" tab
3. Browse node categories:
   - Composites
   - Actions (from catalog)
   - Conditions (from catalog)
   - Decorators (from catalog)
4. Drag node from palette
5. Drop onto graph canvas
```

**Features:**
- 🎯 **Tooltips** - Hover over nodes in palette to see descriptions and parameters
- 📋 **Catalog Integration** - All action/condition/decorator types loaded from JSON catalogs
- 🎨 **Visual Feedback** - Drag preview shows what you're adding

#### Editing Nodes

**Double-click any node to open edit modal:**

**Action Node:**
- ✏️ Edit node name
- 🎯 Change action type (dropdown)
- 🔧 Edit parameters with type-specific widgets:
  - `float` - Drag slider
  - `string` - Text input
  - `bool` - Checkbox
  - Required parameters marked

**Condition Node:**
- ✏️ Edit node name
- 🎯 Change condition type
- 🔧 Edit condition-specific parameters

**Decorator Node:**
- ✏️ Edit node name
- 🎯 Change decorator type
- 🔧 Edit decorator-specific parameters

**Composite Node (Sequence/Selector):**
- ✏️ Edit node name only
- Children managed through links

**Features:**
- 💾 **Auto-save** - Changes saved automatically on OK
- ❌ **Cancel** - Discard changes
- 📝 **Validation** - Required parameters highlighted

#### Deleting Nodes & Links

**Delete Node:**
- Select node → Press `Delete` key
- OR right-click node → "Delete"
- ⚠️ Automatically removes all incoming/outgoing links
- 🔄 Fully undoable

**Delete Link:**
- Select link → Press `Delete` key
- OR right-click link → "Delete Link"
- 🔄 Fully undoable

#### Duplicating Nodes

**Duplicate a node with all its properties:**
```
1. Select node
2. Press Ctrl+D
3. OR right-click → "Duplicate"
4. New node appears offset from original
5. All parameters copied
```

**Note:** Links are NOT duplicated (prevents invalid graphs)

#### Context Menus

**Right-click on Node:**
- ✏️ **Edit** - Open edit modal (same as double-click)
- 📋 **Duplicate** - Create copy with offset position
- 🗑️ **Delete** - Remove node and its links

**Right-click on Empty Canvas:**
- ➕ **Create Node** - Menu with all available node types
- 🎨 **Auto Layout** - (Coming soon)

### Undo/Redo System

All editing operations support full undo/redo:

**Undoable Operations:**
- ✅ Create node
- ✅ Delete node (with links)
- ✅ Move node
- ✅ Edit node properties
- ✅ Create link
- ✅ Delete link
- ✅ Duplicate node

**Keyboard Shortcuts:**
- `Ctrl+Z` - Undo last operation
- `Ctrl+Y` or `Ctrl+Shift+Z` - Redo
- `Delete` - Delete selected node/link
- `Ctrl+D` - Duplicate selected node

**Features:**
- 📜 **Command History** - See History panel for command list
- 🔄 **Stack Limit** - Last 100 commands kept in memory
- 💾 **State Preservation** - Node positions, parameters, all preserved

### Using the Node Palette

The Node Palette provides a drag & drop interface for adding nodes quickly.

**Access:**
```
Asset Browser → "Nodes" tab
```

**Categories:**

**1. Composites** (always available)
- Sequence - Execute children until one fails
- Selector - Execute children until one succeeds

**2. Actions** (loaded from `Catalogues/ActionTypes.json`)
- MoveTo - Move entity to target
- Attack - Attack target entity
- Patrol - Follow patrol route
- Wait - Delay execution
- PlayAnimation - Trigger animation
- Heal - Restore health
- SetVariable - Modify blackboard
- ...and more

**3. Conditions** (loaded from `Catalogues/ConditionTypes.json`)
- TargetVisible - Check line of sight
- HealthBelow - Check health threshold
- DistanceLess - Check distance to target
- HasItem - Check inventory
- ...and more

**4. Decorators** (loaded from `Catalogues/DecoratorTypes.json`)
- Repeat - Execute child N times
- Inverter - Invert child result
- Cooldown - Add delay between executions
- ...and more

**Usage:**
```
1. Open Nodes tab
2. Browse categories (collapsible headers)
3. Hover over node type to see tooltip with:
   - Description
   - Parameter list
   - Required parameters marked
4. Click & drag node type
5. Drop onto graph canvas
6. Node created at drop position
7. Double-click to edit parameters
```

**Tips:**
- 💡 Use tooltips to understand what each node does before adding
- 🎯 Required parameters shown in tooltip
- 📋 Catalog updates automatically when JSON files change
- 🔍 Quickly scan available actions/conditions/decorators

### Validation & Error Checking

The Validation system ensures your behavior trees are correct and complete.

**Validation Panel:**
```
View → Validation (if not visible)
```

**What it Validates:**

**1. Node Type Validation**
- ✅ Action/Condition/Decorator types exist in catalogs
- ❌ Detects invalid or deprecated types
- 🔄 Suggests fixes when catalog changes

**2. Parameter Validation**
- ✅ All required parameters present
- ❌ Detects missing required values
- 💡 Shows which parameters need attention

**3. Link Validation**
- ✅ Composite nodes have children
- ✅ Decorator nodes have exactly one child
- ❌ Detects broken links (missing nodes)
- 💡 Suggests structural improvements

**4. Graph Validation**
- ✅ Root node defined
- ⚠️ Warns about orphaned nodes
- 💡 Suggests best practices

**Error Severity Levels:**
- 🔴 **Critical** - Blocks execution
- 🟠 **Error** - Should be fixed
- 🟡 **Warning** - May cause issues
- 🔵 **Info** - Suggestions

**Using Validation:**

**Auto-Validate Mode:**
```
1. Enable "Auto-validate" checkbox
2. Validation runs every 2 seconds
3. Errors appear in real-time
```

**Manual Validate:**
```
1. Click "Validate Now" button
2. Results appear immediately
```

**Reviewing Errors:**
```
1. Error Summary shows count by severity
2. Error List shows all issues
3. Click error to jump to node (coming soon)
4. Hover for detailed tooltip
```

**Example Validation Errors:**
```
[Error] MoveToGoal: Missing required parameter: speed
[Warning] Root Selector: Composite node has no children
[Error] AttackAction: Invalid ActionType: OldAttackType
```

**Best Practices:**
- ✅ Validate before saving
- ✅ Fix Critical and Error level issues
- ✅ Consider fixing Warnings
- ✅ Run validation after catalog updates

### Position & Metadata Persistence

Node positions and editor state are now saved with your graphs.

**What's Saved:**
- 📍 **Node Positions** - X, Y coordinates
- 🔍 **Editor Zoom** - Zoom level
- 📜 **Scroll Offset** - Canvas pan position
- 🕐 **Last Modified** - Timestamp

**JSON Format:**
```json
{
  "nodes": [
    {
      "id": 1,
      "type": "Action",
      "name": "Move to Goal",
      "position": {"x": 100, "y": 200},
      "actionType": "MoveTo",
      "parameters": {
        "speed": "300.0",
        "target": "player"
      }
    }
  ],
  "editorMetadata": {
    "zoom": 1.0,
    "scrollOffset": {"x": 0, "y": 0},
    "lastModified": "2026-01-08T15:30:00Z"
  }
}
```

**Benefits:**
- 💾 Positions preserved between sessions
- 🎨 Consistent layout across team
- 🔄 Undo/redo preserves positions
- 📋 Easy to review in version control

---

**Enjoy the streamlined Blueprint Editor experience! 🎉**
