# EXIT — DR08-08 : Action Runtime & Interaction physique

> **Statut :** 🟢 REVIEW COMPLETED — 24 septembre 2026  
> **Portée :** consolidation des arbitrages issus des tests AR01–AR08.  
> **Hors portée :** algorithmes runtime, structures de données, API, classes C++ et comportement automatique des NPC.

## Principe directeur

Le système fournit des possibilités, contraintes et conséquences. La cognition du participant arbitre ce qu'il préfère tenter ; l'exécution physique et les conséquences demeurent soumises au World autoritatif.

## D01 — Séparation décision / exécution

🟢 **CANON.** Une Decision, Strategy ou Plan peut changer alors qu'une Action précédente est encore physiquement en cours. Le changement cognitif crée éventuellement une nouvelle séquence préférée ; il n'annule pas implicitement l'exécution courante.

```text
DECISION CHANGE ≠ IMMEDIATE EXECUTION CHANGE
```

## D02 — Séparation Action / Execution / conséquence World

🟢 **CANON.** Une **Action** exprime ce que le participant tente d'accomplir. Son **Execution** décrit comment elle est actuellement réalisée physiquement ou systémiquement. Les systèmes autoritatifs du **World** déterminent les conséquences réellement produites.

```text
ACTION ≠ EXECUTION ≠ WORLD CONSEQUENCES
```

`GoTo(Target)` délègue chemin local et adaptation à la navigation. `Jump(...)` délègue trajectoire et collision à la physique. Une blessure ou une mort relève des systèmes World/Health ; une Action peut réussir tout en ayant une conséquence négative, par exemple `Jump SUCCESS + Injury`.

## D03 — Échec et invalidation par couches

🟢 **CANON.** L'échec ou l'invalidation d'une couche d'exécution ne rend pas automatiquement invalides Plan, Strategy ou Goal.

```text
EXECUTION FAILURE ≠ PLAN FAILURE ≠ STRATEGY FAILURE ≠ GOAL FAILURE
```

Une route bloquée pendant `GoTo(Battery)` peut être recalculée localement par la navigation et conserver la même Action. S'il n'existe plus de route, l'Action peut se terminer/échouer et produire un événement pertinent de réévaluation ; le résultat n'impose pas une nouvelle stratégie.

## D04 — Disponibilité de transition d'exécution

🟢 **CANON.** La catégorie historique « Conditionnally Interruptible » est remplacée par deux questions indépendantes :

1. **Question physique / exécution :** l'exécution courante peut-elle physiquement effectuer une transition maintenant ?
2. **Question cognitive / Appraisal :** compte tenu des conséquences anticipées, tenter cette transition vaut-il la peine ?

```text
CAN INTERRUPT ≠ SHOULD INTERRUPT
```

Un saut aérien peut ne fournir aucune transition volontaire ; un elevator entre deux étages peut devoir attendre une opportunité valide ; un `GoTo` dans une salle toxique peut être physiquement modifiable mais jugé dangereusement coûteux. L'Action Runtime ne bloque pas une transition physiquement possible au motif qu'elle paraît mauvaise : ce jugement relève de l'Appraisal.

## D05 — Interruption sans rollback du World

🟢 **CANON.** Interrompre, remplacer ou abandonner une Action ne rétablit jamais implicitement un état antérieur du World.

```text
INTERRUPT ACTION ≠ UNDO WORLD
```

Un état partiellement modifié reste sous la responsabilité du système autoritatif qui le possède. Interrompre `Seal(Door)` ne remet donc pas magiquement la porte à sa position antérieure.

## D06 — Inventory Action et Inventory Commit

🟢 **CANON.**

- **Inventory Action** : Action qui manipule ou transfère un élément d'inventaire physique : `Take`, `Give`, `Drop`, `Insert`, `Consume`.
- **Inventory Commit** : instant atomique logique où la mutation autoritative d'inventaire devient effective.

```text
PERCEIVED ≠ TARGETED ≠ PLANNED ≠ ACTION STARTED ≠ INVENTORY COMMITTED ≠ OWNED
```

Avant l'Inventory Commit, possession et état autoritatifs n'ont pas changé. Après un Commit réussi, le nouvel état devient autoritatif, indépendamment de toute animation. Pour une mutation concurrente, les préconditions pertinentes sont revalidées au Commit ; le premier Commit valide traité par le système d'inventaire/échange change l'état, et les suivants sont évalués contre ce nouvel état. Il n'existe aucune réservation par perception, ciblage, planification, approche ou début de `Take(Object)`.

Le système autoritatif préserve l'intégrité : aucune double possession et aucun vide de possession non intentionnel lors d'un transfert atomique.

