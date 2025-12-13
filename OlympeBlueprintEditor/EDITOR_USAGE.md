# Olympe Blueprint Editor - Usage Guide

## Overview

The Olympe Blueprint Editor is a functional, interactive tool for creating and editing entity blueprints for the Olympe Engine. This editor allows you to visually manage component properties without directly editing JSON files.

## Features

### Phase 1 - Component Property Editor (Current)

✅ **Fully Implemented:**
- Load entity blueprints from JSON files
- Save blueprints to JSON files
- Create new blueprints from scratch
- View blueprint details and component list
- Add components (Position, BoundingBox, VisualSprite, Movement, PhysicsBody, Health, AIBehavior)
- Remove components
- Edit component properties (numbers, strings, booleans)
- Edit nested properties (e.g., position.x, position.y)
- Unsaved changes warning
- Interactive menu-based interface

### Phase 2 - Behavior Editor (Future)

📋 **Planned:**
- Visual node-based behavior editing
- AI/Behavior logic composition
- Behavior graph visualization
- Condition and trigger system

## Building the Editor

### Linux/Unix

```bash
cd OlympeBlueprintEditor
make clean
make
```

### Windows (Visual Studio)

Open `OlympeBlueprintEditor.sln` in Visual Studio and build the project.

## Running the Editor

### From Command Line

```bash
# Navigate to the engine root directory
cd /path/to/Olympe-Engine

# Run the editor
./OlympeBlueprintEditor/OlympeBlueprintEditor
```

### Running the Demo

```bash
cd OlympeBlueprintEditor
./demo_editor.sh
```

## Using the Editor

### Main Menu

When you start the editor, you'll see the main menu:

```
╔════════════════════════════════════════════════════════════╗
║          OLYMPE BLUEPRINT EDITOR - Phase 1                 ║
║          Component Property Editor                         ║
╚════════════════════════════════════════════════════════════╝

═══════════════ MAIN MENU ═══════════════
1. New Blueprint
2. Load Blueprint
3. Save Blueprint
4. Save Blueprint As...
5. View Blueprint Details
6. Edit Components
7. Exit
═════════════════════════════════════════
```

### Creating a New Blueprint

1. Select option **1** (New Blueprint)
2. Enter a name for your entity (e.g., "MyHero")
3. Enter an optional description
4. The blueprint is created and ready for editing

### Loading a Blueprint

1. Select option **2** (Load Blueprint)
2. Enter the filepath (e.g., `Blueprints/example_entity_simple.json`)
3. The blueprint is loaded and displayed

Example blueprints provided:
- `Blueprints/example_entity_simple.json` - Basic entity with 3 components
- `Blueprints/example_entity_complete.json` - Complete entity with 11 components

### Editing Components

1. Select option **6** (Edit Components)
2. You'll see the component list with options to:
   - **Add Component** - Add a new component to the entity
   - **Edit Component** - Modify properties of an existing component
   - **Remove Component** - Delete a component from the entity

#### Adding a Component

1. Choose **1** (Add Component)
2. Select the component type:
   - **Position** - Entity position (x, y, z)
   - **BoundingBox** - Collision box (x, y, w, h)
   - **VisualSprite** - Sprite rendering (path, dimensions, hotspot)
   - **Movement** - Movement vectors (direction, velocity)
   - **PhysicsBody** - Physics properties (mass, speed)
   - **Health** - HP system (current, max)
   - **AIBehavior** - AI type (idle, patrol, chase, flee, wander)
   - **Custom** - Enter any component type manually

3. Fill in the prompted values for the component properties

#### Editing Component Properties

1. Choose **2** (Edit Component)
2. Enter the component number from the list
3. Choose **1** (Edit property value)
4. Enter the property key to edit:
   - For simple properties: `"currentHealth"`, `"speed"`, `"spritePath"`
   - For nested properties: `"position"` then `"x"` or `"y"`
5. Enter the new value
6. The property is updated

**Example - Editing Position:**
```
Enter property key: position
Nested object detected. Enter sub-key: x
Enter new value: 500
✓ Updated!
```

#### Removing a Component

1. Choose **3** (Remove Component)
2. Enter the component number to remove
3. The component is deleted from the blueprint

### Saving Your Work

1. Select option **3** (Save Blueprint) to save to the current file
2. Select option **4** (Save Blueprint As...) to save to a new file
3. The blueprint is saved in JSON format

