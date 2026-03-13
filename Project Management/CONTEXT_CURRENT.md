# CONTEXT CURRENT — Session Active

**Date**: 2026-03-13
**User**: @Atlasbruce
**Status**: Phase 20-B — Undo/Redo Properties Panel (complete)

---

## Developpement en Cours

- **Fonctionnalite actuelle :** Blueprint Editor stable — toutes les operations undo/redo fonctionnelles y compris le panel Properties
- **Objectif immediat :** Phase 20-B terminee — EditNodePropertyCommand + pattern commit-on-release implemente
- **Blocages connus :** Aucun

---

## Composants Actifs

- **Modules touches :** BlueprintEditor
- **Fichiers modifies (Phase 20-B) :**
  - `Source/BlueprintEditor/UndoRedoStack.h` — MODIFIE : ajout PropertyValue + EditNodePropertyCommand
  - `Source/BlueprintEditor/UndoRedoStack.cpp` — MODIFIE : implementation EditNodePropertyCommand
  - `Source/BlueprintEditor/VisualScriptEditorPanel.h` — MODIFIE : ajout membres snapshot m_prop*
  - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — MODIFIE : RenderProperties() avec undo commit-on-release
- **Dependencies :** `TaskGraphTemplate.h` (TaskNodeDefinition) — stable

---

## Decisions Recentes

- **2026-03-13 (Phase 20-B)** : Implementation de l'undo/redo du panel Properties.
  - `PropertyValue` : struct generique (String/Float/Int) C++14 — pas de std::variant
  - `EditNodePropertyCommand` : commande generique couvrant NodeName, AtomicTaskID, DelaySeconds, ConditionID, BBKey, MathOperator, SubGraphPath
  - Pattern "commit on release" via `IsItemDeactivatedAfterEdit()` — 1 undo entry par edit, pas par keystroke
  - Snapshot capture via `IsItemActivated()` apres chaque widget
  - Reset automatique du snapshot quand le noeud selectionne change (m_propSnapshotNodeID)
  - Architecture extensible pour futurs dropdowns (Combo retourne true immediatement, pas besoin de commit-on-release)

---

## Notes Techniques Importantes

- **C++14 strict** : PropertyValue utilise un enum class + membres explicites, pas std::variant
- **IsItemActivated/IsItemDeactivatedAfterEdit** : appeles APRES le widget ImGui correspondant
- **Snapshot double** : le snapshot est pris une fois avant ET apres le widget pour garantir la capture au focus meme si le widget est rendu pour la premiere fois
- **SYSTEM_LOG** : tous les logs d'undo utilisent SYSTEM_LOG

---

## Prochaines Etapes

1. Phase 20-C : Refonte affichage nodes — parametres inline + bouton Add[+] pour Sequence
2. Phase 20-D : Templates BT preconfigures (Empty, Patrol, Combat...)

---
