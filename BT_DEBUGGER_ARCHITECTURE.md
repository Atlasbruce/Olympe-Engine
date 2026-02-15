# BT Debugger Architecture Diagram

## Window Structure

```
┌────────────────────────────────────────────────────────────┐
│  Main Olympe Engine Window (SDL3)                         │
│  ┌──────────────────────────────────────────────────────┐ │
│  │  Main ImGui Context                                  │ │
│  │  - Game UI                                          │ │
│  │  - Panels                                           │ │
│  │  - Menus                                            │ │
│  └──────────────────────────────────────────────────────┘ │
│                                                            │
│  [F10 Pressed] ─────────────────────────────────────────► │
└────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────┐
│  BT Debugger Window (Separate SDL3 Window)                 │
│  ┌──────────────────────────────────────────────────────┐ │
│  │  Separate ImGui Context                              │ │
│  │  ┌────────────┬─────────────────┬─────────────────┐ │ │
│  │  │ Entity     │  Node Graph     │   Inspector     │ │ │
│  │  │ List       │  (ImNodes)      │   (Blackboard)  │ │ │
│  │  │            │                 │                 │ │ │
│  │  │ - Entity 1 │   ┌──────┐     │   - Variables   │ │ │
│  │  │ - Entity 2 │   │ Root │     │   - Execution   │ │ │
│  │  │ - Entity 3 │   └──┬───┘     │   - Status      │ │ │
│  │  │            │      │          │                 │ │ │
│  │  │ [Filters]  │   ┌─┴──┐       │   [Log]         │ │ │
│  │  │ [Sort]     │   │ Sel│       │                 │ │ │
│  │  └────────────┴───┴────┴───────┴─────────────────┘ │ │
│  └──────────────────────────────────────────────────────┘ │
│                                                            │
│  Can be moved to second monitor ───────────────────────► ◯│
└────────────────────────────────────────────────────────────┘
```

## Context Switching Flow

```
Main Window Events                     Debugger Window Events
       │                                      │
       ▼                                      ▼
┌──────────────────┐                  ┌──────────────────┐
│  SDL_AppEvent()  │                  │  ProcessEvent()  │
└────────┬─────────┘                  └────────┬─────────┘
         │                                     │
         │                                     ▼
         │                            ┌────────────────┐
         │                            │  Check         │
         │                            │  windowID      │
         │                            └────────┬───────┘
         │                                     │
         │                                     ▼
         │                            ImGui::SetCurrentContext
         │                            (m_separateImGuiContext)
         │                                     │
         │                                     ▼
         │                            ImGui_ImplSDL3_ProcessEvent
         │
         ▼
ImGui_ImplSDL3_ProcessEvent
(Main Context - implicit)
         │
         ▼
Game Input Systems
```

## Render Loop Flow

```
SDL_AppIterate()
       │
       ├─────────────────────────────┬─────────────────────────────┐
       │                             │                             │
       ▼                             ▼                             ▼
┌──────────────────┐        ┌─────────────────┐        ┌──────────────────┐
│  Main Window     │        │  BT Debugger    │        │  Present Both    │
│  Rendering       │        │  Rendering      │        │  Windows         │
└────────┬─────────┘        └────────┬────────┘        └──────────────────┘
         │                            │
         │                            ▼
         │                   if (!m_windowCreated)
         │                            │
         │                            ▼
         │                       return early
         │                            │
         │                            ▼
         │                   ImGui::SetCurrentContext
         │                   (m_separateImGuiContext)
         │                            │
         │                            ▼
         │                   ImGui_ImplSDLRenderer3_NewFrame()
         │                   ImGui_ImplSDL3_NewFrame()
         │                   ImGui::NewFrame()
         │                            │
         │                            ▼
         │                   RenderInSeparateWindow()
         │                            │
         │                            ├─ Entity List Panel
         │                            ├─ Node Graph Panel (ImNodes)
         │                            └─ Inspector Panel
         │                            │
         │                            ▼
         │                   ImGui::Render()
         │                   SDL_RenderClear(m_separateRenderer)
         │                   ImGui_ImplSDLRenderer3_RenderDrawData()
         │                   SDL_RenderPresent(m_separateRenderer)
         │
         ▼
Continue main render...
```

