# 📜 Journal des Opérations Mémoire

---

## 2026-03-13 — IMPLEMENTATION Phase 19

**Fichier modifié :** `Source/BlueprintEditor/VisualScriptEditorPanel.cpp`
**Fonction :** `RenderCanvas()` — bloc drag tracking (après `EndNodeEditor()`)

**Root cause :** L'approche `posChanged && mouseDown` ne fonctionnait jamais car `SyncNodePositionsFromImNodes()` met à jour `eNode.posX/Y` chaque frame depuis les positions ImNodes. Ainsi, quand `mouseDown` est vrai, `eNode.posX` a déjà la valeur courante d'ImNodes → `posChanged` est toujours faux → `m_nodeDragStartPositions` n'est jamais peuplé → `MoveNodeCommand` n'est jamais poussé → Ctrl+Z inopérant après drag.

**Fix appliqué (Phase 19 — snapshot-at-click) :**
- `IsMouseClicked` : snapshot de toutes les positions des nodes positionnés dans `m_nodeDragStartPositions`
- `IsMouseDown` : mise à jour de `eNode.posX/Y` depuis ImNodes (support live Save)
- `IsMouseReleased` : pour chaque entrée snapshot, push `MoveNodeCommand` si delta > 1px; log `[VSEditor] MoveNodeCommand pushed node #N (...) -> (...) [UNDOABLE]` ou `[VSEditor] Node #N not moved (delta < 1px), skipping`

**Guard `m_justPerformedUndoRedo` préservé** autour du bloc entier.

**Compliance C++14 :**
- Itérateur explicite `std::unordered_map<int, std::pair<float,float> >::iterator`
- `static_cast<size_t>(...)` pour le log du nombre de snapshots
- Aucun emoji dans les logs, aucun `std::string_view`, aucun `auto& [k,v]`

---

## 2026-03-13 — ARCHIVAGE

**Élément archivé :** Phases 12 à 17 du Blueprint Editor
**Raison :** Terminées — fonctionnalités stables et mergées
**Destination :** `Project Management/CONTEXT_ARCHIVE.md`

**Éléments archivés :**
- Phase 12 : Delete Nodes/Links + Context Menus
- Phase 13 : Exec/Data Connections + Fix Save/Load Positions
- Phase 14 : Fix Undo/Redo Completeness (4 bugs)
- Phase 15 : PerformUndo/Redo Centralisés
- Phase 16 : Blueprint Files Browser
- Phase 17 : ImNodes Context Isolation

---

## 2026-03-13 — ÉTAT DES LIEUX

**Demandé par :** Système (initialisation fichiers mémoire Phase 18)
**Résumé fourni :** Blueprint Editor undo/redo fonctionnel pour toutes opérations sauf drag de nodes (bug pré-population `m_nodeDragStartPositions`). Fix identifié et implémenté (Phase 18 PR en cours). Aucun problème P0 connu après fix.

---

## 2026-03-13 — MISE À JOUR CONTEXT_CURRENT

**Raison :** Lancement Phase 18 — Fix Drag Undo/Redo
**Ancien statut :** CRITICAL BUG — Undo/Redo Non Fonctionnel (PR #361)
**Nouveau statut :** Phase 18 — Fix Drag Undo/Redo (in progress)
**Changements :**
- Suppression du contexte obsolète Phase 12 (déjà résolu)
- Ajout contexte Phase 18 avec root cause et fix identifié
