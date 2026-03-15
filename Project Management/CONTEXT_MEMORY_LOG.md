# 📜 Journal des Opérations Mémoire

---

## 2026-03-15 22:17:00 UTC — HOTFIX BUG-003 & BUG-004 + UX ENHANCEMENTS

**Contexte :** PR #401 — HOTFIX Node Position Offset + Load Crash (ImNodes Viewport Fix)

**BUG-003 Résolu :**
- Root cause : `SyncNodePositionsFromImNodes()` utilisait `GetNodeEditorSpacePos()` (= `Origin + Panning`) au lieu de `GetNodeGridSpacePos()` (= `Origin` pur, pan-indépendant)
- Fix : `GetNodeEditorSpacePos` → `GetNodeGridSpacePos` dans `SyncNodePositionsFromImNodes()`
- Fix : `SetNodeEditorSpacePos` → `SetNodeGridSpacePos` pour tous les restore de positions (Undo/Redo, load, focus)
- Fix safety : `ResetViewportBeforeSave()` + `AfterSave()` (ceinture + bretelles)
- Utilitaire : `ScreenToCanvasPos(ImVec2)` ajouté

**BUG-004 Résolu :**
- Root cause : Causé par BUG-003 (double-offset viewport lors de la restauration)
- Fix : résolu automatiquement par la correction BUG-003

**UX Enhancements :**
- `VariableType::Vector` : affichage read-only `DragFloat3` + label "(auto from entity position)"
- `VariableType::EntityID` : affichage read-only `InputInt` + label "(assigned at runtime)"
- `GetVariablesByType()` : méthode statique utilitaire pour filtrer variables par type

**Tests ajoutés :**
- `Tests/BlueprintEditor/Phase23B2Test.cpp` : 8 tests headless (8/8 ✅)
- `OlympePhase23B2Tests` target ajouté dans `CMakeLists.txt`

**PM mis à jour :**
- `BUG_REGISTRY.md` : BUG-001/002/003/004 archivés FIXED, tableau de bord vidé
- `ROADMAP_V2.md` : HOTFIX BUG-003/004 + Phase 23-B.3 Variable Assignment + calendrier
- `CONTEXT_CURRENT.md` : statut complet des fixes + prochaines priorités
- `CONTEXT_STATUS.md` : composants fonctionnels mis à jour

**Métriques :**
- Bugs P0 actifs : 4 → 0
- Tests total : 23 → 31 (+ 8 Phase23B2Test)
- Fichiers modifiés : 6 (VisualScriptEditorPanel.h/.cpp, CMakeLists.txt, 3 PM docs)
- Fichiers créés : 1 (Phase23B2Test.cpp)

---

## 2026-03-15 15:30:00 UTC — BUG-001 HOTFIX + Phase 23-B IMPLEMENTATION

**Contexte :** PR Phase 23-B — Full Blackboard Properties + BUG-001 P0 Hotfix

**BUG-001 Résolu :**
- Root cause : `VariableType::None` non géré dans `SerializeAndWrite()` → abort() au save
- Fix : skip des entrées invalides (clé vide ou type None) + log SYSTEM_LOG
- Init sécurisé : `Key="NewVariable"`, `Type=Int` (était `"newKey"` + Float)
- Warning UX : badge rouge dans `RenderBlackboard()` si entrées invalides
- 5 tests régression ajoutés (Phase23BTest.cpp)

**Phase 23-B Implémentée :**
- `BlackboardVariablePresetRegistry` : singleton C++14, O(1) lookup, JSON loading
- `Assets/Config/BlackboardVariablePresets.json` : 15 variables, 5 catégories
- `Tests/BlueprintEditor/Phase23BTest.cpp` : 18 tests headless (18/18 ✅)
- `CMakeLists.txt` : ajout target `OlympePhase23BTests`

**Métriques :**
- Fichiers créés : 5 (JSON config, registry .h/.cpp, test, feature context)
- Fichiers modifiés : 4 (VisualScriptEditorPanel.cpp, CMakeLists.txt, ROADMAP_V2.md, CONTEXT_CURRENT.md)
- Tests : 18/18 passants
- Status : IN PROGRESS (PR en cours)

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

