# 📄 Feature Context — Phase 22-C : Parameter Dropdowns & Registries

> **Créé le:** 2026-03-15 10:26:37 UTC  
> **Dernière mise à jour:** 2026-03-15 10:26:37 UTC  
> **Statut:** ✅ COMPLÉTÉ  
> **PR:** [#386](https://github.com/Atlasbruce/Olympe-Engine/pull/386)  
> **Références croisées:** [feature_context_21_A.md](./feature_context_21_A.md) · [feature_context_22_A.md](./feature_context_22_A.md)

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu

**Avant cette feature :** Les paramètres des nœuds (AtomicTaskID, ConditionID, variables Blackboard, opérateurs math/comparaison, chemins SubGraph) étaient saisis comme du texte libre dans des champs `InputText`. L'utilisateur devait connaître par cœur les identifiants exacts disponibles (noms des AtomicTasks enregistrées, clés Blackboard, etc.) ou consulter une documentation externe. Toute faute de frappe produisait un graphe silencieusement invalide (la règle E008/W001 du VSGraphVerifier pouvait le détecter après coup, mais pas au moment de la saisie).

**Après cette feature :** Tous les paramètres sont sélectionnables via des dropdowns guidés. 5 registries centralisées exposent les valeurs disponibles. L'utilisateur ne peut plus saisir de valeur invalide via l'UI — les erreurs de configuration sont éliminées à la source.

### 1.2 But du Système

Le système de Parameter Dropdowns transforme le Blueprint Editor d'un outil "expert" (qui requiert de connaître les identifiants exacts) en outil "découvrable" (où l'utilisateur explore les options disponibles via des menus). C'est la différence entre un éditeur de code brut et un éditeur visuel avec auto-complétion.

Les 5 registries centralisées constituent un annuaire des ressources disponibles dans le projet — elles seront la fondation des phases futures d'intégration avec le système ECS (enregistrement automatique des AtomicTasks, discovery des variables Blackboard, etc.).

### 1.3 Critères de Succès
- [x] 5 registries centralisées créées et fonctionnelles
- [x] Tous les champs de paramètres remplacés par des dropdowns
- [x] `ParameterBindingType` étendu à 7 types (dont 5 nouveaux)
- [x] 8 nouvelles règles de validation (E020–E025, W010–W011)
- [x] Sérialisation JSON mise à jour (version v4)
- [x] 30+ tests passants
- [x] Modifications undoables via les nouvelles commandes

---

## 2. 🏗️ Architecture

### 2.1 Vue d'ensemble

```
ParameterEditorRegistry (Master Registry)
        │
        ├── AtomicTaskUIRegistry ────── "Attack", "Patrol", "Heal"
        ├── ConditionRegistry ─────────  "HealthBelow", "EnemyInRange"
        ├── BBVariableRegistry ────────  Wrapper sur LocalBlackboard
        ├── OperatorRegistry ──────────  "+", "-", "*", "==", "<", ">"
        └── [SubGraphRegistry — implicite via filesystem]
        
Properties Panel
        │
        ├── Nœud AtomicTask
        │   └── AtomicTaskID: [Attack ▼]  ← dropdown AtomicTaskUIRegistry
        ├── Nœud Condition
        │   └── ConditionID: [HealthBelow ▼] ← dropdown ConditionRegistry
        ├── Nœud MathOp
        │   └── Operator: [+ ▼]  ← dropdown OperatorRegistry
        ├── Nœud BBRead/BBWrite
        │   └── BBKey: [bb_health ▼] ← dropdown BBVariableRegistry
        └── Nœud SubGraph
            └── Path: [ai_patrol.graph ▼] ← dropdown filesystem
```

### 2.2 Composants Principaux

| Composant | Rôle | Type |
|-----------|------|------|
| `AtomicTaskUIRegistry` | Registry des AtomicTasks disponibles avec catégories | Nouveau |
| `ConditionRegistry` | Registry des types de conditions disponibles | Nouveau |
| `BBVariableRegistry` | Wrapper sur LocalBlackboard pour exposer les clés connues | Nouveau |
| `OperatorRegistry` | Registry statique des opérateurs math et comparaison | Nouveau |
| `ParameterEditorRegistry` | Master registry — point d'entrée unique pour tous les dropdowns | Nouveau |
| Dropdown UI components | Remplacement des InputText par des ComboBox ImGui | Nouveau |
| `SetParameterBindingCommand` | Commande undoable pour changement de binding de paramètre | Nouveau |
| `SetParameterValueCommand` | Commande undoable pour changement de valeur de paramètre | Nouveau |

