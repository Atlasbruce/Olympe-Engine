# 🐛 BUG REGISTRY — Olympe Engine

**Version:** 1.0  
**Créé le:** 2026-03-15 12:44:21 UTC  
**Dernière mise à jour:** 2026-03-15 22:17:00 UTC  
**Projet:** Olympe Engine  
**GitHub Repo:** https://github.com/Atlasbruce/Olympe-Engine  

---

## 📋 Vue d'Ensemble

Ce registre centralise **tous les bugs connus** du projet Olympe Engine, classés par sévérité. Chaque bug est tracé de sa découverte à sa résolution avec traçabilité complète vers les phases, feature contexts, PRs et GitHub Issues.

### Légende des Statuts

| Statut | Description |
|--------|-------------|
| `OPEN` | Bug découvert, non encore investigué |
| `INVESTIGATING` | Cause racine en cours d'identification |
| `IN_PROGRESS` | Fix en cours de développement |
| `FIXED` | Fix mergé sur master |
| `WONT_FIX` | Bug accepté ou hors périmètre |

### Légende des Sévérités

| Sévérité | Code | Impact |
|----------|------|--------|
| CRITICAL | P0 | Crash / perte de données / blocage total |
| HIGH | P1 | Fonctionnalité majeure cassée, pas de workaround |
| MEDIUM | P2 | Fonctionnalité dégradée, workaround possible |
| LOW | P3 | Mineur, cosmétique, edge case rare |

---

## 🔴 P0 — CRITICAL

### BUG-003 — Node Positions Offset on Save (ImNodes Viewport) — FIXED ✅

Voir fiche détaillée dans la section **Fiches Détaillées** ci-dessous.

### BUG-004 — Crash on Load After Save (Double Viewport Offset) — FIXED ✅

Voir fiche détaillée dans la section **Fiches Détaillées** ci-dessous.

---

## 🟠 P1 — HIGH

*Aucun bug P1 actif.*

---

## 🟡 P2 — MEDIUM

*Aucun bug P2 actif.*

---

## 🔵 P3 — LOW

*Aucun bug P3 actif.*

---

## 📊 Tableau de Bord Actif

| ID | Titre | Sévérité | Phase | PR | Statut | Découvert | Assigné |
|----|-------|----------|-------|----|--------|-----------|---------|

*Aucun bug actif — tous les P0 résolus.*

---

## 📝 Fiches Détaillées — Bugs Actifs

*Aucun bug actif.*

---

## 📈 Métriques

### KPIs Actuels

| Métrique | Valeur | Cible |
|---------|--------|-------|
| Bugs P0 actifs | 0 | 0 |
| Bugs P1 actifs | 0 | ≤ 2 |
| Bugs P2 actifs | 0 | ≤ 5 |
| Bugs P3 actifs | 0 | ≤ 10 |
| MTTR P0 (Mean Time To Resolve) | ~1-2h | < 24h |
| MTTR P1 | N/A | < 72h |
| MTTR P2 | N/A | < 2 semaines |
| MTTR P3 | N/A | < 1 mois |
| Taux de régression (bugs/PR) | 0% | < 5% |
| Couverture tests phases actives | 31/31 (Phase 23-B + HOTFIX BUG-003/004) | > 80% |

### Hotspots (Modules les Plus Touchés)

| Module | Bugs Total | Bugs Actifs | Dernière régression |
|--------|-----------|-------------|---------------------|
| BlueprintEditor — Blackboard Serialization | 2 | 0 | 2026-03-15 (BUG-001/002, FIXED) |
| BlueprintEditor — Node Position Save/Load | 2 | 0 | 2026-03-15 (BUG-003/004, FIXED) |

---

## ✅ Archive — Bugs Résolus

### Format d'Archive

```
**[BUG-XXX]** — [Titre]  
Sévérité: PX | Phase: XX-Y | PR Fix: #XXX  
Découvert: YYYY-MM-DD HH:MM:SS UTC | Résolu: YYYY-MM-DD HH:MM:SS UTC  
Temps de résolution: Xh  
Résumé: [Description courte de la cause et du fix]
```

**[BUG-001]** — Crash on Blackboard Save: abort() on VariableType::None  
Sévérité: P0 | Phase: 22-C (régression) | PR Fix: #387  
Découvert: 2026-03-15 14:30:00 UTC | Résolu: 2026-03-15 15:30:00 UTC  
Temps de résolution: ~1h  
Résumé: abort() lors de la sauvegarde d'un Blackboard contenant une entrée avec VariableType::None ou Key="". Fix: validation pré-save + initialisation safe (Key="NewVariable", Type=Int) + warning UX. 5/5 regression tests.

