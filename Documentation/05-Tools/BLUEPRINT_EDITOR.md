# 🎨 Blueprint Editor

The Olympe Blueprint Editor is an integrated visual tool for creating and editing entity blueprints without writing code.

---

## 🎯 Overview

The Blueprint Editor provides a node-based visual interface for:
- Creating entity blueprints
- Adding and configuring components
- Managing entity properties
- Browsing and selecting assets
- Testing entities in real-time

---

## 🚀 Quick Start

### Opening the Editor

**In-Game**:
- Press **F2** to toggle the Blueprint Editor
- Or use the menu: **Tools → Blueprint Editor**

**Standalone** (if available):
```bash
./OlympeBlueprintEditor/OlympeBlueprintEditorGUI
```

### Creating Your First Blueprint

1. **Open the editor** (F2)
2. **File → New Blueprint**
3. **Enter blueprint name** (e.g., "MyEntity")
4. **Add components**:
   - Click "Add Component"
   - Select component type (Position, Sprite, etc.)
   - Configure properties
5. **Save**: File → Save As → "Blueprints/my_entity.json"

---

## 📦 Editor Features

### Main Interface

The editor is divided into several panels:

```
┌────────────────────────────────────────────────────────┐
│ Menu Bar: File | Edit | View | Tools | Help            │
├────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────────┐  ┌──────────────────────────┐   │
│  │  Asset Browser  │  │   Node Editor            │   │
│  │                 │  │                          │   │
│  │  - Blueprints/  │  │  [Position Component]    │   │
│  │  - AI/          │  │  [Sprite Component]      │   │
│  │  - Resources/   │  │  [Health Component]      │   │
│  │                 │  │                          │   │
│  └─────────────────┘  └──────────────────────────┘   │
│                                                         │
│  ┌───────────────────────────────────────────────────┐│
│  │  Property Inspector                               ││
│  │                                                   ││
│  │  Component: Position                              ││
│  │  ├─ position.x: [100.0]                          ││
│  │  ├─ position.y: [200.0]                          ││
│  │  └─ position.z: [  0.0]                          ││
│  └───────────────────────────────────────────────────┘│
│                                                         │
└────────────────────────────────────────────────────────┘
```

### File Menu

- **New** - Create new blueprint
- **Open** - Load existing blueprint
- **Save** - Save current blueprint
- **Save As** - Save with new name
- **Recent Files** - Quick access to recent blueprints
- **Exit** - Close editor

### Edit Menu

- **Add Component** - Add new component to entity
- **Remove Component** - Remove selected component
- **Duplicate Component** - Copy existing component
- **Clear All** - Remove all components

### Component Library

Available component types:

#### Core Components
- **Position** - Entity location (x, y, z)
- **BoundingBox** - Collision rectangle
- **VisualSprite** - Sprite rendering

#### Physics Components
- **Movement** - Velocity and direction
- **PhysicsBody** - Mass and speed

#### Gameplay Components
- **Health** - Hit points
- **AIBehavior** - AI behavior type
- **TriggerZone** - Detection zone
- **Inventory** - Item storage

#### Input Components
- **Controller** - Gamepad input
- **PlayerController** - Player states

#### Visual/Audio Components
- **Animation** - Sprite animation
- **AudioSource** - Sound effects
- **FX** - Visual effects

#### Other Components
- **Camera** - Camera control
- **NPC** - NPC type data

---

## 🎨 Node Editor

### Adding Components

1. **Right-click** in node editor area
2. Select **Add Component**
3. Choose component type from menu
4. Component node appears in editor

### Editing Properties

1. **Select component node** (click on it)
2. **Property inspector** shows editable fields
3. **Modify values** directly in inspector
4. Changes apply immediately

### Connecting Components

Some components reference others:
- **Camera** → Target Entity (for follow camera)
- **AI** → Target for chase/flee behaviors

Connections are visual lines between nodes.

### Organizing Nodes

- **Drag nodes** to reposition
- **Zoom**: Mouse wheel or Ctrl + scroll
- **Pan**: Middle mouse button drag
- **Frame All**: Press 'F' key

---

## 📁 Asset Browser

The integrated Asset Browser helps you find and use assets:

### Features

- **Tree View** - Hierarchical folder structure
- **Search** - Find assets by name
- **Filter** - Show only specific asset types
- **Preview** - View asset details
- **Quick Insert** - Double-click to use asset

### Using the Asset Browser

1. **Browse folders** - Click to expand/collapse
2. **Search** - Type in search box
3. **Filter by type** - Select EntityBlueprint, BehaviorTree, etc.
4. **Select asset** - Click to view details
5. **Use asset** - Double-click or drag to property field

### Asset Types

- **EntityBlueprint** (.json) - Entity definitions
- **BehaviorTree** (.json) - AI behavior trees
- **Texture** (.png, .jpg) - Sprite images
- **Audio** (.wav, .ogg) - Sound files

See: [Asset Browser Documentation](ASSET_BROWSER.md)

---

## 🔧 Advanced Features

### Property Types

The editor supports various property types:

- **Number** (float, int) - Numeric input fields
- **String** - Text input
- **Vector** - 2D/3D coordinates (x, y, z)
- **Rectangle** - Bounding boxes (x, y, w, h)
- **Boolean** - Checkboxes
- **Enum** - Dropdown selections
- **Array** - Lists of items

### Nested Properties

Some properties contain sub-properties:

