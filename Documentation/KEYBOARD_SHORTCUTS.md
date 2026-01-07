# Keyboard Shortcuts and Productivity Guide

## Overview

The Blueprint Editor provides comprehensive keyboard shortcuts for professional productivity. This guide covers all available shortcuts and tips for efficient workflow.

## Global Editor Shortcuts

### File Operations

| Shortcut | Action | Description |
|----------|--------|-------------|
| **F2** | Toggle Editor | Open/close the Blueprint Editor |
| **Ctrl+N** | New Blueprint | Create a new empty blueprint |
| **Ctrl+O** | Open Blueprint | Open an existing blueprint file |
| **Ctrl+S** | Save | Save current blueprint |
| **Ctrl+Shift+S** | Save As | Save blueprint with new name |
| **Ctrl+Shift+T** | Save as Template | Create a template from current blueprint |

### Edit Operations

| Shortcut | Action | Description |
|----------|--------|-------------|
| **Ctrl+Z** | Undo | Undo the last action |
| **Ctrl+Y** | Redo | Redo the last undone action |
| **Ctrl+Shift+Z** | Redo (Alt) | Alternative redo shortcut |
| **Insert** | Add Component | Open component selection dialog |
| **Delete** | Remove Component | Delete selected component |

### View Operations

All panels can be toggled via the **View** menu:
- Asset Browser
- Asset Info
- Runtime Entities
- Inspector
- Node Graph
- Template Browser
- History

## Workflow Tips

### Fast Blueprint Creation

**Typical Workflow:**
1. Press **F2** to open editor
2. Press **Ctrl+N** for new blueprint
3. Add components via GUI or **Insert** key
4. Configure properties
5. Press **Ctrl+S** to save
6. Press **F2** to return to game

**Time Saved:** ~30 seconds per blueprint vs manual JSON editing

### Template-Based Workflow

**Speed up repetitive tasks:**
1. Create a blueprint once
2. Press **Ctrl+Shift+T** to save as template
3. For new similar blueprints:
   - Open Template Browser
   - Apply template
   - Modify as needed
   - Save with new name

**Time Saved:** ~2-5 minutes per entity

### Undo/Redo Workflow

**Experiment fearlessly:**
1. Make changes to your blueprint
2. Test in game (F2 to close editor)
3. If something breaks:
   - Press **F2** to reopen editor
   - Press **Ctrl+Z** multiple times to undo
   - Or use History panel to see all changes

**Benefits:**
- No fear of breaking things
- Easy experimentation
- Clear history of changes

## Advanced Techniques

### Multi-Component Editing

**Rapid component addition:**
1. Open blueprint
2. Press **Insert** repeatedly
3. Select component type each time
4. Configure all at once
5. **Ctrl+S** to save

### Template Iteration

**Refine templates over time:**
1. Apply existing template
2. Make improvements
3. **Ctrl+Shift+T** to save as new version
4. Update version number in description
5. Delete old version if needed

### History-Based Debugging

**Find what broke:**
1. Open **History** panel (View menu)
2. Review recent commands
3. Identify problematic change
4. Undo to that point
5. Try alternative approach

## Shortcut Customization

*Currently, shortcuts are hard-coded. Custom keybindings will be added in a future update.*

## Panel Management

### Quick Panel Access

**Via Keyboard:**
1. Press **Alt** to activate menu bar
2. Press **V** for View menu
3. Use arrow keys to select panel
4. Press **Enter** to toggle

**Via Mouse:**
- Click **View** menu
- Click panel name to toggle visibility

### Layout Reset

If your layout becomes messy:
1. Go to **View → Reset Layout**
2. All panels return to default visibility
3. Rearrange as needed

## Productivity Patterns

### Pattern 1: Rapid Prototyping

```
Ctrl+N          → New blueprint
Insert x5       → Add 5 components
Configure       → Set properties
Ctrl+S          → Save
F2              → Test in game
F2              → Back to editor
Ctrl+Z x2       → Undo last 2 changes
Ctrl+S          → Save final version
```

### Pattern 2: Template Library Building

```
Create base entity
Ctrl+Shift+T    → Save as template
Ctrl+N          → New blueprint  
Apply template  → From browser
Customize       → Modify for variant
Ctrl+Shift+T    → Save variant template
Repeat for each variant type
```

### Pattern 3: Iterative Development

```
Open existing blueprint
Make changes
Ctrl+S          → Save
F2              → Test
(Find issue)
F2              → Back to editor
Ctrl+Z          → Undo problem
Make fix
Ctrl+S          → Save
F2              → Test again
```

## Common Workflow Scenarios

