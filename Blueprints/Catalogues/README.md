# Olympe Blueprint Editor - Enum Catalogues

## Overview

This directory contains JSON catalogues that define all available Actions, Conditions, and Decorators for the Olympe Blueprint Editor's behavior tree system.

These catalogues serve as:
- **Single source of truth** for valid enum types
- **Documentation** for available node types and their parameters
- **UI metadata** for tooltips, categories, and display names
- **Validation rules** for blueprint JSON files

## Catalogue Files

### ActionTypes.json
Defines all available Action nodes that can be used in behavior trees.
- **Current count**: 18 action types
- **Categories**: Movement, Combat, Timing, Animation, Basic, Blackboard, Audio

### ConditionTypes.json
Defines all available Condition nodes for decision-making in behavior trees.
- **Current count**: 17 condition types
- **Categories**: Target, Health, Perception, Movement, Combat, State, Blackboard, Inventory, Utility

### DecoratorTypes.json
Defines all available Decorator nodes that modify child node behavior.
- **Current count**: 17 decorator types
- **Categories**: Logic, Flow, Timing, Blackboard, Utility

## JSON Format

Each catalogue file follows this structure:

```json
{
  "version": "1.0",
  "types": [
    {
      "id": "MoveTo",
      "name": "Move To",
      "description": "Déplace l'acteur vers une position/acteur cible",
      "category": "Movement",
      "parameters": [
        {
          "name": "target_blackboard_key",
          "type": "string",
          "required": true
        },
        {
          "name": "acceptance_radius",
          "type": "float",
          "default": 100.0
        },
        {
          "name": "speed",
          "type": "float",
          "default": 300.0
        }
      ],
      "tooltip": "Déplace le NPC vers la cible spécifiée dans le blackboard"
    }
  ]
}
```

## Field Descriptions

### Type Object Fields

| Field | Required | Type | Description |
|-------|----------|------|-------------|
| `id` | ✅ | string | Unique identifier (used in JSON files) |
| `name` | ❌ | string | Display name (defaults to `id`) |
| `description` | ❌ | string | Detailed description |
| `category` | ❌ | string | Category for grouping (defaults to "General") |
| `tooltip` | ❌ | string | Tooltip text (defaults to `description`) |
| `parameters` | ❌ | array | List of parameter definitions |

### Parameter Object Fields

| Field | Required | Type | Description |
|-------|----------|------|-------------|
| `name` | ✅ | string | Parameter name |
| `type` | ✅ | string | Data type: "string", "float", "int", "bool" |
| `required` | ❌ | boolean | Whether parameter is required (default: false) |
| `default` | ❌ | any | Default value if not specified |

## How to Add a New Type

### For Designers (No Coding Required)

1. **Open the appropriate catalogue file** in a text editor
   - Actions → `ActionTypes.json`
   - Conditions → `ConditionTypes.json`
   - Decorators → `DecoratorTypes.json`

2. **Copy an existing type entry** that's similar to what you need

3. **Modify the fields**:
   ```json
   {
     "id": "YourNewActionID",           // Must be unique!
     "name": "Your New Action Name",    // Display name in UI
     "description": "What it does...",  // Full description
     "category": "Movement",            // Choose existing category or create new
     "parameters": [
       {
         "name": "speed",
         "type": "float",
         "default": 300.0
       }
     ],
     "tooltip": "Short helpful text for users"
   }
   ```

4. **Save the file** (make sure JSON is valid!)

5. **Test in the editor**:
   - Open the Blueprint Editor
   - Click "🔄 Reload All Catalogues" in the Catalogue Manager panel
   - Your new type should appear in the dropdowns

### For Programmers

If the new type requires actual behavior implementation:

1. **Add the catalogue entry** (as described above)

2. **Implement the behavior** in the corresponding C++ file:
   - Actions: `Source/AI/BehaviorTree.cpp` → `ExecuteAction()`
   - Conditions: `Source/AI/BehaviorTree.cpp` → `EvaluateCondition()`
   - Decorators: `Source/AI/BehaviorTree.cpp` → `ExecuteDecorator()`

3. **Update enum definitions** in `Source/AI/BehaviorTree.h` if needed

4. **Test thoroughly** with various parameters

## Validation

The `EnumCatalogManager` automatically validates:
- ✅ File format and structure
- ✅ Required fields are present
- ✅ Data types are correct
- ✅ No duplicate IDs

The `NodeValidator` validates:
- ✅ All node types are defined in catalogues
- ✅ Required parameters are present
- ✅ Parameter types match definitions

## Categories

### Action Categories
- **Movement**: Moving, pathfinding, positioning
- **Combat**: Attacking, defending
- **Timing**: Waiting, delays
- **Animation**: Playing animations
- **Basic**: Simple actions (Idle, etc.)
- **Blackboard**: Manipulating AI memory
- **Audio**: Sound effects

### Condition Categories
- **Target**: Checking target state
- **Health**: Health-related conditions
- **Perception**: Vision, hearing
- **Movement**: Position and distance checks
- **Combat**: Combat state checks
- **State**: General AI state
- **Blackboard**: Memory checks
- **Inventory**: Item checks
- **Utility**: Random, utility functions

### Decorator Categories
- **Logic**: Inverters, forcers
- **Flow**: Repeaters, loops
- **Timing**: Cooldowns, timeouts, delays
- **Blackboard**: Conditional execution based on memory
- **Utility**: Random, probabilistic decorators

## Best Practices

### Naming Conventions
- **IDs**: PascalCase, descriptive (e.g., `MoveTo`, `IsLowHealth`)
- **Names**: Human-readable with spaces (e.g., "Move To", "Is Low Health")
- **Categories**: Single word, PascalCase (e.g., "Movement", "Combat")

### Parameter Design
- Keep parameter lists **short** (2-4 parameters max)
- Use **sensible defaults** so parameters are optional when possible
- Mark as `required: true` only if absolutely necessary
- Use **descriptive parameter names** (e.g., `acceptance_radius` not `radius`)

### Description Writing
- **Description**: Full sentence explaining what the type does
- **Tooltip**: Short, actionable hint for users (1 sentence max)
- Use **French** for consistency with the project (or your preferred language)

### Categories
- Reuse existing categories when possible
- Create new categories only when there are 3+ types that clearly belong together
- Keep category names concise (1-2 words)

## Hot Reload

The editor supports hot reloading of catalogues without restart:

1. Edit the JSON files while the editor is running
2. Save your changes
3. In the editor, open the "Catalogue Manager" panel
4. Click "🔄 Reload All Catalogues"
5. Changes take effect immediately

⚠️ **Note**: Existing nodes in open blueprints are not automatically updated. You may need to re-select their types.

## Troubleshooting

### "Invalid X type" errors
- Check that the ID in your blueprint JSON matches an ID in the catalogue
- IDs are **case-sensitive**
- Use the Catalogue Manager panel to see all valid IDs

### Catalogue won't load
- Validate JSON syntax using a JSON validator
- Check that `version` and `types` fields are present
- Ensure `types` is an array
- Check console output for specific error messages

### Missing parameters
- Check that all required parameters are present in your node JSON
- Required parameters are marked with `"required": true` in the catalogue
- Use the Inspector panel to see which parameters are required (marked with red `*`)

## Examples

See the existing entries in:
- `ActionTypes.json` for complete examples
- `ConditionTypes.json` for examples with minimal parameters
- `DecoratorTypes.json` for examples with complex logic

## Version History

- **1.0** (2025-01-03): Initial release with 52 total types
  - 18 Action types
  - 17 Condition types
  - 17 Decorator types
