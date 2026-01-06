# Blueprint Editor - Quick Start Guide

## What Was Fixed

This PR addresses all four issues preventing effective use of the Blueprint Editor:

1. **✅ Panels are now fully interactive** - You can click, drag, resize, and dock all panels
2. **✅ Runtime entities are visible** - All entities from the World appear in Asset Browser
3. **✅ Panels synchronize on selection** - Selecting an entity updates all panels automatically
4. **✅ Menus are fully functional** - All menu items work with keyboard shortcuts

## How to Use

### Opening the Editor
- Press **F2** at any time to toggle the Blueprint Editor on/off
- The editor overlays on top of your game
- Press **F2** again to close and return to the game

### Working with Runtime Entities

#### Viewing Entities
1. Open the **Asset Browser** panel (View → Asset Browser)
2. Scroll to the "Runtime Entities" section at the bottom
3. All active game entities are listed with their component counts

#### Inspecting an Entity
1. Click any entity in the Runtime Entities list
2. The **Inspector** panel automatically shows all components
3. The **Node Graph** panel shows the entity's behavior tree (if any)
4. All panels update simultaneously - no need to manually sync

#### Entity Information
- **Hover** over an entity to see its ID and component list
- **Click** to select and view full details
- Entity list updates in **real-time** as entities are created/destroyed

### Using Menus

#### File Menu (Blueprints)
- **Ctrl+N** - Create new blueprint
- **Ctrl+O** - Open blueprint from file
- **Ctrl+S** - Save current blueprint
- **Ctrl+Shift+S** - Save As with new name
- **Reload Assets** - Refresh asset tree from disk
- **F2** - Exit editor

#### Edit Menu
- **Ctrl+Z** - Undo (coming soon)
- **Ctrl+Y** - Redo (coming soon)
- **Insert** - Add component to blueprint
- **Delete** - Remove selected component
- **Preferences** - Editor settings (coming soon)

#### View Menu (Panel Toggles)
- Toggle visibility for any panel:
  - Asset Browser
  - Asset Info
  - Runtime Entities
  - Inspector
  - Node Graph
  - Entity Properties
  - Component Graph
  - Property Panel
- **Reset Layout** - Show all panels

#### Help Menu
- **Documentation** - Open help docs
- **Keyboard Shortcuts** - View all shortcuts
- **About** - Version and feature info

### Panel Management

#### Moving Panels
- Click and hold the panel title bar
- Drag to new position
- Release to drop

#### Resizing Panels
- Hover over panel edge until cursor changes
- Click and drag to resize
- Works on all four edges

#### Docking Panels
- Drag a panel near another panel's edge
- Blue overlay shows where it will dock
- Release to dock panels together
- Create custom layouts as needed

#### Hiding Panels
- Click the **X** button on panel title bar, or
- Use **View** menu to toggle visibility
- Use **View → Reset Layout** to restore all panels

### Keyboard Shortcuts Summary

| Shortcut | Action |
|----------|--------|
| F2 | Toggle Blueprint Editor |
| Ctrl+N | New Blueprint |
| Ctrl+O | Open Blueprint |
| Ctrl+S | Save |
| Ctrl+Shift+S | Save As |
| Insert | Add Component |
| Delete | Remove Component |
| Escape | Exit Application |

### Tips and Tricks

1. **Panel Layouts Are Saved**
   - Your panel positions are saved in `imgui.ini`
   - Close and reopen the editor - layout is restored
   - Delete `imgui.ini` to reset to defaults

2. **Multiple Entity Selection**
   - Select entities from Asset Browser OR Entities Panel
   - Both will stay synchronized
   - Inspector always shows the currently selected entity

3. **Component Editing**
   - Expand components in Inspector panel
   - Edit properties in real-time
   - Changes apply immediately to the entity

4. **Graph Editing**
   - Node Graph shows behavior trees and state machines
   - Currently read-only (editing coming soon)
   - Shows which entity you're viewing at the top

5. **Asset Management**
   - Asset Browser shows both:
     - Blueprint files from disk (top section)
     - Runtime entities from World (bottom section)
   - Use Reload Assets to refresh file list

### Workflow Examples

#### Debugging a Specific Entity
1. Press F2 to open editor
2. Find entity in Runtime Entities list
3. Click entity to select it
4. Inspect components in Inspector panel
5. View behavior tree in Node Graph panel
6. Edit properties as needed

#### Creating a New Blueprint
1. Press Ctrl+N or File → New Blueprint
2. Use Add Component to add components
3. Configure properties in Property Panel
4. Press Ctrl+S to save
5. Blueprint saved to Blueprints folder

#### Organizing Your Workspace
1. Arrange panels how you like them
2. Dock related panels together
3. Hide panels you don't need (View menu)
4. Your layout is automatically saved
5. Use Reset Layout to start over

### Troubleshooting

**Problem: Can't drag panels**
- Make sure you're clicking the title bar (not the panel content)
- If still not working, check TESTING_GUIDE.md for debug steps

**Problem: No entities in Runtime Entities list**
- Entities are only shown when they exist in the World
- Start the game to create entities
- Check that OLYMPE_BLUEPRINT_EDITOR_ENABLED is defined

**Problem: Selection not syncing between panels**
- Make sure you're clicking the entity, not just hovering
- Check that all panels are visible (View menu)
- Refer to TESTING_GUIDE.md for detailed tests

**Problem: Menus not responding**
- Ensure ImGui is receiving events (should be automatic)
- Check console for any error messages
- Try closing and reopening the editor (F2 twice)

### Where to Go from Here

- **TESTING_GUIDE.md** - Comprehensive testing procedures
- **ARCHITECTURE.md** - Technical details and diagrams  
- **IMPLEMENTATION_SUMMARY.md** - Full implementation notes

### Reporting Issues

If you find any problems:
1. Check TESTING_GUIDE.md for known issues
2. Verify all acceptance criteria
3. Note exact steps to reproduce
4. Include screenshot if possible
5. Check console output for errors

## Feature Status

| Feature | Status |
|---------|--------|
| Panel Interactivity | ✅ Complete |
| Runtime Entity Tracking | ✅ Complete |
| Panel Synchronization | ✅ Complete |
| Menu System | ✅ Complete |
| Undo/Redo | 🔄 Coming Soon |
| File Dialogs | 🔄 Coming Soon |
| Advanced Property Editing | 🔄 Coming Soon |

## Keyboard Reference Card

```
┌─────────────────────────────────────┐
│   BLUEPRINT EDITOR SHORTCUTS        │
├─────────────────────────────────────┤
│ F2          Toggle Editor           │
│ Escape      Exit Application        │
│                                     │
│ Ctrl+N      New Blueprint           │
│ Ctrl+O      Open Blueprint          │
│ Ctrl+S      Save                    │
│ Ctrl+Shift+S Save As                │
│                                     │
│ Insert      Add Component           │
│ Delete      Remove Component        │
│                                     │
│ Ctrl+Z      Undo (coming soon)      │
│ Ctrl+Y      Redo (coming soon)      │
└─────────────────────────────────────┘
```

Enjoy the fully interactive Blueprint Editor! 🎉
