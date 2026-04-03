# Entity Prefab Editor Module - Complete Specification

**Version:** 1.0  
**Date:** 2026-04-02  
**Author:** Atlasbruce  
**Status:** Design Phase  

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [UI Design & Mockups](#ui-design--mockups)
4. [Data Structures](#data-structures)
5. [Data Flows](#data-flows)
6. [Dependencies Matrix](#dependencies-matrix)
7. [Integration Points](#integration-points)
8. [Implementation Phases](#implementation-phases)
9. [User Workflows](#user-workflows)
10. [Technical Specifications](#technical-specifications)
11. [Configuration](#configuration)
12. [Testing Strategy](#testing-strategy)
13. [Performance Targets](#performance-targets)
14. [Future Enhancements](#future-enhancements)
15. [API Reference](#api-reference)
16. [Appendix](#appendix)

---

## Overview

### Purpose

Extend the Olympe Engine Blueprint Editor with a new **Entity Prefab Editor Module** that allows developers to:
- **Visualize** entity prefabs from `Gamedata/EntityPrefab/*.json` files
- **Edit** component properties using type-aware editors
- **Create & Manage** components through a drag-and-drop component library
- **Serialize** changes back to JSON files
- **Validate** prefab structure against `ParameterSchemas.json`

### Scope

This specification defines the complete architecture, UI design, data flows, and implementation strategy for the Entity Prefab Editor as an integrated extension to the existing Blueprint Editor.

### Goals

✅ Seamless integration with Blueprint Editor's existing tab system  
✅ Intuitive drag-and-drop component management  
✅ Type-safe property editing based on ParameterSchemas  
✅ Real-time validation and feedback  
✅ Multi-file editing support  
✅ Undo/Redo functionality  
✅ Intuitive visual node-based representation  

---

## Architecture

### 2.1 System Overview

╔═══════════════════════════════════════════════════════════════════════════════╗
║                          OLYMPE ENGINE ARCHITECTURE                           ║
╚═══════════════════════════════════════════════════════════════════════════════╝

                        ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
                        ┃    BLUEPRINT EDITOR          ┃
                        ┃  (Existant - Core System)    ┃
                        ┃  • TaskGraphTemplate         ┃
                        ┃  • NodeEditor                ┃
                        ┃  • Serialization             ┃
                        ┗━━━━━━━━━━┳━━━━━━━━━━━━━━━━━┛
                                   ║
                    Réutilise: Renderer | InputSystem | UIFramework
                                   ║
                                   ▼
        ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
        ┃                                                     ┃
        ┃   ENTITY PREFAB EDITOR MODULE (NEW)                ┃
        ┃                                                     ┃
        ┃   ┌──────────────────────────────────────────────┐ ┃
        ┃   │  🎨 VISUALIZATION LAYER                      │ ┃
        ┃   │  ├─ PrefabFileExplorer                       │ ┃
        ┃   │  ├─ PrefabCanvas                             │ ┃
        ┃   │  └─ ComponentNodeRenderer                    │ ┃
        ┃   └────────────────┬─────────────────────────────┘ ┃
        ┃                    ║                               ┃
        ┃   ┌────────────────▼─────────────────────────────┐ ┃
        ┃   │  ✏️  EDITING LAYER                           │ ┃
        ┃   │  ├─ ComponentNodeEditor                      │ ┃
        ┃   │  ├─ PropertyInspector                        │ ┃
        ┃   │  └─ ComponentLibraryPanel                    │ ┃
        ┃   └────────────────┬─────────────────────────────┘ ┃
        ┃                    ║                               ┃
        ┃   ┌────────────────▼─────────────────────────────┐ ┃
        ┃   │  📊 DATA MANAGEMENT LAYER                    │ ┃
        ┃   │  ├─ PrefabLoader                             │ ┃
        ┃   │  ├─ PrefabSerializer                         │ ┃
        ┃   │  ├─ ComponentLibrary                         │ ┃
        ┃   │  └─ ParameterSchemaRegistry                  │ ┃
        ┃   └────────────────┬─────────────────────────────┘ ┃
        ┃                    ║                               ┃
        ┃   ┌────────────────▼─────────────────────────────┐ ┃
        ┃   │  🎮 CONTROLLER LAYER                         │ ���
        ┃   │  └─ PrefabEditor (Main Orchestrator)         │ ┃
        ┃   └────────────────┬─────────────────────────────┘ ┃
        ┃                    ║                               ┃
        ┗━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                             ║
                    ┌────────┼────────┐
                    ║        ║        ║
                    ▼        ▼        ▼
            ┌──────────┐ ┌──────────┐ ┌──────────┐
            │ FileIO   │ │JSONParser│ │Validator │
            │Existant  │ │Existant  │ │Existant  │
            └──────────┘ └──────────┘ └──────────┘
                    ║        ║        ║
                    └────────┼────────┘
                             ║
                    ┌────────┼────────┬──────────────┐
                    ║        ║        ║              ║
                    ▼        ▼        ▼              ▼
            ┌──────────────┐ ┌──────────┐ ┌──────────────────┐
            │ Gamedata/    │ │Renderer  │ │ InputSystem      │
            │ EntityPrefab/│ │Existant  │ │ Existant         │
            │ *.json       │ └──────────┘ └──────────────────┘
            │ +Parameters  │
            │  Schemas.json│
            └──────────────┘


### 2.2 Module Dependency Matrix

┌─────────────────────────────────────────────────────────────────────────────┐
│ MODULE                      │ DEPENDS ON              │ PROVIDES TO         │
├─────────────────────────────────────────────────────────────────────────────┤
│ PrefabFileExplorer          │ FileSystem              │ PrefabEditor        │
│                             │ UIFramework             │ CanvasTabManager    │
├─────────────────────────────────────────────────────────────────────────��───┤
│ PrefabLoader                │ FileIO                  │ PrefabEditor        │
│                             │ JSONParser              │                     │
├─────────────────────────────────────────────────────────────────────────────┤
│ ComponentLibrary            │ ParameterSchemaRegistry │ ComponentNodeEditor │
│                             │                         │ LibraryPanel        │
├─────────────────────────────────────────────────────────────────────────────┤
│ ParameterSchemaRegistry     │ JSONParser              │ ComponentLibrary    │
│                             │ FileIO                  │ PropertyInspector   │
├─────────────────────────────────────────────────────────────────────────────┤
│ PrefabCanvas                │ Renderer                │ PrefabEditor        │
│                             │ InputSystem             │ ComponentNodeEditor │
├─────────────────────────────────────────────────────────────────────────────┤
│ ComponentNode               │ ComponentLibrary        │ PrefabCanvas        │
│                             │ ParameterSchemaRegistry │ PropertyInspector   │
├───────────────────────────────���─────────────────────────────────────────────┤
│ PropertyInspector           │ ComponentNode           │ PrefabEditor        │
│                             │ ParameterSchemaRegistry │                     │
├─────────────────────────────────────────────────────────────────────────────┤
│ PrefabSerializer            │ JSONParser              │ PrefabEditor        │
│                             │ Validator               │                     │
├─────────────────────────────────────────────────────────────────────────────┤
│ PrefabEditor (Controller)   │ ALL ABOVE MODULES       │ BlueprintEditor     │
└─────────────────────────────────────────────────────────────────────────────┘


### 2.3 File Organization

Source/
├── Editor/
│   ├── Panels/
│   │   └── EntityPrefabEditor/
│   │       ├── PrefabFileExplorer.h
│   │       ├── PrefabFileExplorer.cpp
│   │       ├── PrefabCanvas.h
│   │       ├── PrefabCanvas.cpp
│   │       ├── PropertyInspector.h
│   │       ├── PropertyInspector.cpp
│   │       └── README.md
│   │
│   ├── Nodes/
│   │   ├── ComponentNode.h
│   │   ├── ComponentNode.cpp
│   │   ├── ComponentNodeRenderer.h
│   │   └── ComponentNodeRenderer.cpp
│   │
│   ├── Library/
│   │   ├── ComponentLibrary.h
│   │   ├── ComponentLibrary.cpp
│   │   ├── ParameterSchemaRegistry.h
│   │   └── ParameterSchemaRegistry.cpp
│   │
│   └── Utils/
│       ├── PrefabLoader.h
│       ├── PrefabLoader.cpp
│       ├── PrefabSerializer.h
│       ├── PrefabSerializer.cpp
│       ├── PrefabValidator.h
│       └── PrefabValidator.cpp
│
└── Documentation/
    └── ENTITY_PREFAB_EDITOR_SPECIFICATION.md


---

## UI Design & Mockups

### 3.1 Main Layout Overview

╔════════════════════════════════════════════════════════════════════════════════╗
║                       OLYMPE ENGINE - BLUEPRINT EDITOR                         ║
║  File  Edit  View  Help  Tools                                                 ║
╠════════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  ┌──────────────────────────────────────────────────────────────────────────┐  ║
║  │ 📁 File Browser    [Search: ________]                                    │  ║
║  │ ┌──────────────────────────────────────────────────────────────────────┐ │  ║
║  │ │ ▼ Gamedata/                                                          │ │  ║
║  │ │   ▼ EntityPrefab/                                                    │ │  ║
║  │ │     📄 beacon.json                                                   │ │  ║
║  │ │     📄 collision.json                                                │ │  ║
║  │ │     📄 door.json              ◄── Double-click opens Prefab tab     │ │  ║
║  │ │     📄 guard.json                                                    │ │  ║
║  │ │     📄 item.json                                                     │ │  ║
║  │ │     📄 key.json                                                      │ │  ║
║  │ │     📄 player.json                                                   │ │  ║
║  │ │     📄 zombie.json                                                   │ │  ║
║  │ │                                                                      │ │  ║
║  │ └──────────────────────────────────────────────────────────────────────┘ │  ║
║  │                                                                            │  ║
║  └──────────────────────────────────────────────────────────────────────────┘  ║
║                                                                                ║
║  ┌──────────────────────────────────────────────────┬──────────────────────┐   ║
║  │  [Blueprint] [StateGraph] [Prefab-beacon] [✕]   │  Canvas Toolbar      │   ║
║  │   Graph       Machine        ◄─── NEW TAB       ├──────────────────────┤   ║
║  │                                                  │ [Layout] [Save] [⟲] │   ║
║  │  ┌────────────────────────────────────────────┐ │ [Zoom Controls]      │   ║
║  │  │ PREFAB CANVAS - Entity: Beacon            │ │                      │   ║
║  │  │ ┌──────────────────────────────────────┐  │ │                      │   ║
║  │  │ │                                      │  │ │                      │   ║
║  │  │ │    ┌────────────────────────┐       │  │ │                      │   ║
║  │  │ │    │   [🔷 BEACON]         │       │  │ │                      │   ║
║  │  │ │    │   Entity Center       │       │  │ │                      │   ║
║  │  │ │    └────────────────────────┘       │  │ │                      │   ║
║  │  │ │  ↙         ↓         ↘              │  │ │                      │   ║
║  │  │ │ ┌─────┐ ┌─────┐ ┌─────┐            │  │ │                      │   ║
║  │  │ │ │Iden │ │Pos  │ │Vis  │            │  │ │                      │   ║
║  │  │ │ │tity │ │itio │ │Edit │            │  │ │                      │   ║
║  │  │ │ └─────┘ └─────┘ └─────┘            │  │ │                      │   ║
║  │  │ │                                      │  │ │                      │   ║
║  │  │ │ ┌─────┐ ┌─────┐ ┌──────┐           │  │ │                      │   ║
║  │  │ │ │Vis  │ │Bound│ │  +  │           │  │ │                      │   ║
║  │  │ │ │Spri │ │ingB │ │ Add │           │  │ │                      │   ║
║  │  │ │ └─────┘ └─────┘ └──────┘           │  │ │                      │   ║
║  │  │ │                                      │  │ │                      │   ║
║  │  │ └──────────────────────────────────────┘  │ │                      │   ║
║  │  │ Beacon | 5 Components | Saved            │ │                      │   ║
║  │  └────────────────────────────────────────────┘ │                      │   ║
║  └──────────────────────────────────────────────────┴──────────────────────┘   ║
║                                                                                ║
║  ┌──────────────────────────────────────────────────────────────────────────┐  ║
║  │ RIGHT PANEL - TABS                                                       │  ║
║  │ ┌────────────────────┬────────────────────────────┬──────┐               │  ║
║  │ │ 📋 Properties      │ 📚 Components    [🔍]     │      │               │  ║
║  │ └────────────────────┴────────────────────────────┴──────┘               │  ║
║  │                                                                            │  ║
║  │ ╔════════════════════════════════════════════════════════════════════╗   │  ║
║  │ ║ PROPERTIES INSPECTOR                                              ║   │  ║
║  │ ║ ────────────────────────────────────────────────────────────────  ║   │  ║
║  │ ║ Component: Identity_data                                          ║   │  ║
║  │ ║ ─────────────────────────────────���──────────────────────────────  ║   │  ║
║  │ ║                                                                   ║   │  ║
║  │ ║ name:       ┌──────────────────────────────┐                     ║   │  ║
║  │ ║             │ {instanceName}               │                     ║   │  ║
║  │ ║             └──────────────────────────────┘                     ║   │  ║
║  │ ║ tag:        ┌──────────────────────────────┐                     ║   │  ║
║  │ ║             │ Beacon                       │                     ║   │  ║
║  │ ║             └──────────────────────────────┘                     ║   │  ║
║  │ ║ entityType: ┌──────────────────────────────┐                     ║   │  ║
║  │ ║             │ Beacon                       │                     ║   │  ║
║  │ ║             └──────────────────────────────┘                     ║   │  ║
║  │ ║ isPersistent: ☐                                                   ║   │  ║
║  │ ║                                                                   ║   │  ║
║  │ ╚════════════════════════════════════════════════════════════════════╝   │  ║
║  │                                                                            │  ║
║  └──────────────────────────────────────────────────────────────────────────┘  ║
║                                                                                ║
║  Status: beacon.json | 5 Components | Saved                                    ║
║                                                                                ║
╚════════════════════════════════════════════════════════════════════════════════╝


### 3.2 Component Library Tab Active

╔════════════════════════════════════════════════════════════════════════════════╗
║                 PREFAB EDITOR - COMPONENT LIBRARY VISIBLE                      ║
╠════════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  ┌──────────────────────────────────────────────────┬──────────────────────┐  ║
║  │  [Blueprint] [StateGraph] [Prefab-beacon*] [✕]  │  Canvas Toolbar      │  ║
║  │                                                  ├──────���───────────────┤  ║
║  │  ┌────────────────────────────────────────────┐ │ [–] [1:1] [+] [Fit]  │  ║
║  │  │ PREFAB CANVAS - Entity: Beacon            │ │ [Layout] [Save]      │  ║
║  │  │ ┌──────────────────────────────────────┐  │ │                      │  ║
║  │  │ │   ┌────────────────────────┐         │  │ │                      │  ║
║  │  │ │   │   [🔷 BEACON]         │         │  │ │                      │  ║
║  │  │ │   │   Entity Center       │         │  │ │                      │  ║
║  │  │ │   └────────────────────────┘         │  │ │                      │  ║
║  │  │ │  ↙         ↓         ↘              │  │ │                      │  ║
║  │  │ │ ┌─────┐ ┌─────┐ ┌─────┐            │  │ │                      │  ║
║  │  │ │ │Iden │ │Pos  │ │Vis  │            │  │ │                      │  ║
║  │  │ │ │tity │ │itio │ │Edit │            │  │ │                      │  ║
║  │  │ │ └─────┘ └─────┘ └─────┘            │  │ │                      │  ║
║  │  │ │ ┌─────┐ ┌─────┐ ┌──────┐           │  │ │                      │  ║
║  │  │ │ │Vis  │ │Bound│ │  +  │           │  │ │                      │  ║
║  │  │ │ │Spri │ │ingB │ │ Add │           │  │ │                      │  ║
║  │  │ │ └─────┘ └─────┘ └──────┘           │  │ │                      │  ║
║  │  │ │                                      │  │ │                      │  ║
║  │  │ └──────────────────────────────────────┘  │ │                      │  ║
║  │  │ Beacon | 5 Components | Modified 10m ago │ │                      │  ║
║  │  └────────────────────────────────────────────┘ │                      │  ║
║  └──────────────────────────────────────────────────┴──────────────────────┘  ║
║                                                                                ║
║  ┌──────────────────────────────────────────────────────────────────────────┐  ║
║  │ RIGHT PANEL - TABS                                                       │  ║
║  │ ┌────────────────────┬────────────────────────────┬──────┐               │  ║
║  │ │ 📋 Properties      │ 📚 Components    [🔍]     │      │               │  ║
║  │ └────────────────────┴────────────────────────────┴─��────┘               │  ║
║  │                                                                            │  ║
║  │ 🎯 Drag components below to add them to the entity                       │  ║
║  │ ────────────────────────────────────────────────────────────────────────  │  ║
║  │ 🔍 [Component Search: _______________]                                   │  ║
║  │                                                                            │  ║
║  │ ✅ AVAILABLE COMPONENTS (Load from ParameterSchemas.json):               │  ║
║  │ ┌────────────────────────────────────────────────────────────────────┐   │  ║
║  │ │                                                                    │   │  ║
║  │ │ [📌] Identity_data          ✅ (Already added)                    │   │  ║
║  │ │ [🎯] Position_data          ✅ (Already added)                    │   │  ║
║  │ │ [🎨] VisualSprite_data      ✅ (Already added)                    │   │  ║
║  │ │ [🖼️ ] VisualEditor_data     ✅ (Already added)                    │   │  ║
║  │ │ [📦] BoundingBox_data       ✅ (Already added)                    │   │  ║
║  │ │ [⚡] Movement_data          ❌ (Drag to add)                       │   │  ║
║  │ │ [🔆] PhysicsBody_data       ❌ (Drag to add)                       │   │  ║
║  │ │ [❤️ ] Health_data           ❌ (Drag to add)                       │   │  ║
║  │ │ [🎬] VisualAnimation_data   ❌ (Drag to add)                       │   │  ║
║  │ │ [👤] PlayerBinding_data     ❌ (Drag to add)                       │   │  ║
║  │ │ [🕹️ ] Controller_data       ❌ (Drag to add)                       │   │  ║
║  │ │ [🎮] PlayerController_data  ❌ (Drag to add)                       │   │  ║
║  │ │ [🧠] AIBlackboard_data      ❌ (Drag to add)                       │   │  ║
║  │ │ [👁️ ] AISenses_data         ❌ (Drag to add)                       │   │  ║
║  │ │ [🎭] AIState_data           ❌ (Drag to add)                       │   │  ║
║  │ │ [🌳] BehaviorTreeRuntime... ❌ (Drag to add)                       │   │  ║
║  │ │ [➡️ ] MoveIntent_data       ❌ (Drag to add)                       │   │  ║
║  │ │ [⚔️ ] AttackIntent_data     ❌ (Drag to add)                       │   │  ║
║  │ │ [🚫] CollisionZone_data     ❌ (Drag to add)                       │   │  ║
║  │ │ [🛣️ ] PatrolPath_data       ❌ (Drag to add)                       │   │  ║
║  │ │ [🗺️ ] NavigationAgent_data   ❌ (Drag to add)                       │   │  ║
║  │ │                                                                    │   │  ║
║  │ └────────────────────────────────────────────────────────────────────┘   │  ║
║  │                                                                            │  ║
║  └──────────────────────────────────────────────────────────────────────────┘  ║
║                                                                                ║
╚══════════════════════════════════════════════════════════════════════════���═════╝


### 3.3 Drag-Drop Interaction Sequence

╔════════════════════════════════════════════════════════════════════════════════╗
║             PREFAB EDITOR - DRAG-DROP INTERACTION SEQUENCE                     ║
╠════════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  STEP 1: User drags Movement_data from Components List                        ║
║  ─────────────────────────────────────────────────────────────────────────    ║
║                                                                                ║
║  RIGHT PANEL                    │ CENTER CANVAS                                ║
║                                 │                                              ║
║  [⚡] Movement_data             │ ┌──────────────────────────────────────┐   ║
║       ◀─ DRAG STARTS            │ │  Canvas Accepts Drop               │   ║
║                                 │ │                                    │   ║
║                                 │ │ ┌──────────────────────────┐       │   ║
║                                 │ │ │   [🔷 BEACON]          │       │   ║
║                                 │ │ └──────────────────────────┘       │   ║
║                                 │ │      ↙  ↓  ↘                      │   ║
║                                 │ │   [old components...]              │   ║
║                                 │ │                                    │   ║
║                                 │ │ [HIGHLIGHT: Drop zone active]     │   ║
║                                 │ │                                    │   ║
║                                 │ └──────────────────────────────────────┘   ║
║                                                                                ║
║  STEP 2: User drops Movement_data into Canvas                                 ║
║  ─────────────────────────────────────────────────────────────────────────    ║
║                                                                                ║
║  CENTER CANVAS - AUTO-LAYOUT TRIGGERED                                        ║
║  ┌─────────────────────────────────────────────────────────────────────────┐  ║
║  │                                                                         │  ║
║  │     ┌──────────────────────────────┐                                   │  ║
║  │     │   [🔷 BEACON]               │                                   │  ║
║  │     │   Entity Center             │                                   │  ║
║  │     └──────────────────────────────┘                                   │  ║
║  │   ↙  ↙      ↓ ↓        ↘ ↘  ↘                                          │  ║
║  │ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐                                       │  ║
║  │ │Iden │ │Pos  │ │Vis  │ │Move │ ◄─── NEW! (Default values loaded)  │  ║
║  │ │tity │ │itio │ │Edit │ │ment │                                       │  ║
║  │ └─────┘ └─────┘ └─────┘ └─────┘                                       │  ║
║  │ ┌─────┐ ┌─────┐                                                        │  ║
║  │ │Vis  │ │Bound│                                                        │  ║
║  │ │Spri │ │ingB │                                                        │  ║
║  │ └─────┘ └─────┘                                                        │  ║
║  │                                                                         │  ║
║  │ Status: ✅ Component added! | 6 Components | *Modified                │  ║
║  │                                                                         │  ║
║  └─────────────────────────────────────────────────────────────────────────┘  ║
║                                                                                ║
║  STEP 3: Properties Tab auto-switches to new component                        ║
║  ─────────────────────────────────────────────────────────────────────────    ║
║                                                                                ║
║  RIGHT PANEL - PROPERTIES TAB (AUTO-ACTIVATED)                                ║
║  ┌─────────────────────────────────────────────────────────────────────────┐  ║
║  │ ┌────────────────────┬────────────────────────┬──────┐                 │  ║
║  │ │ 📋 Properties ◄─── │ 📚 Components [🔍]   │      │                 │  ║
║  │ │ (Auto-switched)    │                       │      │                 │  ║
║  │ └────────────────────┴────────────────────────┴──────┘                 │  ║
║  │                                                                         │  ║
║  │ ╔════════════════════════════════════════════════════════════════════╗ │  ║
║  │ ║ Component: Movement_data (NEW)                                    ║ │  ║
║  │ ║ ──────────────────────────────────────────────────────────────── ║ │  ║
║  │ ║                                                                  ║ │  ║
║  │ ║ speed:         ┌──────────────────────────────────┐             ║ │  ║
║  │ ║                │ 100.0                    [🔢]   │             ║ │  ║
║  │ ║                └──────────────────────────────────┘             ║ │  ║
║  │ ║ 📝 Default: 100.0  |  Type: Float                              ║ │  ║
║  │ ║                                                                  ║ │  ║
║  │ ║ acceleration:  ┌──────────────────────────────────┐             ║ │  ║
║  │ ║                │ 500.0                    [🔢]   │             ║ │  ║
║  │ ║                └──────────────────────────────────┘             ║ │  ║
║  │ ║ 📝 Default: 500.0  |  Type: Float                              ║ │  ║
║  │ ║                                                                  ║ │  ║
║  │ ╚════════════════════════════════════════════════════════════════════╝ │  ║
║  │                                                                         │  ║
║  └─────────────────────────────────────────────────────────────────────────┘  ║
║                                                                                ║
╚════════════════════════════════════════════════════════════════════════════════╝


### 3.4 Context Menu

╔════════════════════════════════════════════════════════════════════════════════╗
║            RIGHT-CLICK CONTEXT MENU - Component Node                           ║
╠════════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  USER: Right-clicks on Movement_data node                                      ║
║                                                                                ║
║  ┌──────────────────────────────────────────┐                                 ║
║  │ 📋 Context Menu                          │                                 ║
║  ├──────────────────────────────────────────┤                                 ║
║  │                                          │                                 ║
║  │ ✏️  Edit Properties                      │                                 ║
║  │ 🗑️  Delete Component                     │                                 ║
║  │ ⬆️  Move Up                               │                                 ║
║  │ ⬇️  Move Down                             │                                 ║
║  │ ────────────────────────────────────    │                                 ║
║  │ 📋 Duplicate Component                   │                                 ║
║  │ 🔄 Reset to Defaults                     │                                 ║
║  │ ────────────────────────────────────    │                                 ║
║  │ 📄 Copy to Clipboard                     │                                 ║
║  │ 📋 Paste Component                       │                                 ║
║  │                                          │                                 ║
║  └──────────────────────────────────────────┘                                 ║
║                                                                                ║
╚════════════════════════════════════════════════════════════════════════════════╝


### 3.5 Multiple Tabs Scenario

╔════════════════════════════════════════════════════════════════════════════════╗
║               MULTIPLE PREFABS EDITED SIMULTANEOUSLY                           ║
╠════════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  CENTER CANVAS TABS:                                                           ║
║  ┌─────────────────────────────────────────────────────────────────────────┐  ║
║  │ ┌──────────┬──────────┬──────────────┬──────────────┬──────────────┬──┐  │  ║
║  │ │Blueprint │StateGrph │ Prefab-bc*   │ Prefab-guard │ Prefab-pl*   │+│  │  ║
║  │ │Graph     │Machine   │ unsaved      │ saved        │ unsaved      │ │  │  ║
║  │ └──────────┴──────────┴──────────────┴──────────────┴──────────────┴──┘  │  ║
║  │                                                             ▲               │  ║
║  │                                                      (Active tab - Player)  │  ║
║  │                                                                            │  ║
║  │ ┌────────────────────────────────────────────────────────────────────┐   │  ║
║  │ │ PREFAB CANVAS - Entity: Player* (Modified)                        │   │  ║
║  │ │ ┌────────────────────────────────────────────────────────────────┐│   │  ║
║  │ │ │          ┌──────────��───────────┐                            ││   │  ║
║  │ │ │          │   [👤 PLAYER]       │                            ││   │  ║
║  │ │ │          │   Entity Center    │                            ││   │  ║
║  │ │ │          └──────────────────────┘                            ││   │  ║
║  │ │ │      ↙ ↙      ↓ ↓ ↓     ↘ ↘ ↘                               ││   │  ║
║  │ │ │ ┌────┐ ┌────┐ ┌────┐ ┌────┐                                ││   │  ║
║  │ │ │ │Iden│ │Pos │ │Vis │ │Bound│                               ││   │  ║
║  │ │ │ │tity│ │itio│ │Anim│ │ingB │                               ││   │  ║
║  │ │ │ └────┘ └────┘ └────┘ └────┘                                ││   │  ║
║  │ │ │ ┌────┐ ┌────┐ ┌────┐ ┌────┐                                ││   │  ║
║  │ │ │ │Move│ │Phys│ │Heal│ │Play│                                ││   │  ║
║  │ │ │ │ment│ │icsB│ │th  │ │Bind│                                ││   │  ║
║  │ │ │ └────┘ └────┘ └────┘ └────┘                                ││   │  ║
║  │ │ │ ┌────┐ ┌────┐ ┌────┐ ┌────┐                                ││   │  ║
║  │ │ │ │Ctrl│ │Play│ │Nav │ │[+] │                                ││   │  ║
║  │ │ │ │olle│ │erCt│ │igat│ │ Add │                               ││   │  ║
║  │ │ │ └────┘ └────┘ └────┘ └────┘                                ││   │  ║
║  │ │ │                                                             ││   │  ║
║  │ │ └────────────────────────────────────────────────────────────────┘│   │  ║
║  │ │ Player* | 13 Components | Modified 3m ago | [💾 Save] [↺ Revert] │   │  ║
║  │ │                                                                    │   │  ║
║  │ └────────────────────────────────────────────────────────────────────┘   │  ║
║  │                                                                            │  ║
║  └─────────────────────────────────────────────────────────────────────────┘  ║
║                                                                                ║
║  LEFT STATUS:                                                                  ║
║  📍 Beacon* | Guard | Player* (3 prefabs open, 2 modified)                   ║
║                                                                                ║
╚════════════════════════════════════════════════════════════════════════════════╝

## Data Structures

### 4.1 Core TypeScript Interfaces

```typescript
// Entity Prefab Structure (from JSON)
interface EntityPrefab {
  schema_version: number;
  type: string;
  blueprintType: "EntityPrefab";
  name: string;
  description: string;
  metadata: {
    author: string;
    created: string;        // ISO 8601
    lastModified: string;   // ISO 8601
    tags: string[];
  };
  data: {
    prefabName: string;
    components: ComponentInstance[];
    resources?: {
      sprites: string[];
      audio: string[];
    };
  };
}

// A Component Instance in an Entity
interface ComponentInstance {
  type: string;                          // e.g., "Identity_data"
  properties: Map<string, any>;
}

// Parameter Schema for a Component Type
interface ComponentSchema {
  componentType: string;
  parameters: ParameterDefinition[];
}

// Single Parameter Definition
interface ParameterDefinition {
  name: string;
  type: ParameterType;
  defaultValue: any;
}

type ParameterType = 
  | "String"
  | "Int"
  | "Float"
  | "Bool"
  | "Color"
  | "Vector2"
  | "Array"
  | "Object";

// Component Library Entry
interface ComponentLibraryEntry {
  type: string;
  schema: ComponentSchema;
  isAlreadyInEntity: boolean;
  icon: string;
}

// Tab State (for multiple opened prefabs)
interface TabState {
  tabId: string;
  type: TabType;
  prefabPath: string;
  canvasTransform: {
    positionX: number;
    positionY: number;
    zoom: number;
  };
  selectedNodeId: string | null;
  isDirty: boolean;
  lastModified: Date;
}

enum TabType {
  BLUEPRINT_GRAPH,
  STATE_MACHINE,
  PREFAB_EDITOR,
  CUSTOM
}

// Visual Node in Canvas
interface ComponentNode {
  nodeId: string;
  componentType: string;
  position: { x: number; y: number };
  properties: Map<string, any>;
  isSelected: boolean;
  isHighlighted: boolean;
  schema: ComponentSchema;
}

// Editor State
interface PrefabEditorState {
  currentPrefab: EntityPrefab | null;
  currentFilePath: string | null;
  nodes: Map<string, ComponentNode>;
  selectedNodeId: string | null;
  isDirty: boolean;
  canvasTransform: {
    positionX: number;
    positionY: number;
    zoom: number;
  };
  undoStack: EditAction[];
  redoStack: EditAction[];
}

// Undo/Redo Action
interface EditAction {
  type: "ADD_COMPONENT" | "REMOVE_COMPONENT" | "UPDATE_PROPERTY" | "REORDER_COMPONENT";
  nodeId?: string;
  componentType?: string;
  propertyName?: string;
  oldValue?: any;
  newValue?: any;
  timestamp: Date;
}

// Validation Result
interface ValidationResult {
  isValid: boolean;
  errors: ValidationError[];
  warnings: ValidationWarning[];
}

interface ValidationError {
  nodeId: string;
  propertyName: string;
  message: string;
  severity: "ERROR" | "WARNING" | "INFO";
}

interface ValidationWarning {
  nodeId: string;
  message: string;
}
Data Flows
5.1 Loading a Prefab
Code
USER ACTION: Double-click on beacon.json in File Browser
                         │
                         ▼
            PrefabFileExplorer::onPrefabDoubleClicked()
                         │
    ┌────────────────────┴────────────────────┐
    │                                         │
    ▼                                         ▼
PrefabLoader::load()              ParameterSchemaRegistry::load()
    │                                    (one time at startup)
    │ Read: Gamedata/EntityPrefab/*.json        │
    │ Return: EntityPrefab object                │
    │                                         │
    └────────────────────┬────────────────────┘
                         │
                         ▼
            EntityPrefab Object created
                         │
    ┌────────────────────┴────────────────────────┐
    │                                             │
    ▼                                             ▼
ComponentLibrary::getComponentSchema()   PrefabCanvas::createNodes()
    │ Return schema for each component               │ Create visual nodes
    │                                                │
    └────────────────────┬────────────────────────┘
                         │
                         ▼
            ComponentNode[] created and laid out
                         │
                         ▼
            PrefabCanvas::render()
                         │
                         ▼
         DISPLAY in Blueprint Editor
5.2 Editing a Property
Code
USER ACTION: Modify value in PropertyInspector
                         │
                         ▼
            PropertyInspector::onPropertyChanged()
                         │
    ┌────────────────────┴────────────────────┐
    │                                         │
    ▼                                         ▼
Validator::validate()              ComponentNode::updateProperty()
    (against ParameterSchema)                 │
    │                                        │
    └────────────────────┬───────────────────┘
                         │
                    Valid? YES
                         │
                         ▼
            PrefabEditor::markDirty()
                         │
                         ▼
            PrefabCanvas::updateNode()
                         │
                         ▼
            PrefabCanvas::render()
                         │
                         ▼
         DISPLAY updated
5.3 Adding a Component
Code
USER ACTION: Drag-drop component from ComponentLibraryPanel
                         │
                         ▼
    ComponentLibraryPanel::onComponentDragStart()
                         │
                         ▼
    PrefabCanvas::onComponentDropped()
                         │
    ┌────────────────────┴────────────────────┐
    │                                         │
    ▼                                         ▼
ComponentLibrary::             PrefabEditor::addComponent()
getComponentSchema()                    │
    │ Get schema + defaults      EntityPrefab.data.components.push()
    │                                   │
    └────────────────────┬──────────────┘
                         │
                         ▼
            ComponentNode::create()
            (with default properties)
                         │
                         ▼
            PrefabCanvas::addNode()
                         │
                         ▼
            PrefabCanvas::layout()
                         │
                         ▼
            PrefabCanvas::render()
                         │
                         ▼
         PropertyInspector auto-switches to new component
5.4 Saving a Prefab
Code
USER ACTION: Click "Save" or Ctrl+S
                         │
                         ▼
            PrefabEditor::save()
                         │
    ┌────────────────────┴────────────────────────┐
    │                                             │
    ▼                                             ▼
PrefabValidator::validate()        PrefabSerializer::serialize()
    │ Check schema, required fields            │ Convert to JSON
    │                                         │
    └──────────Validate? YES?──────────────────┘
                     │
                 Valid? YES
                     │
                     ▼
            FileIO::writeFile()
        (Gamedata/EntityPrefab/name.json)
                     │
                     ▼
            PrefabEditor::markClean()
                     │
                     ▼
         CONFIRMATION displayed
Dependencies Matrix
6.1 Internal Dependencies
Code
┌──────────────────────────────────────────────────────────────────────────────┐
│ MODULE                    │ DEPENDS ON                 │ PROVIDES TO         │
├──────────────────────────────────────────────────────────────────────────────┤
│ PrefabFileExplorer       │ FileSystem, UIFramework   │ PrefabEditor        │
│                          │ (existants)               │ CanvasTabManager    │
├──────────────────────────────────────────────────────────────────────────────┤
│ PrefabLoader             │ FileIO, JSONParser        │ PrefabEditor        │
│                          │ (existants)               │                     │
├──────────────────────────────────────────────────────────────────────────────┤
│ ComponentLibrary         │ ParameterSchemaRegistry   │ ComponentNodeEd.    │
│                          │                           │ LibraryPanel        │
├──────────────────────────────────────────────────────────────────────────────┤
│ ParameterSchemaRegistry  │ JSONParser, FileIO        │ ComponentLibrary    │
│                          │ (existants)               │ PropertyInspect.    │
├──────────────────────────────────────────────────────────────────────────────┤
│ PrefabCanvas             │ Renderer, InputSystem     │ PrefabEditor        │
│                          │ (existants)               │ ComponentNodeEd.    │
├──────────────────────────────────────────────────────────────────────────────┤
│ ComponentNode            │ ComponentLibrary          │ PrefabCanvas        │
│                          │ ParameterSchemaRegistry   │ PropertyInspect.    │
├──────────────────────────────────────────────────────────────────────────────┤
│ PropertyInspector        │ ComponentNode             │ PrefabEditor        │
│                          │ ParameterSchemaRegistry   │                     │
├──────────────────────────────────────────────────────────────────────────────┤
│ PrefabSerializer         │ JSONParser, Validator     │ PrefabEditor        │
│                          │ (existants)               │                     │
├──────────────────────────────────────────────────────────────────────────────┤
│ PrefabEditor             │ ALL ABOVE MODULES         │ BlueprintEditor     │
│ (Main Controller)        │                           │                     │
└──────────────────────────────────────────────────────────────────────────────┘
6.2 External Dependencies (Existing Systems)
System	Usage	Notes
FileIO	Load/save prefab JSON files	Existing in project
JSONParser	Parse/serialize JSON	Existing in project
Renderer	Render canvas, nodes, UI	Existing in project
InputSystem	Handle mouse/keyboard input	Existing in project
UIFramework	UI components (panels, buttons)	Existing in project
ValidationSystem	Validate property values	Existing in project
CanvasTabManager	Manage multiple tabs	Extension to Blueprint Editor
Integration Points
7.1 Integration with Blueprint Editor
Code
BlueprintEditor Main Window
│
├─ Menu Bar
│  ├─ File
│  │  ├─ Open Prefab...        (NEW)
│  │  └─ New Prefab...         (NEW)
│  └─ View
│     ├─ Show Component Library (NEW)
│     └─ Show Property Inspector (NEW)
│
├─ Left Panel: File Browser
│  ├─ Existing functionality
│  └─ Prefab folder browsing (NEW)
│
├─ Center Area: Canvas Tabs
│  ├─ [Blueprint Graph] Tab (existing)
│  ├─ [State Machine] Tab (existing)
│  └─ [Prefab Editor] Tab (NEW) ◄─── OUR FOCUS
│
├─ Right Panel: Tabs
│  ├─ [📋 Properties] Tab (NEW)
│  └─ [📚 Components] Tab (NEW)
│
└─ Bottom: Status Bar
   └─ Prefab status info (NEW)
7.2 Canvas Tab System Integration
TypeScript
// Extends existing CanvasTab system
class PrefabEditorTab extends CanvasTab {
  - prefabPath: string
  - prefabEditor: PrefabEditor
  - canvas: PrefabCanvas
  
  + onActivated(): void
  + onDeactivated(): void
  + onClose(): void
  + serialize(): TabState
  + deserialize(state: TabState): void
}

// Integration with existing TabManager
class CanvasTabManager {
  + createTab(name: string, type: TabType): CanvasTab
  + addTab(tab: CanvasTab): void
  + closeTab(tabId: string): void
  + switchToTab(tabId: string): void
  + getCurrentTab(): CanvasTab
}
7.3 Keyboard Shortcuts
Code
Global Shortcuts:
  Ctrl+S         = Save current prefab (if Prefab Editor active)
  Ctrl+Z         = Undo (if Prefab Editor active)
  Ctrl+Y / Ctrl+Shift+Z = Redo (if Prefab Editor active)

Prefab Editor Specific:
  Delete         = Remove selected component
  Ctrl+A         = Select all components
  Right-Click    = Context menu (Add/Remove/Edit)
  Space          = Auto-layout
  Tab            = Switch between Properties & Components tabs
  Alt+1 / Alt+2  = Quick switch tabs
Implementation Phases
8.1 Phase 1: Prefab Visualizer (2-3 sprints)
Objective: Display prefab files and their metadata in file browser

Deliverables:

✅ PrefabFileExplorer integration
✅ PrefabLoader (JSON deserialization)
✅ Prefab metadata display
✅ Component list preview (read-only)
Acceptance Criteria:

 Can double-click prefab.json to see components
 Component names displayed
 Prefab metadata (author, date, tags) visible
 No errors on load for all existing prefabs
Dependencies:

FileIO (existing)
JSONParser (existing)
8.2 Phase 2: Component Node Visualization (1 sprint)
Objective: Render components as visual nodes on canvas

Deliverables:

✅ ComponentNode class
✅ ComponentNodeRenderer
✅ PrefabCanvas (read-only mode)
✅ Auto-layout algorithm (circular around entity)
Acceptance Criteria:

 Nodes render on canvas
 Entity center node visible
 Nodes arranged in circle around entity
 Zoom & pan working
 No performance issues with 20+ components
Dependencies:

Phase 1
Renderer (existing)
8.3 Phase 3: Component Library (1 sprint)
Objective: Load and display available components for selection

Deliverables:

✅ ParameterSchemaRegistry (loads ParameterSchemas.json)
✅ ComponentLibrary
✅ ComponentLibraryPanel UI
✅ Component filter/search
Acceptance Criteria:

 Registry loads all schemas at startup
 Library panel shows all available components
 Can search/filter components
 Shows which components already in entity (✅/❌)
 No lag when searching large component list
Dependencies:

Phase 1 & 2
UIFramework (existing)
8.4 Phase 4: Property Editing (2 sprints)
Objective: Edit component properties with type-safe editors

Deliverables:

✅ PropertyInspector panel
✅ Property editor widgets (Float, Int, Bool, String, Color, Vector2)
✅ Validation against ParameterSchema
✅ Real-time property updates
Acceptance Criteria:

 Can edit all property types
 Validation shows errors for invalid values
 Property changes reflect in canvas nodes
 Default values available for reset
 Type validation working (e.g., Float rejects "abc")
Dependencies:

Phase 1, 2, 3
ValidationSystem (existing)
8.5 Phase 5: Drag-Drop Component Addition (1 sprint)
Objective: Add components via drag-and-drop from library

Deliverables:

✅ Drag-drop mechanism
✅ Drop zone detection
✅ Auto-layout after component addition
✅ Immediate PropertyInspector switch
Acceptance Criteria:

 Can drag component from library to canvas
 Component added with default properties
 Canvas auto-layouts after addition
 PropertyInspector switches to new component
 "Modified" indicator appears
Dependencies:

Phase 1-4
InputSystem (existing)
8.6 Phase 6: Serialization & Complete Workflow (2 sprints)
Objective: Full save/load with undo-redo support

Deliverables:

✅ PrefabSerializer
✅ Save to JSON file
✅ Validation before save
✅ Undo/Redo system
✅ Multi-file editing support
✅ Context menu (delete, move, duplicate)
Acceptance Criteria:

 Can save prefabs to JSON
 Saved files match original schema
 Can undo/redo all operations
 Multiple prefabs can be edited simultaneously
 File modified indicator (*) working
 Validation errors prevent save
 Context menu operations working
Dependencies:

Phase 1-5
FileIO, Validator (existing)
User Workflows
9.1 Workflow: Create a New Prefab
Code
1. USER: File → New Prefab...
   SYSTEM: Opens dialog "Create New Prefab"

2. USER: Enters name: "MyCustomEntity"
   SYSTEM: Shows folder selection dialog

3. USER: Selects Gamedata/EntityPrefab/
   SYSTEM: Creates empty prefab with:
   - name: "MyCustomEntity"
   - empty components array
   - Creates new tab: [Prefab-MyCustomEntity*]
   - Shows Entity Center node

4. USER: Drags "Identity_data" from Components Library
   SYSTEM: 
   - Adds component with defaults
   - Creates new node
   - Auto-layouts
   - Switches to Properties tab
   - Shows Identity_data properties

5. USER: Edits properties:
   - name = "MyCustomEntity_1"
   - tag = "Custom"
   - entityType = "MyCustomEntity"

6. USER: Drags "Position_data" from library
   SYSTEM:
   - Adds component
   - Auto-layouts
   - Node appears

7. USER: Presses Ctrl+S
   SYSTEM:
   - Validates prefab
   - Saves to Gamedata/EntityPrefab/MyCustomEntity.json
   - Removes * from tab name
   - Shows success notification

✅ RESULT: New prefab created and saved
9.2 Workflow: Edit Existing Prefab
Code
1. USER: Double-clicks "beacon.json" in File Browser
   SYSTEM:
   - Loads beacon.json
   - Creates new tab [Prefab-beacon]
   - Displays 5 component nodes on canvas
   - Shows beacon metadata

2. USER: Clicks on "Position_data" node
   SYSTEM:
   - Selects node (highlight)
   - Switches to Properties tab
   - Shows Position_data properties

3. USER: Changes position.x from 0 to 100
   SYSTEM:
   - Validates (100 is valid Float)
   - Updates node visually
   - Marks prefab as dirty: [Prefab-beacon*]

4. USER: Drags "Movement_data" from Components Library
   SYSTEM:
   - Adds component with defaults (speed: 100.0, acceleration: 500.0)
   - Auto-layouts nodes
   - Switches PropertyInspector to Movement_data
   - Shows 6 components total

5. USER: Right-clicks on "VisualEditor_data" node
   SYSTEM: Shows context menu

6. USER: Clicks "Delete Component"
   SYSTEM:
   - Removes component from entity
   - Removes node from canvas
   - Auto-layouts
   - Shows 5 components now

7. USER: Presses Ctrl+Z (Undo)
   SYSTEM:
   - Restores VisualEditor_data
   - Shows 6 components again

8. USER: Presses Ctrl+Y (Redo)
   SYSTEM:
   - Removes VisualEditor_data again
   - Shows 5 components

9. USER: Presses Ctrl+S
   SYSTEM:
   - Validates all components
   - Saves to beacon.json
   - Removes * from tab

✅ RESULT: Prefab edited and changes saved
9.3 Workflow: Edit Multiple Prefabs
Code
1. USER: Double-clicks "player.json"
   SYSTEM: Creates [Prefab-player] tab with 13 components

2. USER: Double-clicks "guard.json"
   SYSTEM: Creates [Prefab-guard] tab with 15 components

3. USER: Double-clicks "zombie.json"
   SYSTEM: Creates [Prefab-zombie] tab with 10 components

4. CANVAS TABS now show:
   [Blueprint] [StateGraph] [Prefab-player] [Prefab-guard*] [Prefab-zombie]

5. USER: On guard tab, modifies AIState_data
   SYSTEM: 
   - Mark guard as dirty: [Prefab-guard*]
   - Shows property changes in PropertyInspector

6. USER: Switches to player tab (click tab)
   SYSTEM:
   - Canvas shows player components
   - PropertyInspector updates to show player's components
   - Right panel shows player-specific component library

7. USER: On player tab, adds PlayerController_data
   SYSTEM:
   - Component added to player
   - Mark player as dirty: [Prefab-player*]

8. USER: Clicks [Prefab-zombie] tab
   SYSTEM: Shows zombie components (unmodified)

9. USER: Saves all: Ctrl+S (or File → Save All)
   SYSTEM:
   - Saves player.json ✓
   - Saves guard.json ✓
   - Removes * from tabs

✅ RESULT: Multiple prefabs edited and saved independently
9.4 Workflow: Validate and Fix Errors
Code
1. USER: Tries to edit "Health_data" component
   USER: Sets maxHealth to "invalid_text"

2. SYSTEM: PropertyInspector shows error:
   ❌ "Invalid value 'invalid_text': expected Float"

3. USER: Corrects value to "150"

4. SYSTEM: Error disappears ✓

5. USER: Tries to save (Ctrl+S)

6. SYSTEM: Validator checks prefab:
   - All components have required properties ✓
   - All types match schemas ✓
   - Entity has at least Identity_data ✓

7. SYSTEM: Saves successfully
   - File written to Gamedata/EntityPrefab/beacon.json
   - Tab name loses *
   - Success notification shown

✅ RESULT: Validation ensures data integrity
Technical Specifications
10.1 Initialization Sequence
Code
Application Startup:
│
├─ 1. Load ParameterSchemas.json
│  │
│  └─→ ParameterSchemaRegistry::initialize()
│     ├─ Parse JSON
│     ├─ Cache schemas in Map<string, ComponentSchema>
│     └─ Validate all schemas
│
├─ 2. Load EntityPrefabSynonymsRegister.json
│  │
│  └─→ EntitySynonymRegistry::initialize()
│     └─ Build type → canonical type mapping
│
├─ 3. Blueprint Editor initialized
│  │
│  └─→ CanvasTabManager created
│     └─ Ready for new tabs
│
├─ 4. PrefabFileExplorer initialized
│  │
│  └─→ File browser ready
│     └─ Can browse Gamedata/EntityPrefab/
│
└─ 5. Ready for user interactions
   ✓ All systems loaded
   ✓ Caches populated
   ✓ UI responsive
10.2 Memory Management
Code
Per Prefab Editor Tab:
├─ EntityPrefab object: ~10-50 KB (depending on components)
├─ ComponentNode array: ~1 KB per component
├─ UI State: ~5 KB
└─ Canvas state: ~2 KB

Total per tab: ~50-100 KB

Caches (shared, not per-tab):
├─ ParameterSchemaRegistry: ~500 KB (loaded once)
├─ EntitySynonymRegistry: ~50 KB (loaded once)
└─ Renderer cache: Variable

Typical usage (3 prefabs open):
├─ Tab 1: 80 KB
├─ Tab 2: 75 KB
├─ Tab 3: 70 KB
├─ Caches: 550 KB
└─ TOTAL: ~850 KB (comfortable)
10.3 Performance Targets
Operation	Target	Notes
Load prefab	< 100 ms	Including node creation
Render canvas	60 FPS	20+ components
Property edit	Instant	Real-time feedback
Save prefab	< 50 ms	Write to disk
Drag-drop	Smooth	60 FPS during drag
Search components	< 50 ms	Filter 100+ items
Undo/Redo	< 30 ms	State restoration
10.4 Threading Model
Code
Main Thread:
├─ UI rendering
├─ Event handling
├─ Canvas updates
└─ User interactions

Worker Thread (optional, for large files):
├─ JSON parsing (load)
├─ JSON serialization (save)
├─ Validation
└─ File I/O

Synchronization:
├─ Editor state locked during save
├─ Validation happens on main thread (fast)
└─ No cross-thread access to prefab data
Configuration
11.1 Configurable Parameters
TypeScript
// Editor Configuration
const CONFIG = {
  // UI
  componentNodeSize: { width: 100, height: 80 },
  componentNodeSpacing: 50,
  entityNodeSize: { width: 120, height: 100 },
  layoutRadius: 300,
  
  // Canvas
  minZoom: 0.1,
  maxZoom: 5.0,
  zoomStep: 1.2,
  panSpeed: 1.0,
  
  // Performance
  renderFPS: 60,
  autoSaveInterval: 60000, // 1 minute
  undoStackLimit: 50,
  
  // File paths
  prefabDirectory: "Gamedata/EntityPrefab/",
  schemaPath: "Gamedata/EntityPrefab/ParameterSchemas.json",
  synonymsPath: "Gamedata/EntityPrefab/EntityPrefabSynonymsRegister.json",
  
  // Behavior
  autoLayoutOnComponentAdd: true,
  autoSwitchToPropertyInspector: true,
  showComponentTooltips: true,
  enableDragDrop: true,
  
  // Validation
  validateOnPropertyChange: true,
  validateBeforeSave: true,
  strict validation: false
};
11.2 User Preferences
Code
Saved per-user:
├─ Last opened prefabs
├─ Tab layout preferences
├─ Right panel width
├─ Canvas zoom level per prefab
├─ Component library filter
└─ Recent searches
Testing Strategy
12.1 Unit Tests
Code
ParameterSchemaRegistry:
├─ Test load valid schema
├─ Test load invalid schema
├─ Test cache retrieval
└─ Test schema validation

ComponentLibrary:
├─ Test get component schema
├─ Test get available components
├─ Test filter by name
└─ Test is component in entity

PrefabLoader:
├─ Test load valid prefab
├─ Test load invalid prefab
├─ Test load with missing file
└─ Test entity creation

PrefabSerializer:
├─ Test serialize to JSON
├─ Test round-trip (load → modify → save)
├─ Test property type conversion
└─ Test validation before save

ComponentNode:
├─ Test node creation
├─ Test property update
├─ Test validation
└─ Test position calculation

PrefabValidator:
├─ Test required properties
├─ Test type validation
├─ Test schema compliance
└─ Test error collection
12.2 Integration Tests
Code
Load Workflow:
├─ Load prefab from file
├─ Create nodes
├─ Verify canvas state
└─ Verify loaded properties match JSON

Edit Workflow:
├─ Load prefab
├─ Modify property
├─ Verify node updated
├─ Verify dirty flag set
└─ Verify PropertyInspector updated

Add Component Workflow:
├─ Load prefab
├─ Drag component from library
├─ Verify component added
├─ Verify auto-layout
├─ Verify default properties loaded
└─ Verify PropertyInspector switched

Save Workflow:
├─ Load prefab
├─ Modify multiple properties
├─ Modify add components
├─ Save (Ctrl+S)
├─ Verify file written
├─ Load same file
├─ Verify all changes persisted

Undo/Redo Workflow:
├─ Load prefab
├─ Make changes (add component, modify property)
├─ Undo (Ctrl+Z)
├─ Verify state reverted
├─ Redo (Ctrl+Y)
├─ Verify state restored
└─ Verify undo stack limit respected

Multi-Tab Workflow:
├─ Open 3 prefabs
├─ Modify each
├─ Switch between tabs
├─ Verify each maintains its state
├─ Save one prefab
├─ Verify others unaffected
└─ Close tabs

Validation Workflow:
├─ Try invalid property value
├─ Verify error shown
├─ Correct value
├─ Verify error cleared
├─ Try to save with invalid value
├─ Verify save blocked
└─ Fix and save successfully
12.3 UI/UX Tests
Code
Visual Tests:
├─ Component nodes render correctly
├─ Entity center node visible
├─ Nodes arranged in circle
├─ Zoom/pan responsive
├─ Tab switching smooth

Interaction Tests:
├─ Drag-drop component works
├─ Right-click context menu shows
├─ Property editor responds to input
├─ Selection highlighting works
├─ Undo/Redo buttons responsive

Performance Tests:
├─ Load large prefab (20+ components) < 100ms
├─ 60 FPS during drag
├─ Search 100 components < 50ms
├─ Render 20 nodes smooth
Performance Targets
13.1 Quantitative Goals
Metric	Target	Current	Status
Prefab load time	< 100 ms	TBD	⏳
Component add	< 50 ms	TBD	⏳
Property edit	Instant	TBD	⏳
Save operation	< 100 ms	TBD	⏳
Canvas FPS	60 FPS	TBD	⏳
Memory per tab	< 100 KB	TBD	⏳
Search latency	< 50 ms	TBD	⏳
Undo/Redo	< 30 ms	TBD	⏳
13.2 Profiling Strategy
Code
Tools:
├─ CPU profiler
├─ Memory profiler
├─ GPU profiler
└─ Frame time analyzer

Checkpoints:
├─ After Phase 2 (basic rendering)
├─ After Phase 4 (property editing)
├─ After Phase 6 (complete system)
└─ Quarterly optimization sprints
Future Enhancements
14.1 Post-MVP Features
Code
Phase 7: Advanced Features
├─ Prefab inheritance (extends other prefabs)
├─ Prefab variants
├─ Batch editing multiple prefabs
├─ Component templates/presets
├─ Import/export prefabs
├─ Prefab versioning
└─ Search across all prefabs

Phase 8: Visual Enhancements
├─ Preview entity sprite in canvas
├─ Property value graph editor
├─ Animation preview
├─ Resource browser
├─ Asset references validator
└─ Visual debugging tools

Phase 9: Collaboration
├─ Multi-user editing
├─ Change history/log
├─ Comments on properties
├─ Version control integration
└─ Diff viewer

Phase 10: AI/Automation
├─ Auto-generate prefabs
├─ Component recommendation
├─ Property auto-fill based on type
├─ Duplicate detection
└─ Schema migration tools
14.2 Known Limitations (MVP)
❌ No prefab inheritance
❌ No batch editing
❌ No visual sprite preview
❌ No multi-user support
❌ Limited to local file system
❌ No diff/merge tools
API Reference
15.1 PrefabEditor (Main Controller)
TypeScript
class PrefabEditor {
  // Initialization
  + constructor(tabManager: CanvasTabManager, fileBrowser: FileBrowser)
  + initialize(): void
  
  // File Operations
  + openPrefab(filePath: string): void
  + savePrefab(): void
  + savePrefabAs(newPath: string): void
  + revertChanges(): void
  + closeCurrentTab(): void
  
  // Component Operations
  + addComponent(componentType: string): ComponentNode
  + removeComponent(nodeId: string): void
  + updateComponentProperty(nodeId: string, propertyName: string, value: any): void
  + duplicateComponent(nodeId: string): ComponentNode
  + resetComponentToDefaults(nodeId: string): void
  
  // Selection
  + selectComponent(nodeId: string): void
  + deselectAll(): void
  + getSelectedComponent(): ComponentNode | null
  
  // Edit History
  + undo(): void
  + redo(): void
  + canUndo(): bool
  + canRedo(): bool
  
  // State
  + isDirty(): bool
  + getCurrentPrefab(): EntityPrefab | null
  + getCurrentFilePath(): string | null
  + getComponentNodes(): ComponentNode[]
  
  // Validation
  + validate(): ValidationResult
  + validateProperty(nodeId: string, propName: string, value: any): bool
}
15.2 PrefabCanvas
TypeScript
class PrefabCanvas {
  // Rendering
  + render(): void
  + update(deltaTime: float): void
  
  // Node Management
  + createNode(componentType: string, properties: Map): ComponentNode
  + removeNode(nodeId: string): void
  + updateNode(nodeId: string, properties: Map): void
  + getNode(nodeId: string): ComponentNode | null
  + getAllNodes(): ComponentNode[]
  
  // Layout
  + autoLayout(): void
  + fitAll(): void
  + zoom(factor: float): void
  + pan(dx: float, dy: float): void
  
  // Interaction
  + onMouseClick(x: float, y: float): void
  + onMouseDrag(x: float, y: float, dx: float, dy: float): void
  + onMouseDrop(x: float, y: float, dragData: any): void
  + onKeyPress(key: string): void
  
  // Selection
  + selectNode(nodeId: string): void
  + deselectNode(nodeId: string): void
  + clearSelection(): void
  + getSelectedNodes(): ComponentNode[]
}
15.3 PropertyInspector
TypeScript
class PropertyInspector {
  // Display
  + displayComponent(node: ComponentNode): void
  + clearDisplay(): void
  + updatePropertyValue(propName: string, value: any): void
  
  // Events
  + onPropertyChanged(propName: string, newValue: any): void
  + onPropertyReset(propName: string): void
  
  // Validation
  + validateProperty(propName: string, value: any): bool
  + showError(message: string): void
  + clearErrors(): void
}
15.4 ComponentLibrary
TypeScript
class ComponentLibrary {
  // Initialization
  + initialize(): void
  
  // Queries
  + getComponentSchema(type: string): ComponentSchema | null
  + getAllSchemas(): ComponentSchema[]
  + getAvailableComponents(): ComponentLibraryEntry[]
  + searchComponents(query: string): ComponentLibraryEntry[]
  + filterByTag(tag: string): ComponentLibraryEntry[]
  
  // Checking
  + isComponentInEntity(entityPrefab: EntityPrefab, type: string): bool
  + getComponentCount(): int
}
15.5 PrefabSerializer
TypeScript
class PrefabSerializer {
  // Serialization
  + serialize(prefab: EntityPrefab): string
  + serializeToFile(prefab: EntityPrefab, path: string): void
  
  // Deserialization
  + deserialize(json: string): EntityPrefab
  + deserializeFromFile(path: string): EntityPrefab
  
  // Validation
  + validateBeforeSerialization(prefab: EntityPrefab): ValidationResult
}
Appendix
A.1 Example: beacon.json Structure
JSON
{
  "schema_version": 2,
  "type": "Beacon",
  "blueprintType": "EntityPrefab",
  "name": "Beacon",
  "description": "Generic Beacon position in the world",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-01-30T02:26:00Z",
    "lastModified": "2026-01-30T02:26:00Z",
    "tags": ["Beacon", "location", "poi", "point"]
  },
  "data": {
    "prefabName": "Beacon",
    "components": [
      {
        "type": "Identity_data",
        "properties": {
          "name": "{instanceName}",
          "tag": "Beacon",
          "entityType": "Beacon"
        }
      },
      {
        "type": "Position_data",
        "properties": {
          "position": {
            "x": 0,
            "y": 0,
            "z": 0
          }
        }
      },
      {
        "type": "VisualSprite_data",
        "properties": {
          "spritePath": "./Resources/Icons/pin-32.png",
          "width": 32,
          "height": 32,
          "layer": 10
        }
      }
    ],
    "resources": {
      "sprites": ["./Resources/Icons/pin-32.png"],
      "audio": []
    }
  }
}
A.2 Example: ParameterSchemas.json Entry
JSON
{
  "componentType": "Movement_data",
  "parameters": [
    {
      "name": "speed",
      "type": "Float",
      "defaultValue": 100.0
    },
    {
      "name": "acceleration",
      "type": "Float",
      "defaultValue": 500.0
    }
  ]
}
A.3 Example: TypeScript Integration
TypeScript
import { PrefabEditor } from "./PrefabEditor";
import { CanvasTabManager } from "./CanvasTabManager";

// Initialize prefab editor
const tabManager = new CanvasTabManager();
const prefabEditor = new PrefabEditor(tabManager, fileBrowser);
prefabEditor.initialize();

// User opens a prefab
prefabEditor.openPrefab("Gamedata/EntityPrefab/beacon.json");

// User adds a component
const newNode = prefabEditor.addComponent("Movement_data");

// User edits a property
prefabEditor.updateComponentProperty(newNode.nodeId, "speed", 120.0);

// User saves
prefabEditor.savePrefab();
A.4 Example: CSS Styling (if using web-based UI)
CSS
/* Component Node */
.component-node {
  width: 100px;
  height: 80px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border: 2px solid #333;
  border-radius: 8px;
  padding: 8px;
  cursor: pointer;
  box-shadow: 0 4px 6px rgba(0,0,0,0.1);
  transition: all 0.3s ease;
}

.component-node:hover {
  transform: scale(1.05);
  box-shadow: 0 8px 12px rgba(0,0,0,0.2);
}

.component-node.selected {
  border-color: #4ade80;
  box-shadow: 0 0 12px rgba(74, 222, 128, 0.5);
}

/* Entity Center Node */
.entity-node {
  width: 120px;
  height: 100px;
  background: radial-gradient(circle, #fbbf24, #f97316);
  border: 3px solid #7c2d12;
  border-radius: 12px;
}

/* Property Inspector */
.property-inspector {
  padding: 16px;
  background: #f8f9fa;
  border-left: 1px solid #dee2e6;
}

.property-row {
  margin-bottom: 16px;
  padding: 12px;
  background: white;
  border-radius: 6px;
}

.property-label {
  font-weight: 600;
  margin-bottom: 8px;
  color: #212529;
}

.property-editor {
  width: 100%;
  padding: 8px;
  border: 1px solid #dee2e6;
  border-radius: 4px;
  font-size: 14px;
}

.property-error {
  color: #dc3545;
  font-size: 12px;
  margin-top: 4px;
}
A.5 Validation Error Examples
Code
❌ Invalid Float value
   "acceleration" should be Float, got "abc"

❌ Required property missing
   Component "Identity_data" requires property "name"

❌ Value out of range
   "speed" must be > 0, got -10

❌ Type mismatch
   "x" should be Float, got Array

❌ Duplicate component
   Entity already has "Identity_data" component

✅ All validations passed
   Prefab is valid and can be saved
Conclusion
This specification defines a complete, production-ready Entity Prefab Editor module for the Olympe Engine. It covers:

✅ Architecture and module organization
✅ UI/UX design with detailed mockups
✅ Data structures and workflows
✅ Integration with existing systems
✅ Phased implementation plan
✅ Testing strategy
✅ Performance targets
✅ Complete API reference

The 6-phase implementation plan is designed to deliver value incrementally, with Phase 1 providing immediate value (visualization) and Phase 6 delivering the complete system (save/undo/multi-file).

Total Estimated Effort: 8-10 sprints (40-50 development days)

Document Version: 1.0
Last Updated: 2026-04-02
Next Review: After Phase 1 completion

Code

---

## 📥 **Fichier disponible**

Le fichier est maintenant disponible à:
./ENTITY_PREFAB_EDITOR_SPECIFICATION.md

Code

Vous pouvez le :
- ✅ **Télécharger directement** depuis le repo
- ✅ **Copier-coller** le contenu complet
- ✅ **Éditer** dans votre système
- ✅ **Partager** avec votre équipe

Le document contient **tous les éléments** définis lors de notre discussion :
- 📊 Architectures complètes avec diagrammes
- 🎨 6 mockups ASCII détaillés
- 📋 Tous les workflows utilisateurs
- 🔧 Spécifications techniques complètes
- 📦 6 phases d'implémentation détaillées
- ✅ Critères d'acceptation pour chaque phase
- 🧪 Stratégie de test complète
- 📚 Référence API complète
- 📈 Cibles de performance

Voulez-vous que je crée maintenant un **pull request** pour démarrer la **Phase 1** en utilisant cette spécification comme référence ? 🚀
Make these code changes?
ENTITY_PREFAB_EDITOR_SPECIFICATION.md

md
# Entity Prefab Editor Specification

## Overview
The Entity Prefab Editor is a critical component of the Olympe Engine, designed to facilitate the creation, management, and utilization of entity prefabs within the game development environment. This document outlines the architecture, UI design, mockups, data structures, workflows, and phases of implementation for the Entity Prefab Editor module.

## Architecture
- **Core Components:**
  - Entity Manager
  - Prefab System
  - Serialization/Deserialization Engine
  - User Interface Components

- **Dependencies:**
  - Game Engine Core
  - Input Management System

## UI Design
- The UI will be structured into several panels:
  - **Inspector Panel:** For editing properties of the selected prefab.
  - **Hierarchy Panel:** Displays a list of available prefabs.
  - **Canvas Area:** For visualizing the prefab and its components.

- **Design Principles:**
  - Intuitive user experience.
  - Responsive and adaptive layouts.

## Mockups
1. **Main Interface:**
   - Preview area of the selected prefab.
   - Properties checklist and sliders for tuning.

2. **Prefab List:**
   - Categorized view of prefabs with search functionality.
   - Option to create or delete prefabs.

## Data Structures
- **Prefab Definition:**
  ```json
  {
    "id": "string",
    "components": [
      {
        "type": "string",
        "properties": { }
      }
    ],
    "metadata": {
      "created_at": "date",
      "updated_at": "date"
    }
  }
Component Schema: Defines parameters for the prefab components like position, scale, rotation, etc.
Workflows
Creating a New Prefab:

User clicks on 'Create New'.
Enters properties and saves.
Modifying an Existing Prefab:

Select prefab from hierarchy.
Edit properties in Inspector and save changes.
Implementation Phases
Phase 1: Requirements Gathering
Phase 2: UI/UX Design
Phase 3: Core Development
Phase 4: Testing and Feedback
Phase 5: Final Review and Launch
Conclusion
The Entity Prefab Editor is an essential module designed to enhance the efficiency of prefab management in the Olympe Engine. This specification serves as a roadmap to guide the development process and ensures all aspects of the module are thoroughly covered.