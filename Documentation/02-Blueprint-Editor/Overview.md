# Blueprint Editor

The Blueprint Editor is the main editor panel for creating and editing visual graph files in Olympe Engine.

## Overview

The Blueprint Editor provides a tabbed interface for managing multiple graph documents simultaneously. It supports:

- **Visual Script graphs** (`.ats` files) – Node-based scripting for entity behavior
- **Behavior Tree graphs** (`.bt` files) – AI decision trees
- **Entity Prefab graphs** (`.json` files) – Component-based entity templates
- **Animation graphs** – Animation state machines

## Architecture

```
BlueprintEditorGUI
    └── TabManager
            ├── VisualScriptEditorPanel  (IGraphRenderer)
            ├── BTEditorPanel            (IGraphRenderer)
            ├── EntityPrefabRenderer     (IGraphRenderer)
            └── AnimationEditorPanel     (IGraphRenderer)
```

## Tab System (Phase 26)

The editor uses a tab-based UI with three right-panel sections:
- **Tab 0**: Preset Bank (Condition Presets)
- **Tab 1**: Local Variables (Local Blackboard)
- **Tab 2**: Global Variables (Global Blackboard)

## Key Files

| File | Description |
|------|-------------|
| `BlueprintEditorGUI.cpp` | Main editor window, menus, tab bar |
| `VisualScriptEditorPanel_*.cpp` | Visual scripting editor (split into ~10 files) |
| `TabManager.h/.cpp` | Manages open graph tabs |
| `IGraphRenderer.h` | Interface all editor types implement |

## Related Documentation

- [Visual Scripting](../03-Visual-Scripting/Graph-System.md)
- [Entity Prefabs](../04-Entity-Prefabs/Prefab-System.md)
- [Canvas System](../06-Canvas-System/Grid-Standardization.md)
- Existing docs: `Source/BlueprintEditor/STANDARDIZATION_EXECUTIVE_SUMMARY.md`
