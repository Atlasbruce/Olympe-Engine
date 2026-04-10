# Minimap State Persistence - Visual Diagrams

## Diagram 1: Memory Layout - Before Fix (BUGGY)

```
Frame 1: Initialization ✅
═══════════════════════════════════════════════════════════════

EntityPrefabRenderer
├─ m_minimapVisible = true          ✅ UI Control
├─ m_minimapSize = 0.15f            ✅ UI Control
├─ m_minimapPosition = 1            ✅ UI Control
└─ m_canvasEditor ─────────────────→ CustomCanvasEditor #1
                                    └─ m_minimapRenderer
                                       ├─ m_visible = true ✅
                                       ├─ m_size = 0.15f ✅
                                       └─ m_position = 1 ✅


Frame N: Canvas Resize ❌ BUG!
═══════════════════════════════════════════════════════════════

STEP 1: Size change detected
  totalWidth = 1024 (changed from 800)
  → Condition (line 79) evaluates TRUE

STEP 2: Create NEW adapter WITHOUT saving minimap
  m_canvasEditor = make_unique<CustomCanvasEditor>(...)
  
                    OLD adapter        NEW adapter
                    (destroyed)        (created)
                         ↓                  ↓
  EntityPrefabRenderer
  ├─ m_minimapVisible = true      ✅ PERSISTS
  ├─ m_minimapSize = 0.15f        ✅ PERSISTS
  ├─ m_minimapPosition = 1        ✅ PERSISTS
  └─ m_canvasEditor ─────────────→ CustomCanvasEditor #2  ← NEW!
                                 └─ m_minimapRenderer     ← NEW!
                                    ├─ m_visible = FALSE  ❌ DEFAULT!
                                    ├─ m_size = ???        (uninitialized)
                                    └─ m_position = ???    (uninitialized)


PROBLEM VISIBILITY:
  ✅ EntityPrefabRenderer members = still hold "true"
  ❌ CustomCanvasEditor.m_minimapRenderer = NEW instance with DEFAULT state
  
  → Two separate copies of state, NEW one has wrong values!


Frame N+1: Rendering ❌ BROKEN
═══════════════════════════════════════════════════════════════

PrefabCanvas::Render()
  │
  ├─ UpdateMinimapNodes(...)      ✅ DATA FED TO NEW MINIMAP
  │  └─ m_canvasEditor->UpdateMinimapNodes()
  │     └─ CustomCanvasEditor #2.m_minimapRenderer->UpdateNodes()
  │        └─ nodes = [6 nodes with positions]
  │
  ├─ UpdateMinimapViewport(...)   ✅ VIEWPORT FED TO NEW MINIMAP
  │  └─ m_canvasEditor->UpdateMinimapViewport()
  │     └─ CustomCanvasEditor #2.m_minimapRenderer->UpdateViewport()
  │        └─ viewport = [visible bounds]
  │
  ├─ RenderMinimap()              ❌ FAILS HERE!
  │  └─ CustomCanvasEditor #2::RenderMinimap()
  │     ├─ if (!m_minimapRenderer->IsVisible())
  │     │  └─ return;  ← EARLY EXIT!
  │     │     m_minimapRenderer->m_visible = FALSE
  │     │
  │     └─ Never reaches:
  │        m_minimapRenderer->RenderCustom()


RESULT:
  ❌ Minimap has DATA but can't render
  ❌ IsVisible() = false prevents rendering
  ❌ User sees EMPTY minimap panel
```

---

## Diagram 2: Memory Layout - After Fix (CORRECT)

