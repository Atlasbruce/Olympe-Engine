# EntityPrefabRenderer.cpp - Code Changes Summary

## File Path
```
Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp
```

## Location of Change
```
Method: EntityPrefabRenderer::RenderLayoutWithTabs()
Lines: 82-102 (in the else if block for canvas resize detection)
```

---

## BEFORE - Broken Code ❌

```cpp
73:     else
74:     {
75:         // Update canvas position/size (may change on window resize)
76:         // We can't directly update these, but CustomCanvasEditor stores them as const members
77:         // For now, we'll reinitialize if size changes significantly
78:         ImVec2 currentSize = ImGui::GetContentRegionAvail();
79:         if (currentSize.x != m_canvasEditor->GetCanvasSize().x || 
80:             currentSize.y != m_canvasEditor->GetCanvasSize().y)
81:         {
82:             // CRITICAL FIX: Save state BEFORE destroying old adapter!
83:             // If we call GetCanvasOffset() after std::make_unique, m_canvasEditor in PrefabCanvas
84:             // still points to the destroyed old adapter, causing a use-after-free crash.
85:             float oldZoom = m_canvasEditor->GetZoom();
86:             ImVec2 oldPan = m_canvasEditor->GetPan();
87: 
88:             // Size changed, reinitialize - old adapter is destroyed here
89:             m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
90:                 "PrefabCanvas",
91:                 canvasScreenPos,
92:                 currentSize,
93:                 oldZoom,  // preserve zoom (already extracted)
94:                 0.1f, 3.0f
95:             );
96: 
97:             // Restore pan to NEW adapter
98:             m_canvasEditor->SetPan(oldPan);
99: 
100:            // CRITICAL: Update reference immediately after creation!
101:             m_canvas.SetCanvasEditor(m_canvasEditor.get());
102:         }
103:     }
```

**Problems**:
- ❌ Line 85-86: Saves zoom and pan
- ❌ Line 89-95: Creates NEW adapter
- ❌ Line 98: Restores pan
- ❌ **MISSING**: NO minimap state saving/restoring!
- ❌ **Result**: m_minimapRenderer defaults to m_visible = false

---

## AFTER - Fixed Code ✅

```cpp
73:     else
74:     {
75:         // Update canvas position/size (may change on window resize)
76:         // We can't directly update these, but CustomCanvasEditor stores them as const members
77:         // For now, we'll reinitialize if size changes significantly
78:         ImVec2 currentSize = ImGui::GetContentRegionAvail();
79:         if (currentSize.x != m_canvasEditor->GetCanvasSize().x || 
80:             currentSize.y != m_canvasEditor->GetCanvasSize().y)
81:         {
82:             // CRITICAL FIX: Save state BEFORE destroying old adapter!
83:             // If we call GetCanvasOffset() after std::make_unique, m_canvasEditor in PrefabCanvas
84:             // still points to the destroyed old adapter, causing a use-after-free crash.
85:             float oldZoom = m_canvasEditor->GetZoom();
86:             ImVec2 oldPan = m_canvasEditor->GetPan();
87:             
88:             // PHASE 37 FIX: Save minimap state before recreating adapter
89:             bool oldMinimapVisible = m_canvasEditor->IsMinimapVisible();
90:             float oldMinimapSize = m_canvasEditor->GetMinimapSize();
91:             int oldMinimapPosition = m_canvasEditor->GetMinimapPosition();
92:
93:             // Size changed, reinitialize - old adapter is destroyed here
94:             m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
95:                 "PrefabCanvas",
96:                 canvasScreenPos,
97:                 currentSize,
98:                 oldZoom,  // preserve zoom (already extracted)
99:                 0.1f, 3.0f
100:            );
101:
102:            // Restore pan to NEW adapter
103:            m_canvasEditor->SetPan(oldPan);
104:            
105:            // PHASE 37 FIX: Restore minimap state to NEW adapter
106:            m_canvasEditor->SetMinimapVisible(oldMinimapVisible);
107:            m_canvasEditor->SetMinimapSize(oldMinimapSize);
108:            m_canvasEditor->SetMinimapPosition(oldMinimapPosition);
109:
110:            // CRITICAL: Update reference immediately after creation!
111:            m_canvas.SetCanvasEditor(m_canvasEditor.get());
112:        }
113:    }
```

**Fixes**:
- ✅ Line 85-86: Saves zoom and pan (unchanged)
- ✅ **Line 89-91**: NEW! Saves minimap state
  - `oldMinimapVisible = IsMinimapVisible()` - Get current visibility from old adapter
  - `oldMinimapSize = GetMinimapSize()` - Get current size from old adapter
  - `oldMinimapPosition = GetMinimapPosition()` - Get current position from old adapter
