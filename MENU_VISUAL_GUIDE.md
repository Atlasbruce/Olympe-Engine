# In-Game Menu Visual Guide

## Menu Layout

```
┌──────────────────────────────────────────────────────────────┐
│                                                                │
│                   SCREEN (800x600 default)                    │
│                                                                │
│    ┌─────────────────────────────────────────────────┐       │
│    │  Semi-transparent overlay (Black 50% alpha)     │       │
│    │                                                  │       │
│    │     ╔═══════════════════════════════════╗       │       │
│    │     ║  MENU PANEL (400x300)             ║       │       │
│    │     ║  Dark gray background             ║       │       │
│    │     ║                                    ║       │       │
│    │     ║  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓   ║       │       │
│    │     ║  ┃ RESUME (Blue)             ┃   ║       │       │
│    │     ║  ┃ 80, 120, 180              ┃   ║       │       │
│    │     ║  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛   ║  ← Selected  │
│    │     ║      (Double yellow border)       ║       │       │
│    │     ║                                    ║       │       │
│    │     ║  ┌───────────────────────────┐    ║       │       │
│    │     ║  │ RESTART (Orange)          │    ║       │       │
│    │     ║  │ 180, 120, 80              │    ║       │       │
│    │     ║  └───────────────────────────┘    ║       │       │
│    │     ║                                    ║       │       │
│    │     ║  ┌───────────────────────────┐    ║       │       │
│    │     ║  │ QUIT (Red)                │    ║       │       │
│    │     ║  │ 180, 80, 80               │    ║       │       │
│    │     ║  └───────────────────────────┘    ║       │       │
│    │     ║                                    ║       │       │
│    │     ╚═══════════════════════════════════╝       │       │
│    │                                                  │       │
│    └─────────────────────────────────────────────────┘       │
│                                                                │
└──────────────────────────────────────────────────────────────┘
```

## Color Palette

### Panel
- **Background**: RGB(40, 40, 40) - Dark gray
- **Border**: RGB(200, 200, 200) - Light gray

### Overlay
- **Background**: RGBA(0, 0, 0, 128) - Semi-transparent black (50% alpha)

### Buttons
| Button | Color | RGB | Purpose |
|--------|-------|-----|---------|
| **RESUME** | Blue | (80, 120, 180) | Safe action - continue playing |
| **RESTART** | Orange | (180, 120, 80) | Warning - restart level |
| **QUIT** | Red | (180, 80, 80) | Danger - exit game |

### Selection Indicator
- **Selected Border**: RGB(255, 255, 0) - Bright yellow
- **Border Width**: Double line (2 rectangles, 2px apart)
- **Unselected Border**: RGB(255, 255, 255) - White

## Dimensions

```
Screen Size (default): 800 x 600

Menu Panel:
├─ Width: 400px
├─ Height: 300px
├─ X: (screenWidth - 400) / 2 = 200px (centered)
└─ Y: (screenHeight - 300) / 2 = 150px (centered)

Buttons:
├─ Width: 300px
├─ Height: 50px
├─ X: panelX + 50px = 250px (centered in panel)
└─ Y Positions:
    ├─ Resume:  panelY + 80px = 230px
    ├─ Restart: panelY + 150px = 300px
    └─ Quit:    panelY + 220px = 370px

Spacing:
├─ Between buttons: 70px
└─ Top margin: 80px
```

## State Diagram

```
┌──────────────┐
│ Game Running │
└──────┬───────┘
       │
       │ [ESC pressed]
       ▼
┌──────────────┐
│  Menu Open   │ ← Resume selected by default
│  Game Paused │
└──────┬───────┘
       │
       ├─ [UP/W] → Select previous (wraps around)
       ├─ [DOWN/S] → Select next (wraps around)
       ├─ [RETURN/SPACE] → Validate selection
       │   ├─ If Resume → Close menu, resume game
       │   ├─ If Restart → Log message (not implemented)
       │   └─ If Quit → Exit game
       │
       └─ [ESC] → Close menu, resume game
```

## Keyboard Controls