```
Frame 1: Initialization ✅ (UNCHANGED)
═══════════════════════════════════════════════════════════════

[Same as before - initialization works]


Frame N: Canvas Resize ✅ FIXED!
═══════════════════════════════════════════════════════════════

STEP 1: Size change detected
  totalWidth = 1024 (changed from 800)
  → Condition (line 79) evaluates TRUE

STEP 2: SAVE minimap state BEFORE destroying old adapter ✅ NEW!
  
  OLD adapter state extraction:
  ┌──────────────────────────────────────┐
  │ CustomCanvasEditor #1                │
  │ └─ m_minimapRenderer                 │
  │    ├─ m_visible = true               │
  │    ├─ m_size = 0.15f                 │
  │    └─ m_position = TopRight (1)      │
  └──────────────────────────────────────┘
           ↓ COPY ↓
  ┌──────────────────────────────────────┐
  │ Local variables (stack)              │
  │ oldMinimapVisible = true             │
  │ oldMinimapSize = 0.15f               │
  │ oldMinimapPosition = 1               │
  └──────────────────────────────────────┘

STEP 3: Create NEW adapter
  m_canvasEditor = make_unique<CustomCanvasEditor>(...)
  
  OLD adapter        NEW adapter
  (destroyed)        (created with defaults)
       ✅                 ✅
     
  CustomCanvasEditor #2
  └─ m_minimapRenderer
     ├─ m_visible = FALSE (default)
     ├─ m_size = 0.0f (default)
     └─ m_position = 0 (default)

STEP 4: RESTORE minimap state to NEW adapter ✅ NEW!
  
  ┌──────────────────────────────────────┐
  │ Local variables (stack)              │
  │ oldMinimapVisible = true             │
  │ oldMinimapSize = 0.15f               │
  │ oldMinimapPosition = 1               │
  └──────────────────────────────────────┘
           ↓ APPLY ↓
  
  m_canvasEditor->SetMinimapVisible(oldMinimapVisible);
  m_canvasEditor->SetMinimapSize(oldMinimapSize);
  m_canvasEditor->SetMinimapPosition(oldMinimapPosition);
  
  ↓↓↓
  
  CustomCanvasEditor #2
  └─ m_minimapRenderer
     ├─ m_visible = TRUE ✅ RESTORED!
     ├─ m_size = 0.15f ✅ RESTORED!
     └─ m_position = TopRight (1) ✅ RESTORED!


Frame N+1: Rendering ✅ WORKS!
═══════════════════════════════════════════════════════════════

PrefabCanvas::Render()
  │
  ├─ UpdateMinimapNodes(...)      ✅
  ├─ UpdateMinimapViewport(...)   ✅
  │
  └─ RenderMinimap()              ✅ SUCCESS!
     └─ CustomCanvasEditor #2::RenderMinimap()
        ├─ if (!m_minimapRenderer->IsVisible())
        │  └─ m_minimapRenderer->m_visible = TRUE  ✅
        │     Condition FALSE → Continue!
        │
        └─ m_minimapRenderer->RenderCustom()  ✅ RENDERS!
           ├─ Draw background
           ├─ Draw nodes
           ├─ Draw viewport rectangle
           └─ Minimap visible in corner! ✅


RESULT:
  ✅ Minimap has DATA
  ✅ IsVisible() = true allows rendering
  ✅ User sees MINIMAP with nodes and viewport
```

---

## Diagram 3: State Flow - Complete Lifecycle

