# 🧪 Protocole de Tests — Olympe Engine

**Version:** 1.0  
**Créé le:** 2026-03-16 14:48:07 UTC  
**Dernière mise à jour:** 2026-03-16 14:48:07 UTC  
**Projet:** Olympe Engine  
**GitHub Repo:** https://github.com/Atlasbruce/Olympe-Engine  

---

## 📋 Vue d'Ensemble

Ce document définit le protocole de tests pour le projet Olympe Engine. Il couvre les tests unitaires, les tests d'intégration, les tests de régression et la procédure de validation avant merge.

---

## 🏗️ Infrastructure de Tests

### Emplacement des Tests

```
Tests/
└── BlueprintEditor/
    ├── Phase21ATest.cpp        — VSGraphVerifier (14 règles)
    ├── Phase21BTest.cpp        — GVS Panel UI
    ├── Phase21DTest.cpp        — Dynamic Pins (25+ tests)
    ├── Phase22ATest.cpp        — VSSwitch Enhancement (10 tests)
    ├── Phase22CTest.cpp        — Parameter Dropdowns & Registries (32 tests)
    ├── Phase23BTest.cpp        — Blackboard Properties + BUG-001/002 (18 tests)
    └── Phase23B2Test.cpp       — BUG-003/004 Position + ScreenToCanvasPos (8 tests)
```

### Build des Tests

```bash
# Configurer avec tests activés
cmake -S . -B /tmp/build -DBUILD_TESTS=ON

# Construire une cible de test spécifique
make OlympePhase23BTests -j4

# Construire tous les tests
make -j4

# Exécuter les tests
ctest -V
```

**Cibles de tests disponibles :**

| Cible CMake | Phase | Tests |
|-------------|-------|-------|
| `OlympePhase21ATests` | 21-A VSGraphVerifier | 14 tests |
| `OlympePhase21BTests` | 21-B GVS UI | ~10 tests |
| `OlympePhase21DTests` | 21-D Dynamic Pins | 25+ tests |
| `OlympePhase22ATests` | 22-A VSSwitch | 10 tests |
| `OlympePhase22CTests` | 22-C Dropdowns | 32 tests |
| `OlympePhase23BTests` | 23-B Blackboard | 18 tests |
| `OlympePhase23B2Tests` | 23-B2 BUG-003/004 | 8 tests |

---

## 1. Tests Unitaires

**Emplacement :** `Tests/BlueprintEditor/`

### Catégories de Tests

#### Filtrage de Types et Variables (Phase 23-B.5 — À Implémenter)

```cpp
// Tests à créer pour BUG-024 et BUG-027
TEST(VariableFiltering, SwitchNodeShowsIntOnly)
TEST(VariableFiltering, ConditionLeftOperandAllNumeric)
TEST(VariableFiltering, ConditionRightOperandMatchesLeft)
TEST(VariableFiltering, OperatorMappingNumericsOnly)
TEST(VariableFiltering, BoolOperatorShowsBoolOnly)
```

#### Persistance des Valeurs Constantes (Phase 23-B.5 — À Implémenter)

```cpp
// Tests à créer pour BUG-025
TEST(ConstValue, PersistenceOnSave)
TEST(ConstValue, SerializationRoundTrip)
TEST(ConstValue, TypeIntPreservedAfterReload)
TEST(ConstValue, TypeFloatPreservedAfterReload)
TEST(ConstValue, TypeBoolPreservedAfterReload)
```

#### Cohérence du Bouton Save (Phase 23-B.5 — À Implémenter)

```cpp
// Tests à créer pour BUG-026
TEST(SaveButton, UnifiedBehaviorWithCtrlS)
TEST(SaveButton, GraphNotDirtyAfterSave)
TEST(SaveButton, NoStateChangeOnCleanGraph)
```

#### Tests Existants (Passants)

```cpp
// Phase23B2Test.cpp — BUG-003/004 regression
TEST(ScreenToCanvasPos, BasicTranslation)
TEST(ScreenToCanvasPos, WithPanOffset)
TEST(GetVariablesByType, FilterIntOnly)
TEST(GetVariablesByType, FilterFloatOnly)
TEST(GetVariablesByType, FilterVectorOnly)
TEST(VectorType, ReadOnly)
TEST(EntityIDType, ReadOnly)
TEST(PositionRoundTrip, GridSpacePreserved)
```

---

## 2. Tests d'Intégration

### Scénario A : Workflow Complet avec Tous les Types de Variables

```
Test : Créer un graphe complexe avec tous les types de variables
1. Créer un graphe avec 10+ variables (Int, Float, Vector, Bool, String, EntityID)
2. Créer des conditions avec les 6 combinaisons d'opérandes
3. Panner le viewport significativement
4. Sauvegarder via bouton Save ET via Ctrl+S
5. Fermer et rouvrir
6. Vérifier : toutes les positions correctes, toutes les valeurs préservées, aucun crash
```

### Scénario B : Cycle de Vie Valeur Constante (BUG-025)

```
Test : Persistance des valeurs constantes
1. Ouvrir les propriétés d'un noeud Condition
2. Sélectionner mode "Const" pour l'opérande droite
3. Sélectionner type Int
4. Entrer la valeur 42
5. Passer à une autre condition
6. Revenir à la première condition
7. Vérifier : valeur toujours 42, type toujours Int
8. Sauvegarder et recharger
9. Vérifier : valeur 42 et type Int toujours présents
```

### Scénario C : Filtrage des Dropdowns (BUG-024)

