# Project Structure

```
Olympe-Engine/
├── Source/                    # C++ source code
│   ├── ECS/                   # Entity-Component-System core
│   ├── AI/                    # AI systems and BT editor
│   ├── Animation/             # Animation system
│   ├── BlueprintEditor/       # Visual editors
│   │   ├── EntityPrefabEditor/  # Entity Prefab Editor (Phase 27+)
│   │   └── Utilities/           # Canvas, Grid, Minimap tools
│   ├── Core/                  # DataManager, utilities
│   ├── Editor/                # Condition presets, panels
│   ├── NodeGraphCore/         # Blackboard, graph templates
│   └── OlympeTilemapEditor/   # Tiled map loading
├── Gamedata/                  # Runtime game data (JSON)
│   ├── BehaviorTrees/         # .bt graph files
│   ├── VisualScripts/         # .ats visual script files
│   ├── PrefabEntities/        # Entity prefab JSON files
│   └── Blueprints/            # Blueprint graph files
├── Documentation/             # Architecture & developer docs (this folder)
├── website/                   # Docusaurus documentation site
│   ├── docs/                  # Markdown documentation pages
│   └── static/api/            # Doxygen HTML output
├── Assets/                    # Game assets (sprites, sounds)
├── Templates/                 # Tiled map templates
├── Tests/                     # Unit & integration tests
├── Doxyfile                   # Doxygen configuration
└── Olympe Engine.sln          # Visual Studio solution
```

## Key Directories

### Source/BlueprintEditor/
Contains all visual editor implementations:
- `VisualScriptEditorPanel_*.cpp` – Visual scripting editor (split into ~10 files)
- `EntityPrefabEditor/` – Entity Prefab Editor components
- `Utilities/` – Shared canvas, grid, minimap utilities

### Gamedata/
All runtime data is JSON. This is the "single source of truth" for game content.

### website/
The Docusaurus documentation website. Run `npm start` from this directory to preview locally.