```
INITIALIZATION PHASE
═══════════════════════════════════════════════════════════════

User opens EntityPrefab
       │
       ↓
TabManager creates EntityPrefabRenderer
       │
       ↓ (Frame 1)
EntityPrefabRenderer::Render()
       ├─ RenderLayoutWithTabs()
       │  ├─ if (!m_canvasEditor)  ← TRUE on first frame
       │  │
       │  ├─ m_canvasEditor = make_unique<CustomCanvasEditor>()
       │  │  └─ CustomCanvasEditor::ctor()
       │  │     └─ m_minimapRenderer = make_unique<CanvasMinimapRenderer>()
       │  │        └─ m_visible = FALSE (default)
       │  │
       │  ├─ m_canvasEditor->SetMinimapVisible(m_minimapVisible);  ← m_minimapVisible = TRUE
       │  │  └─ CustomCanvasEditor::SetMinimapVisible()
       │  │     └─ m_minimapRenderer->SetVisible(TRUE)
       │  │        └─ m_visible = TRUE ✅
       │  │
       │  ├─ m_canvasEditor->SetMinimapSize(m_minimapSize);
       │  │  └─ m_minimapRenderer->SetSize(0.15f) ✅
       │  │
       │  └─ m_canvasEditor->SetMinimapPosition(m_minimapPosition);
       │     └─ m_minimapRenderer->SetPosition(TopRight) ✅
       │
       ├─ RenderToolbar()
       │  └─ [Not called yet on frame 1 setup]


NORMAL RENDERING PHASE (Frames 2-N)
═══════════════════════════════════════════════════════════════

EntityPrefabRenderer::Render()
       ├─ RenderLayoutWithTabs()
       │  ├─ if (!m_canvasEditor)  ← FALSE, adapter persists
       │  │
       │  ├─ else
       │  │  ├─ Check size changed  ← FALSE, no resize
       │  │  └─ m_canvasEditor persists unchanged
       │  │
       │  ├─ RenderToolbar()
       │  │  ├─ Checkbox "Minimap"  (user might toggle)
       │  │  │  └─ if (ImGui::Checkbox(..., &m_minimapVisible))
       │  │  │     └─ m_canvasEditor->SetMinimapVisible(m_minimapVisible)
       │  │  │        └─ m_minimapRenderer->m_visible = [user choice]
       │  │  │
       │  │  ├─ Slider "Size"
       │  │  │  └─ if (ImGui::DragFloat(..., &m_minimapSize))
       │  │  │     └─ m_canvasEditor->SetMinimapSize(m_minimapSize)
       │  │  │        └─ m_minimapRenderer->m_size = [user value]
       │  │  │
       │  │  └─ Combo "Position"
       │  │     └─ if (ImGui::Combo(..., &m_minimapPosition))
       │  │        └─ m_canvasEditor->SetMinimapPosition(m_minimapPosition)
       │  │           └─ m_minimapRenderer->m_position = [user choice]
       │  │
       │  ├─ m_canvas.Render()
       │  │  ├─ UpdateMinimapNodes(...)
       │  │  ├─ UpdateMinimapViewport(...)
       │  │  └─ RenderMinimap()
       │  │     └─ CustomCanvasEditor::RenderMinimap()
       │  │        ├─ if (!m_minimapRenderer->IsVisible())
       │  │        │  return;  ← Skips if user disabled minimap
       │  │        │
       │  │        └─ m_minimapRenderer->RenderCustom(...)
       │  │           └─ Renders overlay ✅


RESIZE PHASE (Frame N when window resizes)
═══════════════════════════════════════════════════════════════

Window resize event
       │
       ↓ (Frame N)
EntityPrefabRenderer::Render()
       ├─ RenderLayoutWithTabs()
       │  ├─ if (!m_canvasEditor)  ← FALSE
       │  │
       │  ├─ else if (size changed)  ← TRUE! NEW!
       │  │  │
       │  │  ├─ SAVE state from OLD adapter
       │  │  │  ├─ oldMinimapVisible = m_canvasEditor->IsMinimapVisible()
       │  │  │  │  └─ Returns m_minimapRenderer->m_visible (e.g., TRUE)
       │  │  │  │
       │  │  │  ├─ oldMinimapSize = m_canvasEditor->GetMinimapSize()
       │  │  │  │  └─ Returns m_minimapRenderer->m_size (e.g., 0.15f)
       │  │  │  │
       │  │  │  └─ oldMinimapPosition = m_canvasEditor->GetMinimapPosition()
       │  │  │     └─ Returns m_minimapRenderer->m_position (e.g., 1)
       │  │  │
       │  │  ├─ OLD adapter deleted ✅
       │  │  │  └─ CustomCanvasEditor #1 destroyed
       │  │  │     └─ m_minimapRenderer destroyed
       │  │  │
       │  │  ├─ CREATE NEW adapter
       │  │  │  └─ m_canvasEditor = make_unique<CustomCanvasEditor>(...)
       │  │  │     └─ CustomCanvasEditor #2 created
       │  │  │        └─ m_minimapRenderer created with DEFAULTS
       │  │  │           ├─ m_visible = FALSE
       │  │  │           ├─ m_size = 0.0f
       │  │  │           └─ m_position = 0
       │  │  │
       │  │  ├─ Restore zoom/pan
       │  │  │  └─ m_canvasEditor->SetPan(oldPan) ✅
       │  │  │
       │  │  └─ RESTORE minimap state to NEW adapter ✅ CRITICAL!
       │  │     ├─ m_canvasEditor->SetMinimapVisible(oldMinimapVisible)
       │  │     │  └─ m_minimapRenderer->m_visible = TRUE
       │  │     │
       │  │     ├─ m_canvasEditor->SetMinimapSize(oldMinimapSize)
       │  │     │  └─ m_minimapRenderer->m_size = 0.15f
       │  │     │
       │  │     └─ m_canvasEditor->SetMinimapPosition(oldMinimapPosition)
       │  │        └─ m_minimapRenderer->m_position = 1


RENDERING AFTER RESIZE (Frame N+1)
═══════════════════════════════════════════════════════════════

EntityPrefabRenderer::Render()
       ├─ RenderLayoutWithTabs()
       │  ├─ if (!m_canvasEditor)  ← FALSE
       │  │
       │  ├─ else if (size changed)  ← FALSE, no more resize
       │  │  └─ m_canvasEditor unchanged (the restored one)
       │  │
       │  ├─ m_canvas.Render()
       │  │  ├─ UpdateMinimapNodes(...)
       │  │  ├─ UpdateMinimapViewport(...)
       │  │  └─ RenderMinimap()
       │  │     ├─ if (!m_minimapRenderer->IsVisible())  ← FALSE
       │  │     │  return;
       │  │     │
       │  │     └─ m_minimapRenderer->RenderCustom(...)  ✅ RENDERS!
       │  │        └─ Minimap displays in corner ✅
```

