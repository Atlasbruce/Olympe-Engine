# EXIT — DR08-07
## Runtime d'appréciation et de décision NPC

**Statut :** 🟢 REVIEW COMPLETED  
**Date :** 22 septembre 2026

## 1. Principe

> L'Appraisal ne cherche pas l'option objectivement optimale. Il construit une appréciation subjective des possibilités que l'agent connaît afin d'arbitrer ce qui vaut la peine d'être tenté maintenant.

Le NPC ne cherche pas la meilleure action dans le Monde ; il cherche, parmi les possibilités qu'il connaît, celle qui mérite d'être tentée compte tenu de sa situation subjective.

## 2. Décisions validées

| ID | Décision | Statut |
|---|---|---|
| D01 | L'Appraisal utilise exclusivement un contexte de décision subjectif : connaissances/mémoire, besoins, buts, état physique, engagements, relations/confiance, personnalité, valeurs, état psycho-émotionnel, confiance en soi, stratégie/plan courants. | 🟢 CANON |
| D02 | Seules les possibilités connues et légitimement accessibles entrent dans l'Appraisal. | 🟢 CANON |
| D03 | Importance et urgence sont distinctes. | 🟢 CANON |
| D04 | Les dimensions peuvent être interdépendantes sans formule universelle. | 🟢 CANON |
| D05 | Faisabilité perçue : FAISABLE, INCERTAINE, PEU PROBABLE, IMPOSSIBLE. | 🟢 CANON |
| D06 | PEU PROBABLE n'est pas automatiquement REJETÉE. | 🟢 CANON |
| D07 | Faisabilité objective ≠ faisabilité perçue. | 🟢 CANON |
| D08 | Appraisal multidimensionnel, jamais UtilityScore universel. | 🟢 CANON |
| D09 | Risque distingue au minimum probabilité et gravité. | 🟢 CANON |
| D10 | Personnalité, valeurs, confiance en soi et état psycho-émotionnel modulent l'appréciation sans falsifier connaissance ni imposer une décision. | 🟢 CANON |
| D11 | Faisabilité perçue, volonté de tenter et effort acceptable sont distincts. | 🟢 CANON |
| D12 | Réévaluation compare au plan courant et peut conserver ou changer ; mise à jour ≠ décision, réévaluation ≠ changement. | 🟢 CANON |

```text
VÉRITÉ DU MONDE ≠ CONNAISSANCE DU PARTICIPANT ≠ POSSIBILITÉS CONNUES
```

Une possibilité inconnue, même objectivement présente, ne devient jamais stratégie ou plan. `IMPOSSIBLE(une voie) ≠ IMPOSSIBLE(le but global)`.

## 3. Cadre d'appréciation

Les dimensions conceptuelles sont : importance, urgence, faisabilité perçue, bénéfice attendu, coût, temps, risque {probabilité, gravité}, fiabilité des connaissances, contribution aux besoins/buts, compatibilité avec les engagements, conséquences sociales et réversibilité. Elles ne valident ni variables numériques, ni pondérations, ni score scalaire.

Importance peut rester très élevée alors que l'urgence passe de modérée à critique après un broadcast Observer : « La porte se referme dans 30 secondes ». L'augmentation d'urgence peut modifier le sens du coût de temps, risque ou réversibilité, sans formule imposée.

```text
Faisabilité objective ≠ faisabilité perçue
→ volonté de tenter → effort acceptable → stratégie → plan/actions → résultat du Monde
```

Un agent optimiste peut estimer un trajet à 45 secondes alors qu'il en reste 30, puis considérer qu'il vaut peut-être la peine d'essayer : l'optimisme ne falsifie jamais l'estimation. Le résultat appartient toujours au Monde et à la simulation physique.

## 4. Pipeline conceptuel

```text
ÉVÉNEMENT SIGNIFICATIF → MISE À JOUR INTERNE → DEMANDE DE RÉÉVALUATION
→ CONSOLIDATION → CONTEXTE DE DÉCISION → POSSIBILITÉS CONNUES
→ APPRAISAL → VOLONTÉ DE TENTER → EFFORT ACCEPTABLE
→ COMPARAISON AU PLAN COURANT → CONSERVER / CHANGER
→ STRATÉGIE → GOAP → PLAN → ACTIONS → MONDE
```

Ce pipeline est conceptuel, non une architecture de classes ni un algorithme. Un compte à rebours peut mettre à jour le contexte sans déclencher un appraisal complet chaque seconde ; raccourci découvert, chute, perte de mobilité, changement de sortie ou communication pertinente peuvent demander une réévaluation.

## 5. Stress-tests

**🟠 Illustrations de robustesse, jamais scripts.**

| Test | Ce qu'il vérifie |
|---|---|
| ST01 — Countdown Observer | Escape reste important ; urgence peut devenir critique et demander une réévaluation. |
| ST02 — Tentative désespérée | 40–50 s pour une sortie fermant dans 30 s peut être peu probable, non impossible. |
| ST03 — Deux urgences critiques | Escape et état physique créent un conflit ; risque distingue probabilité et gravité. |
| ST04 — Meilleure voie inconnue | Un raccourci objectif absent des connaissances ne peut entrer dans les possibilités connues. |
| ST05 — Toutes les options défavorables | L'arbitrage peut sélectionner une possibilité mauvaise mais subjectivement préférable. |

Une option peu probable peut devenir faisable après découverte légitime d'un raccourci durant l'exécution : c'est une nouvelle connaissance, non une connaissance rétroactive.

## 6. WIP

Pondérations, seuils, comparaison multidimensionnelle, volonté de tenter, effort acceptable, dépassement de soi/adrénaline, valeurs de probabilité/gravité, changement significatif, fréquence des appraisals, représentation runtime, inertie/coût de changement de stratégie-plan.

## 7. Cohérence

Cette revue préserve : `ÉTAT ≠ BESOIN ≠ BUT ≠ ACTION`, `FAIT ≠ CONNAISSANCE ≠ INTERPRÉTATION ≠ RÉPONSE ÉMOTIONNELLE ≠ DÉCISION`, `ÉTAT D'ENGAGEMENT ≠ EXÉCUTION COURANTE`, `MISE À JOUR ≠ DÉCISION`, `RÉÉVALUATION ≠ CHANGEMENT`. QuestManager construit le problème ; les agents cherchent comment le résoudre. Aucune contradiction bloquante détectée.

