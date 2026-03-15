# 📄 Feature Context — Phase 21-A : VSGraphVerifier (Stateless Graph Validation)

> **Créé le:** 2026-03-15 10:26:37 UTC  
> **Dernière mise à jour:** 2026-03-15 10:26:37 UTC  
> **Statut:** ✅ COMPLÉTÉ  
> **PR:** [#380](https://github.com/Atlasbruce/Olympe-Engine/pull/380)  
> **Références croisées:** [feature_context_21_B.md](./feature_context_21_B.md) · [feature_context_21_D.md](./feature_context_21_D.md)

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu

**Avant cette feature :** Le `VSConnectionValidator` ne validait que les connexions individuelles au moment de leur création (self-loop, duplicate pin, cycle DFS local). Il était impossible de valider la cohérence globale du graphe — par exemple, un graphe sans EntryPoint ou avec des nœuds orphelins était accepté silencieusement.

**Après cette feature :** Le `VSGraphVerifier` analyse la totalité du graphe et produit une liste structurée de messages d'erreur (ERROR), avertissement (WARNING) et information (INFO) exploitables dans l'éditeur. La validation est globale, non destructive et appelable à tout moment.

### 1.2 But du Système

Le `VSGraphVerifier` est le compilateur-validateur sémantique du Blueprint Editor. Son rôle est d'offrir à l'utilisateur un retour clair et exploitable sur la validité de son graphe avant exécution ou sauvegarde. Il s'inspire des systèmes de lint et de compilation des IDEs modernes (erreurs avec codes, sévérités, localisation dans le graphe).

Il est **stateless** : il ne stocke aucun état entre les appels. Chaque appel à `Verify()` analyse le graphe complet depuis zéro. Cette approche garantit la cohérence et simplifie la testabilité.

### 1.3 Critères de Succès
- [x] 14 règles de validation implémentées et testées (E001–E009, W001–W004, I001)
- [x] Structure `VSVerificationIssue` retournée par `Verify()` (nodeID, severity, ruleID, message)
- [x] Tests headless couvrant toutes les règles
- [x] Aucune modification des structures de données existantes (VSNode, VSLink)

---

## 2. 🏗️ Architecture

### 2.1 Vue d'ensemble

```
VisualScriptEditorPanel
        │
        │ appelle Verify(nodes, links)
        ▼
   VSGraphVerifier  ──────────────────────────────────────
        │                                                 │
        │ retourne vector<VSVerificationIssue>            │
        ▼                                                 │
   Issues Panel (Phase 21-B)      Node highlighting  ←───┘
```

### 2.2 Composants Principaux

| Composant | Rôle | Type |
|-----------|------|------|
| `VSGraphVerifier` | Classe stateless — méthode `Verify()` principale | Nouveau |
| `VSVerificationIssue` | Struct : nodeID, severity, ruleID, message | Nouveau |
| `VSVerificationSeverity` | Enum : ERROR, WARNING, INFO | Nouveau |

### 2.3 Flux de Données

1. L'éditeur appelle `VSGraphVerifier::Verify(nodes, links)` avec l'état actuel du graphe
2. Le verifier exécute les 14 règles séquentiellement sur le graphe en lecture seule
3. Chaque règle violée ajoute un `VSVerificationIssue` à la liste résultat
4. La liste est retournée à l'appelant pour affichage (Phase 21-B) ou blocage (Phase 21-C)

---

## 3. 💡 Décisions Techniques

### 3.1 Décision Principale — Verifier Stateless vs Stateful

**Approche choisie :** Verifier entièrement stateless — `Verify()` prend le graphe en paramètre et retourne les issues sans stocker d'état.

**Pourquoi cette approche :**
- Simplicité de test : pas de setup/teardown d'état entre tests
- Thread-safety implicite (pas de membres mutables)
- Cohérence garantie : chaque appel analyse l'état réel du graphe au moment de l'appel
- Pas de risque de désynchronisation entre état interne et état du graphe

**Alternatives rejetées :**

| Alternative | Raison du rejet |
|-------------|----------------|
| Verifier stateful (cache des résultats) | Risque de désynchronisation si le graphe change entre deux appels ; complexité inutile à ce stade |
| Validation intégrée dans VSNode | Violerait le principe de responsabilité unique ; un nœud ne doit pas connaître le graphe entier |
| Validation au moment de chaque PushCommand | Trop lent pour les grosses opérations en batch (ex: paste multiple nodes) |

**Trade-offs acceptés :**
- ✅ Testabilité maximale
- ✅ Aucune mémoire supplémentaire entre les appels
- ⚠️ Re-analyse complète du graphe à chaque appel (acceptable jusqu'à ~1000 nœuds)

---

### 3.2 Décision Secondaire — Système de codes de règles (E/W/I + numéro)

**Approche choisie :** Codes structurés `E001`, `W001`, `I001` inspirés des compilateurs et linters (GCC, ESLint, Unreal Engine).

**Justification :** Permet à l'utilisateur de chercher la signification d'une règle par son code. Facilite le filtrage (afficher seulement les ERRORs). Prépare l'intégration d'une documentation des règles.

---

## 4. 📦 Implications sur les Modules

### 4.1 Modules Directement Modifiés

| Module | Fichiers | Nature des Changements |
|--------|----------|----------------------|
| `BlueprintEditor` | `VSGraphVerifier.h`, `VSGraphVerifier.cpp` | Création complète |
| `Tests/BlueprintEditor` | `Phase21ATest.cpp` | Tests des 14 règles |
| Build | `CMakeLists.txt` | Ajout cible OlympePhase21ATests |

### 4.2 Risques de Couplage

- **VSGraphVerifier → VSNode/VSLink** : Lecture seule via const refs — couplage minimal et justifié
- **VisualScriptEditorPanel → VSGraphVerifier** : Appel direct de `Verify()` — dépendance unidirectionnelle propre

### 4.3 Impact sur les Phases Futures

- Phase 21-B (Panel UI) : consomme directement le `vector<VSVerificationIssue>` retourné
- Phase 21-C (Pre-save/Pre-exec) : peut bloquer la sauvegarde si `Verify()` retourne des ERRORs
- Phase 22-C (Parameter Dropdowns) : a ajouté des règles E020–E025 qui étendent la liste des 14 règles initiales

---

## 5. ⚠️ Matrice de Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Performance sur très grands graphes (>500 nœuds) | Faible | Moyen | Appel asynchrone ou throttling si nécessaire (Phase future) |
| Faux positifs (règles trop strictes) | Moyen | Élevé | Seuil de sévérité configurable ; WARNING plutôt qu'ERROR pour les cas ambigus |
| Règles manquantes découvertes en production | Moyen | Faible | Système extensible — ajout de règles sans modifier l'API |

---

## 6. 📁 Détails d'Implémentation

### 6.1 Fichiers Créés

| Fichier | Rôle |
|---------|------|
| `Source/BlueprintEditor/VSGraphVerifier.h` | Déclaration de VSGraphVerifier, VSVerificationIssue, VSVerificationSeverity |
| `Source/BlueprintEditor/VSGraphVerifier.cpp` | Implémentation des 14 règles de validation |
| `Tests/BlueprintEditor/Phase21ATest.cpp` | Tests unitaires headless pour les 14 règles |

### 6.2 Fichiers Modifiés

| Fichier | Modification |
|---------|-------------|
| `CMakeLists.txt` | Ajout de la cible de build OlympePhase21ATests |

### 6.3 Patterns Utilisés

- **Stateless Service Pattern** — VSGraphVerifier est une classe sans état mutable
- **Value Object** — VSVerificationIssue est une struct immuable de données
- **Sequential Rule Execution** — les règles s'exécutent dans un ordre déterministe

### 6.4 Règles des 14 Validations

| ID | Sévérité | Règle |
|----|----------|-------|
| E001 | ERROR | Un seul nœud EntryPoint requis dans le graphe |
| E002 | ERROR | Nœud dangling — output non connecté dans un graphe avec ExecFlow |
| E003 | ERROR | Cycle exec détecté (DFS sur les liens exec) |
| E004 | ERROR | SubGraph circulaire — un SubGraph s'appelle lui-même |
| E005 | ERROR | Connexion invalide entre pin Exec et pin Data |
| E006 | ERROR | Types de data pins incompatibles (ex: float → string) |
| E007 | ERROR | Direction de pin inversée (output→output ou input→input) |
| E008 | ERROR | Clé Blackboard référencée inconnue |
| E009 | ERROR | Type Blackboard incompatible avec le pin Data |
| W001 | WARNING | AtomicTask avec AtomicTaskID vide |
| W002 | WARNING | Delay avec DelaySeconds ≤ 0 |
| W003 | WARNING | SubGraph avec SubGraphPath vide |
| W004 | WARNING | MathOp avec MathOperator vide |
| I001 | INFO | Nœud non atteignable depuis l'EntryPoint |

---

## 7. ✅ Règles de Validation et Cas de Tests

### 7.1 Cas de Tests (Phase 21-A)

| # | Scénario | Préconditions | Résultat Attendu | Statut |
|---|----------|--------------|-----------------|--------|
| 1 | Graphe valide minimal | EntryPoint → AtomicTask | 0 issues | ✅ |
| 2 | Aucun EntryPoint | Graphe sans nœud Entry | E001 déclenché | ✅ |
| 3 | Deux EntryPoints | Deux nœuds Entry | E001 déclenché | ✅ |
| 4 | Nœud dangling | Nœud isolé sans connexion | E002 déclenché | ✅ |
| 5 | Cycle exec | A→B→C→A | E003 déclenché | ✅ |
| 6 | Connexion Exec→Data | Pin exec branché sur pin data | E005 déclenché | ✅ |
| 7 | Types incompatibles | Float pin → String pin | E006 déclenché | ✅ |
| 8 | Clé Blackboard inconnue | BBKey "inexistant" dans graphe | E008 déclenché | ✅ |
| 9 | AtomicTask ID vide | AtomicTask sans ID | W001 déclenché | ✅ |
| 10 | Delay ≤ 0 | DelaySeconds = -1 | W002 déclenché | ✅ |
| 11 | SubGraph vide | SubGraphPath = "" | W003 déclenché | ✅ |
| 12 | MathOp vide | MathOperator = "" | W004 déclenché | ✅ |
| 13 | Nœud non atteignable | Nœud présent mais non connecté à EntryPoint | I001 déclenché | ✅ |
| 14 | Tous les warnings en même temps | Multiple problèmes | Toutes issues retournées | ✅ |

---

## 8. 📜 Historique des Propositions Copilot

### Proposition 1 — 2026-03-14 08:00:00 UTC

**Contexte :** Discussion sur la structure du Graph Verifier — stateless vs stateful.

**Proposition :**
- Implémenter VSGraphVerifier comme classe stateless avec méthode `Verify()` pure
  - Prend `const vector<VSNode>&` et `const vector<VSLink>&` en paramètres
  - Retourne `vector<VSVerificationIssue>` (pas de stockage interne)
  - Chaque règle est une méthode privée appelée séquentiellement
- Définir 14 règles initiales couvrant les cas les plus critiques
  - 9 erreurs bloquantes (E001–E009)
  - 4 avertissements (W001–W004)
  - 1 information (I001)
- Codes de règles inspirés des compilateurs (E/W/I + numéro à 3 chiffres)

**Décision :** ✅ Acceptée  
**Raison :** L'approche stateless est plus testable et plus simple à maintenir. Les codes de règles facilitent la documentation et le filtrage futur.

---

## 9. 🏛️ Notes Archéologiques

### Décisions Abandonnées

| Décision | Date | Raison de l'abandon | Leçon tirée |
|----------|------|--------------------|-----------  |
| Validation stateful avec cache | 2026-03-14 09:00:00 UTC | Risque de désynchronisation cache/graphe | Les verifiers doivent être stateless pour garantir la cohérence |
| Validation intégrée dans VSNode | 2026-03-14 09:00:00 UTC | Viole SRP — un nœud ne doit pas connaître le graphe | Séparation des responsabilités : le nœud gère ses données, le verifier gère la sémantique |

### Apprentissages Clés

- Un verifier stateless est beaucoup plus simple à tester et à étendre
- Les codes de règles structurés (E/W/I + numéro) s'avèrent très utiles pour le Panel UI (Phase 21-B)
- Les 14 règles initiales ont été suffisantes pour les phases 21/22 — extensibles sans modification de l'API

---

## 10. 🐛 Problèmes Connus & Régressions

> **Statut de la Phase :** ✅ COMPLÉTÉE — PR [#380](https://github.com/Atlasbruce/Olympe-Engine/pull/380) mergée le 2026-03-15 UTC

### 10.1 Bugs Actifs

*Aucun bug actif sur cette phase.*

| ID | Titre | Sévérité | Statut | Workaround |
|----|-------|----------|--------|-----------|
| — | Aucun | — | — | — |

### 10.2 Problèmes Connus Non Classifiés

*Aucun problème connu non classifié.*

### 10.3 Bugs Corrigés Durant Cette Phase

*Aucun bug corrigé documenté pour cette phase (phase initiale).*

| ID | Titre | Sévérité | PR Fix | Résolu le |
|----|-------|----------|--------|-----------|
| — | — | — | — | — |

### 10.4 Tests de Régression

- **Couverture :** 14 règles de validation testées (E001–E009, W001–W004, I001) — 100% des règles couvertes
- **Fichier de tests :** `Tests/BlueprintEditor/` (voir répertoire Tests pour les fichiers de test Phase 21-A)
- **Tests headless :** Oui — pas de dépendance à l'UI ou à SDL

### 10.5 Références Bug Registry

- [BUG_REGISTRY.md](../BugTracking/BUG_REGISTRY.md) — Registre centralisé
- [BUG_PROTOCOL.md](../BugTracking/BUG_PROTOCOL.md) — Protocole de gestion

---

## 11. 🔗 Références

- **PR GitHub :** [#380](https://github.com/Atlasbruce/Olympe-Engine/pull/380)
- **ROADMAP_V2.md :** Section Initiative A — Phase 21-A
- **Context Files :**
  - [CONTEXT_ARCHIVE.md](../CONTEXT_ARCHIVE.md) — Section Phase 21-A
  - [feature_context_21_B.md](./feature_context_21_B.md) — Phase 21-B (Panel UI, consomme les issues)
  - [feature_context_22_C.md](./feature_context_22_C.md) — Phase 22-C (a étendu les règles avec E020–E025)

---

*Feature context généré le : 2026-03-15 10:26:37 UTC*  
*Dernière mise à jour : 2026-03-15 12:44:21 UTC*
