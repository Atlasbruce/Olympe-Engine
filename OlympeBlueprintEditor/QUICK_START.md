# Olympe Blueprint Editor - Quick Start Guide

## 5-Minute Getting Started

### Step 1: Build the Editor

```bash
cd OlympeBlueprintEditor
make
```

**Output**: `OlympeBlueprintEditor` executable created

### Step 2: Run the Editor

```bash
# From the Olympe-Engine root directory
./OlympeBlueprintEditor/OlympeBlueprintEditor
```

### Step 3: Create Your First Entity

Follow these prompts:

```
1. Select "1" → New Blueprint
2. Name: "MyFirstEntity"
3. Description: "My test entity"
4. Select "6" → Edit Components
5. Select "1" → Add Component
6. Select "1" → Position
7. Enter X: 100
8. Enter Y: 200
9. Select "4" → Back to Main Menu
10. Select "3" → Save Blueprint
11. Filepath: "Blueprints/my_first_entity.json"
```

**Congratulations!** You've created your first blueprint.

## Common Tasks

### Load an Example Blueprint

```
Main Menu → "2" (Load Blueprint)
Enter: Blueprints/example_entity_simple.json
```

### Edit a Component Property

```
Main Menu → "6" (Edit Components)
→ "2" (Edit Component)
→ Enter component number: 1
→ "1" (Edit property value)
→ Property key: "position"
→ Sub-key: "x"
→ New value: 500
```

### Add a Sprite to Your Entity

```
Edit Components → "1" (Add Component)
→ "3" (VisualSprite)
→ Sprite path: "Resources/player.png"
→ Width: 32
→ Height: 32
```

### View Your Blueprint as JSON

```
Main Menu → "5" (View Blueprint Details)
```

## Keyboard Tips

- Press **Enter** after each input
- Menu numbers are shortcuts (1-7)
- Read prompts carefully
- Use "Back" options to navigate up

## Example: Creating a Player Character

```
1. New Blueprint
   Name: "Player"
   Description: "Main player character"

2. Add Position Component
   X: 100, Y: 100, Z: 0

3. Add BoundingBox Component
   X: 0, Y: 0, W: 32, H: 32

4. Add VisualSprite Component
   Path: "Resources/player.png"
   Width: 32, Height: 32

5. Add Health Component
   Max Health: 100

6. Add PhysicsBody Component
   Mass: 1.0, Speed: 100.0

7. Save As: "Blueprints/player.json"
```

**Result**: Complete player entity blueprint ready for the engine!

## Example: Modifying an Enemy

```
1. Load: "Blueprints/example_entity_complete.json"

2. Edit Components → Edit "Health"
   Change currentHealth: 50

3. Edit Components → Edit "PhysicsBody"
   Change speed: 150.0

4. Edit Components → Edit "AIBehavior"
   Change behaviorType: "chase"

5. Save As: "Blueprints/enemy_aggressive.json"
```

## Troubleshooting

### "Failed to load blueprint"
→ Check you're in the `Olympe-Engine` root directory  
→ Verify the file path is correct

### "Failed to save blueprint"
→ Make sure `Blueprints/` directory exists  
→ Check file permissions

### Property not updating
→ Enter exact property key name (case-sensitive)  
→ For nested properties, use parent → child approach

## Next Steps

- Read **EDITOR_USAGE.md** for complete documentation
- Check **Blueprints/README.md** for component reference
- Experiment with different component combinations
- Try modifying the example blueprints

## Quick Command Reference

| Action | Menu Path |
|--------|-----------|
| Create new | Main → 1 |
| Load file | Main → 2 |
| Save | Main → 3 |
| Save As | Main → 4 |
| View details | Main → 5 |
| Edit components | Main → 6 |
| Add component | Edit → 1 |
| Edit component | Edit → 2 |
| Remove component | Edit → 3 |
| Exit editor | Main → 7 |

## Pro Tips

1. **Save frequently**: Use Save (option 3) often
2. **Use Save As**: Create variations of blueprints
3. **View JSON**: Check option 5 to see structure
4. **Start simple**: Begin with 2-3 components
5. **Test loading**: Load your blueprint after saving

## Video Tutorial (Coming Soon)

Phase 2 will include video tutorials showing:
- Creating entities from scratch
- Modifying existing blueprints
- Best practices for organization
- Advanced component combinations

---

**Happy Blueprint Editing!** 🎮

For help: See EDITOR_USAGE.md or Blueprints/README.md
