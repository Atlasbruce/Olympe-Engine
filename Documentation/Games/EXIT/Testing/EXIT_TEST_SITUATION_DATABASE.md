# EXIT — Base de situations de test systémiques

> **Statut :** 🟡 OUTIL DE DESIGN REVIEW — base initiale, évolutive  
> **Rôle :** rendre les situations de stress-test retrouvables, composables et traçables sans les transformer en scripts comportementaux.

## Convention de lecture

Chaque entrée décrit un contexte à éprouver. Les **Expected Properties** indiquent des invariants ou propriétés à vérifier ; elles ne prescrivent jamais ce qu'un participant doit décider.

Les statuts s'appliquent à la fiche de test, non au design gameplay :

| Statut | Sens |
|---|---|
| 🔵 CANDIDATE | idée de situation à préciser avant exécution |
| 🟡 ACTIVE TEST | situation suffisamment définie pour être sélectionnée dans un batch |
| 🟢 REFERENCE | situation de référence déjà utilisée pour une Design Review ; elle reste réutilisable |
| 🟠 HISTORICAL | ancien exemple conservé comme référence historique |
| 🔴 ISSUE FOUND | test ayant révélé un problème à analyser |

## Index synthétique

| ID | Name | Status | Complexity | Theme Tags | Validation Tags | Pipeline Coverage | Source |
|---|---|---:|---|---|---|---|---|
| ST01 | Ressource découverte, non prise | 🟡 ACTIVE TEST | Faible | #knowledge #take | #TEST_REEVALUATION | WORLD, PERCEPTION, KNOWLEDGE, PLAN | DR04, DR08-01 |
| ST02 | Localisation de ressource périmée | 🟢 REFERENCE | Moyenne | #knowledge #moving_target | #TEST_REEVALUATION | WORLD, PERCEPTION, KNOWLEDGE, MEMORY, APPRAISAL | DR04 |
| ST03 | B refuse de révéler le code Generator | 🟡 ACTIVE TEST | Moyenne | #refusal #knowledge | #TEST_INFORMATION_PARITY | COMMUNICATION, KNOWLEDGE, TRUST, APPRAISAL | DR08-05/06 |
| ST04 | B meurt avec le seul code | 🟢 REFERENCE | Moyenne | #death #information_extinct | #TEST_PLAN_INVALIDATION | WORLD, KNOWLEDGE, APPRAISAL, GAME_MANAGER | DR08-05 |
| ST05 | Désinformation d'un Observer | 🟡 ACTIVE TEST | Moyenne | #observer #broadcast #misinformation | #TEST_FALSE_CAUSALITY | OBSERVER, COMMUNICATION, KNOWLEDGE, APPRAISAL | DR08-05 |
| ST06 | Course concurrente vers le Safe | 🟡 ACTIVE TEST | Élevée | #concurrency #target_conflict | #TEST_ACTION_RACE | WORLD, PLAN, ACTION_RUNTIME, WORLD_EFFECT | DR08-05 |
| ST07 | A possède la Key sans connaître l'Exit | 🟢 REFERENCE | Faible | #knowledge #unknown_solution | #TEST_MAGIC_KNOWLEDGE | KNOWLEDGE, APPRAISAL, STRATEGY, PLAN | DR08-05 |
| ST08 | Conversation entendue par un tiers | 🟢 REFERENCE | Moyenne | #overhearing #knowledge | #TEST_INFORMATION_PARITY | COMMUNICATION, PERCEPTION, KNOWLEDGE, MEMORY | DR08-05 |
| ST09 | Cible de plan prise par autrui | 🟢 REFERENCE | Moyenne | #concurrency #take #invalidation | #TEST_PLAN_INVALIDATION | WORLD, PLAN, ACTION_RUNTIME, WORLD_EFFECT | DR08-05 |
| ST10 | Déclaration sans divulgation | 🟢 REFERENCE | Faible | #knowledge #proposal | #TEST_KNOWLEDGE_LEAK | COMMUNICATION, KNOWLEDGE, APPRAISAL | DR08-05 |
| ST11 | Affirmation non vérifiable / bluff | 🟡 ACTIVE TEST | Moyenne | #misinformation #knowledge | #TEST_FALSE_CAUSALITY | COMMUNICATION, KNOWLEDGE, TRUST, APPRAISAL | DR08-05 |
| ST12 | Promesse puis fuite | 🟢 REFERENCE | Élevée | #promise #betrayal #cross_exit | #TEST_PSYCHO_MODULATION | COMMUNICATION, MEMORY, RELATIONSHIP, TRUST, APPRAISAL | DR08-05 |
| ST13 | Promesse non tenue sans intention établie | 🟡 ACTIVE TEST | Élevée | #promise #betrayal | #TEST_FALSE_CAUSALITY | KNOWLEDGE, MEMORY, RELATIONSHIP, TRUST, APPRAISAL | DR08-05 |
| ST14 | Fausse accusation Observer | 🟡 ACTIVE TEST | Élevée | #observer #broadcast #misinformation | #TEST_FALSE_CAUSALITY | OBSERVER, KNOWLEDGE, TRUST, APPRAISAL | DR08-05 |
| ST15 | Proposal pendant un plan valide | 🟢 REFERENCE | Moyenne | #proposal #keep_change | #TEST_PENDING_DECISION | COMMUNICATION, APPRAISAL, STRATEGY, PLAN | DR08-06 |
| ST16 | Événements significatifs simultanés | 🟢 REFERENCE | Élevée | #concurrency #intervention | #TEST_REEVALUATION | WORLD, COMMUNICATION, APPRAISAL, ARBITRATION | DR08-06 |
| ST17 | Commitment temporairement suspendu | 🟢 REFERENCE | Élevée | #commitment #survival | #TEST_LOST_COMMITMENT | NEEDS, APPRAISAL, ARBITRATION, STRATEGY, PLAN | DR08-06 |
| ST18 | Cible perdue durant un Commitment | 🟡 ACTIVE TEST | Élevée | #commitment #moving_target #invalidation | #TEST_PLAN_INVALIDATION | WORLD, KNOWLEDGE, APPRAISAL, PLAN, ACTION_RUNTIME | DR08-06 |
| ST19 | Commitments incompatibles | 🟢 REFERENCE | Élevée | #commitment #target_conflict | #TEST_DEADLOCK | APPRAISAL, ARBITRATION, STRATEGY, PLAN | DR08-06 |
| ST20 | Proposal sans réponse | 🟢 REFERENCE | Moyenne | #proposal | #TEST_BLOCKING #TEST_PENDING_DECISION | COMMUNICATION, MEMORY, APPRAISAL, PLAN | DR08-06 |
| ST21 | Cible sociale morte | 🟡 ACTIVE TEST | Élevée | #death #commitment | #TEST_LOST_COMMITMENT | WORLD, KNOWLEDGE, MEMORY, APPRAISAL, PLAN | DR08-06 |
| ST22 | Proposition coercitive | 🟢 REFERENCE | Élevée | #proposal #threat | #TEST_BLOCKING | COMMUNICATION, KNOWLEDGE, APPRAISAL, ARBITRATION | DR08-06 |
| ST23 | Compte à rebours de l'Exit | 🟢 REFERENCE | Moyenne | #observer #broadcast #countdown #urgency | #TEST_REEVALUATION | OBSERVER, KNOWLEDGE, APPRAISAL, ARBITRATION, PLAN | DR08-07 |
| ST24 | Impossible ou improbable | 🟢 REFERENCE | Moyenne | #unlikely #impossible #effort | #TEST_PSYCHO_MODULATION | KNOWLEDGE, APPRAISAL, ARBITRATION, STRATEGY | DR08-07 |
| ST25 | Deux urgences critiques | 🟢 REFERENCE | Élevée | #urgency #survival | #TEST_PSYCHO_MODULATION | NEEDS, PHYSICAL_STATE, APPRAISAL, ARBITRATION | DR08-07 |
| ST26 | Raccourci objectivement présent mais inconnu | 🟢 REFERENCE | Moyenne | #unknown_solution #route_blocked | #TEST_MAGIC_KNOWLEDGE | WORLD, KNOWLEDGE, APPRAISAL, STRATEGY | DR08-07 |
| ST27 | Aucune bonne option connue | 🟢 REFERENCE | Élevée | #unlikely #urgency | #TEST_DEADLOCK | APPRAISAL, ARBITRATION, STRATEGY, PLAN | DR08-07 |
| AR01 | GoTo + broadcast Observer | 🟢 REFERENCE | Moyenne | #observer #broadcast #interruptible | #TEST_INTERRUPTION #TEST_REEVALUATION | OBSERVER, KNOWLEDGE, APPRAISAL, PLAN, ACTION_RUNTIME | DR08-08 |
| AR02 | GoTo + route bloquée | 🟢 REFERENCE | Moyenne | #route_blocked #invalidation | #TEST_ACTION_INVALIDATION | WORLD, PERCEPTION, APPRAISAL, PLAN, ACTION_RUNTIME | DR08-08 |
| AR03 | Jump + événement stratégique | 🟢 REFERENCE | Élevée | #interruptible #non_interruptible | #TEST_INTERRUPTION #TEST_COMMIT_POINT | WORLD, KNOWLEDGE, APPRAISAL, PLAN, ACTION_RUNTIME | DR08-08 |
| AR04 | Elevator + Exit ouverte | 🟢 REFERENCE | Moyenne | #elevator #exit_open | #TEST_CONDITIONAL_INTERRUPT | WORLD, KNOWLEDGE, APPRAISAL, PLAN, ACTION_RUNTIME | DR08-08 |
| AR05 | Salle toxique + nouvelle priorité | 🟢 REFERENCE | Élevée | #toxic #survival #urgency | #TEST_REEVALUATION #TEST_INTERRUPTION | WORLD, NEEDS, PHYSICAL_STATE, APPRAISAL, ACTION_RUNTIME | DR08-08 |
| AR06 | Sceller une porte face au feu/danger | 🟢 REFERENCE | Élevée | #door #fire #danger_room | #TEST_COMMIT_POINT #TEST_CONDITIONAL_INTERRUPT | WORLD, APPRAISAL, PLAN, ACTION_RUNTIME, WORLD_EFFECT | DR08-08 |
| AR07 | Give interrompu | 🟢 REFERENCE | Élevée | #transaction #give #interruptible | #TEST_TRANSACTION_ATOMICITY #TEST_INTERRUPTION | WORLD, COMMUNICATION, PLAN, ACTION_RUNTIME, WORLD_EFFECT | DR08-08 |
| AR08 | Take concurrent du même objet | 🟢 REFERENCE | Élevée | #transaction #take #concurrency | #TEST_DOUBLE_OWNERSHIP #TEST_ACTION_RACE | WORLD, PLAN, ACTION_RUNTIME, WORLD_EFFECT | DR08-08 |
| AR09 | Giver mort durant la transaction | 🔵 CANDIDATE | Élevée | #transaction #give #death | #TEST_TRANSACTION_ATOMICITY #TEST_ACTION_INVALIDATION | WORLD, KNOWLEDGE, PLAN, ACTION_RUNTIME, WORLD_EFFECT | Préparation DR08-08 |
| CST01 | Multi-Hazard Traversal | 🔵 CANDIDATE | Élevée | #jump #toxic #flood #resource_lost #urgency | #TEST_REEVALUATION #TEST_INTERRUPTION | WORLD, PHYSICAL_STATE, NEEDS, APPRAISAL, PLAN, ACTION_RUNTIME, WORLD_EFFECT | DR08-08 |
| GST01 | Global Vertical Slice Stress Test | 🟢 REFERENCE | Élevée | #knowledge #competition #proposal #exit_open #countdown | #TEST_KNOWLEDGE_LEAK #TEST_INFORMATION_PARITY #TEST_REEVALUATION | WORLD, PERCEPTION, COMMUNICATION, KNOWLEDGE, MEMORY, APPRAISAL, STRATEGY, GOAP, PLAN, ACTION_RUNTIME, WORLD_EFFECT, GAME_MANAGER, OBSERVER | DR08 final |

