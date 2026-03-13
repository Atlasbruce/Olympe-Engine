# CONTEXT CURRENT — Session Active

**Date**: 2026-03-13
**User**: @Atlasbruce
**Status**: Phase 21 — Undo/Redo Panel Properties (complete)

---

## Developpement en Cours

- **Fonctionnalite actuelle :** Blueprint Editor stable — Undo/Redo Panel Properties implemente
- **Objectif immediat :** Phase 21 terminee — toutes modifications du panel Properties undo-ables
- **Blocages connus :** Aucun

---

## Composants Actifs

- **Modules touches :** BlueprintEditor
- **Fichiers modifies (Phase 21) :**
  - `Source/BlueprintEditor/UndoRedoStack.h` — ajout EditNodePropertyCommand + PropertyValue
  - `Source/BlueprintEditor/UndoRedoStack.cpp` — implementation EditNodePropertyCommand
  - `Source/BlueprintEditor/VisualScriptEditorPanel.h` — ajout membres snapshot m_propEditOld*
  - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — RenderProperties() refactor "commit on release"
- **Dependencies :** `TaskGraphTemplate.h` (TaskNodeDefinition) — stable

---

## Decisions Recentes

- **2026-03-13 (Phase 20-B)** : VSConnectionValidator stateless — validation connexions exec (self-loop, duplicate, cycle DFS)
- **2026-03-13 (Phase 21)** : EditNodePropertyCommand generique avec PropertyValue (String/Float)
  - Pattern "commit on release" : IsItemActivated() pour snapshot, IsItemDeactivatedAfterEdit() pour push undo
  - Couvre : NodeName, AtomicTaskID, DelaySeconds, ConditionID, BBKey, MathOperator, SubGraphPath
  - Architecture extensible pour futurs dropdowns (push undo immediat sur Combo)

---

## Notes Techniques Importantes

- **C++14 strict** : PropertyValue utilise union manuelle (pas std::variant)
- **Pattern commit-on-release** : un seul undo entry par champ edite (pas 50 entrees pour une frappe)
- **Idempotence Execute()** : PushCommand() appelle Execute() — valeur deja synchronisee live
- **SYSTEM_LOG** : tous les logs utilisent SYSTEM_LOG, pas std::cout

---

## Prochaines Etapes

1. Phase 22 : Templates BT preconfigures (Empty, Patrol, Combat...)
2. Phase 23 : Refonte affichage nodes inline (parametres inline dans le canvas)

---