```
Test : Filtrage de variables par type dans les dropdowns
1. Créer 3 variables : intVar (Int), vecVar (Vector), boolVar (Bool)
2. Ouvrir le noeud Switch → dropdown de sélection de variable
3. Vérifier : seul intVar est visible
4. Ouvrir le noeud Condition opérande gauche → dropdown variable
5. Vérifier : les 3 variables sont visibles
6. Sélectionner vecVar comme opérande gauche
7. Inspecter le dropdown opérande droite
8. Vérifier : seul vecVar est visible
```

### Scénario D : Cohérence Bouton Save (BUG-026)

```
Test : Le bouton Save et Ctrl+S produisent le même résultat
1. Ouvrir un graphe existant sans modification
2. Cliquer sur le bouton Save
3. Vérifier : pas de flag dirty activé
4. Faire une modification, puis cliquer Save
5. Vérifier : flag dirty effacé après save
6. Répéter étapes 2-5 avec Ctrl+S
7. Vérifier : comportement identique dans les deux cas
```

---

## 3. Tests de Régression

### Protocole de Régression

Pour chaque bug corrigé :

1. Écrire un test unitaire spécifique reproduisant le bug
2. Vérifier que le test échoue avant le fix
3. Appliquer le fix
4. Vérifier que le test passe après le fix
5. Ajouter le test au fichier de régression de la phase
6. Inclure le test dans la cible CMake correspondante

### Fichiers de Régression Existants

| Fichier | Bugs Couverts |
|---------|---------------|
| `Phase23BTest.cpp` | BUG-001, BUG-002 |
| `Phase23B2Test.cpp` | BUG-003, BUG-004 |

### Fichiers de Régression À Créer

| Fichier | Bugs À Couvrir |
|---------|----------------|
| `Phase23B5Test.cpp` | BUG-024, BUG-025, BUG-026, BUG-027 |

---

## 4. Matrice de Tests — Phase 23-B.5

| Test | Bug | Statut | Bloqueur |
|------|-----|--------|---------|
| Switch affiche Int uniquement | BUG-024 | ❌ | BUG-024 OPEN |
| Condition opérande gauche filtrée | BUG-024 | ❌ | BUG-024 OPEN |
| Condition opérande droite filtrée | BUG-024 | ❌ | BUG-024 OPEN |
| Const Int persisté après reload | BUG-025 | ❌ | BUG-025 OPEN |
| Const Float persisté après reload | BUG-025 | ❌ | BUG-025 OPEN |
| Save button = Ctrl+S résultat | BUG-026 | ❌ | BUG-026 OPEN |
| Graphe pas dirty après save | BUG-026 | ❌ | BUG-026 OPEN |
| Dropdown opérateur numérique filtré | BUG-027 | ❌ | BUG-027 OPEN |
| Variable vs Variable condition | — | ✅ | — |
| Vector champs x,y,z affichés | — | ✅ | — |

---

## 5. Checklist Avant Merge (PR)

```
Pré-merge :
- [ ] Tous les tests unitaires passent (make -j4 && ctest -V)
- [ ] Aucun nouveau warning compilateur (-Wall -Wextra)
- [ ] Code review approuvé (@Atlasbruce)
- [ ] Tests d'intégration : matrice 100% pass
- [ ] Tests manuels effectués (Windows)
- [ ] Benchmarks de performance stables (pas de régression)
- [ ] Documentation mise à jour (feature_context, BUG_REGISTRY)
- [ ] Nouveaux tests de régression ajoutés pour les bugs fixés
```

## 6. Checklist Post-Merge (Release)

```
Post-merge :
- [ ] Tests QA finaux sur la branche release
- [ ] Test d'acceptance utilisateur (UAT) — @Atlasbruce
- [ ] Checklist de déploiement complète
- [ ] Issues connues documentées dans CONTEXT_STATUS.md
- [ ] BUG_REGISTRY.md mis à jour (bugs résolus archivés)
- [ ] CONTEXT_CURRENT.md mis à jour
- [ ] ROADMAP_V2.md calendrier mis à jour
```

---

## 7. Échecs de Tests Connus (Issue #414)

```
❌ TestVariableFiltering::TestSwitchShowsIntOnly
   Attendu : seules les variables Int affichées
   Observé : toutes les variables affichées
   Bloqueur : BUG-024

❌ TestConstValue::TestPersistenceOnSave
   Attendu : valeur persiste après save/reload
   Observé : valeur reset à "None"
   Bloqueur : BUG-025

❌ TestSaveButton::TestConsistencyWithCtrlS
   Attendu : même résultat depuis bouton vs raccourci
   Observé : bouton peut marquer le graphe dirty après save
   Bloqueur : BUG-026

❌ TestVariableFiltering::TestOperatorMappingNumericsOnly
   Attendu : seules les variables numériques affichées pour opérateur <
   Observé : toutes les variables affichées
   Bloqueur : BUG-027
```

---

## 🔗 Références Croisées

- [BUG_REGISTRY.md](./BugTracking/BUG_REGISTRY.md) — Registre centralisé des bugs
- [CONTEXT_STATUS.md](./CONTEXT_STATUS.md) — Vue synthétique de l'état courant
- [Features/feature_context_23_B.md](./Features/feature_context_23_B.md) — Phase 23-B feature context
- [ROADMAP_V2.md](./ROADMAP_V2.md) — Planification des phases

---

*Dernière mise à jour : 2026-03-16 14:48:07 UTC*
