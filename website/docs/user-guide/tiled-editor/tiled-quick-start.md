---
id: tiled-quick-start
title: Tiled Editor Quick Start
sidebar_label: Quick Start
---

# Quick Start: Component-Scoped Overrides in Tiled

## Using Component-Scoped Properties in Tiled Editor

### Step 1: Open your TMJ map in Tiled

1. Launch Tiled Map Editor
2. Open or create a `.tmj` map file
3. Add an object layer if you don't have one

### Step 2: Add an object

1. Select the object layer
2. Click the Insert Rectangle/Point/Tile tool
3. Place an object on your map
4. Set the object's "Type" field (e.g., "Enemy", "Player", "Item")

### Step 3: Add component-scoped custom properties

Click on the object, then in the Properties panel:

**Traditional Way (flat properties):**
```
Property Name: width
Property Value: 32
```
⚠️ Problem: If multiple components use "width", they overwrite each other!

**NEW Way (component-scoped):**
```
Property Name: Transform.width
Property Value: 32

Property Name: Transform.height  
Property Value: 64

Property Name: Physics.mass
Property Value: 10.0
```
✅ Solution: Each component has its own namespace!

### Naming Convention

Use dot notation: `ComponentName.parameterName`

**Common Component Names:**
- `Transform` - Position, size, rotation, scale
- `Physics` - Mass, friction, velocity, forces
- `Health` - Max health, current health, regeneration
- `Sprite` - Texture path, frame, animation
- `AI` - Behavior tree, state machine parameters
- `Audio` - Sound effects, volume, pitch

### Example Object Setup

**Object Type:** `Enemy`

**Custom Properties:**
| Name | Type | Value |
|------|------|-------|
| Transform.width | float | 32 |
| Transform.height | float | 64 |
| Transform.speed | float | 5.0 |
| Physics.mass | float | 10.0 |
| Physics.speed | float | 50.0 |
| Health.maxHealth | int | 100 |
| AI.aggressionLevel | int | 3 |

**Result:**
- `Transform.speed` = 5.0 (movement speed)
- `Physics.speed` = 50.0 (velocity)
- ✅ No conflicts! Each component gets its own value.

### Properties Without Component Prefix

Properties without a component prefix still work (legacy mode):

```
Property Name: level
Property Value: 5
```

These are stored as flat properties and mapped to components based on the parameter schema.

### Best Practices

1. **Always use component prefixes for new properties** - Prevents future conflicts
2. **Use consistent naming** - Match component names from your prefab blueprints
3. **Document your components** - Keep a reference of available components and their parameters
4. **Test in-engine** - Load the level and verify properties are applied correctly

### Troubleshooting

**Property not applying?**
- Check component name spelling (case-sensitive!)
- Verify component exists in the prefab
- Check property type matches expected type (float vs int)

**Properties overwriting each other?**
- Add component prefix: `ComponentName.property`
- Ensure you're using the latest version with component-scoped override support

### Backward Compatibility

Old maps with flat properties (no dots) continue to work:
```
width = 32     # Automatically mapped to appropriate component
health = 100   # Automatically mapped to appropriate component
```

New component-scoped properties take precedence over flat properties if both exist.

## Next Steps

1. Try adding component-scoped properties to your objects
2. Load the level in Olympe Engine
3. Verify properties are applied correctly using the debug log
4. See `COMPONENT_SCOPED_OVERRIDES.md` for technical details
