# 📄 Feature Context — [Phase XX-Y] : [Nom de la Feature]

> **Template Version:** 1.0  
> **Créé le:** YYYY-MM-DD HH:MM:SS UTC  
> **Dernière mise à jour:** YYYY-MM-DD HH:MM:SS UTC  
> **Statut:** 🔧 EN COURS | ✅ COMPLÉTÉ | ❌ ABANDONNÉ  
> **PR:** #XXX ([lien GitHub](https://github.com/Atlasbruce/Olympe-Engine/pull/XXX))  
> **Références croisées:** [feature_context_XX_Z.md](./feature_context_XX_Z.md)

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu
<!-- Expliquer quel problème utilisateur ou technique cette feature résout.
     Éviter le code — parler du POURQUOI. -->

**Avant cette feature :** [Description de la situation problématique]  
**Après cette feature :** [Description de la situation améliorée]

### 1.2 But du Système
<!-- Quel est le rôle de ce système dans l'architecture globale d'Olympe Engine ?
     Comment s'intègre-t-il avec les autres composants ? -->

[Description du rôle et de la valeur ajoutée dans le système global]

### 1.3 Critères de Succès
- [ ] [Critère mesurable 1]
- [ ] [Critère mesurable 2]
- [ ] [Critère mesurable 3]

---

## 2. 🏗️ Architecture

### 2.1 Vue d'ensemble

<!-- Diagramme ASCII si utile pour clarifier la structure -->

```
[Composant A] ──── [Composant B]
      │                  │
      ▼                  ▼
[Composant C] ──── [Composant D]
```

### 2.2 Composants Principaux

| Composant | Rôle | Type |
|-----------|------|------|
| `NomComposant` | [description] | Nouveau / Modifié |
| `NomComposant2` | [description] | Nouveau / Modifié |

### 2.3 Flux de Données

<!-- Décrire comment les données circulent dans le système, sans code -->

