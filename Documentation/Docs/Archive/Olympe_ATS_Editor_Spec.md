> **ARCHIVE** - Ce document est obsolete. Archive le 2026-03-13.

---

# Spécifications Olympe ATS Editor (Behavior Tree)

## 1. Vue d'ensemble
- **Type** : Graphes de type Behavior Tree (BT), inspirés Unreal Engine.
- **Niveau** : Intermédiaire (actions précodées, pas de scripting custom).
- **Objectif** : Éditeur visuel pour IA / systèmes de jeu (selecteur, sequence, conditions, actions, services, subtrees).
- **Blackboard** : Local (composants entité) + Global (monde).

## 2. Types de Nodes

### 2.1. Root
- **Rôle** : Point d'entrée unique.
- **Enfants** : 1 Composite (Selector/Sequence).
```
{
  "id": 1,
  "name": "Root",
  "type": "Root",
  "children": [2]
}
```

### 2.2. Composites
| Type     | Sémantique                          |
|----------|-------------------------------------|
| Sequence | AND ordonné (stop 1er FAIL)        |
| Selector | OR priorisé (stop 1er SUCCESS)     |

```
{
  "id": 2,
  "name": "CombatSelector",
  "type": "Selector",
  "children": [3, 5, 8],
  "services": [...]  // Voir section 5
}
```

### 2.3. Actions
- **Rôle** : Feuilles qui agissent (via référentiel).
```
{
  "id": 4,
  "name": "MoveToGoal",
  "type": "Action",
  "actionType": "MoveToGoal",
  "parameters": [
    {"name": "speed", "value": 0.8},
    {"name": "tolerance", "value": 10.0}
  ]
}
```

### 2.4. Conditions
```
{
  "id": 3,
  "name": "TargetInRange",
  "type": "Condition",
  "conditionType": "CompareValue",
  "parameters": [
    {"name": "key", "value": "local:TargetDistance"},
    {"name": "operator", "value": "<="},
    {"name": "value", "value": 60.0}
  ]
}
```

### 2.5. Subtree
```
{
  "id": 42,
  "name": "AttackSubtree",
  "type": "Subtree",
  "subtreePath": "Blueprints/AI/Attack.json",
  "inputOverrides": {"damage": 15}
}
```

## 3. Services (attachés à Composite/Task)
```
"services": [
  {
    "serviceType": "UpdatePerception",
    "interval": 0.2,
    "randomDeviation": 0.1
  }
]
```

## 4. Blackboard References
- **Format** : `scope:path`
  - `local:CurrentState` → AIState.currentState
  - `global:GamePhase` → global_blackboard.GamePhase
- **Types** : bool, int, float, string, vector2/3, entity_handle

## 5. Référentiels (à définir en C++)
### 5.1. Actions
| ID                | Params exemple                  | Usage                              |
|-------------------|---------------------------------|------------------------------------|
| MoveToGoal        | speed, tolerance                | Déplacement vers BB.MoveGoal      |
| ChangeState       | newState                        | AIState.currentState = newState   |

### 5.2. Conditions
| Type           | Params                          | Exemple                           |
|----------------|---------------------------------|-----------------------------------|
| CompareValue   | key, operator, value            | TargetDistance <= 60             |
| IsSet          | key                             | HasTarget                        |

## 6. Règles de validation éditeur
- 1 seul Root
- Root → 1 Composite
- Composites → Composites/Actions/Conditions/Subtree
- Actions/Conditions/Subtree : pas d'enfants
- Pas de cycles
- Services attachés à Composite/Task seulement

## 7. Runtime Execution
- Tick top-down
- States : SUCCESS, FAIL, RUNNING
- Subtree : partage BB parent + overrides

## 8. Exemples (basés sur tes fichiers)
### GuardCombatTree.json (adapté)
```
{
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {"id":1,"type":"Root","children":[2]},
      {"id":2,"name":"Root Selector","type":"Selector","children":[3,6,9],"services":[{"serviceType":"UpdatePerception","interval":0.1}]},
      {"id":3,"name":"AttackSequence","type":"Sequence","children":[4,5]},
      {"id":4,"name":"TargetInRange?","type":"Condition","conditionType":"CompareValue","parameters":[{"name":"key","value":"local:TargetDistance"},{"name":"operator","value":"<="},{"name":"value",60}]},
      {"id":5,"name":"AttackIfClose","type":"Action","actionType":"AttackIfClose","parameters":[{"name":"range",60},{"name":"damage",15}]},
      // ... autres branches
    ]
  }
}
```