### 2.3 Flux de Données

1. Au chargement du projet, les registries sont peuplées (AtomicTasks enregistrées, clés Blackboard, etc.)
2. L'utilisateur ouvre le panneau Properties d'un nœud
3. Le panneau demande à `ParameterEditorRegistry` la liste des options pour chaque champ
4. Les dropdowns sont rendus avec les valeurs disponibles
5. La sélection d'une valeur crée une commande undoable (`SetParameterBindingCommand` ou `SetParameterValueCommand`)
6. La commande met à jour le `ParameterBindingType` et la valeur du paramètre

---

## 3. 💡 Décisions Techniques

### 3.1 Décision Principale — Master Registry Pattern

**Approche choisie :** Un `ParameterEditorRegistry` central qui agrège les 4 registries spécialisées. Le panneau Properties interagit uniquement avec le master registry — il ne connaît pas les registries individuelles.

**Pourquoi cette approche :**
- Point d'entrée unique pour le panneau Properties — simplifie le code UI
- Les registries spécialisées peuvent évoluer indépendamment sans impacter le panneau
- Facilite le remplacement futur d'une registry (ex: AtomicTaskUIRegistry → découverte automatique via ECS)
- Cohérent avec le pattern "facade" — le panneau ne gère pas la complexité de dispatch

**Alternatives rejetées :**

| Alternative | Raison du rejet |
|-------------|----------------|
| Le panneau Properties accède directement aux 4 registries | Couplage fort du panneau sur toutes les registries — toute nouvelle registry nécessite une modification du panneau |
| Registry unique plate avec tous les types mélangés | Manque de typage — difficile de filtrer "uniquement les opérateurs math" vs "uniquement les opérateurs de comparaison" |
| Hardcoded lists dans le panneau | Non extensible — nécessite recompilation pour ajouter une nouvelle AtomicTask |

**Trade-offs acceptés :**
- ✅ Extensibilité : ajouter une nouvelle registry ne nécessite pas de modifier le panneau Properties
- ✅ Testabilité : chaque registry est testable indépendamment
- ⚠️ Indirection supplémentaire : le panneau ne voit pas directement les registries spécialisées

---

### 3.2 Décision Secondaire — Extension de ParameterBindingType

**Approche choisie :** L'enum `ParameterBindingType` est étendu avec 5 nouveaux types en plus des 2 existants (Literal, LocalVariable).

**Nouveaux types :**
- `AtomicTaskID` — référence à une AtomicTask par son ID dans la registry
- `ConditionID` — référence à un type de condition par son ID
- `MathOperator` — opérateur mathématique sélectionné dans la registry
- `ComparisonOp` — opérateur de comparaison sélectionné
- `SubGraphPath` — chemin vers un fichier SubGraph

**Justification :** Étendre l'enum existant est plus cohérent que de créer un nouveau mécanisme de binding. La sérialisation JSON existante gère déjà `ParameterBindingType` — la mise à jour vers v4 est minimale.

---

### 3.3 Décision Tertiaire — Registries peuplées manuellement (Phase 22-C)

**Approche choisie :** Les registries AtomicTask et Condition sont peuplées manuellement dans le code pour Phase 22-C, en anticipation d'un enregistrement automatique via ECS dans une phase future.

**Justification :** L'enregistrement automatique via ECS requiert une infrastructure non encore disponible. Le peuplement manuel permet de livrer les dropdowns fonctionnels maintenant, avec une migration vers l'auto-discovery qui sera transparente pour le panneau Properties (qui passe par `ParameterEditorRegistry`).

---

## 4. 📦 Implications sur les Modules

### 4.1 Modules Directement Modifiés

| Module | Fichiers | Nature des Changements |
|--------|----------|----------------------|
| `BlueprintEditor` | `AtomicTaskUIRegistry.h/.cpp` | Nouveau — registry AtomicTasks avec catégories |
| `BlueprintEditor` | `ConditionRegistry.h/.cpp` | Nouveau — registry types conditions |
| `BlueprintEditor` | `BBVariableRegistry.h/.cpp` | Nouveau — wrapper LocalBlackboard |
| `BlueprintEditor` | `OperatorRegistry.h/.cpp` | Nouveau — registry statique opérateurs |
| `BlueprintEditor` | `ParameterEditorRegistry.h/.cpp` | Nouveau — master registry |
| `BlueprintEditor` | `VisualScriptEditorPanel.cpp` | Remplacement InputText → ComboBox pour tous les paramètres |
| `BlueprintEditor` | `UndoRedoStack.h/.cpp` | SetParameterBindingCommand, SetParameterValueCommand |
| `BlueprintEditor` | `VisualScriptNodes.h/.cpp` | Extension enum ParameterBindingType (7 types) |
| `BlueprintEditor` | JSON serialization | Mise à jour vers v4 avec nouveaux types |
| `Tests/BlueprintEditor` | `Phase22CTest.cpp` | 30+ tests |
| Build | `CMakeLists.txt` | Ajout cibles |