## Fiche standard d'une situation

Utiliser cette structure pour enrichir une entrée avant son exécution. La trace détaillée est consignée dans [le gabarit de Decision Trace](EXIT_TEST_DECISION_TRACE_TEMPLATE.md).

```text
Situation ID / Name / Status / Complexity
Source / Design Review provenance
Theme Tags / Validation Tags / Pipeline Coverage

Initial State
Current Goal / Strategy / Plan / Action (si pertinent)
Perturbation / Trigger
Accessible Information
Questions Tested
Existing CANON Invariants
Expected Properties
Observed Result (après exécution)
Issues / First Invalid Transition
Candidate General Rule
Cross-tests
Arbitration / Result
Open Questions
```

## Fiches initiales : situations importées

Les fiches ci-dessous constituent un niveau d'amorçage : elles décrivent le problème à tester à partir des sources citées. Un résultat observé, une règle candidate ou un arbitrage ne sont ajoutés qu'après exécution documentée.

### ST01 — Ressource découverte, non prise

- **Initial State / trigger :** un participant observe une ressource pertinente, mais choisit ou doit continuer sans la collecter.
- **Question :** la découverte reste-t-elle une Knowledge sans se convertir en possession, réservation ou objectif forcé ?
- **Expected Properties :** `Knowledge(Item, Location)` et l'état physique de possession restent distincts ; l'agent peut mémoriser l'opportunité sans la prioriser immédiatement.
- **CANON mobilisé :** `PERCEIVED ≠ UNDERSTOOD ≠ INTERACTABLE` ; `REEVALUATION ≠ CHANGE`.