### Viewing Blueprint Details

1. Select option **5** (View Blueprint Details)
2. You'll see:
   - Blueprint name and description
   - Component count
   - List of all components
   - Full JSON representation

## Blueprint File Format

Blueprints are stored as JSON files with this structure:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "EntityName",
    "description": "Description here",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {
                    "x": 100.0,
                    "y": 200.0,
                    "z": 0.0
                }
            }
        }
    ]
}
```

## Component Reference

### Position
Properties:
- `position.x` (float) - X coordinate
- `position.y` (float) - Y coordinate
- `position.z` (float) - Z coordinate

### BoundingBox
Properties:
- `boundingBox.x` (float) - X offset
- `boundingBox.y` (float) - Y offset
- `boundingBox.w` (float) - Width
- `boundingBox.h` (float) - Height

### VisualSprite
Properties:
- `spritePath` (string) - Path to sprite image
- `srcRect.x` (float) - Source rectangle X
- `srcRect.y` (float) - Source rectangle Y
- `srcRect.w` (float) - Source rectangle width
- `srcRect.h` (float) - Source rectangle height
- `hotSpot.x` (float) - Hotspot X
- `hotSpot.y` (float) - Hotspot Y

### Movement
Properties:
- `direction.x` (float) - Direction X component
- `direction.y` (float) - Direction Y component
- `velocity.x` (float) - Velocity X component
- `velocity.y` (float) - Velocity Y component

### PhysicsBody
Properties:
- `mass` (float) - Entity mass
- `speed` (float) - Movement speed

### Health
Properties:
- `currentHealth` (int) - Current HP
- `maxHealth` (int) - Maximum HP

### AIBehavior
Properties:
- `behaviorType` (string) - Behavior type: "idle", "patrol", "chase", "flee", "wander"

## Tips and Best Practices

1. **Save Often**: Use Save (option 3) frequently to avoid losing work
2. **Use Descriptive Names**: Give blueprints clear, meaningful names
3. **Test Your Blueprints**: Load them back into the editor to verify they work
4. **Keep Backups**: Use "Save As..." to create versions of your blueprints
5. **Start Simple**: Begin with basic components and add complexity gradually

## Troubleshooting

### "Failed to load blueprint"
- Check that the file path is correct
- Ensure the JSON file is valid
- Make sure you're running from the correct directory

### "Failed to save blueprint"
- Check that the directory exists (e.g., `Blueprints/`)
- Ensure you have write permissions
- Verify the file path is valid

### Property not updating
- Make sure you enter the exact property key name
- For nested properties, edit the parent object first, then the sub-property
- Check the JSON view (option 2) to see the current structure

## Examples

### Example 1: Creating a Simple Player Character

1. New Blueprint → Name: "Player"
2. Add Component → Position → X: 100, Y: 100
3. Add Component → BoundingBox → X: 0, Y: 0, W: 32, H: 32
4. Add Component → VisualSprite → Path: "Resources/player.png", W: 32, H: 32
5. Add Component → Health → Max: 100
6. Add Component → PhysicsBody → Mass: 1.0, Speed: 100.0
7. Save As → "Blueprints/player.json"

### Example 2: Modifying an Enemy

1. Load Blueprint → "Blueprints/example_entity_complete.json"
2. Edit Components → Edit Component → Select "Health"
3. Edit property → "currentHealth" → Enter: 50
4. Edit Components → Edit Component → Select "PhysicsBody"
5. Edit property → "speed" → Enter: 75.0
6. Save As → "Blueprints/enemy_fast.json"

## Architecture

The editor is built with:
- **EntityBlueprint** class - Core blueprint data structure
- **BlueprintEditor** class - Editor logic and UI
- **nlohmann/json** - JSON parsing and serialization
- Console-based menu interface (Phase 1)
- Future: ImGui + ImNodes for visual node editing (Phase 2)

## Next Steps

After Phase 1, the editor will be extended with:
- Visual node-based interface using ImGui
- Graphical component property editing
- Behavior graph editor with ImNodes
- Real-time preview of entities
- Integration with the Olympe Engine runtime

## Support

For more information, see:
- `Blueprints/README.md` - Blueprint system documentation
- `DEVELOPMENT_PLAN.md` - Full development roadmap
- `PHASE1_SUMMARY.md` - Phase 1 completion summary

---

**Olympe Blueprint Editor - Phase 1**  
*Component Property Editor - Fully Functional*
