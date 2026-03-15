# Recent Completions & Next Priority

**Derniere mise a jour :** 2026-03-15 22:17:00 UTC

---

## ✅ HOTFIX P0 — BUG-003 & BUG-004 (FIXED)

### BUG-003 — Node Positions Offset on Save (ImNodes Viewport)
- **Sévérité :** P0 CRITICAL — Offset constant (-24px X, +114px Y) sur toutes les positions après Save
- **Root cause :** `SyncNodePositionsFromImNodes()` utilisait `GetNodeEditorSpacePos()` (= Origin + Panning) au lieu de `GetNodeGridSpacePos()` (= Origin pur, pan-indépendant)
- **Statut :** ✅ FIXED (PR #401)
- **Fix :**
  - `SyncNodePositionsFromImNodes()` : `GetNodeEditorSpacePos` → `GetNodeGridSpacePos`
  - `PerformUndo/Redo()` : `SetNodeEditorSpacePos` → `SetNodeGridSpacePos`
  - `RenderCanvas()` position sync : `SetNodeEditorSpacePos` → `SetNodeGridSpacePos`
  - `ResetViewportBeforeSave()` + `AfterSave()` ajoutés (safety measure + UX continuity)
  - `ScreenToCanvasPos()` utilitaire de conversion coordonnées ajouté
  - 8 regression tests (Phase23B2Test.cpp)

### BUG-004 — Crash on Load After Save
- **Sévérité :** P0 CRITICAL — Crash à `RebuildLinks()` après reload d'un graphe sauvegardé navigué
- **Root cause :** Causé par BUG-003 (double-offset lors de la restauration)
- **Statut :** ✅ FIXED (résolu par fix BUG-003)

---

## ✅ UX Enhancements (PR #401)

### Enhancement #1 — Vector Type Read-Only
- `RenderBlackboard()` : case `VariableType::Vector` → DragFloat3 disabled + label "(auto from entity position)"

### Enhancement #2 — EntityID Type Read-Only
- `RenderBlackboard()` : case `VariableType::EntityID` → InputInt disabled + label "(assigned at runtime)"

### Enhancement #3 — Type-Filtered Variable Dropdowns
- `GetVariablesByType()` utility ajouté (méthode statique VisualScriptEditorPanel)
- Utilisable pour filtrer les dropdowns variable par type attendu

---

## P1 — Phase 23-B.3 : Variable Value Assignment in Properties Panel (PLANNED ⏳)

- **Statut :** ⏳ PLANNED — Priorité P1
- **Description :** Affichage / édition de la valeur courante/par défaut d'une variable Blackboard dans le panel Properties quand un noeud GetBBValue/SetBBValue est sélectionné
- **Effort estimé :** ~0.5 session

---

## Completions Recentes ✅

### HOTFIX BUG-003/004 — Node Position Offset + Load Crash
- **PR :** #401 — MERGED
- **Date :** 2026-03-15 22:17:00 UTC
- Grid-space positions pour Save/Load (GetNodeGridSpacePos / SetNodeGridSpacePos)
- ResetViewportBeforeSave() + AfterSave()
- ScreenToCanvasPos() utilitaire
- Vector/EntityID read-only display
- GetVariablesByType() utility
- 8 regression tests Phase23B2Test.cpp

### Phase 23-B — Full Blackboard Properties + BUG-001/002
- **PR :** #400 — MERGED
- **Date :** 2026-03-15 17:30:00 UTC
- BlackboardVariablePresetRegistry singleton + JSON config
- 2-section Blackboard panel
- Default value editor (Bool, Int, Float, String)
- Resizable properties panel (drag handle)
- 18 tests Phase23BTest.cpp

### Phase 22-C — Parameter Dropdowns & Registries
- **PR :** [#386](https://github.com/Atlasbruce/Olympe-Engine/pull/386) — MERGED
- **Date de merge :** 2026-03-14 21:59:19 UTC
- 5 registries centralisees (AtomicTask, Condition, BBVar, Operator, Master)
- ParameterBindingType etendu (7 types)
- 32 tests passants
- 8 nouvelles regles VSGraphVerifier (E020-E025, W010-W011)

---

## Priorites Suivantes (Post-Merge #401)

**P1 :**
- Phase 23-B.3 — Variable Value Assignment in Properties Panel (~0.5 session)
- Phase 21-C — GVS Pre-save/Pre-exec Validation

**P2 :**
- Phase 22-B — Font Awesome Icons & Design (en attente spec design)
- Phase 23-A — AnimGraph Infrastructure

---

_Last updated: 2026-03-15 22:17:00 UTC_
