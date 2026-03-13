# CONTEXT CURRENT — Session Active

**Date**: 2026-03-13
**User**: @Atlasbruce
**Status**: Phase 20-C — Affichage inline des paramètres nodes + Add[+] VSSequence (complete)

---

## Developpement en Cours

- **Fonctionnalite actuelle :** Blueprint Editor — paramètres affichés inline dans les nœuds canvas
- **Objectif immediat :** Phase 20-C terminee — inline display + dynamic pins VSSequence implementes
- **Blocages connus :** Aucun

---

## Composants Actifs

- **Modules touches :** BlueprintEditor
- **Fichiers modifies (Phase 20-C) :**
  - `Source/TaskSystem/TaskGraphTemplate.h` — ajout `DynamicExecOutputPins` dans `TaskNodeDefinition`
  - `Source/BlueprintEditor/VisualScriptNodeRenderer.h` — nouvelle surcharge `RenderNode` avec `TaskNodeDefinition` + callback `onAddPin`
  - `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp` — implem inline display + bouton [+] VSSequence
  - `Source/BlueprintEditor/UndoRedoStack.h` — ajout `AddDynamicPinCommand`
  - `Source/BlueprintEditor/UndoRedoStack.cpp` — implem `AddDynamicPinCommand`
  - `Source/BlueprintEditor/VisualScriptEditorPanel.h` — ajout `m_pendingAddPin`, `m_pendingAddPinNodeID`, `GetExecOutputPinsForNode()`
  - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — RenderCanvas two-phase add pin, RebuildLinks dynamicPins, SerializeAndWrite dynamicExecPins
  - `Source/TaskSystem/TaskGraphLoader.cpp` — chargement `dynamicExecPins`
  - `Tests/BlueprintEditor/Phase20Test.cpp` — Test_AddDynamicPin_UndoRedo
- **Dependencies :** `TaskGraphTemplate.h`, `UndoRedoStack.h`, `VisualScriptNodeRenderer.h` — stables

---

## Decisions Recentes

- **2026-03-13 (Phase 20-C)** : Inline node parameter display + VSSequence dynamic pins.
  - Nouvelle surcharge `RenderNode(…, const TaskNodeDefinition& def, …, onAddPin callback)`
  - Champs affichés inline : AtomicTaskID, DelaySeconds, BBKey, ConditionID, SubGraphPath (basename), MathOperator
  - `AddDynamicPinCommand` : Execute push / Undo pop sur `DynamicExecOutputPins`
  - Pattern two-phase pour le callback [+] (comme drag-drop node creation)
  - `DynamicExecOutputPins` sérialisé dans JSON sous clé `dynamicExecPins`

---

## Notes Techniques Importantes

- **C++14 strict** : pas de structured bindings, std::optional, std::string_view.
- **Callback C** : `void (*onAddPin)(int, void*)` — pas de `std::function` pour C++14 compatibility
- **SYSTEM_LOG** : tous les logs utilisent SYSTEM_LOG.
- **Two-phase** : le callback [+] stocke la requête, traitée après EndNodeEditor()

---

## Prochaines Etapes

1. Phase 21 : Templates BT préconfigurés (Empty, Patrol, Combat...)

---
