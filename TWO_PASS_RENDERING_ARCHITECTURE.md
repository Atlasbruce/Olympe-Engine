# Two-Pass Rendering Architecture Diagram

## System Overview

```
┌────────────────────────────────────────────────────────────────┐
│                      GameEngine Main Loop                       │
│                         (OlympeEngine.cpp)                      │
└────────────────────────────┬───────────────────────────────────┘
                             │
                             ▼
┌────────────────────────────────────────────────────────────────┐
│                     World::ProcessSystems()                     │
│                         (World.cpp)                             │
└────────────────────────────┬───────────────────────────────────┘
                             │
                ┌────────────┴────────────┐
                │  Systems executed in    │
                │  registration order     │
                └────────────┬────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
   ┌─────────┐        ┌──────────┐        ┌──────────┐
   │ Input   │        │ Physics  │        │ Movement │
   │ Systems │───────>│ Systems  │───────>│ System   │
   └─────────┘        └──────────┘        └──────────┘
                                                 │
                                                 ▼
                                         ┌──────────────┐
                                         │ Camera       │
                                         │ System       │
                                         └──────┬───────┘
                                                │
                                                ▼
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                     RENDERING PIPELINE                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                                │
                ┌───────────────┼───────────────┐
                │               │               │
                ▼               ▼               ▼
        ┌──────────────┐ ┌────────────┐ ┌──────────────────┐
        │ Pass 1:      │ │ Pass 1.5:  │ │ Pass 2:          │
        │ WORLD        │ │ GRID       │ │ UI/HUD/MENU      │
        └──────────────┘ └────────────┘ └──────────────────┘
```

## Detailed Rendering Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│  PASS 1: WORLD RENDERING (RenderingSystem::Render())            │
│  ═══════════════════════════════════════════════════════════    │
│                                                                  │
│  1. For each camera/viewport:                                   │
│     └─> RenderMultiLayerForCamera(cam)                         │
│         │                                                        │
│         ├─> Phase 1: Collect visible items                      │
│         │   ├─ Parallax layers (backgrounds/foregrounds)        │
│         │   ├─ Tiles (with frustum culling)                     │
│         │   └─ Entities (with frustum culling)                  │
│         │       └─> ❌ SKIP if type="UIElement"                 │
│         │                                                        │
│         ├─> Phase 2: Depth sort all items                       │
│         │   └─> std::sort(depth ascending)                      │
│         │                                                        │
│         └─> Phase 3: Render sorted batch                        │
│             ├─ Background parallax (-1000.0)                    │
│             ├─ Tiles (0.0 - 1000.0)                             │
│             ├─ Entities (0.0 - 1000.0)                          │
│             └─ Foreground parallax (10000.0+)                   │
│                                                                  │
│  Result: World rendered with correct depth sorting              │
│  ✅ Entities, tiles, parallax all properly layered             │
│  ❌ UI entities excluded from this pass                         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  PASS 1.5: GRID OVERLAY (GridSystem::Render())                  │
│  ═══════════════════════════════════════════════════════════    │
│                                                                  │
│  1. If grid enabled:                                            │
│     └─> For each camera/viewport:                              │
│         └─> Draw grid lines                                     │
│                                                                  │
│  Result: Debug grid overlay on world                            │
│  ✅ Grid lines visible on top of world                         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  PASS 2: UI RENDERING (UIRenderingSystem::Render())             │
│  ═══════════════════════════════════════════════════════════    │
│                                                                  │
│  1. For each camera/viewport:                                   │
│     ├─> RenderHUD(cam)                                          │
│     │   └─> Render entities where type="UIElement"              │
│     │       ├─ Health bars                                      │
│     │       ├─ Score displays                                   │
│     │       ├─ Ammo counters                                    │
│     │       └─ Other HUD elements                               │
│     │                                                            │
│     ├─> RenderInGameMenu(cam)                                   │
│     │   └─> If GameMenu::IsActive():                            │
│     │       ├─ Draw semi-transparent overlay                    │
│     │       ├─ Draw menu panel                                  │
│     │       └─ Draw buttons (Resume/Restart/Quit)               │
│     │                                                            │
│     └─> RenderDebugOverlay(cam)                                 │
│         └─> Render debug info (FPS, entity count, etc.)        │
│                                                                  │
│  Result: UI always rendered on top                              │
│  ✅ UI never hidden by world objects                           │
│  ✅ No depth sorting conflicts                                 │
│  ✅ Menu always visible when active                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │ Frame Complete   │
                    │ SDL_RenderPresent│
                    └──────────────────┘