```
┌─────────────┬───────────────────────────────────────┐
│    Key      │              Action                   │
├─────────────┼───────────────────────────────────────┤
│ ESC         │ Toggle menu (open/close)              │
│ UP / W      │ Select previous option (up)           │
│ DOWN / S    │ Select next option (down)             │
│ RETURN      │ Validate selection                    │
│ SPACE       │ Validate selection (alternative)      │
└─────────────┴───────────────────────────────────────┘
```

## Selection Behavior

### Navigation
```
Resume ──[DOWN]──> Restart ──[DOWN]──> Quit ──[DOWN]──┐
  ▲                                                     │
  └─────────────────[wraps around]─────────────────────┘

Quit ──[UP]──> Restart ──[UP]──> Resume ──[UP]──┐
  ▲                                               │
  └──────────[wraps around]───────────────────────┘
```

### Default State
- Menu opens with **Resume** selected (blue button with yellow border)
- Selection state persists between menu open/close cycles

## Visual Feedback

### Selected Button
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━┓  ← Outer yellow border
┃ ┏━━━━━━━━━━━━━━━━━━━━━━━┓ ┃  ← Inner yellow border (2px gap)
┃ ┃ BUTTON (Colored fill) ┃ ┃
┃ ┗━━━━━━━━━━━━━━━━━━━━━━━┛ ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

### Unselected Button
```
┌───────────────────────────┐  ← Single white border
│ BUTTON (Colored fill)     │
└───────────────────────────┘
```

## Implementation Details

### Rendering Order (Frame by Frame)
1. **World rendering** (RenderingSystem)
   - Parallax layers
   - Tiles
   - Entities (excluding UI)
2. **Grid overlay** (GridSystem)
3. **UI rendering** (UIRenderingSystem)
   - HUD elements (if any)
   - **In-game menu** (if active)
   - Debug overlay (if enabled)

### Menu Rendering Sequence
1. Draw semi-transparent overlay (full screen)
2. Draw panel background (centered)
3. Draw panel border
4. For each button:
   a. Draw button fill (colored)
   b. Draw border (yellow if selected, white otherwise)
   c. If selected, draw second border 2px inside

## Future Enhancements

### Text Rendering (Planned)
```
┌───────────────────────────┐
│         RESUME            │  ← Centered text
└───────────────────────────┘
```

### Hover Effects (Planned)
- Brighten button color on hover
- Add glow effect
- Show tooltip

### Animations (Planned)
- Fade in/out transitions
- Slide-in panel animation
- Button press animation

## Code Example: Creating a UI Entity

```cpp
// Create a health bar UI entity
EntityID healthBar = World::Get().CreateEntity();

// Identity (MUST set type to "UIElement")
Identity_data id;
id.name = "HealthBar";
id.type = "UIElement";  // ✅ Renders in Pass 2
id.tag = "HUD";
World::Get().AddComponent<Identity_data>(healthBar, id);

// Position (screen coordinates)
Position_data pos;
pos.position = Vector(10.0f, 10.0f, 0.0f);
World::Get().AddComponent<Position_data>(healthBar, pos);

// Visual
VisualSprite_data visual;
visual.sprite = healthBarTexture;
visual.visible = true;
visual.color = {255, 0, 0, 255}; // Red
World::Get().AddComponent<VisualSprite_data>(healthBar, visual);

// Bounding box
BoundingBox_data bbox;
bbox.boundingBox = {0, 0, 200, 30};
World::Get().AddComponent<BoundingBox_data>(healthBar, bbox);
```

## Testing Checklist

- [ ] Menu appears when pressing ESC
- [ ] Game pauses when menu is open
- [ ] Resume button has yellow border by default
- [ ] UP/DOWN keys change selection
- [ ] Yellow border moves to selected button
- [ ] Selection wraps around (Quit → Resume)
- [ ] RETURN key activates selected option
- [ ] Resume closes menu and resumes game
- [ ] Quit exits the game
- [ ] ESC closes menu (same as Resume)
- [ ] Menu renders on top of all world objects
- [ ] Overlay is semi-transparent (can see world behind)

## Performance Notes

- Menu rendering is ~60 draw calls (overlay + panel + buttons)
- No additional sorting overhead
- Minimal CPU/GPU impact
- Rendering only when menu is active (early exit check)

---

**Note**: Text rendering is not yet implemented. Current version uses colored rectangles as button placeholders. This provides a functional menu system with clear visual feedback, ready for text rendering integration in a future update.