## Lifecycle Flow

```
Engine Startup
       │
       ▼
┌────────────────────────────┐
│  g_btDebugWindow = new     │
│  BehaviorTreeDebugWindow() │
└──────────┬─────────────────┘
           │
           ▼
┌────────────────────────────┐
│  Initialize()              │
│  - Create ImNodes context  │
│  - Set m_isInitialized     │
│  - NO window created yet   │
└──────────┬─────────────────┘
           │
           ▼
   ┌───────────────┐
   │ Running...    │
   └───────┬───────┘
           │
           ▼
   [User Presses F10]
           │
           ▼
┌────────────────────────────┐
│  ToggleVisibility()        │
│  m_isVisible = true        │
└──────────┬─────────────────┘
           │
           ▼
┌────────────────────────────┐
│  CreateSeparateWindow()    │
│  - SDL_CreateWindowAndRenderer │
│  - ImGui::CreateContext    │
│  - ImGui_ImplSDL3_Init     │
│  - m_windowCreated = true  │
└──────────┬─────────────────┘
           │
           ▼
   ┌───────────────────┐
   │ Debugger Active   │
   │ - ProcessEvent()  │
   │ - Render() loop   │
   └─────────┬─────────┘
             │
             ▼
   [User Presses F10 or X]
             │
             ▼
┌────────────────────────────┐
│  ToggleVisibility()        │
│  m_isVisible = false       │
└──────────┬─────────────────┘
           │
           ▼
┌────────────────────────────┐
│  DestroySeparateWindow()   │
│  - ImGui_ImplSDLRenderer3_Shutdown │
│  - ImGui_ImplSDL3_Shutdown │
│  - ImGui::DestroyContext   │
│  - SDL_DestroyRenderer     │
│  - SDL_DestroyWindow       │
│  - m_windowCreated = false │
└──────────┬─────────────────┘
           │
           ▼
   ┌───────────────┐
   │ Back to       │
   │ Running...    │
   └───────┬───────┘
           │
           ▼
  Engine Shutdown
           │
           ▼
┌────────────────────────────┐
│  Shutdown()                │
│  - DestroySeparateWindow() │
│  - ImNodes::DestroyContext │
└──────────┬─────────────────┘
           │
           ▼
┌────────────────────────────┐
│  delete g_btDebugWindow    │
└────────────────────────────┘
```

## Layout Algorithm Flow

```
ComputeLayout(tree, spacingX, spacingY, zoom)
       │
       ├──────────────────────────────────────────────┐
       │                                              │
       ▼                                              ▼
┌──────────────────┐                        ┌──────────────────┐
│  Phase 1:        │                        │  Phase 2:        │
│  AssignLayers    │                        │  InitialOrdering │
│  (BFS)           │───────────────────────►│  (BFS order)     │
└──────────────────┘                        └────────┬─────────┘
                                                     │
                                                     ▼
                                            ┌──────────────────┐
                                            │  Phase 3:        │
                                            │  ReduceCrossings │
                                            │  (20 passes)     │◄─┐
                                            └────────┬─────────┘  │
                                                     │             │
                                                     ├─────────────┘
                                                     │ Alternating
                                                     │ Forward/Back
                                                     ▼
                                            ┌──────────────────┐
                                            │  Phase 4:        │
                                            │  Buchheim-Walker │
                                            │  Layout          │
                                            └────────┬─────────┘
                                                     │
                                                     ▼
                                            ┌──────────────────┐
                                            │  Phase 5:        │
                                            │  Collision       │
                                            │  Resolution      │◄─┐
                                            │  (padding 2.5,   │  │
                                            │   30 iterations) │  │
                                            └────────┬─────────┘  │
                                                     │             │
                                                     ├─────────────┘
                                                     │ Until no
                                                     │ collisions
                                                     ▼
                                            ┌──────────────────┐
                                            │  Convert to      │
                                            │  World Coords    │
                                            │  (apply spacing  │
                                            │   and zoom)      │
                                            └────────┬─────────┘
                                                     │
                                                     ▼
                                            Return layouts
```