### 4.2 Risques de Couplage

- **VisualScriptEditorPanel → ParameterEditorRegistry** : Couplage via master registry — justifié par le design
- **BBVariableRegistry → LocalBlackboard** : Wrapper en lecture seule — couplage minimal

### 4.3 Impact sur les Phases Futures

- Phase 21-A/21-B (VSGraphVerifier) : les nouvelles règles E020–E025 étendent les 14 règles initiales
- Phase future (ECS integration) : les registries peuvent être peuplées automatiquement via ECS sans modifier le panneau

---

## 5. ⚠️ Matrice de Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Registries non peuplées au démarrage | Moyen | Élevé | Valeurs par défaut dans chaque registry + assert si registry vide |
| Incompatibilité JSON v3 → v4 | Moyen | Moyen | Migration automatique au chargement : si type inconnu → fallback sur Literal |
| Dropdown avec trop d'options (UX) | Faible | Moyen | Filtrage/recherche dans le ComboBox ImGui pour les registries larges |
| SetParameterBindingCommand et SetParameterValueCommand — Undo partiel | Faible | Moyen | Les deux commandes stockent la valeur complète (binding + value) pour Undo fidèle |

---

## 6. 📁 Détails d'Implémentation

### 6.1 Fichiers Créés

| Fichier | Rôle |
|---------|------|
| `Source/BlueprintEditor/AtomicTaskUIRegistry.h/.cpp` | Registry des AtomicTasks disponibles (avec catégories) |
| `Source/BlueprintEditor/ConditionRegistry.h/.cpp` | Registry des types de conditions |
| `Source/BlueprintEditor/BBVariableRegistry.h/.cpp` | Wrapper sur LocalBlackboard pour les clés connues |
| `Source/BlueprintEditor/OperatorRegistry.h/.cpp` | Registry statique des opérateurs math et comparaison |
| `Source/BlueprintEditor/ParameterEditorRegistry.h/.cpp` | Master registry — point d'entrée unique |
| `Tests/BlueprintEditor/Phase22CTest.cpp` | 30+ tests de validation |

### 6.2 Fichiers Modifiés

| Fichier | Modification |
|---------|-------------|
| `Source/BlueprintEditor/VisualScriptNodes.h` | Enum ParameterBindingType étendu (7 types) |
| `Source/BlueprintEditor/VisualScriptNodes.cpp` | Sérialisation JSON v4 |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | UI dropdowns pour tous les paramètres |
| `Source/BlueprintEditor/UndoRedoStack.h` | Déclaration SetParameterBindingCommand, SetParameterValueCommand |
| `Source/BlueprintEditor/UndoRedoStack.cpp` | Implémentation des deux commandes |
| `Source/BlueprintEditor/VSGraphVerifier.cpp` | Ajout règles E020–E025, W010–W011 |
| `CMakeLists.txt` | Ajout cibles OlympePhase22CTests |

### 6.3 Patterns Utilisés

- **Registry Pattern** — chaque registry expose une liste d'identifiants/labels pour les dropdowns
- **Facade Pattern** — ParameterEditorRegistry masque la complexité des 4 registries spécialisées
- **Command Pattern** — SetParameterBindingCommand et SetParameterValueCommand pour l'Undo/Redo
- **Commit-on-release** — les modifications de paramètres sont committées quand l'utilisateur ferme le dropdown

### 6.4 Nouvelles Règles de Validation VSGraphVerifier

| ID | Sévérité | Description |
|----|----------|-------------|
| E020 | ERROR | AtomicTaskID référencé non trouvé dans AtomicTaskUIRegistry |
| E021 | ERROR | ConditionID référencé non trouvé dans ConditionRegistry |
| E022 | ERROR | Clé Blackboard référencée non trouvée dans BBVariableRegistry |
| E023 | ERROR | MathOperator invalide (non dans OperatorRegistry) |
| E024 | ERROR | SubGraphPath référencé inexistant sur le filesystem |
| E025 | ERROR | Paramètre de condition requis manquant |
| W010 | WARNING | Incompatibilité de type entre clé BBVariable et pin Data |
| W011 | WARNING | Opérandes MathOp de types différents (peut causer conversion implicite) |

