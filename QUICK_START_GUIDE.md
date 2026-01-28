# Quick Start Guide - Two-Pass Rendering System

## 🚀 Getting Started in 5 Minutes

### For Developers

#### 1. Understanding the System (2 minutes)

**What Changed?**
- Added `UIRenderingSystem` that renders **after** world rendering
- UI entities (type="UIElement") automatically render on top
- In-game menu accessible with ESC key

**Why?**
- Guarantees UI is always visible
- No more UI hidden behind world objects
- Clean separation of world and UI rendering

#### 2. Creating UI Elements (2 minutes)

```cpp
// Step 1: Create entity
EntityID myUIElement = World::Get().CreateEntity();

// Step 2: Mark as UI (IMPORTANT!)
Identity_data id;
id.type = "UIElement";  // ✅ This line makes it render on top
World::Get().AddComponent<Identity_data>(myUIElement, id);

// Step 3: Add position (screen coordinates)
Position_data pos;
pos.position = Vector(100, 50, 0);  // X=100, Y=50
World::Get().AddComponent<Position_data>(myUIElement, pos);

// Step 4: Add visual + bounding box (same as before)
// ... your existing code
```

That's it! Your UI element will now always render on top.

#### 3. Testing the Menu (1 minute)

1. Build and run the game
2. Press **ESC** → Menu appears
3. Press **↑/↓** → Navigate options
4. Press **RETURN** → Execute selection
5. Press **ESC** → Menu closes

---

## 📋 Quick Reference

### Menu Controls

| Key | Action |
|-----|--------|
| ESC | Open/Close menu |
| ↑ or W | Previous option |
| ↓ or S | Next option |
| RETURN | Select option |
| SPACE | Select option |

### Menu Options

1. **Resume** (Blue) - Close menu, continue game
2. **Restart** (Orange) - Restart level (placeholder)
3. **Quit** (Red) - Exit game

---

## 🔍 Troubleshooting

### UI Not Rendering on Top?

**Check:**
- ✅ Is `Identity_data.type = "UIElement"`?
- ✅ Does entity have all required components?
  - Identity_data ✓
  - Position_data ✓
  - VisualSprite_data ✓
  - BoundingBox_data ✓

### Menu Not Appearing?

**Check:**
- ✅ Is ESC key working?
- ✅ Check console for "UIEventConsumeSystem: ESC pressed" message
- ✅ Verify `GameMenu::IsActive()` returns true

### Selection Not Visible?

**Check:**
- ✅ Yellow border should appear around selected button
- ✅ Use arrow keys to change selection
- ✅ Check console for navigation messages

---

## 💡 Tips & Best Practices

### 1. UI Entity Naming
```cpp
id.name = "HealthBar";     // Descriptive name
id.type = "UIElement";     // ✅ MUST be "UIElement"
id.tag = "HUD";            // Optional category
```

### 2. Screen Coordinates
```cpp
// UI uses screen space, not world space
pos.position = Vector(10, 10, 0);  // Top-left corner
// NOT: Vector(worldX, worldY, depth)
```

### 3. Z-Ordering in UI
UI elements render in the order they're added to the system.
For explicit ordering, add them in desired sequence.

### 4. Visibility Control
```cpp
visual.visible = true;   // Show UI
visual.visible = false;  // Hide UI
```

---

## 🎨 Customizing the Menu

### Change Button Colors

In `Source/ECS_Systems.cpp`, find `UIRenderingSystem::RenderInGameMenu()`:

```cpp
// Resume button color
SDL_SetRenderDrawColor(renderer, 80, 120, 180, 255);  // Blue

// Change to green:
SDL_SetRenderDrawColor(renderer, 80, 180, 120, 255);  // Green
```

### Change Button Layout

```cpp
// Current layout (centered)
float panelWidth = 400;
float panelHeight = 300;

// Make bigger:
float panelWidth = 600;
float panelHeight = 400;
```

### Add More Menu Options

1. Add to enum in `GameMenu.h`:
```cpp
enum MenuOption {
    Resume = 0,
    Restart = 1,
    Options = 2,  // NEW
    Quit = 3,
    Count = 4
};
```

2. Add button rendering in `UIRenderingSystem::RenderInGameMenu()`
3. Add action in `GameMenu::ValidateSelection()`

---

## 📖 Documentation Reference

| Document | Purpose |
|----------|---------|
| `TWO_PASS_RENDERING_IMPLEMENTATION.md` | Complete implementation details |
| `MENU_VISUAL_GUIDE.md` | Visual design and layout specs |
| `TWO_PASS_RENDERING_ARCHITECTURE.md` | System architecture and diagrams |
| `PR_SUMMARY_TWO_PASS_RENDERING.md` | Executive summary |

---

## 🐛 Common Issues

### Issue: UI flickers
**Solution**: Make sure entity has `visual.visible = true`

### Issue: Menu doesn't pause game
**Solution**: Check `VideoGame::Pause()` is called in `GameMenu::Activate()`

### Issue: Can't navigate menu
**Solution**: Verify UIEventConsumeSystem is processing Input domain events

### Issue: Button not showing as selected
**Solution**: Check `GameMenu::GetSelectedOption()` is called in render loop

---

## 🎯 Next Steps

### For Users
1. Build project on Windows
2. Test menu with ESC key
3. Try all navigation controls
4. Report any issues

### For Developers
1. Create UI entities with type="UIElement"
2. Test UI rendering order
3. Customize menu appearance
4. Add text rendering (future enhancement)

---

## 💬 Need Help?

1. Check console logs for error messages
2. Review documentation files
3. Check component signatures match requirements
4. Verify system registration order in World.cpp

---

## ✅ Checklist: Is Everything Working?

### Basic Functionality
- [ ] Game builds without errors
- [ ] Game runs without crashes
- [ ] Menu appears when pressing ESC
- [ ] Game pauses when menu is open

### Menu Interaction
- [ ] Arrow keys navigate options
- [ ] Yellow border shows selection
- [ ] RETURN validates selection
- [ ] Resume closes menu
- [ ] Quit exits game

### UI Rendering
- [ ] UI entities render on top
- [ ] UI doesn't conflict with world objects
- [ ] Multiple viewports work correctly

---

**That's it!** You're ready to use the two-pass rendering system.

For more details, see the full documentation files.
