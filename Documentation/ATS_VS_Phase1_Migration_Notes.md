# ATS VS Phase 1 – Migration Notes

**Date:** 2026-03-08  
**Status:** Phase 1 complete (foundations only; runtime migration in Phase 2)

---

## Résumé de la Phase 1

Cette phase pose les fondations du nouveau système **ATS Visual Scripting (ATS VS)** — un modèle de graphe flowchart avec pins Exec **et** Data, à la manière d'Unreal Engine Blueprint, mais avec une couche d'exécution ECS propre à Olympe Engine.

### Ce qui a changé

| Fichier | Action |
|---|---|
| `Source/TaskSystem/TaskGraphTypes.h` | Ajout des enums VS (`TaskNodeType` 6-17, `VariableType` 7-8, `DataPinDir`, `ExecPinRole`) + struct `DataPinDefinition` |
| `Source/TaskSystem/TaskGraphTemplate.h` | Ajout de `ExecPinConnection`, `DataPinConnection`, `BlackboardEntry`; extension de `TaskNodeDefinition` (DataPins, ConditionID, BBKey, SubGraphPath, SwitchCases, DelaySeconds, MathOperator); extension de `TaskGraphTemplate` (EntryPointID, GraphType, Blackboard, ExecConnections, DataConnections) |
| `Source/TaskSystem/TaskGraphLoader.h` | Déclarations de `ParseSchemaV4` et des helpers VS (ParseNodeV4, ParseExecPins, ParseDataPins, ParseBlackboard, ParseExecConnections, ParseDataConnections, StringToNodeType, StringToVariableType, StringToDataPinDir, StringToExecPinRole) |
| `Source/TaskSystem/TaskGraphLoader.cpp` | Implémentation complète de `ParseSchemaV4` et helpers; dispatcher mis à jour pour `schemaVersion == 4` |
| `Source/ECS/Components/TaskRunnerComponent.h` | Ajout de `CurrentNodeID`, `LocalBlackboard` typé, `ActiveExecPinName`, `SequenceChildIndex`, `DoOnceFlags`, `DataPinCache`; `CurrentNodeIndex` et `LocalBlackboardData` marqués deprecated |
| `Source/BlueprintEditor/Graph.h` | Remplacé par un stub d'archivage (inclut le legacy) |
| `Source/BlueprintEditor/Graph.cpp` | Remplacé par un stub d'archivage (vide) |
| `Source/_deprecated/Graph_legacy.h` | Archivage de l'original `Graph.h` |
| `Source/_deprecated/Graph_legacy.cpp` | Archivage de l'original `Graph.cpp` |

### Ce qui n'a PAS changé

- `ParseSchemaV2`, `ParseSchemaV3` — inchangés
- `TaskValue` — inchangé
- Valeurs 0-5 de `TaskNodeType` (AtomicTask, Sequence, Selector, Parallel, Decorator, Root) — inchangées
- Valeurs 0-6 de `VariableType` (None, Bool, Int, Float, Vector, EntityID, String) — inchangées
- `BehaviorTreeSystem`, `AIMotionSystem`, `BehaviorTreeRuntime_data`, `AIBlackboard_data` — hors scope Phase 1

---

## Mapping JSON v2/v3 → v4

| Concept | v2 / v3 | v4 |
|---|---|---|
| Version | `"schema_version": 2` ou `3` | `"schema_version": 4` |
| Type de graphe | (implicite BT) | `"graph_type": "VisualScript"` |
| Nœud racine | `"rootNodeId"` dans `data` | `"entryPointId"` dans `data` (type `EntryPoint`) |
| Variables locales | `data.localVariables[{name, type, isLocal}]` | `data.blackboard[{key, type, default, global}]` |
| Connexions | Implicites (ChildrenIDs) | Explicites via `exec_connections` et `data_connections` |
| Type de nœud Action | `"type": "Action"` | `"type": "AtomicTask"` |
| Type de nœud Condition | `"type": "Condition"` | `"type": "AtomicTask"` |
| Séquence BT | `"type": "Sequence"` | `"type": "Sequence"` (→ `VSSequence` en VS) |

---

## Exemple JSON v4 minimal (guard patrol)