### ST02 — Localisation de ressource périmée

- **Initial State / trigger :** une ressource est observée dans une salle à `t0`, puis est absente à `t1`.
- **Question :** la Memory versionne-t-elle l'état sémantique sans qualifier rétroactivement l'observation initiale de fausse ?
- **Expected Properties :** une Knowledge `CURRENT` pertinente remplace la version antérieure ; fiabilité d'acquisition et validité actuelle ne sont pas confondues.
- **CANON mobilisé :** DR04 versioning ; changement temporel ≠ contradiction.

### ST03 à ST05 — Code, extinction d'information et source Observer

- **ST03 :** B refuse de révéler son Code Generator ; le refus ne crée ni accès magique au code ni Goal forcé chez les autres agents.
- **ST04 :** B meurt sans avoir transmis l'unique Code ; ses objets peuvent être exposés selon les règles physiques, mais le Code peut être définitivement perdu.
- **ST05 :** un broadcast Observer fournit une assertion inexacte ; le message est une information sourcée et non une mutation de World Truth.
- **Expected Properties :** `WORLD TRUTH ≠ MY KNOWLEDGE ≠ WHAT I CLAIM ≠ WHAT YOU HEAR ≠ WHAT YOU BELIEVE ≠ WHAT YOU DO`.

### ST06 à ST11 — Accès concurrent, communication et information