---

## 7. ✅ Règles de Validation et Cas de Tests

### 7.1 Sélection des Cas de Tests Critiques

| # | Scénario | Préconditions | Résultat Attendu | Statut |
|---|----------|--------------|-----------------|--------|
| 1 | Dropdown AtomicTask visible | AtomicTaskUIRegistry peuplée | Dropdown affiche les tasks disponibles | ✅ |
| 2 | Sélection AtomicTask | "Attack" sélectionné | ParameterBindingType = AtomicTaskID, value = "Attack" | ✅ |
| 3 | Undo sélection AtomicTask | "Attack" sélectionné puis Undo | Valeur précédente restaurée | ✅ |
| 4 | Save/Load avec AtomicTaskID | Graphe avec AtomicTask "Patrol" | Après reload : "Patrol" toujours sélectionné | ✅ |
| 5 | E020 — AtomicTask introuvable | "TaskInexistante" dans graphe | E020 déclenché par VSGraphVerifier | ✅ |
| 6 | E021 — Condition introuvable | "CondInexistante" dans graphe | E021 déclenché | ✅ |
| 7 | E022 — BBKey introuvable | "clé_inconnue" dans graphe | E022 déclenché | ✅ |
| 8 | W010 — BBKey type incompatible | Float key sur pin String | W010 déclenché | ✅ |
| 9 | Dropdown vide si registry vide | Registry sans entrées | Dropdown affiche "(empty)" | ✅ |
| 10 | Migration JSON v3 → v4 | Fichier JSON avec binding Literal | Chargement sans erreur, type conservé | ✅ |
| 11–30 | [Tests opérateurs, SubGraph, Condition détaillés] | Scénarios variés | Comportement cohérent | ✅ |

---

## 8. 📜 Historique des Propositions Copilot

### Proposition 1 — 2026-03-14 20:00:00 UTC

**Contexte :** Identification du gap UX majeur — impossibilité de découvrir les valeurs valides pour les paramètres.

**Proposition :**
- Créer 5 registries centralisées pour exposer les valeurs disponibles
  - AtomicTaskUIRegistry : tasks avec catégories (Combat, Movement, etc.)
  - ConditionRegistry : types de conditions avec descriptions
  - BBVariableRegistry : wrapper sur LocalBlackboard (clés + types)
  - OperatorRegistry : opérateurs math (+, -, *, /) et comparaison (==, <, >, !=)
  - ParameterEditorRegistry : master registry comme point d'entrée unique
- Remplacer tous les champs InputText par des ComboBox ImGui guidés
  - Filtrage par catégorie dans AtomicTaskUIRegistry
  - Recherche textuelle pour les registries larges
- Étendre ParameterBindingType avec 5 nouveaux types
  - AtomicTaskID, ConditionID, MathOperator, ComparisonOp, SubGraphPath
- Ajouter 8 nouvelles règles de validation au VSGraphVerifier (E020–E025, W010–W011)
- Migrer JSON vers v4 (ajout des nouveaux types de binding)
- Deux nouvelles commandes Undo/Redo
  - SetParameterBindingCommand : changement du type de binding
  - SetParameterValueCommand : changement de la valeur dans le binding courant

**Décision :** ✅ Acceptée par @Atlasbruce le 2026-03-14 22:45:00 UTC  
**Raison :** Résout un problème UX fondamental — sans cette feature, le Blueprint Editor ne peut pas être utilisé sans documentation externe extensive.

---

### Proposition 2 — 2026-03-14 21:00:00 UTC

**Contexte :** Discussion sur l'approche de peuplement des registries (automatique via ECS vs manuel).

**Proposition :**
- Peuplement manuel pour Phase 22-C
  - Listes hardcodées dans les registries pour la phase actuelle
  - Interface claire (`RegisterAtomicTask(id, label, category)`) pour ajout futur
- Peuplement automatique via ECS reporté à une phase future
  - L'API des registries est conçue pour supporter les deux modes
  - Migration transparente — le panneau Properties ne change pas

**Décision :** ✅ Acceptée  
**Raison :** Pragmatique — livre la valeur UX maintenant sans attendre l'infrastructure ECS. L'API est conçue pour évoluer.

---

