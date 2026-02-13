---
id: tiled-introduction
title: Tiled Editor Integration
sidebar_label: Introduction
---

# Tiled Editor Integration

Olympe Engine provides first-class integration with [Tiled MapEditor](https://www.mapeditor.org/), allowing you to create game levels visually and load them seamlessly into your game.

## What is Tiled?

Tiled is a free, open-source, flexible level editor for creating tile-based game levels. It supports:

- **Multiple projections**: Orthogonal, Isometric, Hexagonal
- **Multiple layers**: Tile layers, object layers, image layers
- **Custom properties**: Add metadata to maps, layers, tiles, and objects
- **Tilesets**: Organize and reuse tile graphics
- **Objects**: Place spawn points, triggers, and collision shapes

## Why Use Tiled with Olympe?

Using Tiled with Olympe Engine offers several advantages:

### Visual Level Design
- Create levels visually without writing code
- See your game world as you build it
- Quick iteration and testing

### Powerful Features
- Layer-based organization for ground, obstacles, decorations
- Custom properties for navigation and collision control
- Object layers for entity spawn points
- Support for all three projection types

### Seamless Integration
- Load TMJ/TMX files with a single function call
- Automatic collision and navigation map generation
- Automatic entity spawning from objects
- Real-time debugging with overlay rendering

## Supported Features

Olympe Engine supports these Tiled features:

### ✅ Fully Supported

- **Tile Layers**: All tile layer types with multiple layers
- **Object Layers**: Point, rectangle, and tile objects
- **Orthogonal Maps**: Standard grid-based maps
- **Isometric Maps**: Diamond-shaped isometric projection
- **Hexagonal Maps**: Hexagonal tile grids
- **Custom Properties**: Layer, object, and tile properties
- **Tilesets**: Both embedded and external tilesets
- **Collision Detection**: Automatic from layer properties
- **Navigation Maps**: Automatic pathfinding map generation

### ⚠️ Partial Support

- **Image Layers**: Loaded but not yet rendered
- **Group Layers**: Recognized but flattened

### ❌ Not Supported

- **Infinite Maps**: Must use fixed-size maps
- **Animation**: Tile animations not yet supported
- **Terrain Sets**: Not used by engine
- **Wang Sets**: Not used by engine

## Quick Links

- [Quick Start Guide](./quick-start.md) - Get started with Tiled in 5 minutes
- [Layer Properties](./layer-properties.md) - Configure navigation and collision
- [Object Spawning](./object-spawning.md) - Place entities in your maps

## File Format

Olympe Engine supports **TMJ (JSON)** format:

```json
{
  "type": "map",
  "version": "1.10",
  "orientation": "orthogonal",
  "width": 20,
  "height": 15,
  "tilewidth": 32,
  "tileheight": 32,
  "layers": [...],
  "tilesets": [...]
}
```

**Note**: TMX (XML) format is also supported through the Tiled loader library.

## Basic Workflow

1. **Create Map in Tiled**
   - Set map size and tile size
   - Choose projection (orthogonal/isometric/hexagonal)
   - Add tilesets

2. **Design Levels**
   - Paint ground tiles
   - Add obstacles and decorations
   - Configure layer properties

3. **Add Entities**
   - Create object layer
   - Place spawn points
   - Set entity types and properties

4. **Save and Load**
   - Save as TMJ format
   - Load in Olympe Engine
   - Test and iterate

## Example Code

```cpp
#include "World.h"

// Load a Tiled map
bool success = World::Get().LoadLevel("Gamedata/Levels/dungeon.tmj");

if (success) {
    // Map loaded successfully!
    // - Tiles rendered automatically
    // - Collision map generated
    // - Navigation map generated
    // - Entities spawned from objects
}
```

## Next Steps

- **New to Tiled?** Start with the [Quick Start Guide](./quick-start.md)
- **Need navigation control?** Read about [Layer Properties](./layer-properties.md)
- **Want to spawn entities?** Learn about [Object Spawning](./object-spawning.md)

## Resources

- [Tiled Official Documentation](https://doc.mapeditor.org/)
- [Tiled Downloads](https://www.mapeditor.org/)
- [Olympe Level Examples](https://github.com/Atlasbruce/Olympe-Engine/tree/master/Gamedata/Levels)
