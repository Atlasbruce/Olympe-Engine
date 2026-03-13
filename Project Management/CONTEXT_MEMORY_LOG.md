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

---

## 2026-03-13 — IMPLEMENTATION Phase 19 (PR #374 mergee)

**Demande par :** @Atlasbruce
**Action :** Fix drag detection dans `RenderCanvas()` — approche snapshot-at-click
**Root cause resolue :** `eNode.posX/Y` mis a jour chaque frame pendant `mouseDown` rendait `posChanged` toujours false
**Fix :** Snapshot de toutes les positions a `IsMouseClicked`, commit de `MoveNodeCommand` a `IsMouseReleased`
**Resultat :** Undo/redo 100% fonctionnel pour toutes les operations du Blueprint Editor

---

## 2026-03-13 — ARCHIVAGE Phases 18-19

**Elements archives :** Phase 18 (LoadTemplate cleanup) et Phase 19 (drag detection fix)
**Raison :** Terminees — PRs #373 et #374 mergees
**Destination :** `Project Management/CONTEXT_ARCHIVE.md`

---

## 2026-03-13 — NETTOYAGE BASE DOCUMENTAIRE

**Action :** Deplacement des fichiers .md deprecies vers `docs/archive/`
**Fichiers deplaces :**
- `Documentation/ATS_VS_Phase2_RuntimeNotes.md`
- `Documentation/ATS_VS_Phase4_Complete.md`
- `Documentation/ATS_VS_Phase5_VisualEditor.md`
- `Documentation/ATS_Refactoring_Master_Plan.md`
- `Documentation/CONCEPTION-ATS-VISUAL-SCRIPTING-v2.md`
- `Documentation/Olympe_ATS_Editor_Spec.md`
- `Documentation/PHASE14_UNDO_REDO_FIXES.md`
**Raison :** Contenu obsolete — phases anterieures completees

---

## 2026-03-13 — IMPLEMENTATION Phase 20-B (PR ouverte)

**Demande par :** @Atlasbruce
**Action :** Ajout validation temps reel des connexions exec dans `RenderCanvas()`
**Architecture :** Classe stateless `VSConnectionValidator` extractee pour testabilite sans ImNodes/ImGui
**3 checks implantes :**
  - Check A : self-loop (srcNodeID == dstNodeID)
  - Check B : duplicate output pin (source pin deja connectee)
  - Check C : cycle detection via DFS iteratif sur adjacency list
**Fichiers crees :** `VSConnectionValidator.h/.cpp`, `Tests/BlueprintEditor/Phase20Test.cpp`
**Fichiers modifies :** `VisualScriptEditorPanel.h` (include), `VisualScriptEditorPanel.cpp` (guard ConnectExec), `CMakeLists.txt` (OlympePhase20Tests)
**Tests :** 4/4 passes (self-loop, duplicate, cycle, valid)
**Conformite :** C++14 strict, SYSTEM_LOG, namespace Olympe, pas d'emoji dans les logs
