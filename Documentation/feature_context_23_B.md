# 📄 Feature Context — Phase 23-B : Full Blackboard Properties

> **Créé le:** 2026-03-15 15:30:00 UTC  
> **Dernière mise à jour:** 2026-03-16 14:48:07 UTC  
> **Statut:** 🟠 EN COURS — Phase 23-B.5 BLOQUÉE (Issue #414)  
> **PR:** #400, #401, #404, #406, #408 (mergés) — Phase 23-B.5 en cours  
> **Références croisées:** [feature_context_22_C.md](./feature_context_22_C.md)

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu

**Avant cette feature :** L'éditeur crashait avec `abort()` lors du Save après l'ajout d'une variable Blackboard (BUG-001 P0). Les causes racines identifiées : `VariableType::None` non géré en sérialisation, clé vide non validée avant écriture, buffer ImGui potentiellement corrompu lors d'un realloc. De plus, il n'existait aucun catalogue centralisé de variables Blackboard disponibles pour le projet — les utilisateurs devaient inventer et retaper manuellement les noms de variables dans chaque graphe, source de doublons et d'incohérences.

**Après cette feature :** Le crash P0 est résolu (validation pré-save, init sécurisé, warning UX). Un registre de presets de variables Blackboard est disponible, chargé depuis un fichier JSON de configuration. Les utilisateurs peuvent découvrir les 15 variables pré-déclarées en 5 catégories et les importer dans leurs graphes locaux en un clic.

### 1.2 But du Système

Le système Full Blackboard Properties vise deux objectifs complémentaires :

1. **Stabilité (P0)** — Éliminer le crash save dû à `VariableType::None` et clé vide. Tout état invalide est détecté et signalé à l'utilisateur via un badge rouge dans le panel Blackboard, et ignoré silencieusement lors de la sérialisation avec log diagnostic.

2. **Découvrabilité (P1)** — Permettre aux utilisateurs de découvrir et réutiliser les variables Blackboard disponibles dans le projet. Le `BlackboardVariablePresetRegistry` expose un catalogue centralisé de 15 variables pré-définies en 5 catégories (Targeting, Movement, State, Combat, Misc), configurable via JSON par les non-développeurs.

### 1.3 Critères de Succès

- [x] BUG-001 résolu : save ne crashe plus avec `VariableType::None`
- [x] Init sécurisé : nouvelles entrées créées avec `Key="NewVariable"`, `Type=Int`
- [x] Warning UX : badge rouge si des entrées invalides sont présentes dans le panel
- [x] `BlackboardVariablePresetRegistry` singleton C++14 opérationnel
- [x] `Assets/Config/BlackboardVariablePresets.json` — 15 variables, 5 catégories
- [x] 18 tests headless passants (13 registre + 5 régression BUG-001)
- [x] O(1) lookup par nom via `unordered_map`
- [x] Gestion gracieuse des erreurs JSON (log + fallback vide)
- [x] Doublons ignorés avec log diagnostic
- [ ] Intégration UI dans `RenderBlackboardPanel()` — 2 sections (Available + Declared)
- [ ] Boutons `[+ Add to Local]` connectés au registry

---

## 2. 🏗️ Architecture

### 2.1 Vue d'ensemble

```
BLACKBOARD PANEL (3 sections)
├─ [AVAILABLE VARIABLES FROM CONFIG] (read-only)
│  ├─ ◇ targetDistance (Float)      [+ Add to Local]
│  ├─ ◇ targetSeen (Bool)           [+ Add to Local]
│  ├─ ◇ patrolIndex (Int)           [+ Add to Local]
│  └─ ◇ alertLevel (Int)            [+ Add to Local]
│
├─ [DECLARED LOCAL VARIABLES] (editable)
│  ├─ ◇ targetSeen (Bool) = false    [✎] [x]
│  ├─ ◇ patrolIndex (Int) = 5        [✎] [x]
│  └─ [+ Create Local Variable]
│
└─ [GLOBAL BLACKBOARD] (from ProjectSettings)
   ├─ ◈ playerHealth (Float) = 100.0  (R/W)
   └─ ◈ worldState (String) = "idle"  (R/W)
```

### 2.2 Composants Principaux

| Composant | Rôle | Type |
|-----------|------|------|
| `BlackboardVariablePresetRegistry` | Singleton registry, O(1) lookup | Nouveau |
| `Assets/Config/BlackboardVariablePresets.json` | Source of truth (15 vars, 5 cats) | Nouveau |
| `VisualScriptEditorPanel::SerializeAndWrite()` | Skip invalid entries (BUG-001) | Modifié |
| `VisualScriptEditorPanel::RenderBlackboard()` | Safe init + warning badge (BUG-001) | Modifié |

### 2.3 Flux de Données

1. **Au démarrage de l'éditeur** : `BlackboardVariablePresetRegistry::Instance().LoadFromFile("Assets/Config/BlackboardVariablePresets.json")` — peuple le catalogue
2. **Dans le panel Blackboard** : Section "Available" affiche les presets du registry (read-only)
3. **Clic `[+ Add to Local]`** : Copie le preset dans `m_template.Blackboard` comme entrée locale
4. **Section "Declared"** : Affiche et permet d'éditer les entrées locales déclarées
5. **Pre-save** : `SerializeAndWrite()` valide chaque entrée (skip si clé vide ou type None)

---

## 3. 💡 Décisions Techniques

### 3.1 Configuration JSON = Source of Truth

**Approche choisie :** `Assets/Config/BlackboardVariablePresets.json` contient les 15 variables pré-déclarées avec metadata complète (type, description, catégorie, default). Editable par les non-développeurs.

**Pourquoi cette approche :**
- Flexible : ajouter/modifier des variables sans recompilation
- Lisible : format JSON humain, commenté par champs
- Extensible : autres projets peuvent remplacer ce fichier

**Alternatives rejetées :**

| Alternative | Raison du rejet |
|-------------|----------------|
| Variables codées en dur dans le C++ | Rigide, recompilation requise pour toute modification |
| Base de données SQLite | Surcharge pour un catalogue statique de 15 entrées |

### 3.2 Registry Singleton (O(1) lookup)

**Approche choisie :** `BlackboardVariablePresetRegistry::Instance()` — singleton chargé au démarrage, index `unordered_map<name, index>` pour O(1) lookup.

**Pourquoi cette approche :**
- Performance : O(1) lookup indépendant du nombre de presets
- Cohérence avec le pattern des autres registries (AtomicTaskUIRegistry, ConditionRegistry, etc.)
- Thread-safe en lecture après LoadFromFile()

### 3.3 Scope Storage (validé par @Atlasbruce)

**Approche choisie :** Champ `"scope": "local" | "global"` dans le JSON v5 (backward compat : JSON v4 → fallback local).

### 3.4 Validation Pré-save (BUG-001 Hotfix)

**Approche choisie :** Skip silencieux + log SYSTEM_LOG + warning UX (badge rouge).

**Pourquoi cette approche :**
- Non-bloquant : le save réussit même avec des entrées invalides (entrées ignorées)
- Transparent : l'utilisateur voit le badge et comprend l'action requise
- Diagnostique : SYSTEM_LOG permet le débogage post-mortem

### 3.5 2-Section Panel UI

**Approche choisie (validée par @Atlasbruce) :**
- Section haute : variables disponibles (from config) — read-only + `[+ Add to Local]`
- Section basse locale : variables déclarées — éditables + `[+ Create Local Variable]`
- Section globale : variables projet — R/W (managers de gameplay)
- Code couleur / icône ASCII pour différencier local vs global (◇ local, ◈ global)

---

## 4. 📦 Fichiers Créés/Modifiés

### Créés (5)

| Fichier | Description |
|---------|-------------|
| `Assets/Config/BlackboardVariablePresets.json` | 15 variables, 5 catégories |
| `Source/BlueprintEditor/BlackboardVariablePresetRegistry.h` | Singleton registry header |
| `Source/BlueprintEditor/BlackboardVariablePresetRegistry.cpp` | Implementation ~160 lignes |
| `Tests/BlueprintEditor/Phase23BTest.cpp` | 18 tests headless |
| `Project Management/Features/feature_context_23_B.md` | Ce document |

### Modifiés (2)

| Fichier | Modification |
|---------|-------------|
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | BUG-001 hotfix (SerializeAndWrite + RenderBlackboard) |
| `CMakeLists.txt` | Ajout target OlympePhase23BTests |

---

## 5. ⚠️ Matrice de Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Fichier JSON absent au lancement | Moyen | Faible | Fallback registry vide + log warning |
| JSON malformé | Faible | Faible | Catch exception + fallback vide + log |
| Doublon de nom de preset | Faible | Faible | Skip avec log diagnostic |
| Regression save après hotfix | Faible | Critique | 5 tests régression dédiés |
| Performance O(n) catégories | Très faible | Très faible | Acceptable pour <100 presets |

---

## 6. 🧪 Cas de Tests (18)

### BlackboardVariablePresetRegistry (13)

| # | Test | Statut |
|---|------|--------|
| 1 | LoadFromFile_Success | ✅ PASS |
| 2 | LoadFromFile_MissingFile | ✅ PASS |
| 3 | LoadFromFile_MalformedJSON | ✅ PASS |
| 4 | GetAllPresets | ✅ PASS |
| 5 | GetPreset_ByName | ✅ PASS |
| 6 | GetPreset_NotFound | ✅ PASS |
| 7 | GetByCategory | ✅ PASS |
| 8 | GetAllCategories | ✅ PASS |
| 9 | HasPreset | ✅ PASS |
| 10 | GetCount | ✅ PASS |
| 11 | Clear | ✅ PASS |
| 12 | DuplicatePrevention | ✅ PASS |
| 13 | CategoryFiltering_Accuracy | ✅ PASS |

### BUG-001 Regression (5)

| # | Test | Statut |
|---|------|--------|
| 14 | BugFix_ValidEntry | ✅ PASS |
| 15 | BugFix_EmptyKey | ✅ PASS |
| 16 | BugFix_NoneType | ✅ PASS |
| 17 | BugFix_SafeInit | ✅ PASS |
| 18 | BugFix_AllTypesSkipNone | ✅ PASS |

**Total : 18/18 ✅**

---

## 7. 🔗 Références Croisées

- [BUG_REGISTRY.md](../BugTracking/BUG_REGISTRY.md) — BUG-001 fiche complète
- [feature_context_22_C.md](./feature_context_22_C.md) — Phase 22-C (BBVariableRegistry, base de Phase 23-B)
- [ROADMAP_V2.md](../ROADMAP_V2.md) — Planning Phase 23-B

---

*Dernière mise à jour : 2026-03-16 14:48:07 UTC*

---

## Phase 23-B.5 — Visual Script Editor Polish (EN COURS — BLOQUÉE)

> **Statut :** 🟠 IN PROGRESS (20%) — Bloquée par Issue [#414](https://github.com/Atlasbruce/Olympe-Engine/issues/414)  
> **Démarré le :** 2026-03-16 14:48:07 UTC  
> **PR Référence :** Post-merge PR #408

### Sous-phases Complétées (23-B.1 → 23-B.4)

- ✅ **23-B.1** : Implémentation des noeuds de conditions de base
- ✅ **23-B.2** : Intégration du système de types de variables
- ✅ **23-B.3** : Panneaux UI du builder de conditions (PR #404)
- ✅ **23-B.4** : Éditeur de conditions structuré (PR #406, #408)

### Sous-phase Courante : 23-B.5 — Fixs Post-Merge et UX Polish

**Objectif :** Résoudre les bugs bloquants découverts post-merge PR #408 et finaliser l'UX de l'éditeur de conditions.

#### Blockers Critiques (Issue #414)

| Bug | Sévérité | Titre | Estimation |
|-----|----------|-------|-----------|
| BUG-024 | P1 HIGH | Type Filtering Missing in Variable Dropdowns | 2-3h |
| BUG-025 | P1 HIGH | Const Value Not Persisted on Save | 3-4h |
| BUG-026 | P1 HIGH | Save Button Inconsistent Behavior | 1-2h |
| BUG-027 | P2 MEDIUM | Dropdown Lists Not Filtering by Operator Type | 1-2h |

#### Critères de Succès (Phase 23-B.5)

- [ ] Tous les dropdowns de variables filtrés par type/contexte
- [ ] Valeurs constantes persistées avec type correct (serialisation aller-retour)
- [ ] Bouton Save et Ctrl+S unifiés (même code path)
- [ ] Les 6 combinaisons d'opérandes testables (Variable vs Variable, Variable vs Const, Variable vs Pin, Pin vs Variable, Pin vs Const, Pin vs Pin)
- [ ] Aucun crash au reload avec des blackboards complexes
- [ ] Champs Vector x,y,z correctement affichés et persistés
- [ ] EntityID : indicateur readonly clairement visible

#### Matrice de Tests (Phase 23-B.5)

```
Tests de Conditions :
├─ Variable vs Variable  ✅ (fonctionnel)
├─ Variable vs Const     ❌ (bloqué par BUG-025)
├─ Variable vs Pin       ❌
├─ Pin vs Variable       ❌
├─ Pin vs Const          ❌
└─ Pin vs Pin            ❌

Tests de Filtrage de Types :
├─ Switch affiche Int uniquement     ❌ (bloqué par BUG-024)
├─ Condition opérande gauche (Numeric+Bool+Vector)  ❌
├─ Condition opérande droite filtrée par type gauche  ❌
└─ Type Vector affiche champs x,y,z  ✅
```

#### Architecture Cible (Save Pipeline Unifié)

```
Clic Bouton Save  OU  Ctrl+S
       ↓
PerformSaveOperation()  ← point d'entrée unique
       ↓
ValidateGraph()
       ↓
SerializeToJSON()
  ├─ SerializeNodes()
  ├─ SerializeConnections()
  ├─ SerializeBlackboard()
  └─ SerializeViewport()
       ↓
WriteToFile()  ← écriture atomique (.tmp puis rename)
       ↓
MarkGraphClean()
```

#### Système de Filtrage de Types (À Implémenter)

```
Contexte de Sélection de Variable
  ├─ Noeud Switch
  │  └─ Filtre : Int uniquement
  ├─ Noeud Condition
  │  ├─ Opérande gauche : Numeric + Bool + Vector
  │  └─ Opérande droite : Correspond au type de l'opérande gauche
  ├─ Opérateur de Comparaison
  │  └─ Filtre dynamique selon l'opérateur (mapping opérateur→types)
  └─ Dropdown Générique
     └─ Pas de filtre (afficher tout)
```