**[BUG-002]** — Save Still Crashes After Merge (Validation Not Called)  
Sévérité: P0 | Phase: 23-B (régression PR #387) | PR Fix: PR en cours  
Découvert: 2026-03-15 17:00:00 UTC | Résolu: 2026-03-15 17:30:00 UTC  
Temps de résolution: ~30min  
Résumé: ValidateAndCleanBlackboardEntries() ajouté mais non appelé dans Save(). Fix: appel explicite avant BuildJsonFromTemplate(). Validé via tests Phase23BTest.cpp.

**[BUG-003]** — Node Positions Offset on Save (ImNodes Viewport)  
Sévérité: P0 | Phase: 23-B (régression PR #400) | PR Fix: #401  
Découvert: 2026-03-15 17:30:00 UTC | Résolu: 2026-03-15 22:17:00 UTC  
Temps de résolution: ~5h  
Résumé: SyncNodePositionsFromImNodes() utilisait GetNodeEditorSpacePos() (= Origin + Panning) au lieu de GetNodeGridSpacePos() (= Origin pur). Positions sauvegardées incluaient l'offset viewport → delta constant (-24px X, +114px Y). Fix: utilisation de GetNodeGridSpacePos() pour save + SetNodeGridSpacePos() pour restore. + ResetViewportBeforeSave() safety measure. 8 regression tests (Phase23B2Test.cpp).

**[BUG-004]** — Crash on Load After Save (Double Viewport Offset)  
Sévérité: P0 | Phase: 23-B (causé par BUG-003) | PR Fix: #401  
Découvert: 2026-03-15 17:30:00 UTC | Résolu: 2026-03-15 22:17:00 UTC  
Temps de résolution: ~5h  
Résumé: Positions sauvegardées avec offset viewport → rechargées avec offset → double-offset lors de la restauration via SetNodeEditorSpacePos → calcul de liens invalide → crash. Fix: résolution de BUG-003 (positions en grid space), plus SetNodeGridSpacePos() à la restauration. Couvert par les mêmes 8 tests que BUG-003.

---

## 🔗 Références Croisées

- [BUG_PROTOCOL.md](./BUG_PROTOCOL.md) — Protocole complet de gestion du cycle de vie
- [../Features/feature_context_21_A.md](../Features/feature_context_21_A.md) — Phase 21-A : VSGraphVerifier
- [../Features/feature_context_21_B.md](../Features/feature_context_21_B.md) — Phase 21-B : UI Integration
- [../Features/feature_context_21_D.md](../Features/feature_context_21_D.md) — Phase 21-D : Dynamic Pins
- [../Features/feature_context_22_A.md](../Features/feature_context_22_A.md) — Phase 22-A : VSSwitch
- [../Features/feature_context_22_C.md](../Features/feature_context_22_C.md) — Phase 22-C : Parameter Dropdowns
- [../../.github/ISSUE_TEMPLATE/bug-report.yml](../../.github/ISSUE_TEMPLATE/bug-report.yml) — Template GitHub Issue bug

---

## 📋 Comment Utiliser Ce Registre

### Ajouter un Nouveau Bug

1. Attribuer le prochain ID disponible : `BUG-XXX` (séquentiel global)
2. Créer la fiche détaillée dans la section sévérité appropriée
3. Ajouter la ligne dans le tableau de bord actif
4. Mettre à jour les métriques
5. Créer un GitHub Issue avec le template `bug-report.yml`
6. Mettre à jour la section "Known Issues & Regressions" du feature_context concerné
7. Horodater en UTC ISO 8601 (`YYYY-MM-DD HH:MM:SS UTC`)

### Archiver un Bug Résolu

1. Déplacer la fiche de la section active vers l'Archive
2. Calculer le MTTR et mettre à jour les métriques
3. Retirer la ligne du tableau de bord actif
4. Mettre à jour la section "Known Issues & Regressions" du feature_context concerné
5. Horodater la résolution

---

## 📝 Template Fiche Bug

```markdown
### [BUG-XXX] — [Titre Court]

**ID:** BUG-XXX  
**Sévérité:** PX — [CRITICAL|HIGH|MEDIUM|LOW]  
**Type:** [Regression|New Bug|Edge Case|Performance|Crash]  
**Statut:** [OPEN|INVESTIGATING|IN_PROGRESS|FIXED]  
**Phase Affectée:** XX-Y — [Nom de la phase]  
**Module:** [Nom du module principal impacté]  

**Découvert le:** YYYY-MM-DD HH:MM:SS UTC  
**Découvert par:** @[username] / [PR #XXX si régression]  
**Assigné à:** @[username] ou Non assigné  

**Description:**  
[Description claire du comportement observé vs attendu]

**Étapes de Reproduction:**
1. [Étape 1]
2. [Étape 2]
3. [Résultat observé]

**Comportement Attendu:**  
[Description du comportement correct]

**Comportement Observé:**  
[Description du comportement bugué]

**Impact:**  
[Description de l'impact utilisateur / système]

**Cause Racine:** (remplir après investigation)  
[Description de la cause identifiée]

**Workaround:** [Description du contournement possible, ou "Aucun"]

**GitHub Issue:** [#XXX](https://github.com/Atlasbruce/Olympe-Engine/issues/XXX)  
**PR Introduisant la Régression:** [#XXX](https://github.com/Atlasbruce/Olympe-Engine/pull/XXX) (si régression)  
**PR Fix:** [#XXX](https://github.com/Atlasbruce/Olympe-Engine/pull/XXX) (quand fixé)  
**Feature Context:** [feature_context_XX_Y.md](../Features/feature_context_XX_Y.md)  

**Tests de Régression Ajoutés:** [Oui/Non] — [fichier de test si applicable]  
```

---

*Dernière mise à jour : 2026-03-15 22:17:00 UTC*