## 9. 🏛️ Notes Archéologiques

### Décisions Abandonnées

| Décision | Date | Raison de l'abandon | Leçon tirée |
|----------|------|--------------------|-----------  |
| InputText avec auto-complétion custom | 2026-03-14 20:30:00 UTC | Complexité d'implémentation élevée, ImGui ne supporte pas nativement l'auto-complétion | Le ComboBox ImGui est suffisant et cohérent avec le reste de l'interface |
| Registry plate unique pour tous les types | 2026-03-14 21:00:00 UTC | Manque de typage — impossible de filtrer par type de paramètre | Les registries spécialisées + master registry est le bon équilibre |

### Apprentissages Clés

- Le design "Master Registry + Registries spécialisées" est extensible sans modification du panneau Properties — pattern à réutiliser pour les phases ECS futures
- L'extension de `ParameterBindingType` est la bonne approche — pas un nouveau mécanisme de binding
- Les nouvelles règles de validation (E020–E025) s'intègrent naturellement dans le VSGraphVerifier existant grâce au design extensible de Phase 21-A
- La migration JSON v3 → v4 avec fallback sur Literal est la stratégie de migration la plus sûre

---

## 10. 🐛 Problèmes Connus & Régressions

> **Statut de la Phase :** ✅ COMPLÉTÉE — PR [#386](https://github.com/Atlasbruce/Olympe-Engine/pull/386) mergée le 2026-03-15 UTC

### 10.1 Bugs Actifs

*Aucun bug actif sur cette phase.*

| ID | Titre | Sévérité | Statut | Workaround |
|----|-------|----------|--------|-----------|
| — | Aucun | — | — | — |

### 10.2 Problèmes Connus Non Classifiés

- Le peuplement des registries est **manuel** (hardcodé) dans cette phase — les valeurs disponibles dans les dropdowns ne reflètent pas automatiquement les AtomicTasks/Conditions enregistrées dans l'ECS. Non bloquant car le peuplement automatique via ECS est planifié pour une phase future. L'API des registries est conçue pour supporter les deux modes sans modification du panneau Properties.
- La `BBVariableRegistry` est un wrapper sur `LocalBlackboard` — si le Blackboard est vide ou non initialisé, le dropdown des variables sera vide. Comportement attendu mais peut surprendre si le Blackboard n'est pas configuré.

### 10.3 Bugs Corrigés Durant Cette Phase

*Aucun bug corrigé documenté. Les nouvelles règles E020–E025 et W010–W011 de VSGraphVerifier ont été ajoutées pour détecter les erreurs de configuration des paramètres au niveau validation graphe (comportement préventif, pas correctif).*

| ID | Titre | Sévérité | PR Fix | Résolu le |
|----|-------|----------|--------|-----------|
| — | — | — | — | — |

### 10.4 Tests de Régression

- **Couverture :** 30+ tests passants (Phase22CTest.cpp) couvrant les 5 registries, dropdowns, Undo/Redo, et règles de validation
- **Fichier de tests :** `Tests/BlueprintEditor/Phase22CTest.cpp`
- **Règles testées :** E020 (Invalid AtomicTaskID), E021 (Invalid ConditionID), E022–E025, W010–W011
- **Migration JSON :** v3 → v4 testée avec fallback Literal
- **Point de vigilance :** Toute modification de `ParameterBindingType` dans `TaskGraphTypes.h` est une régression potentielle pour cette phase et pour les phases ECS futures

### 10.5 Références Bug Registry

- [BUG_REGISTRY.md](../BugTracking/BUG_REGISTRY.md) — Registre centralisé
- [BUG_PROTOCOL.md](../BugTracking/BUG_PROTOCOL.md) — Protocole de gestion

---

## 11. 🔗 Références

- **PR GitHub :** [#386](https://github.com/Atlasbruce/Olympe-Engine/pull/386)
- **ROADMAP_V2.md :** Section Phase 22-C
- **Context Files :**
  - [CONTEXT_ARCHIVE.md](../CONTEXT_ARCHIVE.md) — Section Phase 22-C
  - [feature_context_21_A.md](./feature_context_21_A.md) — Phase 21-A (VSGraphVerifier — règles étendues par 22-C)
  - [feature_context_22_A.md](./feature_context_22_A.md) — Phase 22-A (VSSwitch — BBVariableRegistry utilisée pour runtime display)

---

*Feature context généré le : 2026-03-15 10:26:37 UTC*  
*Dernière mise à jour : 2026-03-15 12:44:21 UTC*
