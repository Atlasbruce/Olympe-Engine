# Système de tracking des Pull Requests

Ce document décrit le système automatique de suivi et documentation des Pull Requests mis en place dans ce projet.

## 📋 Vue d'ensemble

Le système de tracking des PRs est composé de 4 éléments principaux :

1. **Template d'issue de tracking** - Formulaire structuré pour documenter les PRs
2. **Workflow de tracking automatique** - Crée et met à jour des issues de tracking automatiquement
3. **Workflow de rapport mensuel** - Génère des statistiques mensuelles
4. **Workflow de backfill** - Crée des issues de tracking pour l'historique des PRs

## 🚀 Fonctionnalités

### 1. Tracking automatique des PRs (`.github/workflows/pr-tracking.yml`)

Ce workflow s'exécute automatiquement à chaque événement de PR :
- **Ouverture d'une PR** : Crée une issue de tracking avec toutes les informations
- **Modification de la PR** : Met à jour l'issue de tracking
- **Merge de la PR** : Ajoute un résumé final et ferme l'issue
- **Fermeture de la PR** : Ferme l'issue correspondante

#### Événements déclencheurs
- `opened` - Ouverture d'une nouvelle PR
- `edited` - Modification du titre ou de la description
- `closed` - Fermeture ou merge de la PR
- `reopened` - Réouverture d'une PR fermée
- `synchronize` - Nouveaux commits ajoutés
- `ready_for_review` - PR prête pour review
- `review_requested` - Review demandée

#### Détection automatique du type de PR

Le système détecte automatiquement le type de PR en analysant le titre :

| Mots-clés dans le titre | Type détecté |
|--------------------------|--------------|
| `fix`, `bug` | bugfix |
| `refactor`, `clean` | refactor |
| `perf`, `optim` | performance |
| `doc` | documentation |
| `test` | test |
| `chore` | chore |
| Par défaut | feature |

#### Informations collectées

Chaque issue de tracking contient :
- 🔗 Lien vers la PR associée
- 📅 Dates de création, merge et fermeture
- 👤 Auteur de la PR
- 📊 Statistiques (fichiers modifiés, lignes ajoutées/supprimées, commits)
- 🏷️ Type de modification
- 📝 Description et objectifs

### 2. Rapport mensuel (`.github/workflows/monthly-report.yml`)

Ce workflow génère automatiquement un rapport mensuel des PRs.

#### Déclenchement
- **Automatique** : Le 1er de chaque mois à 09:00 UTC
- **Manuel** : Via l'onglet "Actions" → "Monthly PR Report" → "Run workflow"

#### Contenu du rapport
- 📈 Statistiques globales (total, taux de merge, taux de fermeture)
- 💻 Statistiques de code (lignes ajoutées/supprimées, fichiers modifiés)
- 🏷️ Répartition par type de PR
- 👥 Répartition par auteur
- 🎯 Recommandations automatiques
- 📋 Liste détaillée de toutes les PRs du mois

### 3. Backfill historique (`.github/workflows/backfill-pr-tracking.yml`)

Ce workflow permet de créer des issues de tracking pour les PRs historiques.

#### Utilisation

1. Aller dans l'onglet "Actions" sur GitHub
2. Sélectionner "Backfill PR Tracking"
3. Cliquer sur "Run workflow"
4. Spécifier le nombre de mois à traiter (défaut : 4)
5. Lancer le workflow

#### Fonctionnement

Le workflow :
- ✅ Récupère toutes les PRs des X derniers mois
- ✅ Vérifie si une issue de tracking existe déjà
- ✅ Crée une issue pour chaque PR sans tracking
- ✅ Ferme immédiatement l'issue (puisque la PR est déjà fermée)
- ✅ Ajoute un commentaire sur la PR originale
- ✅ Génère un rapport de résumé
- ✅ Est idempotent (peut être réexécuté sans créer de doublons)
- ✅ Gère les erreurs avec des logs détaillés

#### Rapport de backfill

À la fin de l'exécution, une issue de rapport est créée avec :
- Paramètres utilisés (période, date)
- Résultats (succès, erreurs)
- Liste détaillée des PRs traitées
- Liste des éventuelles erreurs

### 4. Template d'issue (`.github/ISSUE_TEMPLATE/pr-tracking.yml`)

Le template d'issue est utilisé pour créer manuellement des issues de tracking si nécessaire.