```
Position Component
├─ position
│  ├─ x: 100.0
│  ├─ y: 200.0
│  └─ z: 0.0
```

Edit sub-properties by expanding the parent.

### Validation

The editor validates properties in real-time:

- ✅ **Green** - Valid value
- ⚠️ **Yellow** - Warning (e.g., value out of typical range)
- ❌ **Red** - Invalid value (must fix before saving)

### Hot Reload

Changes can be hot-reloaded without restarting:

1. **Save blueprint** while game is running
2. **File → Reload** or press F5
3. Entities update with new properties

---

## ⌨️ Keyboard Shortcuts

### General
- **F2** - Toggle Blueprint Editor
- **Ctrl+N** - New blueprint
- **Ctrl+O** - Open blueprint
- **Ctrl+S** - Save blueprint
- **Ctrl+Shift+S** - Save As
- **Ctrl+Z** - Undo (planned)
- **Ctrl+Y** - Redo (planned)

### Node Editor
- **F** - Frame all nodes
- **Del** - Delete selected component
- **Ctrl+D** - Duplicate selected component
- **Mouse Wheel** - Zoom in/out
- **Middle Mouse** - Pan view

### Asset Browser
- **Ctrl+F** - Focus search box
- **Enter** - Select highlighted asset
- **Esc** - Clear search

---

## 💡 Workflow Examples

### Creating a Player Character

1. **New Blueprint**: "Player"
2. **Add Position**: Set to spawn point (100, 100, 0)
3. **Add VisualSprite**: Select player sprite texture
4. **Add Health**: Set max health to 100
5. **Add Controller**: Configure for player 0
6. **Add PlayerController**: Enable input handling
7. **Save**: "Blueprints/player.json"

### Creating a Patrolling Enemy

1. **New Blueprint**: "GuardEnemy"
2. **Add Position**: Set initial position
3. **Add VisualSprite**: Select enemy sprite
4. **Add Health**: Set to 50 HP
5. **Add AIBehavior**: Select "patrol" behavior
6. **Add BehaviorTreeRuntime**: Assign patrol tree (ID: 2)
7. **Add PhysicsBody**: Set movement speed
8. **Save**: "Blueprints/enemy_guard.json"

### Creating an NPC Vendor

1. **New Blueprint**: "Shopkeeper"
2. **Add Position**: Set shop location
3. **Add VisualSprite**: Select NPC sprite
4. **Add NPC**: Set type to "vendor"
5. **Add Inventory**: Add initial shop items
6. **Add TriggerZone**: Set interaction radius
7. **Save**: "Blueprints/npc_vendor.json"

---

## 🏗️ Architecture

### Backend (BlueprintEditor Class)

Singleton managing data, state, and business logic:

```cpp
class BlueprintEditor {
public:
    // Asset management
    void ScanAssets(const std::string& directory);
    std::vector<AssetInfo> GetAssets(AssetType filter);
    
    // Blueprint operations
    void LoadBlueprint(const std::string& path);
    void SaveBlueprint(const std::string& path);
    
    // Component management
    void AddComponent(const std::string& type);
    void RemoveComponent(int index);
    ComponentData* GetComponent(int index);
};
```

### Frontend (BlueprintEditorGUI Class)

ImGui-based visual interface:

```cpp
class BlueprintEditorGUI {
private:
    BlueprintEditor* backend;
    
public:
    void Render();
    void RenderMenuBar();
    void RenderNodeEditor();
    void RenderPropertyInspector();
    void RenderAssetBrowser();
};
```

### Separation of Concerns

- **Frontend** - Only UI and user interaction
- **Backend** - Data, logic, file I/O, validation
- **No direct file access from frontend**
- **Clean API boundary for testing**

---

## 🐛 Troubleshooting

### Editor Won't Open

**Problem**: Pressing F2 does nothing

**Solutions**:
- Check console for errors
- Verify ImGui is initialized
- Ensure F2 key isn't bound elsewhere

### Cannot Save Blueprint

**Problem**: Save fails with error

**Solutions**:
- Check Blueprints/ directory exists
- Verify write permissions
- Check for invalid property values (red warnings)
- Look at console for detailed error

### Assets Not Showing

**Problem**: Asset Browser is empty

**Solutions**:
- Check asset directories exist
- Verify JSON files are valid
- Use "Refresh" button to rescan
- Check console for scan errors

### Property Changes Not Applied

**Problem**: Edited values don't save

**Solutions**:
- Press Enter after editing text fields
- Save the blueprint (Ctrl+S)
- Check for validation errors (red fields)

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../README.md)
- 📋 [Blueprint System](../03-Core-Systems/Blueprint/README.md)
- 📁 [Asset Browser](ASSET_BROWSER.md)
- 🛠️ [Tools Overview](README.md)

---

## 📖 Additional Resources

### Source Code
- `Source/BlueprintEditor.h/cpp` - Backend implementation
- `Source/BlueprintEditorGUI.h/cpp` - Frontend UI
- `OlympeBlueprintEditor/` - Standalone editor (if available)

### Architecture Documents
- [Blueprint Editor Architecture](../02-Architecture/BLUEPRINT_ARCHITECTURE.md)
- [Asset Explorer Architecture](../02-Architecture/ASSET_EXPLORER_ARCHITECTURE.md)

---

[← Back to Tools](README.md) | [Next: Asset Browser →](ASSET_BROWSER.md)
