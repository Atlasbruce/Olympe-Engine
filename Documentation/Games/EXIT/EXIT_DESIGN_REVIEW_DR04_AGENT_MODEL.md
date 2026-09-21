# EXIT — Revue de conception DR-04
## Modèle Agent

**Statut :** 🟢 REVIEW COMPLETED  
**Date :** 18 septembre 2026  
**Participants :** Nicolas, ChatGPT  
**Objet :** définir l'état interne exploitable d'un NPC, ses connaissances mémorisées et leurs flux, sans définir l'architecture C++, la psychologie sociale ou le mécanisme de décision.

## 1. Schéma canonique

```text
MONDE / ECS — réalité autoritative
       │ données accessibles
       ▼
SYSTÈMES IA NPC — perception, identification, qualification, évaluation
       │ lit / écrit
       ▼
TABLEAU NOIR IA PRIVÉ — état interne exploitable
├── Données Monde / IA
└── Données de jeu / cognitives
    ├── mémoire : banque sélective de connaissances mémorisées
    ├── besoins et buts
    └── futurs états sociaux et psychologiques
```

Les systèmes IA NPC sont des traitements ; le tableau noir est l'état et les données. Ensemble, ils constituent le cerveau fonctionnel du NPC. Le tableau noir n'est ni le cerveau seul, ni une vérité du Monde, ni une architecture C++ prescrite.

## 2. Décisions validées

### DR04-D01 — Tableau noir IA

**🟢 CANON.** Structure privée représentant l'état interne exploitable du NPC ; elle distingue Données Monde / IA et Données de jeu / cognitives, conformément à DR-03.

### DR04-D02 — Mémoire

**🟢 CANON.** La mémoire est une banque sélective, structurée et réactualisable de connaissances mémorisées par le NPC : son inventaire de connaissances. Elle ne constitue ni snapshot du Monde, ni journal exhaustif d'événements, ni étape obligatoire `mémoire → connaissance`.

Elle conserve les connaissances gameplay pertinentes acquises notamment par perception, observation, événement vécu, interaction, conversation, échange ou transaction. Sa finalité est la stratégie, le raisonnement, l'action et l'échange d'information ; elle ne simule pas exhaustivement une mémoire humaine.

### DR04-D03 — Source et mode d'acquisition

**🟢 CANON.** Le rattachement à une **source** est recommandé, non obligatoire : il indique qui ou quoi est à l'origine d'une information lorsque c'est identifiable et pertinent. Le **mode d'acquisition** indique comment elle a été acquise.

| Exemple | Mode d'acquisition | Source |
|---|---|---|
| Clé rouge observée sur une table | Observation directe | Facultative / aucune |
| Déflagration entendue au loin | Perception auditive | Inconnue / aucune |
| Bob indique une localisation | Conversation | Bob |

Il ne faut pas artificiellement imposer « moi-même » comme source.

### DR04-D04 — Perception, observation et connaissance

**🟢 CANON.** Une perception ou observation ne devient pas automatiquement une connaissance mémorisée.

```text
Monde → perception / événement / interaction → systèmes IA NPC
→ identification / qualification / évaluation
→ information gameplay pertinente ? → connaissance candidate
→ consolidation dans la mémoire
```

Les systèmes IA déterminent si l'information est identifiable, qualifiée et pertinente avant mémorisation.

### DR04-D05 — Politique de mémorisation

**🟢 CANON au niveau des principes.** La mémoire n'est pas un journal d'événements. Une connaissance candidate est retenue lorsqu'elle est utile ou potentiellement pertinente pour raisonnement, stratégie, action ou échange ; qu'elle est rattachable, quand cela a du sens, à un sujet gameplay identifiable ; qu'elle est contextualisable dans le temps ; et qu'elle provient d'un événement d'acquisition pertinent.

Cette politique et taxonomie sont légères et extensibles : aucune ontologie exhaustive n'est créée.

### DR04-D06 — Consolidation et versioning

**🟢 CANON.** Avant ajout, la mémoire recherche une connaissance `CURRENT` correspondante.

```text
Nouvelle information pertinente → recherche de CURRENT liée
  aucune → CREATE : nouvelle connaissance CURRENT
  identique → IGNORE : aucune écriture, aucun timestamp modifié
  état sémantiquement changé → VERSION : ancienne SUPERSEDED, nouvelle CURRENT
```

Les systèmes IA consultent normalement les connaissances `CURRENT`. L'historique reste disponible seulement si un besoin le justifie.

Exemple : `Medikit_17 / Room2 / PRESENT / t0` devient `SUPERSEDED` si une observation ultérieure confirme `Medikit_17 / Room2 / ABSENT / t1`, qui devient `CURRENT`.

### DR04-D07 — Timestamp

**🟢 CANON.** Le timestamp correspond au moment d'acquisition ou mémorisation de l'état pertinent. Une observation identique ne le réécrit pas : observation à 10:15 `CREATE`, confirmations identiques à 10:16 et 10:20 `IGNORE`, absence constatée à 10:27 `VERSION`. L'ancienneté peut être calculée ultérieurement sans écriture permanente.