1. [Étape 1 : d'où viennent les données]
2. [Étape 2 : comment elles sont transformées]
3. [Étape 3 : où elles aboutissent]

---

## 3. 💡 Décisions Techniques

### 3.1 Décision Principale — [Titre de la décision]

**Approche choisie :** [Description de l'approche retenue]

**Pourquoi cette approche :**
- [Raison 1]
- [Raison 2]

**Alternatives rejetées :**

| Alternative | Raison du rejet |
|-------------|----------------|
| [Alternative A] | [Pourquoi rejetée] |
| [Alternative B] | [Pourquoi rejetée] |

**Trade-offs acceptés :**
- ✅ [Avantage]
- ✅ [Avantage]
- ⚠️ [Inconvénient accepté et pourquoi]

---

### 3.2 Décision Secondaire — [Titre si applicable]

**Approche choisie :** [Description]

**Justification :** [Explication concise]

---

## 4. 📦 Implications sur les Modules

### 4.1 Modules Directement Modifiés

| Module | Fichiers | Nature des Changements |
|--------|----------|----------------------|
| `ModuleA` | `File.h`, `File.cpp` | [description] |
| `ModuleB` | `File2.cpp` | [description] |

### 4.2 Risques de Couplage

<!-- Identifier les dépendances nouvelles ou renforcées et pourquoi elles sont acceptables -->

- **[ModuleA] → [ModuleB]** : [Nature du couplage] — [Justification d'acceptabilité]
- **[ModuleC] → [ModuleA]** : [Nature du couplage] — [Justification d'acceptabilité]

### 4.3 Impact sur les Phases Futures

- [Phase XX-Z] sera facilitée/complexifiée par cette décision parce que [raison]

---

## 5. ⚠️ Matrice de Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| [Risque 1] | Faible/Moyen/Élevé | Faible/Moyen/Élevé | [Action concrète] |
| [Risque 2] | Faible/Moyen/Élevé | Faible/Moyen/Élevé | [Action concrète] |
| [Risque 3] | Faible/Moyen/Élevé | Faible/Moyen/Élevé | [Action concrète] |

### Risques Matérialisés en Production
<!-- Remplir après merge si un risque s'est concrétisé -->

- [Date UTC] : [Description de ce qui s'est passé et comment résolu]

---

## 6. 📁 Détails d'Implémentation

### 6.1 Fichiers Créés

| Fichier | Rôle |
|---------|------|
| `Source/Module/NewFile.h` | [Description du rôle du fichier] |
| `Source/Module/NewFile.cpp` | [Description du rôle du fichier] |
| `Tests/Module/PhaseXXYTest.cpp` | [Description des tests] |

### 6.2 Fichiers Modifiés

| Fichier | Modification |
|---------|-------------|
| `Source/Module/ExistingFile.h` | [Description de la modification] |
| `Source/Module/ExistingFile.cpp` | [Description de la modification] |
| `CMakeLists.txt` | Ajout des nouvelles cibles de compilation |

### 6.3 Patterns Utilisés

<!-- Nommer les patterns, pas les implémenter -->

- **[Pattern 1]** (ex: Command Pattern) — utilisé pour [raison]
- **[Pattern 2]** (ex: Registry Pattern) — utilisé pour [raison]

### 6.4 Conventions Respectées

- C++14 strict — pas d'auto, pas de C++17 features
- Nommage : VSNomClasse pour les noeuds visuels, XxxCommand pour les commandes undoable
- [Autre convention spécifique à cette feature]

---

## 7. ✅ Règles de Validation et Cas de Tests

### 7.1 Règles de Validation Créées

| ID | Sévérité | Description | Condition de déclenchement |
|----|----------|-------------|---------------------------|
| E0XX | ERROR | [description] | [quand cette règle s'applique] |
| W0XX | WARNING | [description] | [quand cette règle s'applique] |
| I0XX | INFO | [description] | [quand cette règle s'applique] |

### 7.2 Cas de Tests

| # | Scénario | Préconditions | Résultat Attendu | Statut |
|---|----------|--------------|-----------------|--------|
| 1 | [Description du scénario] | [État initial] | [Résultat attendu] | ✅ / ❌ |
| 2 | [Description du scénario] | [État initial] | [Résultat attendu] | ✅ / ❌ |
| 3 | [Edge case] | [État initial] | [Résultat attendu] | ✅ / ❌ |

### 7.3 Tests de Régression

<!-- Phases précédentes dont les tests ont été vérifiés -->

- Phase XX-Z : [X]/[Total] tests passent après cette modification ✅

---

## 8. 📜 Historique des Propositions Copilot

### Proposition 1 — [YYYY-MM-DD HH:MM:SS UTC]

**Contexte :** [Quelle question ou problème a généré cette proposition]

**Proposition :**
- [Point principal de la proposition]
  - [Détail niveau 2]
  - [Détail niveau 2]
- [Autre point]
  - [Détail niveau 2]

**Décision :** ✅ Acceptée / ❌ Rejetée / 🔄 Modifiée  
**Raison :** [Justification de la décision par l'utilisateur]

---

### Proposition 2 — [YYYY-MM-DD HH:MM:SS UTC]

**Contexte :** [Quelle question ou problème a généré cette proposition]

**Proposition :**
- [Point principal]
  - [Détail]

**Décision :** ✅ Acceptée / ❌ Rejetée / 🔄 Modifiée  
**Raison :** [Justification]

---

## 9. 🏛️ Notes Archéologiques

### Décisions Abandonnées

| Décision | Date | Raison de l'abandon | Leçon tirée |
|----------|------|--------------------|-----------  |
| [Approche abandonnée] | YYYY-MM-DD HH:MM:SS UTC | [Pourquoi abandonnée] | [Ce qu'on a appris] |

### Problèmes Rencontrés et Solutions

| Problème | Date | Solution Adoptée |
|----------|------|-----------------|
| [Description du problème] | YYYY-MM-DD HH:MM:SS UTC | [Comment résolu] |

### Apprentissages Clés

- [Apprentissage 1 applicable aux phases futures]
- [Apprentissage 2]

---

## 10. 🔗 Références

- **PR GitHub :** [#XXX](https://github.com/Atlasbruce/Olympe-Engine/pull/XXX)
- **ROADMAP_V2.md :** Section [Phase XX-Y]
- **Context Files :**
  - [CONTEXT_ARCHIVE.md](../CONTEXT_ARCHIVE.md) — Section Phase XX-Y
  - [feature_context_XX_Z.md](./feature_context_XX_Z.md) — Phase dépendante

---

*Feature context généré le : YYYY-MM-DD HH:MM:SS UTC*  
*Template v1.0 — Olympe Engine PM System*
