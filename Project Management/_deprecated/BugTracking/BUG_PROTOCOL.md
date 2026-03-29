# 🔬 BUG PROTOCOL — Olympe Engine

**Version:** 1.0  
**Créé le:** 2026-03-15 12:44:21 UTC  
**Dernière mise à jour:** 2026-03-15 12:44:21 UTC  
**Projet:** Olympe Engine  
**GitHub Repo:** https://github.com/Atlasbruce/Olympe-Engine  

---

## 🎯 Objectif

Ce protocole définit le **cycle de vie complet** de chaque bug dans le projet Olympe Engine, de sa détection à son archivage. Il garantit la traçabilité, la réactivité et l'apprentissage continu.

---

## 🔄 Cycle de Vie en 5 Phases

```
DETECTION → INVESTIGATION → RESOLUTION → VERIFICATION → LEARNING
```

---

## Phase 1 — DETECTION

### Déclencheurs

- Découverte manuelle pendant le développement ou les tests
- Régression détectée après merge d'une PR
- Retour utilisateur / review
- Alerte CI/build (compilation échoue, test fail)

### Conditions d'Entrée

- Le comportement observé s'écarte du comportement documenté ou attendu
- Le comportement a un impact mesurable sur l'utilisateur ou le système

### Actions Requises (dans l'heure)

1. **Attribuer un ID** : prochain `BUG-XXX` disponible dans BUG_REGISTRY.md
2. **Créer un GitHub Issue** avec le template `bug-report.yml`
3. **Classifier** :
   - Sévérité : P0/P1/P2/P3 (voir tableau de classification)
   - Type : Regression / New Bug / Edge Case / Performance / Crash
4. **Créer la fiche bug** dans `BUG_REGISTRY.md` (section sévérité appropriée)
5. **Mettre à jour** la section "Known Issues & Regressions" du `feature_context` concerné
6. **Horodater** la découverte en UTC ISO 8601 : `YYYY-MM-DD HH:MM:SS UTC`

### Timestamp Requis

```
Découvert le: YYYY-MM-DD HH:MM:SS UTC
```

### Statut Résultant

`OPEN`

---

## Phase 2 — INVESTIGATION

### Conditions d'Entrée

- Bug classé P0 ou P1 : investigation immédiate (max 2h après détection)
- Bug classé P2 : investigation dans les 24h
- Bug classé P3 : investigation dans la semaine

### Actions Requises

1. **Identifier la cause racine** :
   - Examiner les commits récents (git blame, git log)
   - Identifier la PR introduisant la régression (si applicable)
   - Reproduire le bug de manière fiable
2. **Documenter l'analyse** dans la fiche BUG_REGISTRY.md :
   - Section "Cause Racine"
   - Lien vers la PR responsable si régression
3. **Évaluer l'impact réel** : confirmer ou réviser la sévérité
4. **Identifier les modules affectés** : effets de bord potentiels
5. **Mettre à jour** le statut dans BUG_REGISTRY.md

### Timestamp Requis

```
Investigation démarrée: YYYY-MM-DD HH:MM:SS UTC
Cause identifiée: YYYY-MM-DD HH:MM:SS UTC
```

### Statut Résultant

`INVESTIGATING`

### Escalade