## D07 — Social Commitment, Action Execution et Inventory Commit

🟢 **CANON.** Un **Social Commitment** est un engagement social persistant envers un ou plusieurs participants, généralement issu d'une interaction ou d'un Agreement. Il est distinct de l'**Inventory Action** tentée et de l'**Inventory Commit** atomique.

```text
SOCIAL COMMITMENT ≠ ACTION EXECUTION ≠ INVENTORY COMMIT
COMMITMENT STATE ≠ CURRENT EXECUTION STATE
```

Interrompre une transaction ne supprime pas automatiquement l'engagement qui l'avait motivée. Dans l'exemple de deux espions dont l'échange est interrompu par la compromission de leur couverture, l'Action est interrompue, le Commit d'inventaire n'est pas atteint, tandis que l'engagement peut rester actif. Il pourra nourrir une Appraisal ultérieure sans redémarrer automatiquement la transaction.

## Modèle runtime requis — 🟡 WIP d'implémentation

DR08-08 exige conceptuellement de pouvoir représenter : exécution courante, séquence nouvellement préférée mais pas encore exécutable, contexte suspendu si pertinent, préconditions, exécution, disponibilité de transition, conditions de complétion, outcome, Inventory Commit et remontée des changements pertinents vers une réévaluation.

```text
DECIDED ≠ EXECUTING ≠ COMPLETED
OLD PLAN EXISTS ≠ OLD PLAN MUST RESUME
```

Le choix entre stack, queue, priority queue ou autre structure, la représentation des séquences suspendues, les noms de lifecycle, l'API Action, les animations, navigation, historique et mécanisme de reprise restent 🟡 WIP. Un ancien Plan n'est jamais repris mécaniquement parce que son remplaçant échoue : il doit être réévalué dans le Decision Context courant.

## Résultats consolidés AR01–AR08

| Cas | Résultat de Design Review | Décisions confirmées |
|---|---|---|
| AR01 — GoTo + Observer Broadcast | Une information peut demander une réévaluation alors que `GoTo` reste valide ; un nouveau plan préféré doit être concilié avec l'exécution courante. | D01, D03 |
| AR02 — GoTo + Route Blocked | `GoTo(Target)` est l'Action ; le chemin est un détail de son exécution. Recalcul local possible, puis échec/événement si aucune route n'existe. | D02, D03 |
| AR03 — Jump + événement stratégique | La cognition continue pendant le saut ; l'absence de plateforme d'atterrissage n'impose pas un échec immédiat. Physique/collision/Health déterminent ensuite les conséquences. | D01, D02, D04 |
| AR04 — Elevator + Exit Opens | Une préférence peut évoluer pendant le déplacement, avant d'être exécutable, voire devenir obsolète. Une adaptation locale est possible seulement si le World l'autorise. | D01, D04 |
| AR05 — Toxic Room | Séparer disponibilité physique et désirabilité cognitive ; le runtime ne prend pas la décision stratégique. | D04 |
| AR06 — Seal Door + Fire | Une interruption peut avoir un coût sévère ; elle ne rollback pas le mécanisme. Aucun concept de Commit général n'est déduit. | D05 |
| AR07 — Give + Interruption | `Give` est une transaction directe, non `Drop + Take`. Avant Commit l'objet est au giver ; après Commit il appartient au receiver. Mort/absence avant Commit invalide les préconditions ; après Commit, le nouvel état prévaut. | D06, D07 |
| AR08 — Concurrent Take | Plusieurs plans sont légitimes. Le premier Inventory Commit valide obtient l'objet ; une connaissance de l'auteur n'est créée que s'il est effectivement perçu. | D06 |

Ces cas sont des preuves de robustesse et non des scripts de comportement : ils ne prescrivent pas la Strategy, le Plan ni l'Action qu'un NPC doit sélectionner.

## Préparation DR08-09 — 🔵 périmètre seulement

La prochaine revue envisagée est **DR08-09 — Vertical Slice Runtime Contract** : incapacité, mort, traversée physique de l'Exit, `locked / unlocked / open`, `CROSS EXIT = ESCAPED`, état `ESCAPED`, parité Player/NPC, interprétation par le GameManager et fin de tentative/session.

Les distinctions déjà établies sont conservées :

```text
HAS KEY ≠ EXIT UNLOCKED ≠ EXIT OPEN ≠ ESCAPED
CROSS EXIT = ESCAPED
QUEST FAILURE ≠ PLAYER FAILURE ≠ SESSION END
```

La mort d'un participant ne termine pas automatiquement l'Attempt. Aucun contrat DR08-09 n'est ici arbitré.
