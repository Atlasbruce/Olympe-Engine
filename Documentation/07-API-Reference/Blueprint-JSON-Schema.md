# Blueprint JSON Schema Reference

## Graph Schema (v4)

All graph files (Visual Script, BT, Entity Prefab) use schema version 4.

```json
{
  "version": 4,
  "nodes": [ "<Node[]>" ],
  "connections": [ "<Connection[]>" ],
  "presets": [ "<ConditionPreset[]>" ],
  "localBlackboard": { "<BlackboardData>" },
  "canvasState": { "<CanvasState>" }
}
```

## Node Object

```json
{
  "nodeId": 1,
  "type": "Action",
  "actionId": "MoveToTarget",
  "label": "Move To Player",
  "position": [100.0, 200.0],
  "size": [150.0, 60.0],
  "parameters": {
    "speed": 3.5,
    "tolerance": 10.0
  },
  "pins": {
    "input": [ { "pinId": "pin_in_1", "type": "exec" } ],
    "output": [ { "pinId": "pin_out_1", "type": "exec" } ]
  }
}
```

## Connection Object

```json
{ "sourceNodeId": 1, "sourcePinId": "pin_out_1",
  "targetNodeId": 2, "targetPinId": "pin_in_1" }
```

## ConditionPreset Object (Phase 24)

```json
{
  "id": "preset_001",
  "name": "PlayerNearby",
  "conditions": [
    {
      "leftPinID": "pin_distance",
      "op": "<",
      "rightValue": 100.0
    }
  ]
}
```

Operator values: `"=="`, `"!="`, `"<"`, `"<="`, `">"`, `">="`.

Note: When reading, both `"op"` and `"operator"` keys are accepted (legacy compatibility).

## BlackboardData Object

```json
{
  "speed":     { "type": "float",  "value": 5.0 },
  "isAlerted": { "type": "bool",   "value": false },
  "state":     { "type": "string", "value": "idle" },
  "position":  { "type": "vector3","value": [0.0, 0.0, 0.0] }
}
```

## CanvasState Object

```json
{
  "zoom": 1.0,
  "offsetX": 0.0,
  "offsetY": 0.0
}
```

## Entity Prefab Schema

Entity prefabs use a subset of the graph schema:

```json
{
  "version": 4,
  "nodes": [
    {
      "nodeId": 1,
      "componentType": "Transform",
      "componentName": "Position",
      "position": [100.0, 150.0],
      "parameters": { "x": 0.0, "y": 0.0, "rotation": 0.0 }
    }
  ],
  "connections": [ { "sourceId": 1, "targetId": 2 } ],
  "canvasState": { "zoom": 1.0, "offset": [0.0, 0.0] }
}
```