---

## 2026-03-14 — NOUVELLE SPEC Phase 20-D (Dynamic Pins UX)

**Demande par :** @Atlasbruce
**Contexte :** Screenshot fourni montrant le comportement attendu pour les nodes Sequence/Switch
**Spec documentee :**
  - Bouton [+] (tooltip "Add Execution Output") TOUJOURS positionne EN DESSOUS du dernier pin exec out
  - Bouton [-] (tooltip "Remove Execution Output") inline a cote de chaque pin exec out ajoute
  - Suppression d'un pin connecte retire automatiquement la liaison (DeleteLinkCommand embarque)
  - Add -> `AddExecPinCommand` (undoable), Remove -> `RemoveExecPinCommand` (undoable)
**Fichiers impactes :**
  - `VisualScriptNodeRenderer.h/.cpp` — ordre rendu [+]/[-]
  - `VisualScriptEditorPanel.cpp` — logique Add/Remove + Undo/Redo
  - `UndoRedoStack.h/.cpp` — nouveaux commands AddExecPinCommand, RemoveExecPinCommand
  - `Tests/BlueprintEditor/Phase20DTest.cpp` — NOUVEAU
**Destination roadmap :** Initiative E — Phase 20-D

---

## 2026-03-14 — NOUVELLE SPEC Phase 20-E (Properties Dropdowns + Runtime Pipeline)

**Demande par :** @Atlasbruce
**Contexte :** Besoin de dropdowns intelligents pour toutes les properties + specification pipeline runtime complet
**Spec documentee :**

Properties Dropdowns :
  - Branch.ConditionKey -> dropdown cles Blackboard local
  - GetBBValue/SetBBValue.BlackboardKey -> dropdown cles Blackboard local
  - GetBBValue.VariableType -> dropdown enum VariableType
  - MathOp.MathOperator -> dropdown (+, -, *, /)
  - AtomicTask.AtomicTaskID -> dropdown AtomicTaskRegistry::GetRegisteredIDs()
  - SubGraph.SubGraphPath -> file picker .ats
  - Switch.SwitchCases -> liste editable avec [+]/[-]
  - Tous les changements dropdown couverts par EditNodePropertyCommand (Undo/Redo)

Pipeline Runtime complet (editor -> entities -> systemes) :
  - Edition standalone : RenderProperties() edite TaskNodeDefinition en memoire
  - Save : serialisation JSON schema v4
  - Verification : VSGraphVerifier::Verify() avant save (Phase 21-C)
  - Chargement runtime : TaskGraphLoader -> ParseSchemaV4 -> AssetManager
  - Execution : TaskSystem::Process() -> ExecuteVSFrame() -> VSGraphExecutor::ExecuteFrame()
  - Blackboard : LocalBlackboard per-entity depuis TaskGraphTemplate::Blackboard
  - AtomicTask : IAtomicTask lifecycle Execute/Running/Success/Failure/Abort

Validation end-to-end :
  - Round-trip JSON sans perte
  - VSGraphVerifier detecte incoherences (cle BB introuvable, type mismatch)
  - VSGraphExecutor utilise exactement les memes champs que l'editeur

**Tests requis :** `Tests/BlueprintEditor/Phase20ETest.cpp` (round-trip, dropdown valide/invalide, execution)
**Destination roadmap :** Initiative F — Phase 20-E

---

## 2026-03-15 10:26:37 UTC — CRÉATION SYSTÈME PM V2

**Demandé par :** @Atlasbruce (via issue GitHub)
**Action :** Création du système de gestion de projet automatisé complet

