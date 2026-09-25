# EXIT — DR08-09 : Attempt, Exit & Game Mode Runtime

> **Statut :** 🟢 REVIEW COMPLETED — 25 septembre 2026  
> **Portée :** contrat conceptuel des états participant, de l'Exit, de la fin d'Attempt et de leur interprétation par Game Mode.  
> **Limite :** aucun algorithme, API, timer chiffré ou structure C++ n'est défini.

## 1. Dimensions indépendantes du Participant Runtime State

Un Participant Runtime State n'est pas une unique jauge.

| Dimension | États / sens |
|---|---|
| Physical State | `NORMAL`, `INJURED`, `UNCONSCIOUS`, `DEAD` |
| Level Presence | `IN_LEVEL`, `OUT_OF_LEVEL` |
| Agency | `ACTIVE`, `SUSPENDED` |
| Participant Outcome | non résolu, `ESCAPED` / `WON` selon Game Mode, `LOST` |
| Attempt Runtime State | `ACTIVE`, `SUSPENDED` |
| Session State | le participant reste présent dans la Session tant qu'elle continue |

```text
PHYSICAL STATE ≠ LEVEL PRESENCE ≠ AGENCY
≠ PARTICIPANT OUTCOME ≠ ATTEMPT RUNTIME STATE
```

## 2. UNCONSCIOUS et Recovery

🟢 **CANON.** `UNCONSCIOUS` est récupérable pendant l'Attempt ; il peut être temporaire ou persister jusqu'à intervention. Durées et algorithmes restent 🟡 WIP.

Agency est suspendue : aucune perception consciente, communication cognitive normale, Appraisal, Goal/Strategy/Plan volontaire ni Action volontaire. Le corps reste dans le World, peut subir des effets, être soigné ou fouillé. Son inventaire reste attaché jusqu'à un `Take` effectif.

```text
SEARCH ≠ TAKE
ABSENCE DE RÉSISTANCE ≠ CONSENTEMENT COGNITIF
```

Un participant inconscient ne produit jamais un faux `ACCEPT`. Dans `B Search(A) → B Take(Battery) → Inventory Commit`, B est le seul Actor de la mutation.

Pendant l'inconscience, aucune Knowledge n'est normalement acquise par perception/communication. Au Recovery, le participant peut connaître les faits directement liés à son rétablissement — par exemple « B m'a soigné » — sans acquérir les événements environnants.

```text
KNOWS "B HEALED ME" ≠ TRUSTS B ≠ LIKES B ≠ WILL COOPERATE WITH B
```

Cette Knowledge peut alimenter Memory, Appraisal, Relationship, Trust, réaction ou Empreinte émotionnelle sans imposer leurs résultats. Le nom d'un éventuel Recovery Context reste 🟡 WIP.

## 3. DEAD

🟢 **CANON.** Dans EXIT, `DEAD` est terminal pour l'Attempt courant, mais non pour le Participant.

```text
Physical State = DEAD
Level Presence = IN_LEVEL (corps)
Agency = SUSPENDED
Session = IN_SESSION
Recovery during Attempt = impossible
DEAD ≠ DESTROYED
```

Le corps reste matérialisé. L'inventaire physique est libéré/exposé selon les règles World ; Knowledge et Memory ne deviennent jamais du loot. Les autres participants, le World et l'Attempt continuent. Un Restart restaure état, Agency et position initiale conformément à DR07.

## 4. Exit : CLOSED/LOCKED puis OPEN

🟢 **CANON.** `UNLOCKED` n'est pas un état gameplay distinct nécessaire dans le Vertical Slice final.

```text
EXIT CLOSED / LOCKED
        ↓ Quest-defined Exit Opening Conditions satisfied
EXIT OPEN
```

Les conditions d'ouverture relèvent du Quest et peuvent être simples ou composées. Dans la variante simple, `Use(Key, Exit) → Key CONSUMED → Exit OPEN`. La Key n'est donc pas universellement l'unique mécanisme d'ouverture. L'Exit reste ouverte jusqu'à la fin de l'Attempt, puis Restart restaure l'état initial. Aucun participant ne la possède ni ne la réserve.

```text
EXIT OPEN ≠ ESCAPED
```

## 5. CrossExit et ESCAPED

🟢 **CANON.** `valid CrossExit START = ESCAPED`, sans attendre la fin d'une animation. Cette transition est valide seulement si l'Attempt permet le franchissement, si l'Exit est OPEN et si ses préconditions sont satisfaites.

```text
CrossExit START → ESCAPED → présentation/animation éventuelle
```