- ✅ Line 94-100: Creates NEW adapter (unchanged)
- ✅ Line 103: Restores pan (unchanged)
- ✅ **Line 106-108**: NEW! Restores minimap state
  - `SetMinimapVisible(oldMinimapVisible)` - Apply old visibility to new adapter
  - `SetMinimapSize(oldMinimapSize)` - Apply old size to new adapter
  - `SetMinimapPosition(oldMinimapPosition)` - Apply old position to new adapter
- ✅ **Result**: m_minimapRenderer inherits previous state

---

## Diff View

```diff
73:     else
74:     {
75:         ImVec2 currentSize = ImGui::GetContentRegionAvail();
76:         if (currentSize.x != m_canvasEditor->GetCanvasSize().x || 
77:             currentSize.y != m_canvasEditor->GetCanvasSize().y)
78:         {
79:             float oldZoom = m_canvasEditor->GetZoom();
80:             ImVec2 oldPan = m_canvasEditor->GetPan();
81:+
82:+            // PHASE 37 FIX: Save minimap state before recreating adapter
83:+            bool oldMinimapVisible = m_canvasEditor->IsMinimapVisible();
84:+            float oldMinimapSize = m_canvasEditor->GetMinimapSize();
85:+            int oldMinimapPosition = m_canvasEditor->GetMinimapPosition();
86:+
87:             m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(
88:                 "PrefabCanvas",
89:                 canvasScreenPos,
90:                 currentSize,
91:                 oldZoom,
92:                 0.1f, 3.0f
93:             );
94:
95:             m_canvasEditor->SetPan(oldPan);
96:+
97:+            // PHASE 37 FIX: Restore minimap state to NEW adapter
98:+            m_canvasEditor->SetMinimapVisible(oldMinimapVisible);
99:+            m_canvasEditor->SetMinimapSize(oldMinimapSize);
100:+            m_canvasEditor->SetMinimapPosition(oldMinimapPosition);
101:
102:            m_canvas.SetCanvasEditor(m_canvasEditor.get());
103:        }
104:    }
```

---

## Method Signatures Used

### From CustomCanvasEditor (Source/BlueprintEditor/Utilities/CustomCanvasEditor.h)

```cpp
// Lines 288: Get minimap visibility
virtual bool IsMinimapVisible() const override;

// Line 300: Get minimap size ratio
virtual float GetMinimapSize() const override;

// Line 310-311: Get minimap position
virtual int GetMinimapPosition() const override;

// Line 282: Set minimap visibility
virtual void SetMinimapVisible(bool enabled) override;

// Line 294: Set minimap size ratio
virtual void SetMinimapSize(float scale) override;

// Line 306: Set minimap position
virtual void SetMinimapPosition(int position) override;
```

### From ICanvasEditor (Source/BlueprintEditor/Utilities/ICanvasEditor.h)

These are abstract methods that CustomCanvasEditor implements. All getters and setters are part of the standard interface.

---

## State Variables in EntityPrefabRenderer

### Persistent Members (from EntityPrefabRenderer.h lines 58-60)

```cpp
// Phase 37 — Minimap toolbar controls
bool m_minimapVisible = true;
float m_minimapSize = 0.15f;
int m_minimapPosition = 1;  // 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight
```

These members:
- Are initialized with defaults
- Persist across canvas adapter recreation
- Are synced from RenderToolbar() UI controls (lines 349-370)
- Are used to initialize first adapter (lines 68-70)
- Are now also used to restore state after resize (lines 106-108) ✅ **NEW**

---

## Execution Flow - How It Works

### Frame N: Canvas Resizes

```cpp
// Line 89-91: SAVE minimap state from OLD adapter
bool oldMinimapVisible = m_canvasEditor->IsMinimapVisible();
  // Calls: CustomCanvasEditor::IsMinimapVisible()
  // Returns: m_minimapRenderer->m_visible
  // Example result: true

float oldMinimapSize = m_canvasEditor->GetMinimapSize();
  // Calls: CustomCanvasEditor::GetMinimapSize()
  // Returns: m_minimapRenderer->m_size
  // Example result: 0.15f

int oldMinimapPosition = m_canvasEditor->GetMinimapPosition();
  // Calls: CustomCanvasEditor::GetMinimapPosition()
  // Returns: static_cast<int>(m_minimapRenderer->m_position)
  // Example result: 1 (TopRight)

// Line 94-100: CREATE NEW adapter
m_canvasEditor = std::make_unique<Olympe::CustomCanvasEditor>(...);
  // CustomCanvasEditor::ctor() creates NEW m_minimapRenderer
  // CanvasMinimapRenderer::ctor() sets m_visible = false (DEFAULT)
  
// Line 106-108: RESTORE minimap state to NEW adapter
m_canvasEditor->SetMinimapVisible(oldMinimapVisible);  // true
  // Calls: CustomCanvasEditor::SetMinimapVisible(true)
  // Calls: m_minimapRenderer->SetVisible(true)
  // Result: m_minimapRenderer->m_visible = true ✅

m_canvasEditor->SetMinimapSize(oldMinimapSize);  // 0.15f
  // Calls: CustomCanvasEditor::SetMinimapSize(0.15f)
  // Calls: m_minimapRenderer->SetSize(0.15f)
  // Result: m_minimapRenderer->m_size = 0.15f ✅

m_canvasEditor->SetMinimapPosition(oldMinimapPosition);  // 1
  // Calls: CustomCanvasEditor::SetMinimapPosition(1)
  // Calls: m_minimapRenderer->SetPosition(TopRight)
  // Result: m_minimapRenderer->m_position = TopRight ✅
```