---

## Diagram 4: Object Lifetime - Before vs After

```
BEFORE FIX (BROKEN)
═══════════════════════════════════════════════════════════════

Time ──────────────────────────────────────────────────────────────→

Frame 1: CustomCanvasEditor #1 created
│        │
│        ├─ m_minimapRenderer created
│        │  └─ SetMinimapVisible(true) called
│        │     └─ m_visible = TRUE
│        │
│        └─ (exists until resize)
│
├─────── Frames 2-N: Normal rendering
│        │ m_minimapRenderer persists
│        │ Minimap renders: IsVisible() = TRUE ✅
│        │
├─────── Frame N: Window resize
│        │
│        ├─ NEW CustomCanvasEditor #2 created
│        │  │
│        │  ├─ NEW m_minimapRenderer created
│        │  │  └─ m_visible = FALSE (default) ❌
│        │  │
│        │  └─ NO SetMinimapVisible() called ❌
│        │
│        └─ CustomCanvasEditor #1 + old m_minimapRenderer destroyed
│
├─────── Frames N+1 onwards: Broken rendering
│        │ m_minimapRenderer persists (new one)
│        │ Minimap fails: IsVisible() = FALSE ❌
│        │ User sees EMPTY minimap
│        │
│        └─ (remains invisible until next action)


AFTER FIX (CORRECT)
═══════════════════════════════════════════════════════════════

Time ──────────────────────────────────────────────────────────────→

Frame 1: CustomCanvasEditor #1 created
│        │
│        ├─ m_minimapRenderer created
│        │  └─ SetMinimapVisible(true) called
│        │     └─ m_visible = TRUE
│        │
│        └─ (exists until resize)
│
├─────── Frames 2-N: Normal rendering
│        │ m_minimapRenderer persists
│        │ Minimap renders: IsVisible() = TRUE ✅
│        │
├─────── Frame N: Window resize
│        │
│        ├─ SAVE state before destruction ✅ NEW
│        │  ├─ oldMinimapVisible = TRUE
│        │  ├─ oldMinimapSize = 0.15f
│        │  └─ oldMinimapPosition = 1
│        │
│        ├─ NEW CustomCanvasEditor #2 created
│        │  │
│        │  ├─ NEW m_minimapRenderer created
│        │  │  └─ m_visible = FALSE (default, temporary)
│        │  │
│        │  └─ RESTORE state immediately ✅ NEW
│        │     ├─ SetMinimapVisible(oldMinimapVisible)
│        │     │  └─ m_visible = TRUE ✅
│        │     ├─ SetMinimapSize(oldMinimapSize)
│        │     │  └─ m_size = 0.15f ✅
│        │     └─ SetMinimapPosition(oldMinimapPosition)
│        │        └─ m_position = 1 ✅
│        │
│        └─ CustomCanvasEditor #1 + old m_minimapRenderer destroyed
│
├─────── Frames N+1 onwards: Working rendering ✅
│        │ m_minimapRenderer persists (new one, with restored state)
│        │ Minimap renders: IsVisible() = TRUE ✅
│        │ User sees MINIMAP in correct corner with correct size
│        │
│        └─ (remains visible, toolbar controls work)
```