- **ST06 :** plusieurs participants convoitent le Safe sans réserver la ressource par leur seul plan ; l'action appliquée au World tranche l'état autoritatif.
- **ST07 :** A porte la Key mais ignore l'Exit ; posséder la ressource ne révèle ni son usage concret ni la localisation de sa cible.
- **ST08 :** un tiers à portée entend une conversation ; entendre ne signifie ni mémoriser, ni croire, ni répondre.
- **ST09 :** une cible de `Take` est prise par autrui ; le plan devenu incompatible est réévalué.
- **ST10 :** un participant déclare connaître l'Exit sans en révéler la localisation ; le destinataire apprend la prétention, pas son contenu.
- **ST11 :** une affirmation non vérifiable peut être vraie, fausse, partielle ou bluffée ; son impact dépend de la source, du contexte et de l'évaluation subjective.

### ST12 à ST14 — Promesses, interprétation et accusation

- **ST12 :** un participant promet de guider A, reçoit la Key puis s'échappe ; A peut interpréter l'expérience comme une trahison si son contexte le justifie.
- **ST13 :** une promesse non tenue n'établit pas automatiquement l'intention de trahir : erreur, empêchement, changement du World ou autre contexte restent possibles.
- **ST14 :** un Observer formule une accusation inexacte ; elle reste une assertion dont la source et la fiabilité doivent être évaluées.
- **Expected Properties :** `FACT ≠ KNOWLEDGE ≠ INTERPRETATION ≠ EMOTIONAL RESPONSE ≠ DECISION` ; aucune conversion directe `événement social → Trust`.

