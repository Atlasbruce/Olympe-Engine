# CONTEXT CURRENT — Session Active

**Date**: 2026-03-13
**User**: @Atlasbruce
**Status**: Phase 20 — Validation des connexions (a planifier)

---

## Developpement en Cours

- **Fonctionnalite actuelle :** Blueprint Editor stable — toutes les operations undo/redo fonctionnelles
- **Objectif immediat :** Planifier et specifier Phase 20 — validation des connexions (cycles, types compatibles, max children)
- **Blocages connus :** Aucun

---

## Composants Actifs

- **Modules touches :** BlueprintEditor (stable), VisualScriptEditorPanel (stable)
- **Fichiers modifies (Phase 19) :**
  - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — fix drag tracking (snapshot-at-click)
- **Dependencies :** `UndoRedoStack.h/.cpp`, `MoveNodeCommand` — stables, aucune modification requise

---

## Decisions Recentes

- **2026-03-13 (Phase 19)** : Remplacement du bloc drag tracking par une approche snapshot-at-click.
  - Au `IsMouseClicked(Left)` : snapshot de toutes les positions dans `m_nodeDragStartPositions`
  - Pendant `IsMouseDown(Left)` : mise a jour live de `eNode.posX/Y`
  - Au `IsMouseReleased(Left)` : creation du `MoveNodeCommand` si delta > 1px
  - Resultat : logs `[VSEditor] Drag start node #X` et `[VSEditor] MoveNodeCommand pushed` fonctionnels

---

## Notes Techniques Importantes

- **Approche snapshot** : La position de depart est capturee une seule fois au clic souris, avant tout mouvement.
- **Flag `m_justPerformedUndoRedo`** : protege le bloc drag contre les faux declenchements apres undo/redo.
- **`m_nodeDragStartPositions`** : snapshote TOUTES les positions a `IsMouseClicked`, pas seulement les nodes qui bougent.

---

## Prochaines Etapes

1. Definir Phase 20 : validation des connexions (cycles, types, max children)
2. Phase 21 : edition avancee des parametres de nodes (panel Properties)
3. Phase 22 : templates BT preconfigures (Empty, Patrol, Combat...)

---

**Last Updated**: 2026-03-13
**Next Review**: Debut Phase 20
