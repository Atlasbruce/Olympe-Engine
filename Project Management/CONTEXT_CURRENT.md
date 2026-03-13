# 🎯 CONTEXT CURRENT — Session Active

**Date**: 2026-03-13  
**User**: @Atlasbruce  
**Status**: 🟡 **Phase 19 — Fix Drag Detection for MoveNodeCommand (in progress)**

---

## 🔥 Développement en Cours

- **Fonctionnalité actuelle :** Fix détection de drag dans `VisualScriptEditorPanel` — approche snapshot-at-click (Phase 19)
- **Objectif immédiat :** Remplacer le bloc `posChanged && mouseDown` par l'approche `IsMouseClicked` snapshot + `IsMouseReleased` commit pour que `MoveNodeCommand` soit bien poussé sur l'undo stack après chaque drag de node.
- **Blocages connus :** Aucun — fix implémenté (Phase 19 PR en cours)

---

## 🧩 Composants Actifs

- **Modules touchés :** `VisualScriptEditorPanel` (Blueprint Editor)
- **Fichiers modifiés :**
  - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp`
  - `Project Management/CONTEXT_CURRENT.md`
  - `Project Management/CONTEXT_STATUS.md`
  - `Project Management/CONTEXT_MEMORY_LOG.md`
- **Dépendances :** `UndoRedoStack.h/.cpp` (MoveNodeCommand — aucune modification requise)

---

## 💡 Décisions Récentes

- **2026-03-13** : Phase 19 — Remplacement du bloc `posChanged && mouseDown` par l'approche snapshot-at-click. Root cause : `eNode.posX` est mis à jour chaque frame par `SyncNodePositionsFromImNodes()`, donc `posChanged` est toujours false au moment où `mouseDown` est vrai. Nouveau flux : snapshot au clic, sync pendant mouseDown, commit au release.
- **2026-03-13** : Phase 18 — Suppression du bloc de pré-population de `m_nodeDragStartPositions` dans `LoadTemplate()`.

---

## 📝 Notes Techniques Importantes

- **Phase 19 — Approche snapshot-at-click** :
  - `IsMouseClicked` : snapshot toutes les positions des nodes positionnés dans `m_nodeDragStartPositions`
  - `IsMouseDown` : sync `eNode.posX/Y` depuis ImNodes (live Save support)
  - `IsMouseReleased` : pour chaque entrée snapshot, push `MoveNodeCommand` si delta > 1px
- `m_nodeDragStartPositions` est vidé au clic et au release — jamais en dehors de ce bloc
- `m_justPerformedUndoRedo` guard préservé autour du bloc entier

---

## ⏭️ Prochaines Étapes

1. Merger la PR Phase 19
2. Valider manuellement : ouvrir un graphe, déplacer un node, vérifier le log `[VSEditor] Mouse clicked: snapshot N node positions`, puis `[VSEditor] MoveNodeCommand pushed node #X (...) -> (...) [UNDOABLE]`
3. Tester Ctrl+Z : le node doit revenir à sa position initiale
4. Sprint suivant : Phase 20 — fonctionnalités UI/UX à définir

---

**Last Updated**: 2026-03-13  
**Next Review**: After Phase 19 PR merge