```json
{
  "schema_version": 4,
  "graph_type": "VisualScript",
  "name": "PatrolGuard",
  "description": "Simple patrol guard that checks if target is in range",
  "data": {
    "entryPointId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "EntryPoint",
        "name": "Start",
        "exec_pins": [{"name": "Out", "role": "Out"}],
        "data_pins": []
      },
      {
        "id": 2,
        "type": "Branch",
        "name": "TargetInRange?",
        "conditionId": "CompareValue",
        "exec_pins": [
          {"name": "In",   "role": "In"},
          {"name": "Then", "role": "Out"},
          {"name": "Else", "role": "OutElse"}
        ],
        "data_pins": [
          {"name": "Condition", "type": "Bool", "dir": "Input"}
        ],
        "parameters": {
          "key": "local:TargetDistance",
          "operator": "<=",
          "value": 60.0
        }
      },
      {
        "id": 3,
        "type": "AtomicTask",
        "name": "ChaseTarget",
        "exec_pins": [
          {"name": "In",  "role": "In"},
          {"name": "Out", "role": "Out"}
        ],
        "data_pins": [],
        "parameters": { "taskId": "ChaseTask" }
      },
      {
        "id": 4,
        "type": "AtomicTask",
        "name": "PatrolWaypoint",
        "exec_pins": [
          {"name": "In",  "role": "In"},
          {"name": "Out", "role": "Out"}
        ],
        "data_pins": [],
        "parameters": { "taskId": "PatrolTask" }
      }
    ],
    "exec_connections": [
      {"from_node": 1, "from_pin": "Out",  "to_node": 2, "to_pin": "In"},
      {"from_node": 2, "from_pin": "Then", "to_node": 3, "to_pin": "In"},
      {"from_node": 2, "from_pin": "Else", "to_node": 4, "to_pin": "In"}
    ],
    "data_connections": [],
    "blackboard": [
      {"key": "TargetDistance", "type": "Float", "default": 9999.0, "global": false}
    ]
  }
}
```

---

## Guide : Ajouter un nouveau type de node VS

Suivre cette checklist pour ajouter un nouveau type de nœud ATS VS :

1. **Ajouter la valeur `TaskNodeType` enum** dans `Source/TaskSystem/TaskGraphTypes.h`
   - Choisir une valeur entière après la dernière (actuellement 17 = SubGraph)
   - Ajouter le commentaire `///< description`

2. **Ajouter le cas dans `StringToNodeType()`** dans `Source/TaskSystem/TaskGraphLoader.cpp`
   - Ajouter `if (s == "NomDuType") { return TaskNodeType::NouveauType; }`
   - Préciser si le mapping dépend du `graphType`

3. **Gérer le parsing dans `ParseNodeV4()`** dans `Source/TaskSystem/TaskGraphLoader.cpp`
   - Extraire les champs spécifiques depuis `nodeJson["parameters"]`
   - Stocker dans les champs étendus de `TaskNodeDefinition` (ou ajouter un nouveau champ)

4. **Implémenter l'exécution dans le runtime (Phase 2)**
   - Ajouter un case dans le dispatcher du `TaskSystem` (ou `TaskSystemVS`)
   - Utiliser `CurrentNodeID`, `ActiveExecPinName`, `DataPinCache` de `TaskRunnerComponent`

---

## Fichiers archivés

| Fichier original | Archivé dans | Raison |
|---|---|---|
| `Source/BlueprintEditor/Graph.h` | `Source/_deprecated/Graph_legacy.h` | Struct Graph (id/type/x/y) insuffisante pour ATS VS |
| `Source/BlueprintEditor/Graph.cpp` | `Source/_deprecated/Graph_legacy.cpp` | Idem |

Les fichiers originaux sont remplacés par des stubs qui incluent le legacy
pour ne pas casser les includes existants pendant la migration.

---

## TODO Phase 2

- [ ] Implémenter le runtime VS dans `TaskSystem` : dispatcher exec/data pins
- [ ] Migrer `BehaviorTreeSystem` → `TaskSystemVS` (support graphType VisualScript)
- [ ] API typée pour `LocalBlackboard` (get/set avec type safety)
- [ ] Supprimer `LocalBlackboardData` (raw bytes) et `CurrentNodeIndex` (deprecated)
- [ ] Migrer les entités AI existantes vers le schéma v4
- [ ] Ajouter le support éditeur (Blueprint/Debugger) pour les graphes VS
- [ ] Implémenter les nodes de boucle (While, ForEach) dans le runtime
- [ ] Implémenter les nodes data (GetBBValue, SetBBValue, MathOp) dans le runtime
- [ ] Implémenter les nodes de contrôle (DoOnce, Delay, SubGraph) dans le runtime


---

## Deprecation Final (Phase 7)

All legacy BT v2 files have been removed or superseded.

### Files Removed in Earlier Phases
| File | Removed In | Replacement |
|------|-----------|-------------|
| `Source/BlueprintEditor/Graph_legacy.h/.cpp` | Phase 4 | `Source/BlueprintEditor/Graph.h/.cpp` (VS v4) |
| `Source/TaskSystem/TaskGraphLoader_v3_legacy.h/.cpp` | Phase 4 | `Source/TaskSystem/TaskGraphLoader.h/.cpp` |
| `Source/_deprecated/BehaviorTreeParser_v1.cpp` | Phase 4 | `BTtoVSMigrator` |

### NodeGraphPanel Status
`Source/BlueprintEditor/NodeGraphPanel.h/.cpp` is **deprecated** but retained for
`BehaviorTreeDebugWindow` (BT debug visualisation).  New VS v4 code should use
`VisualScriptEditorPanel`.

### Migration Complete
All 11 Blueprints/AI/*.json assets are now schema v4 VisualScript format.
Auto-migration is supported via `BTtoVSMigrator` and the unified router in
`BlueprintEditorGUI::LoadBlueprint()`.
