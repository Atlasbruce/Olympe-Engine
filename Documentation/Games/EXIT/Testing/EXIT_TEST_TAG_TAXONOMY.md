# EXIT — Taxonomie des tags de test

**Statut :** 🟡 Taxonomie initiale extensible.

## Thèmes

| Famille | Tags | Sens |
|---|---|---|
| État participant | `#injured #unconscious #death` | État corporel ou disponibilité du participant. |
| Survie/danger | `#survival #danger_room #toxic #fire #flood #underwater #low_air` | Pression physique/environnementale. |
| Runtime action | `#jump #interruptible #non_interruptible #conditional_interrupt #invalidation #commit_point` | Hypothèses ou validations de cycle d'action ; `#jump` identifie une traversée physique relevant de la simulation. `#conditional_interrupt` est un tag historique de recherche : il ne désigne plus une catégorie runtime CANON depuis DR08-08. |
| Social | `#proposal #commitment #refusal #promise #betrayal #threat` | Échange et attente sociale ; `#threat` décrit une menace communiquée, jamais une action forcée. |
| Transaction | `#transaction #give #take #drop #exchange` | Possession et transfert. |
| Information | `#knowledge #misinformation #overhearing #unknown_solution` | Accès, qualité et circulation de l'information. |
| Observer | `#observer #broadcast #intervention` | Direction diégétique et annonces. |
| Décision | `#urgency #unlikely #impossible #keep_change #effort` | Appraisal et réévaluation. |
| Multi-agent | `#competition #concurrency #target_conflict` | Conflit ou simultanéité entre participants. |
| Monde | `#door #elevator #moving_target #route_blocked` | État d'élément ou de route. |
| Quête | `#branch_invalidated #resource_lost #information_extinct` | État de dépendance/solvabilité. |
| Sortie | `#exit_open #cross_exit #countdown` | Évasion et temporalité. |

## Cibles de validation

`#TEST_KNOWLEDGE_LEAK #TEST_MAGIC_KNOWLEDGE #TEST_DEADLOCK #TEST_REEVALUATION #TEST_PLAN_INVALIDATION #TEST_ACTION_INVALIDATION #TEST_INTERRUPTION #TEST_CONDITIONAL_INTERRUPT #TEST_COMMIT_POINT #TEST_DOUBLE_OWNERSHIP #TEST_TRANSACTION_ATOMICITY #TEST_ACTION_RACE #TEST_LOST_COMMITMENT #TEST_FALSE_CAUSALITY #TEST_BLOCKING #TEST_PSYCHO_MODULATION #TEST_INFORMATION_PARITY #TEST_PENDING_DECISION`

Chaque tag désigne la propriété que le test cherche à éprouver : fuite/connaissance magique, blocage, réévaluation, invalidation, interruption, propriété, atomicité, concurrence, engagement, causalité, modulation ou parité. Ils ne prescrivent jamais un résultat comportemental.

## Couverture pipeline

`WORLD, PERCEPTION, COMMUNICATION, KNOWLEDGE, MEMORY, NEEDS, PHYSICAL_STATE, RELATIONSHIP, TRUST, PERSONALITY, VALUES, PSYCHO_EMOTIONAL, APPRAISAL, ARBITRATION, STRATEGY, GOAP, PLAN, ACTION_RUNTIME, WORLD_EFFECT, GAME_MANAGER, OBSERVER`.