### ST15 à ST22 — Communication, Commitments et absence de blocage

- **ST15 :** une Proposal arrive alors qu'un plan valide est en cours ; elle peut être mémorisée sans imposer une interruption ni un changement.
- **ST16 :** plusieurs événements pertinents surviennent simultanément ; les mises à jour sont consolidées avant l'arbitrage éventuel.
- **ST17 :** un Need critique suspend temporairement un Commitment actif ; le Commitment peut rester connu et actif sans être exécuté à cet instant.
- **ST18 :** la cible liée à un Commitment est perdue ; le résultat doit être réévalué sans présumer de la politique future de récupération.
- **ST19 :** plusieurs Commitments entrent en conflit ; ils participent à l'Appraisal, aucun n'obtient une autorité d'exécution autonome.
- **ST20 :** une Proposal non répondue reste non bloquante et peut redevenir pertinente selon le contexte.
- **ST21 :** un participant visé par une attente sociale meurt ; le savoir local de ce décès conditionne la réévaluation, sans créer de connaissance magique.
- **ST22 :** « Suis-moi ou je te tue » est une communication conditionnelle : une menace ≠ capacité prouvée ≠ action future garantie ≠ compliance forcée.

### ST23 à ST27 — Appraisal sous contraintes

- **ST23 :** un broadcast « La porte se referme dans 30 secondes » peut faire évoluer l'Urgency d'Escape sans changer son Importance fondamentale.
- **ST24 :** une sortie estimée atteignable avec une probabilité faible reste `UNLIKELY`, non nécessairement `IMPOSSIBLE` ; une tentative reste concevable sans être prescrite.
- **ST25 :** Escape et survie physique peuvent être simultanément critiques ; l'arbitrage rend le conflit visible, il ne produit pas une priorité automatique.
- **ST26 :** un raccourci objectivement existant mais inconnu ne peut entrer dans les Known Possibilities du participant.
- **ST27 :** toutes les possibilités connues peuvent être défavorables ; le système ne garantit pas l'existence d'une bonne option.

## Résultats de référence DR08-08

Les cas AR01–AR08 ont été utilisés par la Design Review DR08-08. Ils sont 🟢 REFERENCE en tant que situations de test, sans prescrire le comportement d'un participant.

| ID | Perturbation minimale | Propriété à tester | Éléments restant à préciser |
|---|---|---|---|
| AR01 | un broadcast pertinent survient durant `GoTo` | une réévaluation peut créer une nouvelle préférence sans annuler `GoTo` automatiquement | politiques de réconciliation runtime |
| AR02 | la route devient indisponible durant `GoTo` | la navigation peut recalculer localement ; sans route, l'Action peut produire un événement pertinent | détection, récupération et lifecycle d'action |
| AR03 | un événement stratégique survient durant `Jump` | la cognition continue ; la physique reste autoritative jusqu'à collision/outcome pertinent | phases, opportunités de transition, sécurité physique |
| AR04 | l'Exit s'ouvre durant un déplacement en elevator | une opportunité nouvelle peut être mémorisée/réévaluée sans téléporter ou réserver une cible | accès, sortie de l'elevator, timing |
| AR05 | une nouvelle priorité apparaît dans une zone toxique | besoin, danger et plan courant sont représentés sans `état → action` direct | dommages, seuils, interruptibilité |
| AR06 | une porte est scellée face au feu/danger | l'interruption ne rollback pas les modifications physiques partielles | interaction, effets, phases irréversibles |
| AR07 | une transaction `Give` subit une perturbation | `Give` est direct ; l'Inventory Commit conserve une possession univoque | protocole détaillé, moment exact du transfert |
| AR08 | deux participants exécutent `Take` sur le même objet | le premier Commit valide obtient l'objet ; aucune double possession | ordre d'application et notification |
| AR09 | le giver meurt durant `Give` | **🔵 Candidate :** état cohérent à vérifier sans inventer d'issue transactionnelle | décès, exposition inventaire, point de transfert |

### CST01 — Multi-Hazard Traversal — 🔵 CANDIDATE

