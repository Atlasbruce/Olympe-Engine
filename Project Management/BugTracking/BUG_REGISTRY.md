# 🐛 BUG REGISTRY — Olympe Engine

**Version:** 1.0  
**Créé le:** 2026-03-15 12:44:21 UTC  
**Dernière mise à jour:** 2026-03-15 15:30:00 UTC  
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

### BUG-001 — Crash Save Blackboard (VariableType::None) — IN_PROGRESS 🔧

Voir fiche détaillée dans la section **Fiches Détaillées — Bugs Actifs** ci-dessous.

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
| BUG-001 | Crash Save Blackboard (VariableType::None) | P0 CRITICAL | 23-B | En cours | IN_PROGRESS | 2026-03-15 15:30:00 UTC | @Atlasbruce |

---

## 📝 Fiches Détaillées — Bugs Actifs

### [BUG-001] — Crash Save Blackboard (VariableType::None)

**ID:** BUG-001  
**Sévérité:** P0 — CRITICAL  
**Type:** Crash  
**Statut:** IN_PROGRESS  
**Phase Affectée:** 23-B — Full Blackboard Properties  
**Module:** VisualScriptEditorPanel  

**Découvert le:** 2026-03-15 15:30:00 UTC  
**Découvert par:** @Atlasbruce  
**Assigné à:** @Atlasbruce  

**Description:**  
L'éditeur crashe avec `abort()` lors du Save après l'ajout d'une variable Blackboard via le bouton `[+]` dans le panel. Le crash est reproductible à chaque save après ajout sans modification du type.

**Étapes de Reproduction:**
1. Ouvrir un graphe VS dans le Blueprint Editor
2. Dans le panel Blackboard, cliquer `[+]` pour ajouter une variable
3. Ne pas modifier le type (laisse `None` ou `Float` selon la version)
4. Appuyer sur Save (Ctrl+S)
5. L'éditeur crashe avec `abort()`

**Comportement Attendu:**  
Le graphe est sauvegardé normalement, les variables Blackboard valides sont persistées.

**Comportement Observé:**  
Crash `abort()` dans la sérialisation JSON. Le fichier graphe peut être corrompu ou non écrit.

**Impact:**  
Bloquant pour toute utilisation du Blackboard dans le Blueprint Editor. Impossible de sauvegarder un graphe après ajout de variable.

**Cause Racine:**  
- `VariableType::None` non géré dans le `switch` de `SerializeAndWrite()` → default case écrit `"type": "None"` mais lors du reload, le deserializer ne gère pas ce type → crash potentiel
- Clé vide non validée : `entry.Key = "newKey"` peut être effacée par l'utilisateur, laissant une clé vide qui corrompt le JSON
- Buffer ImGui potentiellement corrompu lors de realloc du vecteur `m_template.Blackboard`

**Workaround:** Toujours assigner un type valide et une clé non-vide avant de sauvegarder. Contournement fragile.

**GitHub Issue:** N/A  
**PR Fix:** Phase 23-B (en cours)  
**Feature Context:** [feature_context_23_B.md](../Features/feature_context_23_B.md)  

**Tests de Régression Ajoutés:** Oui — `Tests/BlueprintEditor/Phase23BTest.cpp` (5 tests BUG-001)

---

## 📈 Métriques

### KPIs Actuels

| Métrique | Valeur | Cible |
|---------|--------|-------|
| Bugs P0 actifs | 1 (BUG-001) | 0 |
| Bugs P1 actifs | 0 | ≤ 2 |
| Bugs P2 actifs | 0 | ≤ 5 |
| Bugs P3 actifs | 0 | ≤ 10 |
| MTTR P0 (Mean Time To Resolve) | En cours | < 24h |
| MTTR P1 | N/A | < 72h |
| MTTR P2 | N/A | < 2 semaines |
| MTTR P3 | N/A | < 1 mois |
| Taux de régression (bugs/PR) | 0% | < 5% |
| Couverture tests phases actives | 18/18 (Phase 23-B) | > 80% |

### Hotspots (Modules les Plus Touchés)

| Module | Bugs Total | Bugs Actifs | Dernière régression |
|--------|-----------|-------------|---------------------|
| — | — | — | — |

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

*Aucun bug archivé.*

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

*Dernière mise à jour : 2026-03-15 15:30:00 UTC*
