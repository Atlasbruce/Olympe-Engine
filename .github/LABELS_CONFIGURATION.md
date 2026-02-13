# Configuration des labels pour le système de tracking des PRs

Ce fichier contient la configuration des labels nécessaires pour le système de tracking des Pull Requests.

## Labels requis

Les labels suivants doivent être créés dans le dépôt GitHub pour que le système fonctionne correctement :

### Label principal
- **pr-tracking** (couleur: `#0e8a16`) - Label principal pour toutes les issues de tracking
- **documentation** (couleur: `#0075ca`) - Pour marquer les issues de documentation
- **report** (couleur: `#1d76db`) - Pour les rapports mensuels et de backfill

### Types de PR
- **feature** (couleur: `#a2eeef`) - Nouvelle fonctionnalité
- **bugfix** (couleur: `#d73a4a`) - Correction de bug
- **refactor** (couleur: `#fbca04`) - Refactoring du code
- **performance** (couleur: `#0e8a16`) - Optimisation de performance
- **test** (couleur: `#bfd4f2`) - Tests
- **chore** (couleur: `#fef2c0`) - Maintenance et tâches diverses

### Status
- **draft** (couleur: `#6c6c6c`) - PR en mode brouillon
- **in-progress** (couleur: `#fbca04`) - PR en cours de développement
- **merged** (couleur: `#6f42c1`) - PR mergée
- **closed** (couleur: `#d73a4a`) - PR fermée sans merge

### Priorité (optionnel)
- **low** (couleur: `#c2e0c6`) - Priorité basse
- **medium** (couleur: `#fbca04`) - Priorité moyenne
- **high** (couleur: `#ff9800`) - Priorité haute
- **critical** (couleur: `#d73a4a`) - Priorité critique

## Création des labels

### Méthode 1 : Via l'interface GitHub

1. Aller dans l'onglet "Issues" du dépôt
2. Cliquer sur "Labels"
3. Cliquer sur "New label"
4. Entrer le nom, la description et la couleur
5. Cliquer sur "Create label"

### Méthode 2 : Via GitHub CLI

Si vous avez [GitHub CLI](https://cli.github.com/) installé, vous pouvez créer tous les labels en une seule commande :

```bash
# Labels principaux
gh label create "pr-tracking" --color "0e8a16" --description "Issue de tracking de PR"
gh label create "documentation" --color "0075ca" --description "Documentation"
gh label create "report" --color "1d76db" --description "Rapport mensuel ou de backfill"

# Types de PR
gh label create "feature" --color "a2eeef" --description "Nouvelle fonctionnalité"
gh label create "bugfix" --color "d73a4a" --description "Correction de bug"
gh label create "refactor" --color "fbca04" --description "Refactoring du code"
gh label create "performance" --color "0e8a16" --description "Optimisation de performance"
gh label create "test" --color "bfd4f2" --description "Tests"
gh label create "chore" --color "fef2c0" --description "Maintenance"

# Status
gh label create "draft" --color "6c6c6c" --description "PR en mode brouillon"
gh label create "in-progress" --color "fbca04" --description "PR en cours"
gh label create "merged" --color "6f42c1" --description "PR mergée"
gh label create "closed" --color "d73a4a" --description "PR fermée sans merge"

# Priorité (optionnel)
gh label create "low" --color "c2e0c6" --description "Priorité basse"
gh label create "medium" --color "fbca04" --description "Priorité moyenne"
gh label create "high" --color "ff9800" --description "Priorité haute"
gh label create "critical" --color "d73a4a" --description "Priorité critique"
```

### Méthode 3 : Via l'API GitHub

Vous pouvez aussi utiliser l'API REST de GitHub pour créer les labels :

```bash
# Exemple pour créer le label pr-tracking
curl -X POST \
  -H "Accept: application/vnd.github+json" \
  -H "Authorization: token YOUR_TOKEN" \
  https://api.github.com/repos/Atlasbruce/Olympe-Engine/labels \
  -d '{"name":"pr-tracking","color":"0e8a16","description":"Issue de tracking de PR"}'
```

## Vérification

Pour vérifier que tous les labels sont créés, allez sur :
https://github.com/Atlasbruce/Olympe-Engine/labels

## Notes

- Les couleurs sont données en hexadécimal sans le `#`
- Certains labels peuvent déjà exister dans le dépôt (comme `documentation`, `bug`, `enhancement`, etc.)
- Vous pouvez adapter les couleurs selon vos préférences
- Les labels de priorité sont optionnels et ne sont pas utilisés automatiquement par les workflows

## Compatibilité avec les labels existants

Si votre dépôt a déjà des labels similaires (par exemple `bug` au lieu de `bugfix`), vous pouvez :
1. Renommer les labels existants
2. Ou modifier les workflows pour utiliser vos labels existants

## Support

Pour toute question sur la configuration des labels, consultez :
- [Documentation GitHub sur les labels](https://docs.github.com/en/issues/using-labels-and-milestones-to-track-work/managing-labels)
- `PR_TRACKING_SYSTEM.md` pour la documentation complète du système