Dès la transition : Outcome/escape state = `ESCAPED`, Level Presence = `OUT_OF_LEVEL`, Level Agency = `SUSPENDED`. Le participant quitte la simulation active du Level.

## 6. ESCAPED et waiting room

🟢 **CANON.** `ESCAPED` ne retire pas le participant de la Session.

```text
ESCAPED = removed from active Level simulation + waiting for Debrief
```

Le participant attend dans une waiting room individuelle/privée : constantes physiques restaurées pour l'attente, aucune interaction ou perception du Level, aucune communication avec les participants `IN_LEVEL` ni entre `OUT_OF_LEVEL`, aucune Knowledge sur la suite du Level et cognition stratégique Level suspendue. Memory, Knowledge, Empreintes émotionnelles et histoire déjà acquises restent préservées.

## 7. Game Mode et outcomes

🟢 **CANON.** Le World produit `Participant X = ESCAPED`. Le GameManager + Game Mode en interprètent la conséquence ; l'Exit ne connaît pas Cooperative/Competitive.

### Competitive

```text
first valid CrossExit START
→ participant ESCAPED + WINNER
→ Observer announcement
→ END ATTEMPT
→ participants restants LOST
→ DEBRIEF
```

`DEAD ≠ END ATTEMPT`, mais `first ESCAPED + Competitive → END ATTEMPT`.

### Cooperative

`Cooperative` décrit les outcomes, jamais une obligation morale de coopération. Au premier CrossExit valide : participant `ESCAPED`, Observer Broadcast, Exit Timer, Evacuation Alarm et `EVACUATION PHASE`. Les suivants peuvent franchir l'Exit, qui reste ouverte.

L'Attempt termine si aucun participant **CONSCIOUS** ne reste `IN_LEVEL`, ou si l'Exit Timer expire. `UNCONSCIOUS` et `DEAD` ne comptent pas comme participants conscients. Un participant conscient peut encore soigner un inconscient, agir ou sortir.

## 8. Information d'évacuation

| Mécanisme | Nature | Information accessible |
|---|---|---|
| Observer Broadcast | communication ponctuelle | qui s'est échappé, pour les conscients `IN_LEVEL` au moment de l'émission |
| Evacuation Alarm | état World persistant perceptible | phase d'évacuation active |
| Exit Timer Displays | représentation World visible/lisible | temps restant après perception/lecture légitime |

Le GameManager possède le timer, mais le World rend son information perceptible :

```text
GameManager Timer → World representation → perception/read → Knowledge
GameManager Timer → magical NPC Knowledge   [INTERDIT]
PERCEIVES ALARM ≠ KNOWS WHO ESCAPED
```

Un participant inconscient pendant un broadcast ne l'acquiert jamais rétroactivement. Après Recovery, il peut percevoir l'alarme ou lire le timer sans connaître l'identité du participant échappé.

## 9. END ATTEMPT

🟢 **CANON.** `END ATTEMPT` est une frontière de simulation, non un état physique. Tous les participants deviennent runtime `SUSPENDED` jusqu'au Debrief : `ESCAPED`, `CONSCIOUS IN_LEVEL`, `UNCONSCIOUS IN_LEVEL` et `DEAD IN_LEVEL`.

Il ne tue, ne soigne, ne réveille, ne ressuscite et n'efface aucune expérience. En Cooperative : `ESCAPED → WON`; les participants restants `IN_LEVEL` au Timer expiry sont `LOST`. Restart porte les transformations prévues par DR07.

```text
QUEST FAILURE ≠ PLAYER FAILURE ≠ SESSION END
```

## 10. Authoritative Event Ordering

🟢 **CANON.** Des événements visuellement simultanés sont traités selon un ordre autoritatif déterministe. Chaque événement est évalué contre l'état issu du précédent.

```text
VISUAL SIMULTANEITY ≠ LOGICAL SIMULTANEITY
```

Cette règle générale est cohérente avec l'Inventory Commit. En Competitive, `CrossExit START(A)` peut produire `A ESCAPED → END ATTEMPT`; un CrossExit B traité ensuite est réévalué contre l'Attempt terminé. Si `TimerExpired` précède CrossExit C, celui-ci est invalide ; s'il le suit, C est déjà échappé.

## 11. WIP explicitement préservés

- durée Exit Timer et durée d'inconscience ;
- algorithme de Recovery et nom/structure éventuelle du Recovery Context ;
- présentation de l'alarme, du timer et de la waiting room ;
- seuils/fréquences d'Appraisal ;
- API/lifecycle Action, GOAP, navigation, animation ;
- event queue et tie-breaking techniques.

Le contrat de design est CANON ; l'implémentation reste libre tant qu'elle le respecte.
