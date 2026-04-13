---
id: editor-guide
title: Entity Prefab Editor Guide
sidebar_label: Editor Guide
sidebar_position: 3
---

# Entity Prefab Editor Guide

## Opening the Editor

1. Go to **File → New Entity Prefab** (Ctrl+Alt+N) to create a new prefab
2. Go to **File → Open** to load an existing `.json` prefab file

## Interface Overview

```
┌─────────────────────────────────────────────────────────┐
│  Canvas (75%)              │  Component Palette (25%)   │
│                            │                            │
│  [Node] ──── [Node]        │  🔍 Search...              │
│     │                      │  ─ All ─ Core ─ AI ─       │
│  [Node]                    │  • Transform               │
│                            │  • Sprite                  │
│                            │  • AIBlackboard            │
└─────────────────────────────────────────────────────────┘
```

## Canvas Controls

| Action | Input |
|--------|-------|
| Pan | Middle-mouse drag |
| Zoom | Mouse scroll wheel |
| Select node | Left-click |
| Multi-select | Ctrl+Click |
| Rectangle selection | Left-drag on empty area |
| Move node | Left-drag on node |
| Delete selected | Delete key |
| Reset view | Double-click empty |

## Adding Components

**Method 1 – Drag & Drop:**
1. Search for a component in the palette
2. Drag it onto the canvas

**Method 2 – Double-click:**
1. Double-click a component in the palette
2. Node appears at the default position

## Creating Connections

1. Hover over the output pin of a source node
2. Left-click and drag to the input pin of a target node
3. Release to create the connection

## Saving

- **Ctrl+S** – Save the current prefab
- Unsaved changes show a `*` indicator in the tab title

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+S | Save |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Delete | Delete selected nodes |
| Ctrl+A | Select all |
| Ctrl+Alt+N | New Entity Prefab |
| Escape | Deselect all |
