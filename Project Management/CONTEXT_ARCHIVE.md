# Context Archive — Historical Phases

## Phase 22-A — Switch Node Enhancement

**Date Completed:** 2026-03-14  
**PR:** #384  
**Status:** ✅ MERGED  

### Summary
Enhanced the VSSwitch node with dynamic case labels and improved UI:
- Cases now support custom labels ("Combat", "Patrol" etc. instead of "Case_5")
- Pin labels right-aligned dynamically (no center-alignment issues)
- Runtime variable display (e.g., "var: bb_ai_mode = 5")
- Full Undo/Redo support
- Validation rules E010, E011, E012

### Test Coverage
10/10 tests passing

### Impact
Switch nodes now properly editable with intuitive UX.

---

## Phase 21-D — Dynamic Pins for Sequence/Switch

**Status:** ✅ COMPLETED (referenced in 22-A dependencies)

Dynamic execution pins on control flow nodes.

---

## Phase 21-A — Graph Verifier (VSGraphVerifier)

**Status:** ✅ COMPLETED (PR #380)

Graph validation system with error/warning registry.

---

# 📦 Olympe Engine — Contexte Archivé

**Dernière mise à jour :** 2026-03-13

---

## ✅ Fonctionnalités Terminées

### Phase 12 — Delete Nodes/Links + Context Menus
- **Objectif :** Permettre suppression de nodes/links avec undo, menus contextuels
- **Implémentation :** `DeleteNodeCommand`, `DeleteLinkCommand`, menus right-click (node, link, canvas), touche Delete, F9 breakpoints
- **Fichiers concernés :** `VisualScriptEditorPanel.cpp`, `UndoRedoStack.h/.cpp`
- **Archivé le :** 2026-03-13
- **Raison :** Terminé — PR #361 mergée

### Phase 13 — Exec/Data Connections + Fix Save/Load Positions
- **Objectif :** Undo/redo pour liens exec et données, persistance des positions de nodes
- **Implémentation :** `AddConnectionCommand`, `AddDataConnectionCommand`, `__posX/__posY` dans Parameters
- **Fichiers concernés :** `VisualScriptEditorPanel.cpp`, `UndoRedoStack.h/.cpp`
- **Archivé le :** 2026-03-13
- **Raison :** Terminé

### Phase 14 — Fix Undo/Redo Completeness
- **Objectif :** Corriger 4 bugs undo/redo interconnectés
- **Implémentation :**
  - Fix 1 : `RebuildLinks()` dans `SyncEditorNodesFromTemplate()` → élimine ghost links
  - Fix 2 : Pré-population `m_nodeDragStartPositions` dans `LoadTemplate()` (remplacé Phase 18)
  - Fix 3 : `m_needsPositionSync` déclenche `SetNodeEditorSpacePos` après undo/redo
  - Fix 4 : `m_skipPositionSyncNextFrame` évite écrasement positions correctes
- **Fichiers concernés :** `VisualScriptEditorPanel.cpp`, `PHASE14_UNDO_REDO_FIXES.md`
- **Archivé le :** 2026-03-13
- **Raison :** Terminé (Fix 2 partiellement remplacé par Phase 18)

### Phase 15 — PerformUndo/Redo Centralisés + Context Menus
- **Objectif :** Centraliser les effets secondaires undo/redo, polir les menus contextuels
- **Implémentation :** `PerformUndo()`, `PerformRedo()` avec `SyncEditorNodesFromTemplate()` + `RebuildLinks()` + `m_justPerformedUndoRedo`
- **Fichiers concernés :** `VisualScriptEditorPanel.cpp`
- **Archivé le :** 2026-03-13
- **Raison :** Terminé

### Phase 16 — Blueprint Files Browser
- **Objectif :** Navigateur de fichiers .ats dans l'éditeur
- **Implémentation :** `EditorLayout::RenderBlueprintFileBrowser()`, multi-onglets
- **Fichiers concernés :** `EditorLayout.cpp`
- **Archivé le :** 2026-03-13
- **Raison :** Terminé

### Phase 17 — ImNodes Context Isolation
- **Objectif :** Isolation des contextes ImNodes par onglet (multi-tab support)
- **Implémentation :** `m_imnodesContext` par `VisualScriptEditorPanel`, `EditorContextSet()` au début de `RenderCanvas()`
- **Fichiers concernés :** `VisualScriptEditorPanel.h/.cpp`
- **Archivé le :** 2026-03-13
- **Raison :** Terminé

---

## 🗑️ Décisions Obsolètes

### FIX 2 Phase 14 — Pré-population de m_nodeDragStartPositions dans LoadTemplate()
- **Contexte :** Évitait la position (0,0) au premier drag après chargement d'un fichier
- **Remplacé par :** Phase 18 — utilisation de `eNode.posX/Y` comme position de départ (correct car maintenu à jour chaque frame pendant mouseDown)
- **Archivé le :** 2026-03-13

---

## 🧪 Expérimentations Abandonnées

- Aucune expérimentation abandonnée à ce jour.

### Phase 18 — Fix LoadTemplate + Commentaires
- **Objectif :** Nettoyer `LoadTemplate()` — supprimer le bloc de pre-population de `m_nodeDragStartPositions`
- **Implementation :** Suppression du bloc "FIX 2" dans `LoadTemplate()`, ajout de commentaires explicatifs
- **Fichiers concernes :** `VisualScriptEditorPanel.cpp`
- **Archive le :** 2026-03-13
- **Raison :** Termine — PR #373 mergee

### Phase 19 — Fix Drag Detection (Snapshot-at-Click)
- **Objectif :** Corriger la detection du drag de nodes pour que `MoveNodeCommand` soit pousse sur l'undo stack
- **Root cause :** Guard `posChanged` toujours false car `eNode.posX/Y` mis a jour chaque frame pendant `mouseDown`
- **Implementation :**
  - `IsMouseClicked(Left)` : snapshot de toutes les positions dans `m_nodeDragStartPositions`
  - `IsMouseDown(Left)` : mise a jour live de `eNode.posX/Y` pour Save()
  - `IsMouseReleased(Left)` : creation de `MoveNodeCommand` si delta > 1px, clear du snapshot
  - Logs `[VSEditor] Drag start node #N` et `[VSEditor] MoveNodeCommand pushed` operationnels
- **Fichiers concernes :** `VisualScriptEditorPanel.cpp`
- **Archive le :** 2026-03-13
- **Raison :** Termine — PR #374 mergee. Undo/redo 100% fonctionnel.
