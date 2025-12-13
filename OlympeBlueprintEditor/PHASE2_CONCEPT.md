# Phase 2: Visual Node-Based Blueprint Editor

## Concept Overview

Phase 2 will extend the current console-based editor with a visual, node-based interface using ImGui and ImNodes libraries. This will provide a more intuitive, graphical way to edit blueprints and compose behaviors.

## Planned Features

### Visual Component Editor

Instead of menu-based property editing, components will be displayed as visual nodes:

```
┌─────────────────────────────┐
│      Position Component     │
├─────────────────────────────┤
│  X: [100.0]  ▲▼             │
│  Y: [200.0]  ▲▼             │
│  Z: [  0.0]  ▲▼             │
└─────────────────────────────┘

┌─────────────────────────────┐
│   VisualSprite Component    │
├─────────────────────────────┤
│  Sprite: [Browse...]        │
│  Width:  [32]    ▲▼         │
│  Height: [32]    ▲▼         │
│  HotSpot X: [16] ▲▼         │
│  HotSpot Y: [16] ▲▼         │
└─────────────────────────────┘
```

### Behavior Graph Editor

Behaviors will be composed visually with nodes and connections:

```
     ┌─────────────┐
     │   Detect    │○────┐
     │   Player    │     │
     └─────────────┘     │
                         │
                         ▼
     ┌─────────────┐  ┌────────────┐
     │   Patrol    │  │   Chase    │
     │   Points    │  │   Target   │
     └─────────────┘  └────────────┘
          │                 │
          └────────┬────────┘
                   ▼
          ┌─────────────┐
          │   Move To   │
          │  Position   │
          └─────────────┘
```

### Component Property Binding

Visual connections between component properties:

```
┌─────────────┐             ┌─────────────┐
│  Position   │             │   Camera    │
│             │●────────────●│             │
│  X: 100     │  Tracking   │  Target     │
│  Y: 200     │             │             │
└─────────────┘             └─────────────┘
```

## UI Layout (ImGui)

The editor window will have multiple panels:

```
╔═══════════════════════════════════════════════════════════════╗
║  Olympe Blueprint Editor                           [_][□][X]  ║
╠═══════════════════════════════════════════════════════════════╣
║ File  Edit  View  Components  Behaviors  Help                 ║
╠══════════════════╦════════════════════════╦═══════════════════╣
║                  ║                        ║                   ║
║  Entity List     ║    Component Graph     ║   Properties      ║
║  ────────────    ║    ──────────────      ║   ──────────      ║
║                  ║                        ║                   ║
║  ▶ Player        ║  [Visual Node Graph]   ║   Position        ║
║  ▶ Enemy_01      ║                        ║   ─────────       ║
║  ▶ NPC_Vendor    ║   Components are       ║   X: ▲▼ 100.0    ║
║  ▷ Item_Sword    ║   displayed as nodes   ║   Y: ▲▼ 200.0    ║
║                  ║   with connections     ║   Z: ▲▼   0.0    ║
║  [+ New Entity]  ║                        ║                   ║
║                  ║   Pan: Mouse Drag      ║   [Apply]         ║
║                  ║   Zoom: Scroll         ║                   ║
║                  ║                        ║                   ║
╠══════════════════╩════════════════════════╩═══════════════════╣
║  Ready | SimpleCharacter.json | 3 components | Modified       ║
╚═══════════════════════════════════════════════════════════════╝
```

## Implementation Plan

### Sprint 4: Visual Editor (3 weeks)

**Week 1: ImGui Integration**
- Set up ImGui rendering loop
- Create main window and panels
- Implement docking system
- Basic file menu (New, Open, Save, Exit)

**Week 2: Component Panel**
- Display components as ImGui widgets
- Property editors (sliders, text fields, color pickers)
- Add/Remove component buttons
- Component templates/presets

**Week 3: ImNodes Integration**
- Set up ImNodes for node graph
- Display components as nodes
- Node connections visualization
- Basic node editing

### Sprint 5: Behavior System (Future)