```

## Entity Filtering Logic

```
┌────────────────────────────────────────────────────────────┐
│  Entity Processing in RenderMultiLayerForCamera()          │
└────────────────────────────────────────────────────────────┘
                          │
                          ▼
              For each entity in RenderingSystem
                          │
                          ▼
            ┌─────────────────────────────┐
            │ Has Identity_data?          │
            └─────────┬───────────────────┘
                      │
         ┌────────────┼────────────┐
         │ NO         │ YES        │
         ▼            ▼            
    ┌────────┐  ┌──────────────┐
    │ Render │  │ Check type   │
    │ in     │  └──────┬───────┘
    │ Pass 1 │         │
    └────────┘    ┌────┴────┐
                  │         │
      type=="UIElement"?    type!="UIElement"
                  │         │
                  ▼         ▼
            ┌──────────┐  ┌────────┐
            │ SKIP     │  │ Render │
            │ (Pass 2) │  │ in     │
            └──────────┘  │ Pass 1 │
                          └────────┘
```

## Depth Sorting Example

```
Pass 1: World Rendering
═══════════════════════

Depth Range: -2000.0 to +12000.0

-2000.0  ┌──────────────────────────────┐
         │ Far Background Parallax      │
-1000.0  ├──────────────────────────────┤
         │ Near Background Parallax     │
    0.0  ├──────────────────────────────┤
         │ Tiles (Layer 0)              │
  100.0  ├──────────────────────────────┤
         │ Entities (Players, NPCs)     │
  200.0  ├──────────────────────────────┤
         │ Tiles (Layer 1)              │
  500.0  ├──────────────────────────────┤
         │ Effects (Particles)          │
 1000.0  ├──────────────────────────────┤
         │ Tiles (Layer 2)              │
10000.0  ├──────────────────────────────┤
         │ Near Foreground Parallax     │
12000.0  ├──────────────────────────────┤
         │ Far Foreground Parallax      │
         └──────────────────────────────┘

UI entities (type="UIElement") are NOT in this list!

Pass 2: UI Rendering
═══════════════════════

NO DEPTH SORTING - Render order:
1. HUD entities (screen space)
2. In-game menu (if active)
3. Debug overlay

Always rendered AFTER Pass 1, on top of everything.
```

## Example Frame Composition

```
Layer Stack (bottom to top):
═══════════════════════════

┌──────────────────────────────────────┐  ▲
│ Debug Overlay (FPS, entity count)    │  │ Pass 2: UI
├──────────────────────────────────────┤  │
│ Menu (if active)                     │  │
│  ├─ Overlay                          │  │
│  └─ Buttons (Resume/Restart/Quit)    │  │
├──────────────────────────────────────┤  │
│ HUD Elements                         │  │
│  ├─ Health Bar                       │  │
│  ├─ Score                            │  │
│  └─ Ammo Counter                     │  │
╞══════════════════════════════════════╡  ▼
│ Grid Overlay (debug)                 │  ─ Pass 1.5
╞══════════════════════════════════════╡
│ Foreground Parallax                  │  ▲
├──────────────────────────────────────┤  │
│ Player Entity                        │  │
├──────────────────────────────────────┤  │
│ Tiles (Layer 2)                      │  │
├──────────────────────────────────────┤  │
│ NPC Entity                           │  │ Pass 1: World
├──────────────────────────────────────┤  │
│ Tiles (Layer 1)                      │  │
├──────────────────────────────────────┤  │
│ Item Entities                        │  │
├──────────────────────────────────────┤  │
│ Tiles (Layer 0)                      │  │
├──────────────────────────────────────┤  │
│ Background Parallax                  │  │
└──────────────────────────────────────┘  ▼
```

## Code Flow Diagram

```
World::Initialize_ECS_Systems()
│
├─> Add_ECS_System(RenderingSystem)         ← Pass 1
│   └─> requiredSignature: Identity, Position, VisualSprite, BoundingBox
│
├─> Add_ECS_System(GridSystem)              ← Pass 1.5
│   └─> requiredSignature: (none)
│
└─> Add_ECS_System(UIRenderingSystem)       ← Pass 2
    └─> requiredSignature: Identity, Position, VisualSprite, BoundingBox

