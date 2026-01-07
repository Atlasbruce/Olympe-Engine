# Blueprint Templates - User Guide

## Overview

The Blueprint Template system allows you to save and reuse blueprints as templates, dramatically improving productivity and ensuring consistency across your project.

## What are Templates?

Templates are saved blueprints that can be:
- **Reused** to quickly create new entities with predefined components
- **Organized** by categories (AI, Character, Enemy, Puzzle, etc.)
- **Shared** with your team (stored as JSON files)
- **Searched** by name and filtered by category

## Creating Templates

### From the Current Blueprint

1. **Open or create a blueprint** in the Blueprint Editor
2. **Configure the blueprint** with the components and properties you want
3. **Save as template:**
   - Click **File → Save as Template...**
   - Or press **Ctrl+Shift+T**

4. **Fill in template information:**
   - **Template Name:** A descriptive name (e.g., "Basic Enemy AI")
   - **Description:** What the template does and when to use it
   - **Category:** Group similar templates together (AI, Character, etc.)

5. **Click Save**

The template is now saved in `Blueprints/Templates/` and will appear in the Template Browser.

## Using Templates

### Opening the Template Browser

- **File → Template Browser**
- Or **View → Template Browser**

### Finding Templates

**Search by Name:**
- Type in the search box at the top
- Only templates matching your search will appear

**Filter by Category:**
- Use the category dropdown
- Select "All" to see all templates

### Applying a Template

**Method 1: Context Menu**
1. Right-click on a template in the browser
2. Select "Apply to Current Blueprint"

**Method 2: Selection Button**
1. Click on a template to select it
2. Click "Apply Selected Template" button at the bottom

**Result:** The current blueprint is replaced with the template content. You can then modify it as needed.

## Managing Templates

### Viewing Template Details

Hover over any template in the browser to see:
- Full description
- Category
- Author
- Version
- Creation date

### Deleting Templates

1. Right-click on the template
2. Select "Delete"
3. Confirm deletion

**Warning:** This permanently deletes the template file from disk.

### Refreshing Templates

Click the **Refresh** button in the Template Browser to reload all templates from disk. Useful if templates are added externally.

## Template Organization

### Categories

Organize templates using categories:
- **AI/** - Behavior trees and AI logic
- **Character/** - Player and NPC character setups
- **Enemy/** - Enemy entity configurations
- **Puzzle/** - Puzzle mechanics and interactions
- **Custom/** - Your custom categories

**Tip:** Use consistent category names across your project for better organization.

### Naming Conventions

Good template names are:
- **Descriptive:** "Guard Patrol AI" vs "Template1"
- **Specific:** "2D Platformer Character" vs "Character"
- **Consistent:** Follow your team's naming standards

## Best Practices

### When to Create Templates

✅ **Good candidates for templates:**
- Common entity types (basic enemy, collectible item)
- Complex configurations you use frequently
- Standard starting points for new content
- Team-approved blueprints

❌ **Poor candidates for templates:**
- One-off unique entities
- Constantly changing configurations
- Experimental work-in-progress

### Template Descriptions

Write clear descriptions including:
- **Purpose:** What the template is for
- **Components:** Key components included
- **Usage:** When and how to use it
- **Variants:** Related templates to consider

**Example:**
```
Name: Guard Patrol AI
Description: Standard patrol behavior for guard NPCs. 
Includes patrol route following, alert detection, and 
return to patrol. Use for basic area guards.
Category: AI/Enemy
```

### Version Control

Templates are stored as JSON files in `Blueprints/Templates/`. 
**Recommended:** Commit these files to version control to share with your team.

## Technical Details

### File Format

Templates are stored as JSON with this structure:
```json
{
    "id": "unique-uuid",
    "name": "Template Name",
    "description": "Description",
    "category": "Category",
    "author": "User",
    "version": "1.0",
    "created": "2026-01-07T12:00:00Z",
    "modified": "2026-01-07T12:00:00Z",
    "blueprintData": {
        // Complete blueprint JSON
    }
}
```

### Template Location

- **Directory:** `Blueprints/Templates/`
- **Filename:** `{UUID}.json`
- **Example:** `abc123-def4-5678-90ab-cdef12345678.json`

### Sharing Templates

To share a template with teammates:
1. Copy the template JSON file from `Blueprints/Templates/`
2. Send it to your teammate
3. They place it in their `Blueprints/Templates/` directory
4. Click Refresh in the Template Browser

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **Ctrl+Shift+T** | Open "Save as Template" dialog |

## Troubleshooting

### Template Not Appearing in Browser

**Solution:**
- Click the **Refresh** button in Template Browser
- Check that the file is in `Blueprints/Templates/` directory
- Verify the JSON file is valid

### Cannot Apply Template

**Possible causes:**
- Template file is corrupted
- JSON syntax error
- Missing blueprint data

**Solution:**
- Check editor console for error messages
- Verify template JSON structure
- Try creating a new template

### Template Shows Wrong Information

**Solution:**
- Edit the template JSON file directly
- Or delete and recreate the template
- Remember to refresh the Template Browser

## Examples

### Example 1: Character Template Library

Create templates for different character archetypes:
- "Player Character (2D Platformer)"
- "Flying Enemy (Basic)"
- "Ground Enemy (Patrol)"
- "Boss Character (Template)"

### Example 2: AI Behavior Templates

Build a library of AI behaviors:
- "Idle Behavior"
- "Patrol Route Following"
- "Chase Player"
- "Attack Melee"
- "Flee from Danger"

### Example 3: Item Templates

Standardize collectible items:
- "Health Pickup"
- "Ammo Pickup"
- "Key Item"
- "Power-up"

## Tips and Tricks

💡 **Template Hierarchies:** Create base templates, then customize them for specific use cases

💡 **Regular Cleanup:** Delete unused templates to keep the browser manageable

💡 **Documentation:** Keep a separate document listing all team templates and their purposes

💡 **Versioning:** Include version numbers in descriptions when templates evolve

💡 **Backup:** Keep backups of important templates outside the project directory

---

**Need more help?** Check the [Keyboard Shortcuts Guide](KEYBOARD_SHORTCUTS.md) or [Blueprint Editor Documentation](BLUEPRINT_EDITOR.md).
