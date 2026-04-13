# Component Library

The Component Library is defined in `Gamedata/PrefabEntities/ComponentsParameters.json`.

## Standard Components (10 types)

| Name | Category | Key Parameters |
|------|----------|---------------|
| Transform | Core | x, y, rotation, scaleX, scaleY |
| Identity | Core | name (string), tags (array) |
| Movement | Core | velocityX, velocityY, speed (float) |
| Sprite | Graphics | texturePath (string), layer (int), visible (bool) |
| Collision | Physics | shape (string), layer (int), isTrigger (bool) |
| Health | Gameplay | maxHp (int), currentHp (int) |
| AIBlackboard | AI | blackboardFile (string) |
| BehaviorTree | AI | graphFile (string) |
| VisualSprite | Graphics | spritesheet (string), frameW/H (int) |
| AnimationController | Graphics | animBankFile (string), defaultAnim (string) |

## JSON Format

```json
{
  "components": [
    {
      "name": "Transform",
      "category": "Core",
      "description": "2D position, rotation, and scale",
      "parameters": [
        { "name": "x",        "type": "float",  "default": 0.0 },
        { "name": "y",        "type": "float",  "default": 0.0 },
        { "name": "rotation", "type": "float",  "default": 0.0 },
        { "name": "scaleX",   "type": "float",  "default": 1.0 },
        { "name": "scaleY",   "type": "float",  "default": 1.0 }
      ]
    }
  ]
}
```

## Adding Components

1. Edit `Gamedata/PrefabEntities/ComponentsParameters.json`
2. Add your component entry (name, category, description, parameters)
3. Restart the editor – the palette auto-reloads

No C++ recompile needed for new component definitions.
