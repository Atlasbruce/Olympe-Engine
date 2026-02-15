---
sidebar_position: 1
---

# Grid System Overview

The Olympe Engine Grid System provides real-time, visual debugging grids for game development. It supports multiple projection types (orthogonal, isometric, hexagonal) and includes overlay visualization for collision and navigation systems.

## Features

- **Multiple Grid Types**: Orthogonal, Isometric, and Hexagonal (axial) grids
- **Real-time Control**: ImGui panel for live configuration (press F4)
- **Camera Integration**: Automatic frustum culling and viewport adaptation
- **LOD Support**: Level-of-detail system to prevent visual clutter at low zoom
- **Overlay Visualization**: View collision and navigation layers in real-time
- **Performance Optimized**: Line budget system for consistent frame rates

## Quick Start

### Opening the Grid Panel

Press **F4** in the engine to open the Grid Settings panel.

### Enabling the Grid

1. Open the Grid Panel (F4)
2. Check "Grid Enabled" or press **TAB** to toggle

### Changing Grid Type

In the Grid Panel:
1. Select from "Grid Projection Type" dropdown:
   - **Orthogonal**: Standard square grid
   - **Isometric**: Diamond-shaped isometric grid
   - **Hexagonal (Axial)**: Hexagonal grid with pointy-top orientation

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| **TAB** | Toggle grid on/off |
| **F4** | Toggle Grid Settings panel |
| **C** | Toggle collision overlay |
| **N** | Toggle navigation overlay |

## Next Steps

- [Grid Configuration](./grid-configuration.md) - Detailed configuration guide
- [Technical Reference](/technical-reference/architecture) - ECS architecture details