## Memory Layout

```
BehaviorTreeDebugWindow
┌─────────────────────────────────────────┐
│  Main Data                              │
│  - m_entities (vector)                  │
│  - m_filteredEntities (vector)          │
│  - m_executionLog (deque)               │
│  - m_currentLayout (vector)             │
│                                         │
│  Window State                           │
│  - m_isVisible (bool)                   │
│  - m_isInitialized (bool)               │
│  - m_windowCreated (bool)               │
│                                         │
│  SDL3 Resources (HEAP)                  │
│  ┌─────────────────────────────────┐   │
│  │ m_separateWindow*               │   │
│  │   ├─> SDL_Window (OS managed)   │   │
│  │   │                             │   │
│  │ m_separateRenderer*             │   │
│  │   ├─> SDL_Renderer (GPU)        │   │
│  │   │                             │   │
│  │ m_separateImGuiContext*         │   │
│  │   └─> ImGuiContext              │   │
│  │       ├─ ImGui State            │   │
│  │       ├─ ImNodes State          │   │
│  │       └─ Backend State          │   │
│  └─────────────────────────────────┘   │
│                                         │
│  Layout Engine                          │
│  - m_layoutEngine (BTGraphLayoutEngine) │
│                                         │
└─────────────────────────────────────────┘

Cleanup Order (DestroySeparateWindow):
1. Set current context (m_separateImGuiContext)
2. Shutdown ImGui renderer backend
3. Shutdown ImGui SDL3 backend
4. Destroy ImGui context
5. Destroy SDL renderer
6. Destroy SDL window
7. Set pointers to nullptr
8. Set m_windowCreated = false
```

## Key Design Decisions

### Why Separate ImGui Context?
```
Problem: Each SDL window needs its own ImGui state
Solution: Create separate ImGuiContext per window

Benefits:
✅ Independent UI state
✅ No conflicts between windows
✅ Can close/reopen without affecting main window

Requirement:
⚠️  MUST call ImGui::SetCurrentContext() before ANY ImGui calls
```

### Why NOT ImGui Viewports?
```
Problem: SDL3 backend doesn't support ConfigFlags_ViewportsEnable
Solution: Use native SDL3 window instead

Limitations of ImGui Viewports in SDL3:
❌ ConfigFlags_ViewportsEnable not available
❌ UpdatePlatformWindows() not available
❌ RenderPlatformWindowsDefault() not available

Benefits of Native SDL3:
✅ Full control over window
✅ Works with current SDL3 backend
✅ OS-level window management
✅ Can move to second monitor
```

### Why Deferred Window Creation?
```
Problem: Creating window at startup wastes resources
Solution: Create window only when F10 pressed

Benefits:
✅ No overhead when not using debugger
✅ Faster engine startup
✅ Resources allocated on-demand

Pattern:
Initialize() → Set up infrastructure (ImNodes context)
F10 pressed → CreateSeparateWindow() → Actually create window
F10 again → DestroySeparateWindow() → Free resources
```

## Performance Characteristics

```
Operation                  Time        Notes
─────────────────────────────────────────────────────────
CreateSeparateWindow()     5-10ms      One-time cost
DestroySeparateWindow()    5-10ms      One-time cost
ProcessEvent()             < 0.1ms     Per event
Render() [empty tree]      1-2ms       Per frame
Render() [10 nodes]        2-3ms       Per frame
ComputeLayout() [10 nodes] < 5ms       On entity change
ComputeLayout() [50 nodes] 15-25ms     On entity change
CountEdgeCrossings()       < 5ms       Debug only

Memory:
─────────────────────────────────────────────────────────
Window closed              0 bytes     No overhead
Window open (no entity)    ~50KB       SDL + ImGui state
Window open (10 entities)  ~500KB      + Entity data
```

---

**Legend:**
- `┌─┐ └─┘` = Boxes/containers
- `│ ─ ┬ ┴` = Connections
- `◯` = External resource
- `▼ ► ◄` = Flow direction
- `─►` = Pointer/reference
