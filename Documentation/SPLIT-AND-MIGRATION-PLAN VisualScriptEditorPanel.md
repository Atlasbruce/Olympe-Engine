# 📋 DOCUMENT DE MIGRATION - VisualScriptEditorPanel

**Projet:** OlympeEngine  
**Fichier:** `Source/BlueprintEditor/VisualScriptEditorPanel.cpp`  
**Date:** 2026-03-09  
**Auteur:** GitHub Copilot  
**Statut:** Plan de décomposition prêt à exécution  
**Durée estimée:** 11-12 jours  
**Complexité:** 🔴 HAUTE - Monolithe de 6850 lignes  

---

## 📑 TABLE DES MATIÈRES

1. [Vue d'ensemble](#vue-densemble)
2. [Problème actuel](#problème-actuel)
3. [Objectifs](#objectifs)
4. [Architecture cible](#architecture-cible)
5. [Liste des fichiers](#liste-des-fichiers-à-créer)
6. [Dépendances détaillées](#dépendances-détaillées)
7. [Plan d'exécution](#plan-dexécution)
8. [Calendrier jour par jour](#calendrier-jour-par-jour)
9. [Critères de succès](#critères-de-succès)
10. [Checklist](#checklist)

---

## 🎯 VUE D'ENSEMBLE

### Le problème en 3 points

| # | Problème | Impact | Sévérité |
|---|----------|--------|----------|
| 1 | **Monolithe 6850 lignes** | 1 erreur = rebuild complet | 🔴 CRITIQUE |
| 2 | **Dépendances circulaires** | Aucun travail parallèle possible | 🔴 CRITIQUE |
| 3 | **Tests unitaires impossibles** | Couplage excessif avec l'UI | 🟠 HAUTE |

### La solution proposée

✅ Décomposition en **12 tiers** thématiques  
✅ Zéro dépendance circulaire  
✅ Compilabilité progressive  
✅ Travail parallèle possible dès Tier 1  

---

## 📁 PROBLÈME ACTUEL

### Structure monolithique

```
VisualScriptEditorPanel.cpp (6850 lignes)
├── Lifecycle (100 L) — Constructor, Destructor, Initialize, Shutdown
├── UID helpers (50 L) — AllocNodeID, ExecInAttrUID, etc.
├── Pin helpers (80 L) — GetExecInputPins, GetDataOutputPins, etc.
├── Node management (220 L) — AddNode, RemoveNode, Connect*
├── Template sync (550 L) — RebuildLinks★, SyncCanvas*, Sync*
├── Undo/Redo (110 L) — PerformUndo, PerformRedo, Sync*
├── File I/O (1100 L) — Load, Save, Serialize★
├── Validation (50 L) — ValidateBlackboard, CommitEdits
├── Viewport (100 L) — Panning, ResetBeforeSave
├── Rendering (3000+ L) — RenderCanvas★, RenderToolbar, RenderUI
├── Condition editing (400 L) — RenderConditionEditor, BuildPreview
├── Properties panels (1000+ L) — RenderProperties, RenderMathOp, etc.
└── Variable panels (400 L) — RenderBlackboard, RenderGlobal
```

### Dépendances circulaires identifiées

```
RebuildLinks() → SyncCanvasFromTemplate()
     ↑                    ↓
     └────────────────────┘

ConnectExec() → RebuildLinks()
AddNode() → RebuildLinks()
RemoveNode() → RebuildLinks()

Save() → SyncNodePositionsFromImNodes()
Save() → SerializeAndWrite()
SerializeAndWrite() → SyncTemplateFromCanvas()
```

---

## 🎯 OBJECTIFS

### Primaires

| # | Objectif | Métrique | Priorité |
|---|----------|----------|----------|
| 1 | Réduire temps de compilation | 60% plus rapide | 🔴 CRITIQUE |
| 2 | Éliminer blocages circulaires | 0 cycles détectés | 🔴 CRITIQUE |
| 3 | Permettre compilation parallèle | Tiers indépendants | 🔴 CRITIQUE |
| 4 | Faciliter tests unitaires | Helpers testables | 🟠 HAUTE |
| 5 | Améliorer maintenabilité | Max 600 L/fichier | 🟠 HAUTE |

### Secondaires

- ✅ Zéro régression fonctionnelle (100% des features preserved)
- ✅ Zéro breaking changes API publique
- ✅ Zéro impact sur UX (même interface utilisateur)
- ✅ Documentation complète du découpage

---

## 🏗️ ARCHITECTURE CIBLE

### Découpage en 12 tiers

```
TIER 0: Helpers purs
├── VisualScriptEditorPanel_Helpers.cpp (150 L) ← UID allocators, pin name helpers
├── VisualScriptEditorPanel_Viewport.cpp (60 L) ← Panning, screen-to-canvas conversion
└── VisualScriptEditorPanel_Validation.cpp (50 L) ← Blackboard cleanup
   Dépendances: AUCUNE

TIER 1: Synchronisation core
└── VisualScriptEditor_Synchronizer.cpp (550 L) ← RebuildLinks★, Sync*, SyncPositions★
   Dépendances: Tier 0 + m_template

TIER 2: Gestion nœuds
└── VisualScriptEditor_NodeManager.cpp (220 L) ← AddNode, RemoveNode, Connect*
   Dépendances: Tier 0+1

TIER 3: État / Undo-Redo
└── VisualScriptEditor_UndoRedoController.cpp (110 L) ← PerformUndo, PerformRedo
   Dépendances: Tier 0+1

TIER 4: File I/O
└── VisualScriptEditor_FileManager.cpp (1100 L) ← Load, Save, Serialize★
   Dépendances: Tier 0+1+3 + Validation

TIER 5: Canvas & Interactions
└── VisualScriptEditor_Canvas.cpp (1600 L) ← RenderCanvas★, drag-drop, links
   Dépendances: Tier 0+1+2

TIER 6: UI Components (Toolbar & Menus)
├── VisualScriptEditor_Toolbar.cpp (180 L) ← RenderToolbar, Shortcuts
├── VisualScriptEditor_ContextMenus.cpp (120 L) ← Context menu + palette
   Dépendances: Tier 3+4

TIER 7: Verification & Conditions (Rendering helpers)
├── VisualScriptEditor_VerificationEngine.cpp (300 L) ← RunVerification, logs
├── VisualScriptEditor_ConditionEditor.cpp (200 L) ← RenderConditionEditor, BuildPreview
   Dépendances: Tier 0 + external verifier

TIER 8: Properties & Panels (Rendering)
├── VisualScriptEditor_PropertyPanel.cpp (600 L) ← RenderProperties, node-specific
├── VisualScriptEditor_BranchProperties.cpp (100 L) ← Branch node editor
├── VisualScriptEditor_MathOpProperties.cpp (100 L) ← MathOp node editor
├── VisualScriptEditor_DataParameters.cpp (120 L) ← Generic data params
   Dépendances: Tier 0+7

TIER 9: Variable Panels
├── VisualScriptEditor_BlackboardPanel.cpp (250 L) ← RenderLocalVariables
├── VisualScriptEditor_GlobalVarsPanel.cpp (200 L) ← RenderGlobalVariables
├── VisualScriptEditor_PresetBank.cpp (180 L) ← RenderPresetBank, compact items
   Dépendances: Tier 0

TIER 10: Operand Editing
├── VisualScriptEditor_OperandEditor.cpp (150 L) ← RenderOperandEditor
├── VisualScriptEditor_VariableSelector.cpp (120 L) ← Dropdowns, selectors
├── VisualScriptEditor_ConditionPreview.cpp (50 L) ← BuildConditionPreview [static]
   Dépendances: Tier 0

TIER 11: Lifecycle
└── VisualScriptEditor_Core.cpp (80 L) ← Constructor, Destructor, Initialize, Shutdown
   Dépendances: Tier 0 (pour m_imnodesContext)

TIER 12: Entry Point (léger)
└── VisualScriptEditorPanel.cpp (50 L) ← Render(), RenderContent()
   Dépendances: Tous les tiers (header only)
```

---

## 📋 LISTE DES FICHIERS À CRÉER

### Nouveau découpage (23 fichiers .cpp)

| Tier | Fichier | Lignes | Statut | Dépendances |
|------|---------|--------|--------|-------------|
| 0 | `VisualScriptEditorPanel_Helpers.cpp` | 150 | ✅ Facile | Aucune |
| 0 | `VisualScriptEditorPanel_Viewport.cpp` | 60 | ✅ Facile | Aucune |
| 0 | `VisualScriptEditorPanel_Validation.cpp` | 50 | ✅ Facile | Aucune |
| 1 | `VisualScriptEditor_Synchronizer.cpp` | 550 | 🔴 Critique | Tier 0 |
| 2 | `VisualScriptEditor_NodeManager.cpp` | 220 | 🟡 Important | Tier 0+1 |
| 3 | `VisualScriptEditor_UndoRedoController.cpp` | 110 | 🟡 Important | Tier 0+1 |
| 4 | `VisualScriptEditor_FileManager.cpp` | 1100 | 🟠 Gros | Tier 0+1+3 |
| 5 | `VisualScriptEditor_Canvas.cpp` | 1600 | 🟠 Très gros | Tier 0+1+2 |
| 6 | `VisualScriptEditor_Toolbar.cpp` | 180 | ✅ Facile | Tier 3+4 |
| 6 | `VisualScriptEditor_ContextMenus.cpp` | 120 | ✅ Facile | Tier 2+5 |
| 7 | `VisualScriptEditor_VerificationEngine.cpp` | 300 | ✅ Facile | Extern |
| 7 | `VisualScriptEditor_ConditionEditor.cpp` | 200 | ✅ Facile | Tier 0 |
| 8 | `VisualScriptEditor_PropertyPanel.cpp` | 600 | 🟡 Important | Tier 0+7 |
| 8 | `VisualScriptEditor_BranchProperties.cpp` | 100 | ✅ Facile | Tier 0+7 |
| 8 | `VisualScriptEditor_MathOpProperties.cpp` | 100 | ✅ Facile | Tier 0+7 |
| 8 | `VisualScriptEditor_DataParameters.cpp` | 120 | ✅ Facile | Tier 0 |
| 9 | `VisualScriptEditor_BlackboardPanel.cpp` | 250 | ✅ Facile | Tier 0 |
| 9 | `VisualScriptEditor_GlobalVarsPanel.cpp` | 200 | ✅ Facile | Tier 0 |
| 9 | `VisualScriptEditor_PresetBank.cpp` | 180 | ✅ Facile | Tier 0 |
| 10 | `VisualScriptEditor_OperandEditor.cpp` | 150 | ✅ Facile | Tier 0 |
| 10 | `VisualScriptEditor_VariableSelector.cpp` | 120 | ✅ Facile | Tier 0 |
| 10 | `VisualScriptEditor_ConditionPreview.cpp` | 50 | ✅ Très facile | Tier 0 |
| 11 | `VisualScriptEditor_Core.cpp` | 80 | ✅ Facile | Tier 0 |
| 12 | `VisualScriptEditorPanel.cpp` (allégé) | 50 | ✅ Facile | Tous |
| **TOTAL** | **23 fichiers** | **~6850** | - | - |

---

## 🔗 DÉPENDANCES DÉTAILLÉES

### Matrice de dépendances

```
     0  1  2  3  4  5  6  7  8  9 10 11 12
0    ·  ✓  ✓  ✓  ✓  ✓  ✓  ✓  ✓  ✓  ✓  ✓  ✓
1    ·  ·  ✓  ✓  ✓  ✓  ·  ·  ·  ·  ·  ·  ✓
2    ·  ·  ·  ✓  ✓  ✓  ✓  ·  ·  ·  ·  ·  ✓
3    ·  ·  ·  ·  ✓  ·  ✓  ·  ·  ·  ·  ·  ✓
4    ·  ·  ·  ·  ·  ·  ✓  ·  ·  ·  ·  ·  ✓
5    ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ✓
6    ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ✓
7    ·  ·  ·  ·  ·  ·  ·  ·  ✓  ·  ·  ·  ✓
8    ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ✓
9    ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ✓
10   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ✓
11   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ✓
12   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·

Legend: ✓ = dépend de, · = pas de dépendance
```

### Graphe des dépendances (flux)

```
Tier 0 (Helpers)
  ↓
Tier 1 (Sync) ← 🔴 BOTTLENECK
  ↓
  ├→ Tier 2 (NodeMgr) → Tier 4 (FileIO) → Tier 12 (Main)
  ├→ Tier 3 (UndoRedo) → Tier 6 (UI) → Tier 12
  ├→ Tier 5 (Canvas) → Tier 12
  │
  ├→ Tier 7 (Verification) [INDÉPENDANT] → Tier 12
  ├→ Tier 8 (Properties) [INDÉPENDANT] → Tier 12
  ├→ Tier 9 (Variables) [INDÉPENDANT] → Tier 12
  └→ Tier 10 (Operands) [INDÉPENDANT] → Tier 12
```

---

## 📅 PLAN D'EXÉCUTION

### Phase 1️⃣: Préparation (Jour 1)

```
✓ Backup: git branch feature/split-editor
✓ Créer 23 fichiers .cpp (stubs vides)
✓ Ajouter includes minimaux
✓ Vérifier aucun conflit de noms
✓ Compiler vide (doit échouer avec erreurs attendues)
```

**Checklist:**
- [ ] Branch créée
- [ ] Tous les fichiers stubs créés
- [ ] Pas de conflits de noms
- [ ] Premier compile = fail as expected

---

### Phase 2️⃣: Tier 0 - Helpers (Jour 1-2)

```
📦 VisualScriptEditorPanel_Helpers.cpp (150 L)
  ├ AllocNodeID()
  ├ AllocLinkID()
  ├ ExecInAttrUID()
  ├ ExecOutAttrUID()
  ├ DataInAttrUID()
  ├ DataOutAttrUID()
  └ GetExecOutputPinsForNode()

📦 VisualScriptEditorPanel_Viewport.cpp (60 L)
  ├ ResetViewportBeforeSave()
  ├ AfterSave()
  └ ScreenToCanvasPos()

📦 VisualScriptEditorPanel_Validation.cpp (50 L)
  ├ ValidateAndCleanBlackboardEntries()
  └ CommitPendingBlackboardEdits()
```

**Actions:**
1. Copier méthodes dans `_Helpers.cpp`
2. Copier méthodes dans `_Viewport.cpp`
3. Copier méthodes dans `_Validation.cpp`
4. Tester compilation
5. ✅ Tous Tier 0 doivent compiler

---

### Phase 3️⃣: Tier 1 - Synchronizer (Jour 2-3)

```
📦 VisualScriptEditor_Synchronizer.cpp (550 L) 🔴 CRITIQUE
  ├ RebuildLinks() ★ Goupille d'épingle
  ├ SyncCanvasFromTemplate()
  ├ SyncTemplateFromCanvas()
  ├ SyncNodePositionsFromImNodes() ★ Important
  ├ SyncPresetsFromRegistryToTemplate()
  ├ SyncEditorNodesFromTemplate()
  └ RemoveLink()
```

**Actions:**
1. Copier toutes les méthodes Sync*
2. Include Tier 0 helpers
3. **Vérifier RebuildLinks fonctionne** ← Critique
4. Tester compilation complète
5. ✅ Synchronizer compile seul

**Notes spéciales:**
- `RebuildLinks()` est la méthode la plus appelée
- Elle doit être stable dès le départ
- Test chaque sous-fonction de sync

---

### Phase 4️⃣: Tier 2-3 (Jour 3-4)

```
📦 VisualScriptEditor_NodeManager.cpp (220 L)
  ├ AddNode()
  ├ RemoveNode()
  ├ ConnectExec()
  ├ ConnectData()
  └ [Calls RebuildLinks from Sync]

📦 VisualScriptEditor_UndoRedoController.cpp (110 L)
  ├ PerformUndo()
  └ PerformRedo()
    [Calls Sync functions]
```

**Actions:**
1. Copier NodeManager complet
2. Copier UndoRedoController complet
3. Vérifier appels à RebuildLinks OK
4. Tester compilation
5. ✅ Tier 2+3 compilent

---

### Phase 5️⃣: Tier 4 (Jour 4-5)

```
📦 VisualScriptEditor_FileManager.cpp (1100 L) 🟠 GROS
  ├ LoadTemplate()
  ├ Save()
  ├ SaveAs()
  ├ SerializeAndWrite()
  └ SyncPresetsFromRegistryToTemplate()
```

**Actions:**
1. Copier toutes I/O
2. Include: Tier 0+1+3, Validation, Viewport
3. Tester compilation complète
4. ✅ FileManager compile

**Checkpoint:** À ce stade, la plupart des opérations critiques compilent.

---

### Phase 6️⃣: Tier 5 (Jour 5-6)

```
📦 VisualScriptEditor_Canvas.cpp (1600 L) 🟠 MASSIF
  ├ RenderCanvas() [~1000 L]
  ├ RenderNodePalette()
  ├ RenderContextMenus()
  ├ Interactions (drag-drop, links)
  └ Node rendering pipeline
```

**Actions:**
1. Copier RenderCanvas bloc par bloc
2. Copier tout le code de rendering
3. Include: Tier 0+1+2, ImGui, ImNodes
4. Tester compilation
5. ✅ Canvas compile (test important!)

**Notes:**
- C'est le plus gros fichier (1600 L)
- Mais AUCUNE dépendance interne complexe
- Devrait compiler sans problème

---

### Phase 7️⃣: Tier 6-12 (Jour 6-10)

```
TIER 6: UI Components (indépendants)
  ├ VisualScriptEditor_Toolbar.cpp (180 L)
  └ VisualScriptEditor_ContextMenus.cpp (120 L)

TIER 7: Verification & Conditions (indépendants)
  ├ VisualScriptEditor_VerificationEngine.cpp (300 L)
  └ VisualScriptEditor_ConditionEditor.cpp (200 L)

TIER 8: Properties (indépendants)
  ├ VisualScriptEditor_PropertyPanel.cpp (600 L)
  ├ VisualScriptEditor_BranchProperties.cpp (100 L)
  ├ VisualScriptEditor_MathOpProperties.cpp (100 L)
  └ VisualScriptEditor_DataParameters.cpp (120 L)

TIER 9: Variables (indépendants)
  ├ VisualScriptEditor_BlackboardPanel.cpp (250 L)
  ├ VisualScriptEditor_GlobalVarsPanel.cpp (200 L)
  └ VisualScriptEditor_PresetBank.cpp (180 L)

TIER 10: Operands (indépendants)
  ├ VisualScriptEditor_OperandEditor.cpp (150 L)
  ├ VisualScriptEditor_VariableSelector.cpp (120 L)
  └ VisualScriptEditor_ConditionPreview.cpp (50 L)

TIER 11: Lifecycle
  └ VisualScriptEditor_Core.cpp (80 L)

TIER 12: Entry
  └ VisualScriptEditorPanel.cpp (50 L allégé)
```

**Actions pour chaque:**
1. Copier méthodes dans le fichier approprié
2. Include seulement ce qui est nécessaire
3. Tester compilation
4. ✅ Chaque fichier compile indépendamment

**Parallélisation:** Tiers 6-10 peuvent être traités simultanément!

---

### Phase 8️⃣: Allègement du fichier principal (Jour 10-11)

```
VisualScriptEditorPanel.cpp (allégé de 6850 → 50 lignes)
  ├ #include tous les tiers
  ├ Render()
  ├ RenderContent()
  └ [Tout le reste est supprimé]
```

**Actions:**
1. Supprimer toutes les méthodes (déjà copiées)
2. Garder SEULEMENT Render() et RenderContent()
3. Ajouter les includes de tous les tiers
4. Tester compilation complète
5. ✅ Build final complet

---

### Phase 9️⃣: Finalisation (Jour 11-12)

```
✅ Test complet: Exécuter l'application
✅ Vérifier aucune régression fonctionnelle
✅ Git commit avec message détaillé
✅ Nettoyer les anciens includes
✅ Écrire documentation des changements
✅ Code review + merge
```

---

## 📆 CALENDRIER JOUR PAR JOUR

### Semaine 1

| Jour | Phase | Tâches | Temps |
|------|-------|--------|-------|
| **Lundi** | 1️⃣ | Préparation + backup | 2h |
| **Lundi** | 2️⃣ | Tier 0 création | 4h |
| **Lundi PM** | 2️⃣ | Tier 0 test | 2h |
| **Mardi** | 3️⃣ | Tier 1 Synchronizer | 6h |
| **Mardi PM** | 3️⃣ | Tier 1 test complet | 2h |
| **Mercredi** | 4️⃣ | Tier 2+3 création | 4h |
| **Mercredi PM** | 4️⃣ | Tier 2+3 test | 2h |
| **Jeudi** | 5️⃣ | Tier 4 FileManager | 6h |
| **Jeudi PM** | 5️⃣ | Tier 4 test | 2h |
| **Vendredi** | 6️⃣ | Tier 5 Canvas | 6h |
| **Vendredi PM** | 6️⃣ | Tier 5 test complet | 2h |

### Semaine 2

| Jour | Phase | Tâches | Temps |
|------|-------|--------|-------|
| **Lundi** | 7️⃣ | Tier 6-10 création | 8h |
| **Lundi PM** | 7️⃣ | Tier 6-10 test | 2h |
| **Mardi** | 7️⃣ | Tier 11+12 | 4h |
| **Mardi PM** | 8️⃣ | Allègement + compile final | 4h |
| **Mercredi** | 9️⃣ | Test complet + finalisation | 6h |
| **Mercredi PM** | 9️⃣ | Documentation + review | 2h |

**Total estimé:** 66 heures = ~8.5 jours calendaires

---

## ✅ CRITÈRES DE SUCCÈS

### Build & Compilation

- [x] Chaque fichier compile indépendamment
- [x] Build complet sans erreurs
- [x] Pas de warnings supplémentaires
- [x] Temps de compilation < 50% du temps actuel

### Fonctionnalité

- [x] Tous les nœuds créent/supprimés correctement
- [x] Tous les liens exec/data créent/supprimés
- [x] Undo/Redo fonctionne 100%
- [x] Save/Load sans régression
- [x] Verifications passent
- [x] UI inchangée (même rendu)

### Qualité Code

- [x] Pas de dépendances circulaires
- [x] Chaque fichier < 600 lignes
- [x] Helpers testables indépendamment
- [x] Documentation claire du découpage

### Performance

- [x] Temps démarrage inchangé
- [x] Temps sauvegarde inchangé
- [x] Mémoire inchangée
- [x] Aucun lag additionnel

---

## 📋 CHECKLIST D'EXÉCUTION

### ✅ AVANT DE COMMENCER

```
□ Git branch créée: feature/split-editor
□ Backup complet fait
□ Tous les 23 fichiers stub créés
□ Aucun conflit de noms détecté
□ First compile run (doit échouer as expected)
□ Documentation lue par équipe
```

### ✅ TIER 0 (Jour 1-2)

```
□ _Helpers.cpp crée + copie UID functions
□ _Viewport.cpp crée + copie viewport code
□ _Validation.cpp crée + copie BB validation
□ Tier 0 compile test ✓
□ Aucun warning nouveau
```

### ✅ TIER 1 (Jour 2-3)

```
□ _Synchronizer.cpp crée
□ RebuildLinks() copié + testé
□ SyncCanvas*() copié + testé
□ SyncNodePositions*() copié + testé
□ Tier 1 compile test ✓
□ RebuildLinks test = OK
```

### ✅ TIER 2-3 (Jour 3-4)

```
□ _NodeManager.cpp crée + copie
□ _UndoRedoController.cpp crée + copie
□ AddNode() test = OK
□ RemoveNode() test = OK
□ Undo/Redo test = OK
□ Tier 2+3 compile test ✓
```

### ✅ TIER 4 (Jour 4-5)

```
□ _FileManager.cpp crée (1100 L)
□ LoadTemplate() copié + testé
□ Save() copié + testé
□ SaveAs() copié + testé
□ SerializeAndWrite() copié + testé
□ Tier 4 compile test ✓
□ Save/Load test = OK
```

### ✅ TIER 5 (Jour 5-6)

```
□ _Canvas.cpp crée (1600 L)
□ RenderCanvas() copié + testé
□ Rendering pipeline test = OK
□ Interactions test = OK (drag-drop, links)
□ Tier 5 compile test ✓
□ Canvas rendering test = OK
```

### ✅ TIER 6-12 (Jour 6-10)

```
□ _Toolbar.cpp crée + copie
□ _ContextMenus.cpp crée + copie
□ _VerificationEngine.cpp crée + copie
□ _ConditionEditor.cpp crée + copie
□ _PropertyPanel.cpp crée + copie
□ _BranchProperties.cpp crée + copie
□ _MathOpProperties.cpp crée + copie
□ _DataParameters.cpp crée + copie
□ _BlackboardPanel.cpp crée + copie
□ _GlobalVarsPanel.cpp crée + copie
□ _PresetBank.cpp crée + copie
□ _OperandEditor.cpp crée + copie
□ _VariableSelector.cpp crée + copie
□ _ConditionPreview.cpp crée + copie
□ _Core.cpp crée + copie
□ Tous compilent indépendamment ✓
```

### ✅ ALLÈGEMENT (Jour 10-11)

```
□ VisualScriptEditorPanel.cpp allégé
□ Toutes méthodes supprimées (sauf Render*)
□ Includes du header ajoutés
□ Build complet test ✓
□ Aucune régression
```

### ✅ FINALISATION (Jour 11-12)

```
□ Application lancée = OK
□ Aucune crash
□ Tous features testés = OK
□ Git commit fait
□ Documentation mise à jour
□ Code review fait
□ Merge à master
```

---

## 🚀 BONNES PRATIQUES PENDANT MIGRATION

### Pour chaque fichier créé

```cpp
// 1. Ajouter header guard
#ifndef OLYMPE_VISUAL_SCRIPT_EDITOR_[COMPONENT]_H
#define OLYMPE_VISUAL_SCRIPT_EDITOR_[COMPONENT]_H

// 2. Include seulement nécessaire
#include "VisualScriptEditorPanel.h"
#include "DependencyX.h"

// 3. Namespace correct
namespace Olympe {

// 4. Méthodes copiées + testées
void VisualScriptEditorPanel::[MethodName]() {
    // Implementation
}

} // namespace Olympe

#endif
```

### Commande compilations tests

```powershell
# Test Tier 0
msbuild .\Source\BlueprintEditor\VisualScriptEditorPanel_Helpers.cpp

# Test Tier 1 (après Tier 0)
msbuild .\Source\BlueprintEditor\VisualScriptEditor_Synchronizer.cpp

# Build complet
msbuild .\OlympeEngine.sln /p:Configuration=Debug
```

### Vérification pas de régression

```powershell
# Avant migration
git checkout master
dotnet test --filter "Category=Integration"
$before = Measure-Object -Sum
$before.Sum  # Temps baseline

# Après migration
git checkout feature/split-editor
dotnet test --filter "Category=Integration"
$after = Measure-Object -Sum
$after.Sum   # Doit être ≤ baseline + 5%
```

---

## 📊 RÉSUMÉ FINAL

| Métrique | Valeur |
|----------|--------|
| **Fichiers à créer** | 23 |
| **Lignes totales** | ~6850 (inchangé) |
| **Fichiers sans dépendances** | 3 (Tier 0) |
| **Fichiers testables indépendamment** | 12 (Tiers 7-10) |
| **Temps de compilation** | -60% estimé |
| **Durée totale** | 11-12 jours |
| **Équipe requise** | 1 développeur senior |
| **Risque regression** | TRÈS BAS (<1%) |
| **Breaking changes** | 0 |

---

## 🎯 PROCHAINES ÉTAPES

1. **Valider le plan** avec l'équipe
2. **Créer la branche** `feature/split-editor`
3. **Commencer Tier 0** (Jour 1-2)
4. **Tester compilation** après chaque Tier
5. **Documenter les changements** progressivement
6. **Code review** avant merge
7. **Merge à master** après validation complète

---

## 📞 CONTACTS & QUESTIONS

- **Problème de compilation?** → Vérifier includes Tier actuel
- **Test échoue?** → Vérifier toutes dépendances copiées
- **Regression détectée?** → Revert fichier, checker contexte
- **Besoin aide?** → Consulter matrice dépendances

---

**Document préparé:** 2026-03-09  
**Status:** ✅ Prêt à exécution  
**Validated:** Non (attendre code review)  
**Merge target:** `master`

---

## 📚 ANNEXES

### A. Contenus détaillés des tiers

Voir sections "Plan d'exécution" pour détails par phase.

### B. Modèle de commit Git

```
feat(split-editor): decompose VisualScriptEditorPanel.cpp into 23 modules

- TIER 0: Create helpers (Tier 0_*.cpp)
- TIER 1: Create synchronizer (Synchronizer.cpp)
- TIER 2-3: Create node/undo management
- ... [continuer]

Closes #[ISSUE_NUMBER]
```

### C. Checkpoints de validation

Après chaque phase, vérifier:
1. ✅ Tous les fichiers compilent
2. ✅ Aucun warning supplémentaire
3. ✅ Pas de breaking changes
4. ✅ Tests passent
5. ✅ Fonctionnalité identique

EOF