### Frame N+1: Rendering Works Again ✅

```cpp
PrefabCanvas::Render()
  m_canvasEditor->RenderMinimap()
    CustomCanvasEditor::RenderMinimap()
      if (!m_minimapRenderer || !m_minimapRenderer->IsVisible())
        // m_minimapRenderer->m_visible = true (restored!)
        // Condition is FALSE → continue
      
      m_minimapRenderer->RenderCustom(...)
        // ✅ Renders minimap overlay
```

---

## Testing Scenarios

### Scenario 1: Initial Load
- User opens EntityPrefab
- Minimap appears ✅
- Toolbar shows "Minimap: ☑ Visible" ✅

### Scenario 2: Resize Window
- User resizes window (width changes)
- Canvas width recalculated
- Resize condition (line 79-80) triggers ✅
- State saved and restored (lines 89-91, 106-108) ✅
- Frame N+1: Minimap still visible ✅

### Scenario 3: Toggle Visibility
- User unchecks "Minimap" checkbox
- RenderToolbar() → m_minimapVisible = false
- SetMinimapVisible(false) → m_visible = false
- User resizes window
- Resize detected (line 79-80) ✅
- oldMinimapVisible = false (saved correctly)
- After restoration: m_visible = false (correct) ✅
- Minimap remains hidden as expected ✅

### Scenario 4: Change Size
- User moves "Size" slider to 0.25f
- RenderToolbar() → m_minimapSize = 0.25f
- SetMinimapSize(0.25f) → m_size = 0.25f
- User resizes window
- Resize detected (line 79-80) ✅
- oldMinimapSize = 0.25f (saved correctly)
- After restoration: m_size = 0.25f (correct) ✅
- Minimap maintains larger size ✅

### Scenario 5: Change Position
- User selects "Bottom-Left" in combo
- RenderToolbar() → m_minimapPosition = 2
- SetMinimapPosition(2) → m_position = BottomLeft
- User resizes window
- Resize detected (line 79-80) ✅
- oldMinimapPosition = 2 (saved correctly)
- After restoration: m_position = BottomLeft (correct) ✅
- Minimap appears in bottom-left corner ✅

---

## Build Verification

**Command**: `dotnet build` / Visual Studio Build

**Result**: ✅ Génération réussie
- No compilation errors
- No warnings
- All method calls resolve correctly
- ICanvasEditor interface properly implemented

**Methods Called**:
- `m_canvasEditor->IsMinimapVisible()` ✅ Exists
- `m_canvasEditor->GetMinimapSize()` ✅ Exists
- `m_canvasEditor->GetMinimapPosition()` ✅ Exists
- `m_canvasEditor->SetMinimapVisible()` ✅ Exists
- `m_canvasEditor->SetMinimapSize()` ✅ Exists
- `m_canvasEditor->SetMinimapPosition()` ✅ Exists

---

## Summary

| Aspect | Details |
|--------|---------|
| **Files Changed** | 1 file: `EntityPrefabRenderer.cpp` |
| **Lines Added** | 9 lines (3 save + blank line + 3 restore + 1 comment + 1 blank) |
| **Lines Removed** | 0 lines |
| **Net Change** | +9 lines |
| **Methods Used** | 6 existing methods from ICanvasEditor |
| **Build Status** | ✅ 0 errors, 0 warnings |
| **Bug Fixed** | ✅ Minimap disappears after canvas resize |
| **Side Effects** | None - pure preservation of existing state |

---

## Code Pattern for Future Use

This is the standard pattern for preserving object state across recreation:

```cpp
// Pattern: Save-Delete-Restore

// 1. SAVE state before destroying old object
OldState oldState = oldObject->GetState();
OtherState otherState = oldObject->GetOther();

// 2. CREATE new object (old object destroyed if unique_ptr)
object = std::make_unique<NewType>(...);

// 3. RESTORE state to new object immediately
object->SetState(oldState);
object->SetOther(otherState);
```

This ensures seamless state preservation across object lifecycle transitions.