---

## Diagram 5: Code Path Comparison

```
RENDERING WITH OLD ADAPTER - WORKING
═════════════════════════════════════════════════════════════════

PrefabCanvas::Render()
       │
       ├─ m_canvasEditor points to CustomCanvasEditor #1
       │  ├─ m_minimapRenderer → original instance
       │  │  └─ m_visible = true (from initialization)
       │  │
       │  ├─ UpdateMinimapNodes(nodeData)
       │  │  └─ CustomCanvasEditor::UpdateMinimapNodes()
       │  │     └─ m_minimapRenderer->UpdateNodes(nodeData)
       │  │        └─ [Node data stored in renderer]
       │  │
       │  ├─ UpdateMinimapViewport(viewportData)
       │  │  └─ CustomCanvasEditor::UpdateMinimapViewport()
       │  │     └─ m_minimapRenderer->UpdateViewport(viewportData)
       │  │        └─ [Viewport bounds stored in renderer]
       │  │
       │  └─ RenderMinimap()
       │     └─ CustomCanvasEditor::RenderMinimap()
       │        ├─ if (!m_minimapRenderer || !m_minimapRenderer->IsVisible())
       │        │  └─ m_visible = true → condition FALSE
       │        │
       │        └─ m_minimapRenderer->RenderCustom(...)
       │           └─ ✅ Renders minimap with data


RENDERING WITH NEW ADAPTER - BROKEN (BEFORE FIX)
═════════════════════════════════════════════════════════════════

PrefabCanvas::Render()
       │
       ├─ m_canvasEditor points to CustomCanvasEditor #2 (NEW!)
       │  ├─ m_minimapRenderer → NEW instance
       │  │  └─ m_visible = false (DEFAULT - NOT restored!)
       │  │
       │  ├─ UpdateMinimapNodes(nodeData)
       │  │  └─ CustomCanvasEditor::UpdateMinimapNodes()
       │  │     └─ m_minimapRenderer->UpdateNodes(nodeData)
       │  │        └─ [Node data stored in NEW renderer]
       │  │
       │  ├─ UpdateMinimapViewport(viewportData)
       │  │  └─ CustomCanvasEditor::UpdateMinimapViewport()
       │  │     └─ m_minimapRenderer->UpdateViewport(viewportData)
       │  │        └─ [Viewport bounds stored in NEW renderer]
       │  │
       │  └─ RenderMinimap()
       │     └─ CustomCanvasEditor::RenderMinimap()
       │        ├─ if (!m_minimapRenderer || !m_minimapRenderer->IsVisible())
       │        │  └─ m_visible = false → condition TRUE
       │        │
       │        └─ return;  ❌ EARLY EXIT! Data not rendered!
       │           [RenderCustom() never called]


RENDERING WITH NEW ADAPTER - FIXED (AFTER FIX)
═════════════════════════════════════════════════════════════════

PrefabCanvas::Render()
       │
       ├─ m_canvasEditor points to CustomCanvasEditor #2 (NEW!)
       │  ├─ m_minimapRenderer → NEW instance
       │  │  └─ m_visible = true (RESTORED from old state!)
       │  │
       │  ├─ UpdateMinimapNodes(nodeData)
       │  │  └─ CustomCanvasEditor::UpdateMinimapNodes()
       │  │     └─ m_minimapRenderer->UpdateNodes(nodeData)
       │  │        └─ [Node data stored in NEW renderer]
       │  │
       │  ├─ UpdateMinimapViewport(viewportData)
       │  │  └─ CustomCanvasEditor::UpdateMinimapViewport()
       │  │     └─ m_minimapRenderer->UpdateViewport(viewportData)
       │  │        └─ [Viewport bounds stored in NEW renderer]
       │  │
       │  └─ RenderMinimap()
       │     └─ CustomCanvasEditor::RenderMinimap()
       │        ├─ if (!m_minimapRenderer || !m_minimapRenderer->IsVisible())
       │        │  └─ m_visible = true → condition FALSE
       │        │
       │        └─ m_minimapRenderer->RenderCustom(...)
       │           └─ ✅ Renders minimap with data
```