### DR04-D08 — Fiabilité

**🟢 CANON au niveau du principe.** Chaque connaissance peut porter une fiabilité subjective. Répéter la même information ne la renforce pas automatiquement. Une évolution éventuelle provient d'un élément nouveau : vérification directe, contradiction, source indépendante ou corroboration pertinente.

Le calcul exact est 🟡 WIP. Aucune seconde jauge persistante de fraîcheur n'est introduite automatiquement : le timestamp apporte le contexte temporel permettant l'évaluation future.

### DR04-D09 — États sémantiques

**🟢 CANON.** La mémoire retient les changements sémantiques utiles au gameplay, non les changements techniques continus. Sont pertinents : porte fermée→ouverte, objet présent→absent, générateur éteint→en marche, générateur intact→détruit, NPC vivant→mort, objet salle 2→salle 5. Ne le sont pas : progression d'animation, image d'animation ou rotation technique.

### DR04-D10 — Changement temporel et contradiction

**🟢 CANON.** Une version ancienne n'est pas nécessairement fausse. `Door4 = CLOSED` à `t0`, puis `Door4 = OPEN` à `t1`, signifie que la première version est historiquement valide et la seconde actuellement pertinente. Ceci se distingue de deux informations incompatibles pour un contexte temporel comparable. Leur gestion détaillée reste à approfondir.

### DR04-D11 — Inventaire

**🟢 CANON.** L'inventaire physique est un état factuel et autoritatif de possession du NPC. Les systèmes IA peuvent en détenir une représentation opérationnelle dans le tableau noir. Il n'est pas nécessaire de créer une connaissance subjective avec fiabilité pour chaque objet porté. Inventaire physique, mémoire et connaissance restent distincts.

### DR04-D12 — Capacités et grammaire systémique

**🟢 CANON.** Les NPC possèdent les mêmes capacités systémiques fondamentales. Si réparer est une capacité fondamentale, tous savent réparer. Les objets et éléments interactifs portent une grammaire commune, connue des systèmes IA et du joueur : nature, fonction, actions possibles, champs d'application, cibles compatibles, combinaisons et effets.

Les comportements divergent par positions, inventaires, connaissances, états, besoins, relations et décisions — non par une asymétrie arbitraire de connaissance des règles. La disponibilité d'une action dépend du contexte et de ses préconditions.

### DR04-D13 — Besoin

**🟢 CANON.**

```text
état factuel → évaluation IA → besoin
```

`ÉTAT ≠ BESOIN ≠ BUT ≠ ACTION`. La décision reste DR-06.

### DR04-D14 — Inférence

**🔵 PROPOSITION / non requise au modèle de base.** L'inférence n'est pas obligatoire. Le versioning permet déjà de savoir que la clé rouge n'est plus à son dernier emplacement connu, sans spéculer automatiquement sur qui l'a prise ou pourquoi. Elle ne devra être ajoutée que si un cas de gameplay démontre une valeur impossible à obtenir avec le modèle courant.

## 3. Taxonomie légère de connaissances

**🟡 WIP / extensible.** Elle sera testée sur des situations EXIT et n'accueillera une nouvelle catégorie que lorsqu'un cas réel ne peut être représenté correctement.

```text
CONNAISSANCE
├── ENTITÉ / ÉLÉMENT : identité, localisation, état, disponibilité
├── ÉVÉNEMENT : événement gameplay pertinent perçu ou vécu
├── INTERACTION / TRANSACTION : échange, conversation, action significative
└── SITUATION / ÉTAT DU MONDE : état systémique pertinent
```

## 4. Scénarios de validation

Les scénarios à tester sont : clé observée puis disparue ; medikit observé puis consommé ; porte fermée puis ouverte ; générateur fonctionnel puis détruit ; information répétée par une même source ; confirmation indépendante par conversation puis observation ; déflagration sans source identifiable ; objet porté par le NPC ; objet connu mais distant ; combinaison systémique identique pour joueur et NPC.

Pour chacun : `Monde/événement → acquisition → qualification → décision mémoire → connaissance CURRENT → éventuelle version précédente`. Stratégie et planification restent exclues.

## 5. Éléments reportés

- **DR-05 :** personnalité, état psychologique, émotions, relations et confiance.
- **DR-06 :** arbitrage, stratégie, planification, révision de plan et décision d'action.
- **DR-07 :** branches irréalisables, défaite, récupération et Failure Contract.
- **Backlog :** calcul de fiabilité, taxonomie enrichie et valeur gameplay éventuelle de l'inférence.

## 6. Registre de revue

| Champ | Valeur |
|---|---|
| Décisions validées | DR04-D01 à DR04-D13 |
| Proposition conservée | DR04-D14 — inférence non requise |
| Statut | 🟢 REVIEW COMPLETED |

## Référence DR-07

Lors d'une relance du niveau, les mêmes NPC conservent mémoire et connaissances acquises ; lors d'un rechargement du niveau, de nouvelles instances NPC repartent de la configuration initiale. Cette distinction n'altère pas le modèle de mémoire DR-04.