### Scenario 1: Creating Enemy Variants

**Goal:** Create 5 enemy types quickly

**Steps:**
1. Create first enemy blueprint manually
2. Save as "Enemy Base" template
3. For each variant:
   - Apply "Enemy Base" template
   - Modify specific properties (health, damage, speed)
   - Save with variant name
   
**Time:** 2 minutes per variant vs 10 minutes manual

### Scenario 2: Debugging Component Issues

**Goal:** Fix broken blueprint

**Steps:**
1. Open problematic blueprint
2. View **History** panel
3. Review recent changes
4. Press **Ctrl+Z** to undo suspicious changes one at a time
5. Test after each undo (F2 toggle)
6. When working, redo up to working state
7. Make proper fix

**Benefit:** No need to remember what changed

### Scenario 3: Batch Creating Items

**Goal:** Create 20 collectible items

**Steps:**
1. Create one complete collectible blueprint
2. **Ctrl+Shift+T** → Save as "Collectible Template"
3. For each item (20 times):
   - Apply template
   - Change sprite path and item ID
   - **Ctrl+S** → Save with item name

**Time:** 30 seconds per item vs 5 minutes manual

## Performance Tips

### Minimize Mode Switches

❌ **Slow:** 
```
Edit blueprint
F2 → Test
F2 → Edit one property
F2 → Test
F2 → Edit another property
F2 → Test
```

✅ **Fast:**
```
Edit blueprint completely
F2 → Test all changes at once
If issues: F2 → Edit, then test again
```

### Use Templates Wisely

❌ **Don't:** Create a new template for every single entity

✅ **Do:** Create templates for categories of entities

**Good Categories:**
- Base Enemy
- Base Player Character
- Base Collectible
- Base Interactive Object

Then customize from these bases.

### Leverage Undo/Redo

❌ **Don't:** Manually revert changes by typing

✅ **Do:** Use **Ctrl+Z** to instantly undo

**Benefits:**
- Faster
- More reliable
- Can redo if you undo too far

## Shortcuts Cheat Sheet

### Print This Reference

```
┌─────────────────────────────────────────────┐
│   BLUEPRINT EDITOR - KEYBOARD SHORTCUTS     │
├─────────────────────────────────────────────┤
│ F2            │ Toggle Editor               │
│ Ctrl+N        │ New Blueprint               │
│ Ctrl+O        │ Open Blueprint              │
│ Ctrl+S        │ Save                        │
│ Ctrl+Shift+S  │ Save As                     │
│ Ctrl+Shift+T  │ Save as Template            │
│ Ctrl+Z        │ Undo                        │
│ Ctrl+Y        │ Redo                        │
│ Insert        │ Add Component               │
│ Delete        │ Remove Component            │
└─────────────────────────────────────────────┘
```

## Tips and Tricks

💡 **Muscle Memory:** Practice shortcuts on simple tasks first

💡 **Menu Hints:** All shortcuts are shown in menus as reminders

💡 **Undo Safety:** You can undo up to 100 commands

💡 **Save Often:** **Ctrl+S** is quick - use it frequently

💡 **Template Names:** Use clear, searchable names for templates

💡 **History Panel:** Keep it open when doing complex edits

💡 **Test Frequently:** Toggle editor with F2 to test changes quickly

💡 **Categories:** Organize templates by category from the start

## Troubleshooting

### Shortcut Not Working

**Check:**
- Is a text field focused? (Shortcuts disabled during text input)
- Is the editor active? (F2 to toggle)
- Is the Blueprint Editor window focused?

**Solution:**
- Click on the editor window
- Make sure no text field has focus
- Try the menu alternative

### Lost Changes

**If you accidentally closed without saving:**
- ❌ Changes are lost (no auto-save yet)
- ✅ Save frequently with **Ctrl+S**
- ✅ Auto-save feature coming in future update

### Undo Went Too Far

**Solution:**
- Press **Ctrl+Y** or **Ctrl+Shift+Z** to redo
- Check History panel to see where you are
- Redo multiple times if needed

## Future Enhancements

Planned shortcuts and features:
- **Ctrl+D** - Duplicate selected node/component
- **Ctrl+A** - Select all nodes
- **Ctrl+X/C/V** - Cut/Copy/Paste nodes
- **Arrow Keys** - Navigate node graph
- **Space+Drag** - Pan view
- **Custom Keybindings** - User-defined shortcuts

---

**See Also:**
- [Templates Guide](TEMPLATES_GUIDE.md)
- [Blueprint Editor Documentation](../QUICKSTART.md)
- [Architecture Documentation](../ARCHITECTURE.md)
