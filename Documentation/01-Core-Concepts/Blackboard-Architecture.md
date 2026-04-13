# Blackboard Architecture

The Blackboard is a key-value store shared across AI nodes, enabling stateful behavior without tight coupling.

## Types of Blackboards

| Type | Scope | Class |
|------|-------|-------|
| Local Blackboard | Per-graph | `BlackboardSystem` |
| Global Blackboard | Engine-wide | `GlobalBlackboard` |
| Template Blackboard | Per-graph-template | `GlobalTemplateBlackboard` |

## Supported Value Types

```cpp
enum class BlackboardType : uint8_t {
    Int, Float, Bool, String, Vector3
};
```

## API

```cpp
BlackboardSystem bb;
bb.SetFloat("speed", 5.0f);
float s = bb.GetFloat("speed");

bb.SetBool("isAlerted", true);
if (bb.GetBool("isAlerted")) { ... }
```

## Serialization (JSON)

Local blackboard is embedded in graph files:

```json
{
  "localBlackboard": {
    "speed":     { "type": "float",  "value": 5.0 },
    "isAlerted": { "type": "bool",   "value": false },
    "state":     { "type": "string", "value": "idle" }
  }
}
```

## Global Blackboard

The `GlobalBlackboard` is a singleton accessible from any system:

```cpp
auto& g = Olympe::NodeGraph::GlobalBlackboard::GetInstance();
g.SetFloat("world_time", elapsed);
float t = g.GetFloat("world_time");
```

## Condition Presets (Phase 24)

Presets are stored directly in graph JSON (v4 schema) – graphs are self-contained:

```json
{
  "presets": [
    {
      "id": "preset_001",
      "name": "LowHealth",
      "conditions": [
        { "leftPinID": "pin_health", "op": "<", "rightValue": 30 }
      ]
    }
  ]
}
```

## Related

- [ECS System](ECS-System.md)
- [Visual Scripting](../03-Visual-Scripting/Graph-System.md)
