# Entity Prefab Editor – Complete Guide

## Opening the Editor

| Method | Action |
|--------|--------|
| **File → New Entity Prefab** | Ctrl+Alt+N |
| **File → Open** | Browse to existing `.json` prefab |

## Canvas Navigation

| Input | Action |
|-------|--------|
| Middle-mouse drag | Pan canvas |
| Scroll wheel | Zoom (0.1× – 3.0×) |
| Left-click | Select node |
| Ctrl+Click | Multi-select |
| Left-drag (empty) | Rectangle selection |
| Left-drag (node) | Move node |
| Delete | Remove selected nodes |
| Ctrl+A | Select all |
| Escape | Deselect all |

## Component Palette

The right panel (25%) shows all available components:
- **Search box** – Filter by name (case-insensitive)
- **Category tabs** – All / Core / Physics / Graphics / AI / Gameplay
- **Double-click** – Add node at default position
- **Drag onto canvas** – Add node at cursor position

## Creating Connections

1. Hover over a node's output port (right edge)
2. Click and drag to another node's input port (left edge)
3. Release to finalize the connection
4. Valid connections are green; invalid connections are rejected

## Properties Panel

Select a node to open its **Properties Panel** (bottom-right):
- Edit string/float/int/bool parameters inline
- Changes are reflected immediately in the document

## Saving and Loading

```
Ctrl+S        → Save current prefab
File → Open   → Open existing prefab
File → Save As → Save with new name
```

Files are saved to `Gamedata/PrefabEntities/`.

## Context Menu

Right-click on canvas for contextual actions:
- **On a node**: Delete, Rename, Properties
- **On empty canvas**: Select All, Reset View, Clear

## Dirty Flag

The tab title shows `*` when there are unsaved changes.