**Fichiers créés :**
- `Project Management/Copilot_Memory_Project_Management_Instructions_V2.md` — Instructions PM V2 avec workflow automatisé post-merge
- `Project Management/FEATURE_LAUNCH_CHECKLIST.md` — Checklist de validation (18 questions, 7 sections)
- `Project Management/Features/FEATURE_CONTEXT_TEMPLATE.md` — Template standardisé pour les feature docs
- `Project Management/Features/feature_context_21_A.md` — VSGraphVerifier (retroactif, PR #380)
- `Project Management/Features/feature_context_21_B.md` — GVS UI Integration (retroactif, PR #381)
- `Project Management/Features/feature_context_21_D.md` — Dynamic Pins (retroactif, PR #382)
- `Project Management/Features/feature_context_22_A.md` — VSSwitch Enhancement (retroactif, PR #384)
- `Project Management/Features/feature_context_22_C.md` — Parameter Dropdowns (retroactif, PR #386)

**Fichiers mis à jour :**
- `Project Management/CONTEXT_CURRENT.md` — Corrigé : 22-C COMPLETED, 21-C NEXT
- `Project Management/CONTEXT_STATUS.md` — Corrigé : 21-B/21-D/22-A/22-C en Fonctionnels
- `Project Management/ROADMAP_V2.md` — Statuts COMPLETED pour toutes phases mergées + liens features

**Incohérences corrigées :**
- Phase 21-B : était "EN COURS" → maintenant ✅ COMPLETED (PR #381)
- Phase 21-D : était "PRIORITAIRE P0 EN DEV" → maintenant ✅ COMPLETED (PR #382)
- Phase 22-A : timestamp sans heure → 2026-03-14 18:46:14 UTC
- Phase 22-C : était "ACTIVE 🔧 IN PROGRESS" → maintenant ✅ COMPLETED (PR #386)
- Calendrier ROADMAP : 21-B absent → ajouté avec statut COMPLETED

---

## 2026-03-15 15:45:00 UTC — PROJECT MANAGEMENT DOCUMENTATION SYNC

**Trigger :** Post-Phase-23-B-Hotfix-P0 documentation update

**Action :** Synchronisation complete des documents de suivi

**Documents mis a jour :**
1. ✅ CONTEXT_ARCHIVE.md
   - Phase 22-C archivee (completed 2026-03-14 21:59:19 UTC, PR #386)
   - Phase 21-A/B/D archivees avec dates, PRs et liens feature context
   - Phase 22-A enrichie avec lien feature context
   - Sections historiques reorganisees

2. ✅ CONTEXT_CURRENT.md
   - Recent completions : 22-C, HOTFIX P0 (BUG-001), Phase 23-B spec finalized
   - P1 priority : Phase 23-B coding phase (2-section BB panel + preset config)
   - Timeline mise a jour (week of 2026-03-17)

3. ✅ CONTEXT_STATUS.md
   - All functional components listed avec details
   - BUG-001 marque RESOLVED (PR #387)
   - Phase 23-B SPEC ajoutee aux composants fonctionnels
   - Progression globale mise a jour

4. ✅ ROADMAP_V2.md
   - Calendrier complet avec HOTFIX entry (P0 COMPLETED PR #387)
   - Phase 23-B renommee : "Full Blackboard Properties" (plus "LevelScript + Cinematic")
   - Phase 23-B spec documentee : 2-section panel + JSON preset config approach
   - Phase 23-C renommee : "LevelScript + Cinematic" (ancienne 23-B)
   - Phase 21-C status : QUEUED (plus NEXT)
   - Header timestamp mis a jour

5. ✅ CONTEXT_MEMORY_LOG.md
   - Cette entree ajoutee (2026-03-15 15:45:00 UTC)

6. ✅ BUG_REGISTRY.md
   - BUG-001 entry ajoutee : Crash on Blackboard Save (abort())
   - Status : FIXED (PR #387)
   - Resolution date : 2026-03-15 15:30:00 UTC
   - Workaround : N/A (fixed)
   - Tests : 5/5 regression passing

**Coherence verifiee :**
- ✅ Tous timestamps format YYYY-MM-DD HH:MM:SS UTC
- ✅ Toutes references croisees valides
- ✅ Phase numbering consistent
- ✅ PR numbers linkees correctement
- ✅ Status tags synchronises (✅ COMPLETED, ⏳ QUEUED, etc.)

**Metrics :**
- Total files updated: 6
- Total phases documented: 24+
- Total PRs referenced: 9 (#380, #381, #382, #384, #386, #387, #388)
- Total bugs tracked: 1 (BUG-001 — now resolved)

**Status :** All PM docs synchronized and up-to-date