- **Situation :** traversée de plateformes avec sauts, plateformes susceptibles de s'effondrer, gaz toxique progressif, eau glacée montante, Medikit et injection d'adrénaline opportunistes, Key nécessaire au fond de la salle, puis échelle vers une trappe d'évasion déjà ouverte.
- **But du test :** éprouver conjointement Action/Execution, transition physique, Jump/Physics/Collision, dangers continus, conséquences Health, opportunités de ressources, Appraisal sous urgences concurrentes, réévaluations et persistance/évolution Goal-Strategy.
- **Expected Properties :** aucune décision de traversée, de soin ou de fuite n'est prescrite ; les conséquences physiques restent autoritatives et toute transition cognitive doit être justifiée dans la trace.
- **À préciser avant exécution :** topologie, vitesses et conséquences physiques, règles de disparition des plateformes, accessibilité réelle des ressources et critères de sélection du batch.

## Matrices de couverture initiale

`●` indique que la situation traverse explicitement la dimension ; l'absence n'indique pas qu'elle ne pourra jamais la traverser après enrichissement.

### Situation × thèmes (échantillon de référence)

| Situation | Info | Social | Concurrence/transaction | Danger/survie | Observer | Exit | Action runtime |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| ST02 | ● |  |  |  |  |  |  |
| ST04 | ● |  |  |  |  |  |  |
| ST08 | ● | ● |  |  |  |  |  |
| ST09 |  |  | ● |  |  |  | ● |
| ST12 | ● | ● |  |  | ● | ● |  |
| ST16 |  | ● | ● |  | ● |  |  |
| ST17 |  | ● |  | ● |  |  | ● |
| ST22 | ● | ● |  |  |  |  |  |
| ST23 | ● |  |  |  | ● | ● |  |
| ST25 |  |  |  | ● |  | ● |  |
| AR07 |  | ● | ● |  |  |  | ● |
| AR08 |  |  | ● |  |  |  | ● |

### Situation × couches du pipeline (échantillon de référence)

| Situation | WORLD | PERCEPTION | COMMUNICATION | KNOWLEDGE | MEMORY | NEEDS | PHYSICAL_STATE | RELATIONSHIP | TRUST | APPRAISAL | ARBITRATION | STRATEGY | GOAP | PLAN | ACTION_RUNTIME | WORLD_EFFECT | GAME_MANAGER | OBSERVER |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| ST02 | ● | ● |  | ● | ● |  |  |  |  | ● |  | ● |  |  |  | ● |  |  |
| ST08 | ● | ● | ● | ● | ● |  |  |  |  | ● |  |  |  |  |  |  |  |  |
| ST09 | ● |  |  | ● |  |  |  |  |  | ● | ● | ● | ● | ● | ● | ● |  |  |
| ST12 | ● | ● | ● | ● | ● |  |  | ● | ● | ● | ● |  |  |  |  | ● |  | ● |
| ST16 | ● |  | ● | ● |  |  |  |  |  | ● | ● | ● | ● | ● | ● | ● |  | ● |
| ST17 | ● |  | ● | ● | ● | ● | ● | ● | ● | ● | ● | ● | ● | ● | ● | ● |  |  |
| ST23 | ● |  | ● | ● |  | ● |  |  |  | ● | ● | ● | ● | ● |  | ● |  | ● |
| ST25 | ● |  |  | ● | ● | ● | ● |  |  | ● | ● | ● | ● | ● |  | ● |  |  |
| AR07 | ● |  | ● | ● |  |  |  | ● | ● | ● | ● | ● | ● | ● | ● | ● |  |  |
| AR08 | ● | ● |  | ● |  |  |  |  |  | ● | ● | ● | ● | ● | ● | ● |  |  |

## Lecture de couverture et prochaines lacunes visibles

- La banque initiale couvre déjà fortement Information, Social, Appraisal et réévaluation.
- Les couches `GOAP` et `ACTION_RUNTIME` sont présentes surtout à titre de traçage ou dans les candidats AR ; leurs règles restent à arbitrer lors de DR08-08.
- Les cas `#flood`, `#underwater`, `#low_air`, `#drop`, `#resource_lost` et `#branch_invalidated` ne disposent pas encore d'une situation dédiée : ce sont des lacunes de couverture à traiter par futurs batches, non des défauts de design déjà constatés.
