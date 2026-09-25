# EXIT — Systemic Situation Test Framework

**Statut :** 🟡 Outil méthodologique de Design Review  
**Portée :** ni mécanique gameplay, ni architecture C++.

## Workflow

```text
Thèmes de validation → base de situations → filtrage par tags → batch de test
→ trace de décision / stress-test → observations → analyse d'échec
→ règle générale candidate → tests croisés → arbitrage → CANON / WIP
```

> Lorsqu'une situation révèle un problème, on ne la répare pas immédiatement avec une exception. On cherche d'abord quelle règle générale manque, puis on éprouve cette règle sur d'autres situations.

## Diagnostic

Lorsqu'un résultat paraît incohérent, remonter la chaîne et identifier la **première transition invalide**, plutôt que corriger le résultat final.

```text
Monde → perception → connaissance → possibilités connues → appréciation
→ arbitrage → stratégie → GOAP → plan → action → résultat Monde
```

## Fichiers associés

- [Taxonomie des tags](EXIT_TEST_TAG_TAXONOMY.md)
- [Base de situations](EXIT_TEST_SITUATION_DATABASE.md)
- [Template de trace](EXIT_TEST_DECISION_TRACE_TEMPLATE.md)
- `Batches/` : sélections référentielles, sans duplication des situations.

## Analyse d'échec

```text
Problème observé → remonter le pipeline → première transition invalide
→ CANON existant violé ?
   oui : incohérence modèle/implémentation
   non : règle générale manquante candidate
→ test croisé → affiner ou arbitrer → CANON / WIP
```

Un cas spécial n'est jamais ajouté simplement pour faire réussir une situation.

## Sélection de batch

```text
Objectif de validation → thèmes/cibles requis → requête dans la base
→ candidats → matrice de couverture → retirer redondances inutiles
→ ajouter cas inter-systèmes → batch
```

Une situation peut appartenir à plusieurs batches. Un batch référence des identifiants existants ; il ne recopie jamais une situation.

## Matrice de couverture

La base fournit les tableaux `Situation × Thème` et `Situation × Couche pipeline`. Ils servent à repérer lacunes, redondances et combinaisons à tester, jamais à calculer un score de qualité.

## Référence globale

[GST01 — Global Vertical Slice Stress Test](Batches/GST01_GLOBAL_VERTICAL_SLICE_STRESS_TEST.md) est 🟢 REFERENCE. Il exerce la chaîne complète du Vertical Slice et confirme `NEW STRUCTURAL BLOCKER = NONE`. Toute évolution structurante doit pouvoir y être confrontée sans accès illégitime au Quest Graph, réservation magique, causalité sociale scriptée ni rupture de parité Player/NPC.
