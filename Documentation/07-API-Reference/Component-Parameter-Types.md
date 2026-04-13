# Component Parameter Types

## Parameter Type Enum

```cpp
enum class ComponentParameterType {
    Bool,
    Int,
    Float,
    String,
    Vector2,
    Vector3,
    Color,
    Array,
    EntityRef
};
```

## JSON Representation

| C++ Type | JSON key | Example |
|----------|----------|---------|
| `Bool` | `"bool"` | `true` |
| `Int` | `"int"` | `42` |
| `Float` | `"float"` | `3.14` |
| `String` | `"string"` | `"hello"` |
| `Vector2` | `"vector2"` | `[1.0, 2.0]` or `{"x":1.0,"y":2.0}` |
| `Vector3` | `"vector3"` | `[1.0, 2.0, 0.0]` or `{"x":1.0,"y":2.0,"z":0.0}` |
| `Color` | `"color"` | `[255, 128, 0, 255]` (RGBA) |
| `Array` | `"array"` | `[1, 2, 3]` |
| `EntityRef` | `"entityRef"` | `"target_entity_name"` |

## Vector Format

Both formats are accepted (parsed by `ParseParameterWithSchema()`):

```json
// Array format (preferred)
"position": [100.0, 200.0, 0.0]

// Object format (human-readable)
"position": { "x": 100.0, "y": 200.0, "z": 0.0 }
```

## Color Format

Colors are stored as RGBA arrays with values 0-255:

```json
"tint": [255, 128, 0, 255]
```

## EntityRef

A string reference to another entity by name:

```json
"target": "boss_enemy_01"
```

Resolved at runtime via entity lookup by name.

## Usage in ComponentsParameters.json

```json
{
  "components": [
    {
      "name": "Projectile",
      "parameters": [
        { "name": "speed",    "type": "float",     "default": 10.0 },
        { "name": "damage",   "type": "int",        "default": 25 },
        { "name": "color",    "type": "color",      "default": [255, 255, 0, 255] },
        { "name": "target",   "type": "entityRef",  "default": "" },
        { "name": "offset",   "type": "vector2",    "default": [0.0, 0.0] }
      ]
    }
  ]
}
```