- Si P3 réévalué comme P1/P0 après investigation → escalade immédiate (voir Règles d'Escalade)
- Si cause introuvable après 4h (P0) ou 24h (P1) → escalade à @Atlasbruce

---

## Phase 3 — RESOLUTION

### Conditions d'Entrée

- Cause racine identifiée
- Fix approuvé par @Atlasbruce (P0/P1) ou décision autonome (P2/P3)

### Actions Requises

1. **Créer une branche de fix** : `bugfix/BUG-XXX-short-description`
2. **Développer le fix** :
   - Fix ciblé et minimal (pas de refactoring non lié)
   - Respecter les règles de code C++14 (voir COPILOT_CODING_RULES.md)
3. **Ajouter un test de régression** :
   - Test qui échoue avant le fix et passe après
   - Nommage : `Test_Regression_BUG_XXX_[Description]`
   - Documenter dans la fiche bug : "Tests de Régression Ajoutés: Oui — [fichier]"
4. **Mettre à jour** BUG_REGISTRY.md :
   - Statut → `IN_PROGRESS`
   - PR Fix référencée
5. **Créer une PR** de fix avec référence au GitHub Issue `Fixes #XXX`

### Timing

| Sévérité | Délai Max pour Démarrer le Fix |
|---------|-------------------------------|
| P0 | Immédiatement après investigation |
| P1 | Dans les 4h après investigation |
| P2 | Dans les 48h après investigation |
| P3 | Planifié dans le prochain sprint |

### Timestamp Requis

```
Fix démarré: YYYY-MM-DD HH:MM:SS UTC
PR créée: YYYY-MM-DD HH:MM:SS UTC
```

### Statut Résultant

`IN_PROGRESS`

---

## Phase 4 — VERIFICATION

### Conditions d'Entrée

- PR de fix mergée sur master

### Actions Requises

1. **Vérifier** que le bug ne se reproduit plus
2. **Confirmer** que les tests de régression passent
3. **Vérifier l'absence d'effets de bord** sur les modules connexes
4. **Mettre à jour** BUG_REGISTRY.md :
   - Statut → `FIXED`
   - PR Fix : lien complet
   - Date de résolution
5. **Calculer le MTTR** (découverte → merge PR fix)
6. **Mettre à jour** la section "Fixed Bugs During This Phase" dans `feature_context_XX_Y.md`
7. **Archiver** la fiche bug dans la section Archive de BUG_REGISTRY.md
8. **Mettre à jour les métriques** (MTTR, regression rate)

### Timestamp Requis

```
Résolu le: YYYY-MM-DD HH:MM:SS UTC
```

### Statut Résultant

`FIXED` → archivé

---

## Phase 5 — LEARNING

### Conditions d'Entrée

- Bug P0 ou P1 résolu

### Actions Requises

1. **Post-Mortem** : remplir le template post-mortem (voir section dédiée)
2. **Identifier les améliorations** : code, tests, process
3. **Mettre à jour** les règles de code ou les checklists si une leçon est généralisable
4. **Partager les apprentissages** dans les notes archéologiques du feature_context concerné
5. **Mettre à jour le BUG_PROTOCOL.md** si le protocole doit être amélioré

### Timing

- Post-mortem P0 : dans les 48h après résolution
- Post-mortem P1 : dans la semaine suivant la résolution

---

## 🏷️ Classification des Bugs

### Par Sévérité

| Sévérité | Code | Critères | Délai Résolution Cible |
|---------|------|----------|----------------------|
| CRITICAL | P0 | Crash moteur / éditeur, perte de données, corruption fichiers, blocage total du workflow | < 24h |
| HIGH | P1 | Fonctionnalité majeure cassée, pas de workaround viable, régression sur feature livrée | < 72h |
| MEDIUM | P2 | Fonctionnalité dégradée mais workaround disponible, impact UX significatif | < 2 semaines |
| LOW | P3 | Mineur, cosmétique, edge case rare, pas d'impact sur le workflow principal | < 1 mois |

### Par Type

| Type | Description | Exemple |
|------|-------------|---------|
| **Regression** | Comportement qui fonctionnait avant une PR et ne fonctionne plus | Serialisation JSON cassée après PR #386 |
| **New Bug** | Bug dans une feature nouvellement développée | Dropdown ne s'affiche pas correctement |
| **Edge Case** | Bug dans un scénario limite non couvert par les tests | Graphe avec 0 nœud provoque un assert |
| **Performance** | Dégradation mesurable des performances | FPS divisé par 2 avec 50+ nœuds |
| **Crash** | Crash / segfault / exception non gérée | Null pointer dans VSGraphVerifier::Verify() |

---

## 🚨 Règles d'Escalade

### Escalade Automatique à @Atlasbruce

1. **Tout P0** découvert → notification immédiate
2. **P1 non résolu** après 48h → escalade
3. **Cause racine introuvable** après 4h (P0) ou 24h (P1)
4. **P3 réévalué comme P1/P0** après investigation
5. **Régression sur feature validée** (PR mergée il y a moins de 48h)

### Format d'Escalade

```
🚨 ESCALADE — [BUG-XXX]
Sévérité: PX
Phase affectée: XX-Y
Problème: [description courte]
Tentatives: [ce qui a été essayé]
Besoin: [décision / information / validation]
Découvert le: YYYY-MM-DD HH:MM:SS UTC
```

---

## 🔗 Intégration GitHub

### GitHub Issue Template

Fichier : `.github/ISSUE_TEMPLATE/bug-report.yml`

Chaque bug doit avoir un GitHub Issue correspondant créé avec ce template.

### Labels GitHub

| Label | Couleur | Usage |
|-------|---------|-------|
| `bug` | `#d73a4a` | Tout bug confirmé |
| `P0-critical` | `#b60205` | Sévérité critique |
| `P1-high` | `#d4ac0d` | Sévérité haute |
| `P2-medium` | `#0075ca` | Sévérité moyenne |
| `P3-low` | `#cfd3d7` | Sévérité basse |
| `regression` | `#e99695` | Régression identifiée |
| `needs-investigation` | `#d93f0b` | Cause inconnue |

### Project Board — Colonnes

| Colonne | Bugs Éligibles |
|---------|---------------|
| **Backlog** | P3 non planifiés |
| **To Investigate** | OPEN P0/P1/P2 |
| **In Progress** | IN_PROGRESS |
| **In Review** | PR créée, en attente de merge |
| **Done** | FIXED ce sprint |

### Liaisons Automatiques

- PR de fix : `Fixes #XXX` dans la description → ferme automatiquement l'Issue
- Référence régression : mentionner `#XXX` dans la fiche BUG_REGISTRY

---

## 🤖 Commandes Copilot pour la Gestion des Bugs

### Déclarer un Nouveau Bug

```
@Copilot NOUVEAU BUG: [description courte]
Phase: XX-Y
Sévérité suspectée: PX
PR potentiellement responsable: #XXX (optionnel)
Reproduction: [étapes]
```

### Mettre à Jour un Bug

```
@Copilot UPDATE BUG BUG-XXX
Statut: [INVESTIGATING|IN_PROGRESS|FIXED]
Cause: [description si identifiée]
```

### Demander un Résumé des Bugs Actifs

```
@Copilot BUG STATUS
```
→ Copilot liste tous les bugs actifs par sévérité avec leur statut.

### Demander la Vérification des Bugs d'une Phase

```
@Copilot BUG CHECK PHASE XX-Y
```
→ Copilot lit le BUG_REGISTRY.md et le feature_context_XX_Y.md et présente tous les bugs associés.

### Démarrer un Post-Mortem

```
@Copilot POST-MORTEM BUG-XXX
```
→ Copilot remplit le template post-mortem avec les informations disponibles.

---

## 📊 Template Métriques Mensuelles

```markdown
## Rapport Bugs — [Mois YYYY]

**Période:** YYYY-MM-01 00:00:00 UTC → YYYY-MM-30 23:59:59 UTC

### Bugs par Sévérité
| Sévérité | Découverts | Résolus | Actifs fin de période |
|---------|-----------|---------|----------------------|
| P0 | X | X | X |
| P1 | X | X | X |
| P2 | X | X | X |
| P3 | X | X | X |
| **Total** | **X** | **X** | **X** |

### MTTR (Mean Time To Resolve)
| Sévérité | MTTR Ce Mois | MTTR Cible | Tendance |
|---------|-------------|-----------|---------|
| P0 | Xh | < 24h | ↑/↓/→ |
| P1 | Xh | < 72h | ↑/↓/→ |
| P2 | Xj | < 14j | ↑/↓/→ |
| P3 | Xj | < 30j | ↑/↓/→ |

### Régressions
- Taux de régression : X% (X bugs régression / X PRs mergées)
- PRs ayant introduit des régressions : #XXX, #XXX
- Modules les plus touchés : [Module1], [Module2]

### Apprentissages
- [Leçon 1]
- [Leçon 2]
```

---

## 🔍 Template Post-Mortem (P0/P1)

```markdown
## Post-Mortem — [BUG-XXX] : [Titre]

**Date du Post-Mortem:** YYYY-MM-DD HH:MM:SS UTC  
**Participants:** @Atlasbruce, Copilot  
**Bug résolu le:** YYYY-MM-DD HH:MM:SS UTC  
**MTTR:** Xh  

---

### 1. Résumé de l'Incident

[Description concise du bug, de son impact et de sa résolution]

### 2. Chronologie

| Heure (UTC) | Événement |
|-------------|-----------|
| YYYY-MM-DD HH:MM:SS | Bug découvert |
| YYYY-MM-DD HH:MM:SS | Investigation démarrée |
| YYYY-MM-DD HH:MM:SS | Cause identifiée |
| YYYY-MM-DD HH:MM:SS | Fix développé |
| YYYY-MM-DD HH:MM:SS | PR créée |
| YYYY-MM-DD HH:MM:SS | PR mergée — Bug résolu |

### 3. Cause Racine

**Cause technique :**  
[Description précise de la cause]

**Cause organisationnelle / process :**  
[Pourquoi ce bug n'a pas été détecté plus tôt]

### 4. Impact

- **Modules affectés :** [liste]
- **Fonctionnalités impactées :** [liste]
- **Durée de l'impact :** Xh (entre découverte et résolution)
- **Phases bloquées :** [liste ou "Aucune"]

### 5. Ce Qui A Bien Fonctionné

- [Point positif 1]
- [Point positif 2]

### 6. Ce Qui A Mal Fonctionné

- [Point négatif 1]
- [Point négatif 2]

### 7. Actions Correctives

| Action | Responsable | Délai | Statut |
|--------|-------------|-------|--------|
| [Action 1] | @[username] | YYYY-MM-DD | À faire |
| [Action 2] | @[username] | YYYY-MM-DD | À faire |

### 8. Tests de Régression Ajoutés

- [Test 1] — fichier : [path]
- [Test 2] — fichier : [path]

### 9. Leçons Tirées

- [Leçon 1 : généralisation de l'apprentissage]
- [Leçon 2]
```

---

## 🔗 Références

- [BUG_REGISTRY.md](./BUG_REGISTRY.md) — Registre centralisé de tous les bugs
- [../Copilot_Memory_Project_Management_Instructions_V2.md](../Copilot_Memory_Project_Management_Instructions_V2.md) — Instructions PM complètes
- [../../.github/ISSUE_TEMPLATE/bug-report.yml](../../.github/ISSUE_TEMPLATE/bug-report.yml) — Template GitHub Issue
- [../Features/](../Features/) — Feature context files (section "Known Issues & Regressions")

---

*Dernière mise à jour : 2026-03-15 12:44:21 UTC*
