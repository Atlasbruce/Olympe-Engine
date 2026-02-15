# Grid Panel UI Mockup

## Window: "Grid Settings (F4)"

```
┌─────────────────────────────────────────────────────────┐
│ Grid Settings (F4)                                    X │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ECS Grid System Configuration                         │
│  ────────────────────────────────────────────────────  │
│                                                         │
│  [✓] Grid Enabled                    (Toggle with TAB) │
│                                                         │
│  Grid Projection Type:                                 │
│  [Orthogonal           ▼]                              │
│  Ortho: Standard square grid                           │
│  Iso: Diamond-shaped isometric grid                    │
│  Hex: Hexagonal axial grid (pointy-top)                │
│                                                         │
│  ────────────────────────────────────────────────────  │
│                                                         │
│  Grid Dimensions:                                      │
│  Cell Size (World Units):                              │
│  Width    [░░░░░░▓░░░░░░░░] 32.0                      │
│  Height   [░░░░░░▓░░░░░░░░] 32.0                      │
│                                                         │
│  ────────────────────────────────────────────────────  │
│                                                         │
│  Visual Settings:                                      │
│  Grid Color: [████] R: 180  G: 180  B: 180  A: 255    │
│                                                         │
│  Max Lines    [░░░░░░▓░░░░░░░░] 1200                  │
│  Performance budget for line rendering                 │
│                                                         │
│  Level of Detail (LOD):                                │
│  LOD Zoom Threshold [░░░▓░░░░░░░] 0.50                │
│  Apply LOD when zoom is below this value               │
│                                                         │
│  LOD Skip Factor [░░░░▓░░░░░░░░░░] 10                 │
│  Draw 1 line every N when LOD active                   │
│                                                         │
│  ────────────────────────────────────────────────────  │
│                                                         │
│  Overlay Visualization:                                │
│  [✓] Show Collision Overlay          (Toggle with C)  │
│      Collision Layer [░░░░░░░░] 0                     │
│      Layer 0: Ground, 1: Sky, 2: Underground, 3: Vol  │
│                                                         │
│  [ ] Show Navigation Overlay         (Toggle with N)  │
│                                                         │
│  ────────────────────────────────────────────────────  │
│                                                         │
│  Quick Actions:                                        │
│  [Reset to Defaults]  [Reload from ECS]                │
│                                                         │
│  Keyboard shortcuts:                                   │
│    TAB: Toggle grid                                    │
│    C: Toggle collision overlay                         │
│    N: Toggle navigation overlay                        │
│    F4: Toggle this panel                               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## Visual Style

- **Window Size**: 420x600 pixels (resizable)
- **Colors**: 
  - Background: Dark gray (ImGui Dark theme)
  - Header: Blue-tinted text
  - Sliders: Standard ImGui style
  - Separators: Thin horizontal lines
- **Font**: ImGui default font
- **Controls**: 
  - Checkboxes for toggles
  - Dropdown for projection type
  - Drag sliders for numeric values
  - Color picker for grid color
  - Text for help/descriptions
  - Buttons for actions

## Interaction

1. **Opening**: Press F4 or call `gridPanel->Toggle()`
2. **Grid Toggle**: Check/uncheck "Grid Enabled" or press TAB
3. **Projection Change**: Select from dropdown, changes apply immediately
4. **Size Adjustment**: Drag sliders, changes apply immediately
5. **Color Picker**: Click color square for full RGBA editor
6. **Overlays**: Check boxes or press C/N keys
7. **Reset**: Click "Reset to Defaults" to restore all settings
8. **Reload**: Click "Reload from ECS" to sync with component data

## States

### Default State (First Open)
- Grid Enabled: Unchecked
- Projection: Orthogonal
- Cell Size: 32x32
- Color: Light gray (180, 180, 180, 255)
- Max Lines: 1200
- LOD Threshold: 0.5
- LOD Skip: 10
- Overlays: Disabled

### Active Grid State
- Grid Enabled: Checked
- Grid visible in viewport
- Respects camera frustum culling
- LOD applied based on zoom

### Overlay Active State
- Collision or Navigation checkbox checked
- Layer slider visible and active
- Colored overlays drawn on grid
- Performance impact visible

## Integration with Game View

When grid is enabled:
```
┌──────────────────────────────────────────────┐
│ Olympe Engine 2.0                            │
├──────────────────────────────────────────────┤
│                                              │
│  ┌────┬────┬────┬────┐                     │
│  │    │    │    │    │  ← Grid lines      │
│  ├────┼────┼────┼────┤     (light gray)    │
│  │    │ ◆  │    │    │  ← Sprite          │
│  ├────┼────┼────┼────┤                     │
│  │    │    │    │    │                     │
│  └────┴────┴────┴────┘                     │
│                                              │
│  [Grid Enabled] [Collision Overlay]          │
│  Projection: Isometric                       │
│                                              │
└──────────────────────────────────────────────┘
```

## Keyboard Shortcuts (Global)

- **TAB**: Toggle grid (anywhere in engine)
- **F4**: Open/close Grid Settings panel
- **C**: Toggle collision overlay (if grid enabled)
- **N**: Toggle navigation overlay (if grid enabled)

These work regardless of whether the Grid Settings panel is open.
