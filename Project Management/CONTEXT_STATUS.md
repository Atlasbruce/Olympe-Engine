# Olympe Engine — Etat des Lieux

**Date :** 2026-03-16 14:48:07 UTC

---

## 🚧 Phase 23-B.5 — Visual Script Editor Polish (EN COURS — BLOQUÉE)

**Statut :** 🟠 IN PROGRESS — Bloquée par Issue [#414](https://github.com/Atlasbruce/Olympe-Engine/issues/414)  
**PR Référence :** Post-merge PR #408  
**Avancement :** ~20%

### Bugs Actifs (Issue #414)

- ❌ **BUG-024** (P1) — Type Filtering Missing in Variable Dropdowns → noeuds Switch voient toutes les variables
- ❌ **BUG-025** (P1) — Const Value Not Persisted on Save → valeurs constantes perdues au reload
- ❌ **BUG-026** (P1) — Save Button Inconsistent Behavior → bouton Save ≠ Ctrl+S
- ❌ **BUG-027** (P2) — Dropdown Lists Not Filtering by Operator Type → UX dégradée

### Composants Bloqués

- Condition Editor : operandes Const inutilisables (BUG-025)
- Switch Node : sélection de variable non filtrée (BUG-024)
- Fiabilité Save : comportement non déterministe (BUG-026)

---

## Composants Fonctionnels ✅

- **Blueprint Editor — Parameter Dropdowns & Registries (Phase 22-C)**
  - 5 registries centralisees (AtomicTask, Condition, BBVar, Operator, Master)
  - Dropdowns guides pour tous les parametres
  - ParameterBindingType etendu (7 types)
  - 32+ tests passants

- **Blueprint Editor — VSGraphVerifier (Phase 21-A + 21-B)**
  - 22 regles de validation globales (E001-E009, E020-E025, W001-W004, W010-W011, I001)
  - Panel scrollable d'issues + badge toolbar + auto-verify + navigation noeud

- **Blueprint Editor — VSSequence/Switch Dynamic Pins (Phase 21-D PR #382)**
  - Boutons [+]/[-] pour ajouter/retirer pins exec out
  - AddExecPinCommand + RemoveExecPinCommand (Undo/Redo complet)
  - 25+ tests passants

- **Blueprint Editor — VSSwitch Enhanced (Phase 22-A PR #384)**
  - Labels personnalises par case
  - Alignement droite des pin labels
  - Runtime variable display
  - 10/10 tests passants

- **Blueprint Editor — Add Node** : Drag & drop depuis palette -> AddNodeCommand (undoable)
- **Blueprint Editor — Delete Node** : Touche Delete / menu contextuel -> DeleteNodeCommand (undoable)
- **Blueprint Editor — Move Node** : Drag sur canvas -> MoveNodeCommand (undoable)
- **Blueprint Editor — Add Exec Link** : Drag pin a pin -> AddConnectionCommand (undoable)
- **Blueprint Editor — Add Data Link** : Drag pin donnees -> AddDataConnectionCommand (undoable)
- **Blueprint Editor — Delete Link** : Ctrl+click ou menu contextuel -> DeleteLinkCommand (undoable)
- **Blueprint Editor — Undo/Redo** : Ctrl+Z / Ctrl+Y via PerformUndo() / PerformRedo() — 100% fonctionnel
- **Blueprint Editor — Save/Load** : Serialisation JSON avec positions grid-space (BUG-003/004 FIXED)
- **Blueprint Editor — Context Menus** : Node, Link, Canvas (right-click)
- **Blueprint Editor — Connection Validation** : VSConnectionValidator bloque self-loops, duplicate pins, cycles
- **Blueprint Editor — Properties Undo/Redo** : EditNodePropertyCommand + commit-on-release pour tous les champs
- **Blueprint Editor — Inline Node Display** : Parametres cles affiches directement sur le canvas
- **Blueprint Editor — Blackboard Panel** : Default value editor (Bool, Int, Float, String); Vector/EntityID read-only
- **Blueprint Editor — Resizable Properties Panel** : Drag handle entre canvas et panel droite
- **Blueprint Editor — Type-Filtered Dropdowns** : GetVariablesByType() utility
- **ECS System** : Composants auto-enregistres
- **AI System** : Behavior Tree + Debugger
- **Animation System** : Animation Editor standalone

---

## Problemes Connus

**Phase 23-B.5 — 4 bugs actifs (Issue #414) :**

- ⚠️ **BUG-024** (P1) — Type filtering missing in variable dropdowns (Switch + Condition nodes)
- ⚠️ **BUG-025** (P1) — Const value not persisted on save (data loss risk)
- ⚠️ **BUG-026** (P1) — Save button inconsistent with Ctrl+S (use Ctrl+S as workaround)
- ⚠️ **BUG-027** (P2) — Dropdown lists not filtering by operator type (UX issue)

Voir [BUG_REGISTRY.md](./BugTracking/BUG_REGISTRY.md) pour les fiches détaillées.

---

## Composants En Developpement

- **Blueprint Editor — Phase 23-B.5 (BLOQUÉE)** : Visual Script Editor Polish
  - Fix BUG-024 : type filtering dans les dropdowns variables
  - Fix BUG-025 : persistance valeurs constantes
  - Fix BUG-026 : unification Save button + Ctrl+S
  - Fix BUG-027 : filtrage dropdown par type d'opérateur
  - Effort estimé : ~1 semaine — Issue #414

---

## Problemes Connus & Resolus

### BUG-001 ✅ RESOLVED (PR #387)
- **Severite :** P0 CRITICAL
- **Issue :** Crash on Blackboard Save (abort())
- **Root Causes :** 3 identified (VariableType::None, empty key, buffer corruption)
- **Solution :** Pre-save validation + safe init + warning UX
- **Status :** MERGED 2026-03-15 15:30:00 UTC
- **Regression Tests :** 5/5 passing

### BUG-024 / BUG-025 / BUG-026 / BUG-027 🔴 OPEN (Issue #414)
- **Severite :** P1/P2 — post-PR #408
- **Issue :** Condition editor UX blockers (type filtering, const persistence, save consistency)
- **Status :** OPEN — fixs planifiés Phase 23-B.5
- Voir [BUG_REGISTRY.md](./BugTracking/BUG_REGISTRY.md) pour les fiches détaillées

### Aucun autre P0 connu

---

## Progression Globale

- **Architecture globale :** ~82% (core engine stable)
- **Blueprint Editor :** ~96% (22 regles de validation, dropdowns guides, dynamic pins, Switch enhanced, BB crash fixed — bloqué sur condition editor polish par Issue #414)
- **ECS System :** ~85% (fonctionnel, extensions possibles)
- **AI System :** ~75% (BT + debugger, extensions pathfinding)

---

## Charge Contextuelle

- **Conversations actives :** 1 (PM system V2 + coherence post-hotfix)
- **Sujets non archives :** Phase 23-B spec (a implementer)
- **Derniere purge :** 2026-03-15 15:45:00 UTC

---

**Last Updated**: 2026-03-16 14:48:07 UTC
