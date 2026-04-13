# Olympe Engine – Architecture Overview

## System Map

```mermaid
graph TD
    A[Main Application] --> B[ECS World]
    A --> C[Blueprint Editor]
    A --> D[Tiled Map Editor]

    B --> E[Systems]
    B --> F[Components]
    B --> G[Entities]

    C --> H[Visual Script Editor]
    C --> I[Behavior Tree Editor]
    C --> J[Entity Prefab Editor]
    C --> K[Animation Editor]

    H --> L[TaskGraphLoader]
    H --> M[BlackboardSystem]
    I --> N[BTGraphLoader]
    J --> O[PrefabLoader]

    L --> P[.ats JSON Files]
    N --> Q[.bt JSON Files]
    O --> R[.json Prefab Files]
```

## Core Principles

1. **JSON as Single Source of Truth** – All game data lives in JSON files
2. **ECS Architecture** – Entities are data, Systems are logic
3. **Visual Scripting** – Game behavior defined without C++ code
4. **Component-Based Prefabs** – Entities assembled from reusable components

## Module Breakdown

### Source/ Layout
```
Source/
├── ECS/                 # ECS core (Entity, Component, System)
├── AI/                  # AI systems and editors
├── Animation/           # Animation system
├── BlueprintEditor/     # All visual editors
│   ├── EntityPrefabEditor/   # Phase 27-31
│   ├── Utilities/            # Canvas, Grid, Minimap
│   └── VisualScript*         # Visual scripting panels
├── Core/                # Engine core utilities
├── Editor/              # Condition presets, panels, modals
├── NodeGraphCore/       # Blackboard, graph templates
└── OlympeTilemapEditor/ # Tiled map integration
```

## Data Flow

```
JSON File → Loader → In-Memory Model → Editor Panel → Serializer → JSON File
                                     ↓
                              Runtime Executor
                                     ↓
                              ECS World (entities + components)
```

## Key Design Decisions

See [08-Development/Architecture-Decisions.md](08-Development/Architecture-Decisions.md) for the full ADR log.