---

## Diagram 6: Minimap Visibility State Machine

```
┌─────────────────────────────────────────────────────────────────┐
│                    MINIMAP STATE MACHINE                        │
└─────────────────────────────────────────────────────────────────┘

                           [START]
                             │
                             ↓
                  ┌─────────────────────┐
                  │  Initialize Frame 1 │
                  ├─────────────────────┤
                  │ Create CustomCanvas │
                  │ Adapter #1          │
                  │ m_visible = FALSE   │
                  └──────────┬──────────┘
                             │
                             ↓
                  ┌─────────────────────┐
                  │  SetMinimapVisible  │
                  │        (true)       │
                  ├─────────────────────┤
                  │ m_visible = TRUE    │
                  │ ✅ State: VISIBLE   │
                  └──────────┬──────────┘
                             │
                ┌────────────┴────────────┐
                │                        │
                ↓                        ↓
    ┌─────────────────────┐  ┌──────────────────────┐
    │ Frames 2-N: Normal  │  │ User toggles OFF     │
    │ Rendering           │  │ Checkbox             │
    ├─────────────────────┤  ├──────────────────────┤
    │ m_visible unchanged │  │ m_visible = FALSE    │
    │ ✅ RENDERS          │  │ ❌ State: HIDDEN     │
    └──────────┬──────────┘  └─────────────────────┘
               │
               │
        ┌──────┴──────┐
        │ Size Change │ (Window/Canvas resized)
        │ Detected!   │
        └──────┬──────┘
               │
     ┌─────────┴──────────┐
     │                    │
     ↓ BEFORE FIX         ↓ AFTER FIX
  ❌ BROKEN           ✅ CORRECT
  
  ┌─────────────────┐   ┌──────────────────┐
  │ Create Adapter  │   │ SAVE state:      │
  │ Adapter #2      │   │ oldMinimapVis=T  │
  ├─────────────────┤   │ oldMinimapSiz=…  │
  │ m_visible=FALSE │   │ oldMinimapPos=…  │
  │ (default!)      │   │                  │
  │ ❌ State:HIDDEN │   │ Create Adapter   │
  │                 │   │ Adapter #2       │
  │ RenderMinimap() │   ├──────────────────┤
  │ → Early return  │   │ m_visible=FALSE  │
  │ ❌ NO RENDER    │   │ (temporary)      │
  └─────────────────┘   │                  │
                        │ RESTORE state:   │
                        │ m_visible=TRUE ✅│
                        │ m_size restored  │
                        │ m_position restor│
                        │ ✅ State:VISIBLE │
                        │                  │
                        │ RenderMinimap()  │
                        │ → Renders! ✅    │
                        └──────────────────┘
               │
               ↓ Frames N+1+
    ┌─────────────────────┐
    │ Continue Rendering  │
    ├─────────────────────┤
    │ With restored state │
    │ ✅ VISIBLE (fixed)  │
    └─────────────────────┘
```

---

## Key Takeaway

The fix implements a **two-checkpoint save-restore pattern**:

1. **Before Destruction**: Extract all state from dying object
2. **After Creation**: Immediately restore all state to new object
3. **Result**: Seamless state preservation across object recreation

This ensures the minimap remains visible and maintains its configuration even when the canvas adapter is recreated during window resize events.