**Behavior Node Types:**
- **Condition Nodes**: Check game state, distance, health, etc.
- **Action Nodes**: Move, Attack, Wait, Interact
- **Control Nodes**: Sequence, Selector, Parallel
- **Event Nodes**: OnCollision, OnDamage, OnTrigger

**Example Behavior Graph:**

```json
{
    "type": "BehaviorGraph",
    "name": "GuardAI",
    "nodes": [
        {
            "id": 1,
            "type": "Patrol",
            "position": {"x": 100, "y": 100},
            "properties": {
                "waypoints": [[0,0], [100,0], [100,100]],
                "loop": true
            }
        },
        {
            "id": 2,
            "type": "DetectPlayer",
            "position": {"x": 100, "y": 200},
            "properties": {
                "radius": 150
            }
        },
        {
            "id": 3,
            "type": "ChaseTarget",
            "position": {"x": 200, "y": 200},
            "properties": {
                "speed": 100
            }
        }
    ],
    "connections": [
        {"from": 2, "to": 3, "condition": "player_detected"}
    ]
}
```

## Technical Requirements

### Dependencies
- **ImGui** (v1.89+) - Immediate mode GUI library
- **ImNodes** (v0.5+) - Node editor extension for ImGui
- **SDL3** or **GLFW** - Window/context management
- **OpenGL 3.0+** - Rendering backend

### Build System Updates

**CMake** (recommended for cross-platform):
```cmake
find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(OlympeBlueprintEditor
    src/main_gui.cpp
    src/BlueprintEditorGUI.cpp
    src/ImGuiLayer.cpp
    src/NodeEditor.cpp
    # ... existing files
)

target_link_libraries(OlympeBlueprintEditor
    SDL3::SDL3
    OpenGL::GL
    imgui
    imnodes
)
```

## Migration Path

Phase 1 (Console) → Phase 2 (Visual) will be seamless:

1. **Keep Console Editor**: Remain as fallback and for scripting
2. **Shared Backend**: Both UIs use the same `EntityBlueprint` class
3. **Dual Entry Points**: 
   - `main.cpp` - Console editor
   - `main_gui.cpp` - Visual editor
4. **Command Line Switch**: `--gui` flag to choose interface

## User Workflow (Phase 2)

### Creating an Entity Visually

1. **File → New Blueprint**
2. **Drag "Position" from component palette** → Node appears
3. **Edit properties** in the node or properties panel
4. **Drag "VisualSprite" component** → Node appears
5. **Connect components** (optional, for dependencies)
6. **File → Save**

### Editing Behaviors

1. **Open entity blueprint**
2. **Switch to "Behaviors" tab**
3. **Drag behavior nodes** from palette
4. **Connect nodes** with logic flow
5. **Set conditions** on connections
6. **Test in preview panel**
7. **Save behavior graph**

## Benefits Over Console Editor

- **Visual Feedback**: See component relationships
- **Faster Editing**: Drag-and-drop interface
- **Less Error-Prone**: Type-safe UI widgets
- **Real-Time Preview**: See changes immediately
- **Easier Learning**: Self-documenting interface
- **Better Scaling**: Handle complex entities easily

## Backwards Compatibility

All Phase 2 features will maintain full compatibility with Phase 1:
- Same JSON format
- Same file structure
- Console editor still functional
- Can switch between editors

## Timeline Estimate

- **Sprint 4**: ImGui integration + Component editing (3 weeks)
- **Sprint 5**: ImNodes integration + Behavior graphs (3 weeks)
- **Polish**: Testing, documentation, examples (1 week)

**Total: ~7 weeks for full visual editor**

## Current Status

✅ **Phase 1 Complete**: Console editor fully functional  
📋 **Phase 2 Planned**: Visual editor design complete  
⏳ **Phase 2 Implementation**: Awaiting green light

---

**Note**: Phase 1 provides all core functionality. Phase 2 adds a better user experience but is not required for the editor to be useful. The console editor is production-ready and can be used immediately.
