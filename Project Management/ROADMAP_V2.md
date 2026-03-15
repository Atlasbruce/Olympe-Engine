# Olympe Engine — Roadmap V2

**Date :** 2026-03-15  
**Auteur :** @Atlasbruce  
**Derniere mise a jour :** 2026-03-15 15:45:00 UTC  
**Statut :** Planification active — Phase 23-B SPEC DONE (P1) + Phase 21-C QUEUED (P1)

---

## Vue d'ensemble

La Roadmap V2 definit les initiatives majeures post-Phase 20 pour le Blueprint Editor et l'engine.

| Initiative | Titre | Priorite | Statut |
|---|---|---|---|
| A | Graph Verification System (GVS) | P0 | Phase 21-A/B ✅ DONE — 21-C NEXT |
| E | Dynamic Pins Sequence/Switch (UX) | P0 | Phase 21-D ✅ DONE (PR #382) |
| B | Design & Icons Font Awesome | P2 | En attente spec design |
| C | Diversification Types de Graphes | P2 | Phase 23+ |
| D | Runtime Execution & Debugger | P3 | Phase 24+ |

---

## Initiative A — Graph Verification System (GVS)

### Contexte

Le VSConnectionValidator actuel valide uniquement les connexions individuelles (self-loop, duplicate pin, cycle DFS).
L'Initiative A implemente un validateur/compilateur global de graphe capable d'analyser la totalite
du graphe et de produire des messages d'erreur/warning exploitables dans l'editeur.

### Spec de reference

Fournie par @Atlasbruce le 2026-03-14.

### Phase 21-A — VSGraphVerifier stateless (TERMINEE — PR #380)

**Feature Context :** [feature_context_21_A.md](./Features/feature_context_21_A.md)

Nouveaux fichiers :
- Source/BlueprintEditor/VSGraphVerifier.h
- Source/BlueprintEditor/VSGraphVerifier.cpp
- Tests/BlueprintEditor/Phase21ATest.cpp
- Mise a jour CMakeLists.txt

14 regles implementees :

| ID | Severite | Regle |
|---|---|---|
| E001 | ERROR | Un seul EntryPoint requis |
| E002 | ERROR | Noeud dangling |
| E003 | ERROR | Cycle exec detecte (DFS) |
| E004 | ERROR | SubGraph circulaire |
| E005 | ERROR | Connexion Exec-Data ou Data-Exec |
| E006 | ERROR | Types de data pins incompatibles |
| E007 | ERROR | Direction de pin inversee |
| E008 | ERROR | Cle Blackboard inconnue |
| E009 | ERROR | Type Blackboard incompatible |
| W001 | WARNING | AtomicTask avec AtomicTaskID vide |
| W002 | WARNING | Delay avec DelaySeconds <= 0 |
| W003 | WARNING | SubGraph avec SubGraphPath vide |
| W004 | WARNING | MathOp avec MathOperator vide |
| I001 | INFO | Noeud non atteignable depuis EntryPoint |

### Phase 21-B — Panel Validation UI (TERMINEE — PR #381)

**Feature Context :** [feature_context_21_B.md](./Features/feature_context_21_B.md)

Fonctionnalites :
- Panel scrollable listant tous les VSVerificationIssue avec icone couleur
- Click sur une issue -> selection du node fautif dans le canvas
- Badge rouge sur toolbar si des erreurs sont presentes
- Toggle Auto-verify : re-declenche Verify() apres chaque PushCommand

### Phase 21-C — Integration pre-save + pre-exec

Pre-save : Dialog de confirmation si erreurs presentes.
Pre-exec : Blocage si ERROR, avertissement log si WARNING seulement.

---

## Initiative E — Dynamic Pins Sequence/Switch (UX) — TERMINEE (PR #382)

**Feature Context :** [feature_context_21_D.md](./Features/feature_context_21_D.md)

### Contexte

Les nodes VSSequence et VSSwitch doivent permettre a l'utilisateur d'ajouter et
retirer des pins exec out dynamiquement depuis le canvas, avec un rendu UX
coherent (bouton [+] en bas, [-] par pin), et une couverture Undo/Redo complete.

### Spec validee par @Atlasbruce — 2026-03-14

#### Comportement UX

| Element | Position | Tooltip |
|---|---|---|
| Bouton [+] | Toujours EN DESSOUS du dernier pin exec out | "Add Execution Output" |
| Bouton [-] | Inline a droite de chaque pin exec out (sauf pin de base) | "Remove Execution Output" |

**Regles :**
- Le pin de base (Out pour Sequence, Case_0 pour Switch) n'est PAS supprimable (pas de bouton [-])
- L'ajout cree un nouveau pin nomme automatiquement : Out_1, Out_2... (Sequence) ou Case_1, Case_2... (Switch)
- La suppression d'un pin connecte retire automatiquement la liaison (equivalente a DeleteLinkCommand)
- Apres add/remove : `RebuildLinks()` appele pour synchroniser ImNodes

#### Couverture Undo/Redo

| Action | Command | Undo | Redo |
|---|---|---|---|
| Add pin exec | `AddExecPinCommand` | Retire le pin ajoute + retire lien si cree | Re-ajoute le pin |
| Remove pin exec | `RemoveExecPinCommand` | Re-ajoute le pin + restaure lien si existait | Retire le pin + lien |

**RemoveExecPinCommand stocke :**
- `nodeID`, `pinName`, `pinIndex` (position dans la liste)
- `linkedTargetNodeID`, `linkedTargetPinName` (si pin avait un lien, -1 sinon)

#### Fichiers impactes

| Fichier | Modification |
|---|---|
| `Source/BlueprintEditor/VisualScriptNodeRenderer.h/.cpp` | Rendu [+] apres pins out, [-] inline par pin |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | Logique add/remove + appel commands |
| `Source/BlueprintEditor/UndoRedoStack.h/.cpp` | AddExecPinCommand + RemoveExecPinCommand |
| `Tests/BlueprintEditor/Phase21DTest.cpp` | NOUVEAU — tests headless |
| `CMakeLists.txt` | Ajout cible OlympePhase21DTests |

#### Tests requis (Phase 21-D)

1. Add pin sur Sequence -> pin cree, undoable, redoable
2. Remove pin sans lien -> pin retire, undoable, redoable
3. Remove pin avec lien -> pin retire + lien retire, undo restaure les deux
4. Add puis Remove -> etat initial restaure apres double undo
5. Pin de base non supprimable (pas de bouton [-])

---

## Initiative B — Font Awesome Icons & Design

### Statut

En attente du document de specifications design de @Atlasbruce.

### Scope prevu

- Icones par type de node dans la palette et sur le canvas
- Icones pour les pins (exec = triangle, data = cercle colore)
- Icones dans le panel Properties et la toolbar
- Integration de la police FA comme texture ImGui (font .ttf embarquee, C++14 compatible)

---

## Initiative C — Diversification des Types de Graphes

Types de graphes prevus :

| Type | Description |
|---|---|
| AIGraph | Graphe comportemental AI (actuel) |
| AnimGraph | Transitions d animations |
| LevelScript | Scripts de niveau |
| CinematicGraph | Scenes cinematiques |
| MenuGraph | Definition de menus UI |
| GlobalRules | Regles de jeu globales |

Phases prevues :
- Phase 23-A : Infrastructure GraphType + AnimGraph
- Phase 23-B : LevelScript + CinematicGraph
- Phase 23-C : MenuGraph + GlobalRules

---

## Initiative D — Runtime Execution & Debugger Multi-Instances

Fonctionnalites prevues :
- Multi-instances : Plusieurs entites AI executent le meme graphe avec des Blackboards independants
- Visualisation read-only dans le Debugger avec node actif surbrillance par instance
- Breakpoints par instance
- Step-through

Phases prevues :
- Phase 24-A : GraphRuntimeInstance + GraphRuntimeManager (single-thread)
- Phase 24-B : Integration Debugger Panel multi-instances
- Phase 24-C : Thread safety + performance

---

## Phase 22 — Node Enhancement & UI Improvements

### Phase 22-A — Switch Node Enhancement (COMPLETED ✅)

**Feature Context :** [feature_context_22_A.md](./Features/feature_context_22_A.md)

**Status:** PR #384 MERGED  
**Date Completed:** 2026-03-14 18:46:14 UTC  
**Changes:**
- Dynamic switch case labels ("Combat", "Patrol" etc.)
- Right-aligned pin labels (fixes center-alignment with long names)
- Runtime variable display (var: bb_ai_mode = 5)
- No backward compatibility (Switch v1 → v2)

**Tests:** 10/10 passing ✅

---

### Phase 22-C — Parameter Dropdowns & Registries (COMPLETED ✅)

**Feature Context :** [feature_context_22_C.md](./Features/feature_context_22_C.md)

**Status:** PR #386 MERGED  
**Priority:** P0 CRITICAL (completed)  
**Date Started:** 2026-03-14 22:45:00 UTC  
**Date Completed:** 2026-03-14 21:59:19 UTC

**Why This Phase?** 
Users had no way to discover available:
- AtomicTask IDs
- Condition types
- Blackboard variables
- Math/Comparison operators
- SubGraph paths

Result: Impossible to author graphs without documentation/guessing.

**Solution:**
- 5 centralized registries (AtomicTask, Condition, BBVar, Operator, Master)
- Dropdown UI for all parameters (no more text fields)
- Extended ParameterBindingType (6 types)
- Intelligent filtering and categorization

**Deliverables:**
- 10 new source files (registries + UI)
- 2 new Undo/Redo commands
- Updated JSON serialization (v4)
- 8 new validation rules (E020-E025, W010-W011)
- 30+ test cases

---

### Phase 22-B — Font Awesome Icons & Design (DEFERRED → P2)

**Status:** Awaiting design specifications  
**Priority:** P2 (MEDIUM)  
**Estimated Duration:** ~2 sessions (after spec)  
**Blocker:** Design spec document from @Atlasbruce

**Scope:**
- Icons by node type in palette and canvas
- Icons for pins (exec=triangle, data=circle)
- Icons in Properties panel and toolbar
- FontAwesome TTF embedded, C++14 compatible

## Phase 23-B — Full Blackboard Properties (SPEC FINALIZED — Implementation Pending)

**Feature Context:** (in progress — PR #388)

**Status:** ⏳ SPEC DONE → CODING
**Priority:** P1
**Date Spec Finalized:** 2026-03-15 15:30:00 UTC

**Why This Phase?**
Users starting a new graph face an empty Blackboard with no guidance on available variables.
Instead of a discovery panel, variables are pre-declared via a JSON config preset file that populates
the "Available Variables" list at the top of the Blackboard panel.

**Solution:**
- JSON config preset file listing all available blackboard variable templates (name, type, default)
- 2-section Blackboard panel:
  - Section 1 (top): Available Variables (from preset config, read-only reference)
  - Section 2 (bottom): Declared/Local Variables (created by user, editable)
- [+ Create Local Variable] button to instantiate a new local variable
- 15 variables pre-declared in config preset
- 15 headless tests

**Deliverables:**
- BlackboardPresetConfig (JSON loader + registry)
- Refactored RenderBlackboardPanel() → 2-section layout
- [+ Create Local Variable] dialog
- Config file: Assets/Config/BlackboardPreset.json

---

| Phase | Initiative | Estimation | Statut | Priority |
|---|---|---|---|---|
| 21-A | GVS — VSGraphVerifier | ~1 session | ✅ COMPLETED (PR #380) | — |
| 21-B | GVS — Panel Validation UI | ~1 session | ✅ COMPLETED (PR #381) | — |
| 21-D | Dynamic Pins Seq/Switch | ~1 session | ✅ COMPLETED (PR #382) | — |
| 22-A | Switch Node Enhancement | ~1 session | ✅ COMPLETED (PR #384) | — |
| 22-C | Parameter Dropdowns & Registries | ~1 session | ✅ COMPLETED (PR #386) | — |
| **HOTFIX** | **P0 Crash Blackboard Save (BUG-001)** | **~2h** | **✅ COMPLETED (PR #387)** | **P0** |
| **21-C** | **GVS — Pre-save/Pre-exec Validation** | **~0.5 session** | **⏳ QUEUED** | **P1** |
| **23-B** | **Full Blackboard Properties** | **2-2.5 sessions** | **⏳ SPEC DONE → CODING** | **P1** |
| 22-B | Font Awesome Icons & Design | ~2 sessions | ⏳ Deferred (spec needed) | P2 |
| 23-A | AnimGraph Infrastructure | ~2 sessions | Planned | — |
| 23-C | LevelScript + Cinematic | ~2 sessions | Planned | — |
| 24-A | Runtime Instance | ~2 sessions | Future | — |
| 24-B | Debugger Multi-instances | ~2 sessions | Future | — |
| 24-C | Thread Safety | ~1 session | Future | — |

---

**Last Updated**: 2026-03-15 15:45:00 UTC