GameEngine::Run() (each frame)
│
├─> World::ProcessSystems()
│   ├─> InputEventConsumeSystem::Process()
│   │   └─> Handle ESC key for menu toggle
│   │
│   ├─> ... (other systems)
│   │
│   ├─> RenderingSystem::Render()           ← Pass 1: World
│   │   └─> RenderMultiLayerForCamera()
│   │       ├─> Collect parallax layers
│   │       ├─> Collect tiles (frustum culling)
│   │       ├─> Collect entities (frustum culling)
│   │       │   └─> Skip if type="UIElement"
│   │       ├─> Sort by depth
│   │       └─> Render batch
│   │
│   ├─> GridSystem::Render()                ← Pass 1.5: Grid
│   │   └─> Draw grid lines
│   │
│   └─> UIRenderingSystem::Render()         ← Pass 2: UI
│       ├─> RenderHUD()
│       │   └─> Render entities where type="UIElement"
│       ├─> RenderInGameMenu()
│       │   └─> If GameMenu::IsActive()
│       │       ├─> Draw overlay
│       │       ├─> Draw panel
│       │       └─> Draw buttons
│       └─> RenderDebugOverlay()
│
└─> SDL_RenderPresent()
```

## Multi-Viewport Support

```
┌─────────────────────────────────────────────────────────┐
│  Split-Screen Example (2 players)                       │
└─────────────────────────────────────────────────────────┘

┌──────────────────────────┬──────────────────────────┐
│ Player 1 Viewport        │ Player 2 Viewport        │
│ (0, 0, 400, 600)         │ (400, 0, 400, 600)       │
│                          │                          │
│ ┌─ Pass 1: World ─────┐ │ ┌─ Pass 1: World ─────┐ │
│ │ Parallax, tiles,    │ │ │ Parallax, tiles,    │ │
│ │ entities            │ │ │ entities            │ │
│ └─────────────────────┘ │ └─────────────────────┘ │
│                          │                          │
│ ┌─ Pass 2: UI ────────┐ │ ┌─ Pass 2: UI ────────┐ │
│ │ HUD for P1          │ │ │ HUD for P2          │ │
│ │ Menu (if active)    │ │ │ Menu (if active)    │ │
│ └─────────────────────┘ │ └─────────────────────┘ │
└──────────────────────────┴──────────────────────────┘

Each viewport gets its own:
- Camera transform
- Frustum culling
- UI rendering

UI is rendered per-viewport, ensuring each player sees their own HUD.
```

## Benefits Summary

```
┌────────────────────────────────────────────────────────┐
│  Two-Pass Rendering Benefits                           │
└────────────────────────────────────────────────────────┘

1. GUARANTEED UI VISIBILITY
   ✅ UI always on top
   ✅ No depth sorting conflicts
   ✅ Predictable behavior

2. CLEAR SEPARATION
   ✅ World logic in Pass 1
   ✅ UI logic in Pass 2
   ✅ Easy to maintain

3. PERFORMANCE
   ✅ Single sort per frame
   ✅ Efficient frustum culling
   ✅ Minimal overhead

4. FLEXIBILITY
   ✅ Easy to add UI elements
   ✅ Multi-viewport support
   ✅ Compatible with existing systems

5. DEBUGGING
   ✅ Clear render order
   ✅ Easy to isolate issues
   ✅ Simple to add debug overlays
```

## Performance Metrics

```
Typical Frame Budget:
═══════════════════════

Total Frame Time: 16.67ms (60 FPS)

├─ Input/Physics/Logic: 5ms
├─ Pass 1 (World):      8ms
│  ├─ Frustum culling:  1ms
│  ├─ Depth sorting:    1ms
│  └─ Rendering:        6ms
├─ Pass 1.5 (Grid):     0.5ms
└─ Pass 2 (UI):         1ms
   ├─ HUD entities:     0.5ms
   └─ Menu (if active): 0.5ms

Overhead: <0.5ms
Remaining budget: 2.17ms
```

---

## Summary

The two-pass rendering system provides:
- **Guaranteed UI visibility** - Always rendered on top
- **Clean architecture** - Clear separation of world and UI
- **High performance** - Minimal overhead
- **Easy maintenance** - Simple to extend and debug

UI entities (type="UIElement") are automatically excluded from world rendering and rendered in a separate pass, ensuring they're always visible regardless of world depth sorting.