#### Champs du formulaire
- Numéro de la PR et lien
- Dates de création et merge
- 🎯 Objectifs
- ⚠️ Problèmes rencontrés
- ✅ Solutions apportées
- 📊 Impact sur le projet
- Type de modification (dropdown)
- Priorité (dropdown)

## 🏷️ Labels utilisés

Le système applique automatiquement les labels suivants :

| Label | Description |
|-------|-------------|
| `pr-tracking` | Label principal pour toutes les issues de tracking |
| `documentation` | Marque les issues de documentation |
| `feature` | Nouvelle fonctionnalité |
| `bugfix` | Correction de bug |
| `refactor` | Refactoring |
| `performance` | Optimisation |
| `test` | Tests |
| `chore` | Maintenance |
| `draft` | PR en mode brouillon |
| `in-progress` | PR en cours |
| `merged` | PR mergée |
| `closed` | PR fermée sans merge |
| `report` | Rapports mensuels |

## 📊 Exemple d'utilisation

### Cycle de vie d'une PR avec tracking

1. **Ouverture de la PR #42**
   - ✅ Issue de tracking #43 créée automatiquement
   - 📋 Commentaire ajouté sur la PR avec lien vers l'issue
   - 🏷️ Labels : `pr-tracking`, `documentation`, `feature`, `in-progress`

2. **Modifications de la PR**
   - 🔄 Issue #43 mise à jour automatiquement
   - 📊 Statistiques actualisées

3. **PR prête pour review**
   - 💬 Commentaire ajouté sur l'issue #43
   - 🏷️ Label `in-progress` maintenu

4. **Merge de la PR #42**
   - ✅ Commentaire final avec statistiques sur l'issue #43
   - 🔒 Issue #43 fermée automatiquement
   - 🏷️ Label changé en `merged`

## 🔧 Configuration technique

### Permissions requises

Tous les workflows utilisent les permissions suivantes :

```yaml
permissions:
  issues: write
  pull-requests: read
  contents: read
```

### Authentification

Les workflows utilisent le token GitHub standard :
```yaml
github-token: ${{ secrets.GITHUB_TOKEN }}
```

Aucune configuration supplémentaire n'est nécessaire.

### Rate limiting

Le workflow de backfill inclut :
- ⏱️ Pause d'1 seconde entre chaque création d'issue
- 📦 Pagination (100 PRs par requête)
- 🔄 Gestion des erreurs avec logs détaillés

## 🎯 Bonnes pratiques

### Titres de PR

Pour une détection automatique optimale du type de PR, utilisez des préfixes clairs :

- ✅ `Fix: Correction du bug de mémoire`
- ✅ `Feature: Ajout du système de rendu 3D`
- ✅ `Refactor: Nettoyage du code ECS`
- ✅ `Perf: Optimisation du pathfinding`
- ✅ `Doc: Mise à jour du README`
- ✅ `Test: Tests unitaires pour GameEngine`
- ✅ `Chore: Mise à jour des dépendances`

### Documentation des issues de tracking

Bien que les issues soient créées automatiquement, il est recommandé de :
- 📝 Compléter les sections "Problèmes rencontrés" et "Solutions apportées"
- 💬 Ajouter des commentaires sur les décisions importantes
- 🔗 Référencer les issues liées

## 🔍 Consultation des données

### Rechercher les issues de tracking

Utilisez les filtres GitHub :
- `label:pr-tracking` - Toutes les issues de tracking
- `label:pr-tracking label:feature` - Issues de type feature
- `label:pr-tracking is:open` - Issues de tracking ouvertes
- `label:pr-tracking is:closed` - Issues de tracking fermées

### Rapports mensuels

- `label:report` - Tous les rapports
- `label:report label:pr-tracking` - Rapports PR uniquement

## 🆘 Dépannage

### Le workflow ne se déclenche pas

1. Vérifier que les permissions sont correctes dans les paramètres du repo
2. Vérifier que les workflows sont activés (Settings → Actions → General)
3. Vérifier les logs dans l'onglet "Actions"

### Issue de tracking non créée

1. Vérifier les logs du workflow dans l'onglet "Actions"
2. Vérifier que le label `pr-tracking` existe dans le repo
3. Créer manuellement l'issue avec le template si nécessaire

### Doublons d'issues de tracking

Le workflow de backfill est idempotent et vérifie les issues existantes avant création.
Si des doublons existent, les fermer manuellement.

## 📚 Références

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [actions/github-script](https://github.com/actions/github-script)
- [GitHub REST API](https://docs.github.com/en/rest)

---

_Ce système a été mis en place pour améliorer la traçabilité et la documentation du projet._
