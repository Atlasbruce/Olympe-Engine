# JSON as Single Source of Truth

All game data in Olympe Engine is stored as **JSON files**. C++ code only interprets this data – it never hardcodes game content.

## Principle

> The JSON file IS the game object. The C++ code only provides the runtime machinery.

## File Categories

| Category | Directory | Format |
|----------|-----------|--------|
| Visual Scripts | `Gamedata/VisualScripts/` | `.ats` JSON (v4) |
| Behavior Trees | `Gamedata/BehaviorTrees/` | `.bt` JSON |
| Entity Prefabs | `Gamedata/PrefabEntities/` | `.json` (v4) |
| Blueprints | `Gamedata/Blueprints/` | `.json` |
| Component Definitions | `Gamedata/PrefabEntities/ComponentsParameters.json` | JSON array |
| Global Blackboard | `Config/global_blackboard.json` | JSON object |
| Tiled Maps | `Gamedata/Maps/` | `.tmx` / `.tmj` |

## Schema Version

All graph files include a `version` field. Current version: **4**.

```json
{
  "version": 4,
  "nodes": [...],
  "connections": [...],
  "presets": [...],
  "localBlackboard": {...}
}
```

## Benefits

1. **No recompile for content changes** – Edit JSON, run the game
2. **Human-readable** – Inspect and debug game state directly
3. **Version control friendly** – Diffs show meaningful changes
4. **Tool-agnostic** – Any editor can produce valid JSON
5. **Self-contained graphs** – Condition presets and blackboard embedded in graph files

## Loaders

| Loader | File | Purpose |
|--------|------|---------|
| `TaskGraphLoader` | `BlueprintEditor/` | Visual script JSON → `TaskGraphTemplate` |
| `PrefabLoader` | `EntityPrefabEditor/` | Prefab JSON → `EntityPrefabGraphDocument` |
| `BTGraphLoader` | `AI/` | BT JSON → `BTGraphTemplate` |
| `TiledLevelLoader` | `TiledLevelLoader/` | Tiled `.tmj` → ECS entities |
