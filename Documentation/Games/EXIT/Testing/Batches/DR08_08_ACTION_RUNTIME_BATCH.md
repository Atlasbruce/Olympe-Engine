# EXIT — Batch préparatoire DR08-08 : Action Runtime

> **Statut :** 🟢 REVIEW COMPLETED — 24 septembre 2026  
> **Périmètre :** résultats de Design Review Action Runtime ; les détails d'implémentation restent 🟡 WIP.

## Objectif de validation

Éprouver, à partir de situations référencées, la manière dont une Action en cours peut coexister avec un événement significatif, une invalidation du World ou une nouvelle priorité connue du participant.

## Références de situations

| Cas batch | Situation database | Question principale | Tags de validation candidats |
|---|---|---|---|
| AR01 — GoTo + Observer Broadcast | [AR01](../EXIT_TEST_SITUATION_DATABASE.md#ar01--goto--broadcast-observer) | Un broadcast déclenche-t-il une mise à jour et, si pertinent, une réévaluation sans décision directe ? | #TEST_INTERRUPTION, #TEST_REEVALUATION |
| AR02 — GoTo + Route Blocked | [AR02](../EXIT_TEST_SITUATION_DATABASE.md#ar02--goto--route-bloquée) | Une route invalidée rend-elle visible le premier point de transition à corriger ? | #TEST_ACTION_INVALIDATION |
| AR03 — Jump + Strategic Event | [AR03](../EXIT_TEST_SITUATION_DATABASE.md#ar03--jump--événement-stratégique) | Quels faits doivent être tracés sans présumer de phases interruptibles ? | #TEST_INTERRUPTION, #TEST_COMMIT_POINT |
| AR04 — Elevator + Exit Opens | [AR04](../EXIT_TEST_SITUATION_DATABASE.md#ar04--elevator--exit-ouverte) | Une nouvelle opportunité est-elle mémorisée sans disponibilité magique ? | #TEST_CONDITIONAL_INTERRUPT |
| AR05 — Toxic Room + New Priority | [AR05](../EXIT_TEST_SITUATION_DATABASE.md#ar05--salle-toxique--nouvelle-priorité) | Le danger met-il à jour le contexte sans règle `Need → Action` directe ? | #TEST_REEVALUATION, #TEST_INTERRUPTION |
| AR06 — Seal Door + Lethal Danger | [AR06](../EXIT_TEST_SITUATION_DATABASE.md#ar06--sceller-une-porte-face-au-feudanger) | Quels éléments du lifecycle nécessitent un futur arbitrage de commit ? | #TEST_COMMIT_POINT, #TEST_CONDITIONAL_INTERRUPT |
| AR07 — Give Transaction + Interruption | [AR07](../EXIT_TEST_SITUATION_DATABASE.md#ar07--give-interrompu) | La possession demeure-t-elle univoque à toute phase de la transaction ? | #TEST_TRANSACTION_ATOMICITY, #TEST_INTERRUPTION |
| AR08 — Concurrent Take Same Object | [AR08](../EXIT_TEST_SITUATION_DATABASE.md#ar08--take-concurrent-du-même-objet) | Une course ne crée-t-elle jamais de double ownership ? | #TEST_DOUBLE_OWNERSHIP, #TEST_ACTION_RACE |

## Résultats transversaux consolidés

- Une réévaluation ou un `CHANGE` n'annule jamais automatiquement l'exécution physique courante.
- Une invalidation de route peut être résolue localement par Navigation sans invalider Action, Plan, Strategy ou Goal.
- Les transitions physiques disponibles et leur désirabilité cognitive sont deux questions distinctes.
- Interrompre une Action n'implique jamais un rollback automatique du World.
- Les mutations d'inventaire utilisent un **Inventory Commit** atomique ; ni perception ni plan ne réservent un objet.
- `Give` est une transaction directe et non une composition `Drop + Take`.
- Un Social Commitment peut survivre à l'interruption de l'Action qui devait le concrétiser.

## Règles de traçage et de réutilisation

1. Chaque cas doit être enrichi depuis la fiche standard de la base avant exécution.
2. Chaque exécution utilise le [Decision Trace Template](../EXIT_TEST_DECISION_TRACE_TEMPLATE.md).
3. Les résultats sont évalués à partir de propriétés et d'invariants, jamais d'une action prédéterminée d'un NPC.
4. Une anomalie est tracée jusqu'au **First Invalid Transition** ; elle ne justifie pas à elle seule un special case.
5. `AR09 — Giver dies during transaction` est conservé dans la base comme cas adjacent, mais n'est pas inclus dans ce premier squelette afin de ne pas présumer de son périmètre.

## Questions explicitement 🟡 WIP

- modèle définitif d'interruptibilité et phases non interruptibles ;
- commit point définitif ;
- lifecycle d'Action Runtime ;
- comportement précis de `Give` ;
- timers et ordonnancement technique ;
- architecture C++.

Ces points restent 🟡 WIP jusqu'à une Design Review dédiée.
