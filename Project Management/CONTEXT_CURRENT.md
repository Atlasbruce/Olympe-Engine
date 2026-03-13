# 🎯 CONTEXT CURRENT — Session Active

**Date**: 2026-03-13  
**User**: @Atlasbruce  
**Status**: 🟡 **Phase 18 — Fix Drag Undo/Redo (in progress)**

---

## 🔥 Développement en Cours

- **Fonctionnalité actuelle :** Fix Undo/Redo pour le drag de nodes dans le Blueprint Editor
- **Objectif immédiat :** Corriger la détection de drag dans `VisualScriptEditorPanel` pour que Ctrl+Z restaure correctement la position d'un node déplacé
- **Blocages connus :** Aucun — fix identifié et implémenté (Phase 18 PR en cours)

---

## 🧩 Composants Actifs

- **Modules touchés :** `VisualScriptEditorPanel` (Blueprint Editor)
- **Fichiers modifiés :**
  - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp`
  - `Documentation/GRAPH_EDITING_FLOW.md`
  - `Project Management/CONTEXT_CURRENT.md`
  - `Project Management/CONTEXT_STATUS.md` (créé)
  - `Project Management/CONTEXT_ARCHIVE.md` (créé)
  - `Project Management/CONTEXT_MEMORY_LOG.md` (créé)
- **Dépendances :** `UndoRedoStack.h/.cpp` (MoveNodeCommand — aucune modification requise)

---

## 💡 Décisions Récentes

- **2026-03-13** : Suppression du bloc de pré-population de `m_nodeDragStartPositions` dans `LoadTemplate()` — ce bloc empêchait la détection correcte du début de drag car la clé existait déjà, court-circuitant le guard `find == end()`.
- **2026-03-13** : Ajout de logs `SYSTEM_LOG` pour tracer drag start, commit de commande et restauration via `SyncEditorNodesFromTemplate()`.

---

## 📝 Notes Techniques Importantes

- `eNode.posX/Y` est maintenu à jour chaque frame pendant `mouseDown == true` — il représente la position du frame précédent, i.e. la position **avant** le delta de déplacement courant. C'est la valeur correcte à utiliser comme "start of drag".
- `m_nodeDragStartPositions` doit rester **vide** après `LoadTemplate()`. Les entrées sont créées à la première détection de mouvement et effacées à la fin du drag (mouse release).
- `PerformUndo/Redo` appellent déjà `m_nodeDragStartPositions.clear()` — correct.
- `SyncEditorNodesFromTemplate()` appelle déjà `m_nodeDragStartPositions.clear()` — correct.

---

## ⏭️ Prochaines Étapes

1. Merger la PR Phase 18
2. Valider manuellement : ouvrir un graphe, déplacer un node, Ctrl+Z, vérifier que le node revient à sa position initiale
3. Vérifier les logs : `[VSEditor] Drag start node #X at (...)` → `[VSEditor] MoveNodeCommand pushed node #X (...) -> (...) [UNDOABLE]`
4. Sprint suivant : Phase 19 — fonctionnalités UI/UX à définir

---

**Last Updated**: 2026-03-13  
**Next Review**: After Phase 18 PR merge