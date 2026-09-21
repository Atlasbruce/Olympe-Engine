# EXIT Design Bible
## 20 - Personnalité, état psychologique, émotions et besoins

**Version :** 0.1  
**Statut :** 🟡 WIP - modèle conceptuel, paramètres non validés  
**Dépendances :** Audit Phase 1, 11 connaissance, 18 NPC Agents, 19 GOAP, 21 Relations  
**Dernière mise à jour :** 17 septembre 2026

---

## 1. Intention

Les NPC d'EXIT doivent prendre des décisions différentes face à une même situation, et évoluer quand les événements les affectent. Ce système ne sert pas à simuler exhaustivement la psychologie humaine ni à décorer les dialogues. Il doit modifier de manière lisible les priorités, l'évaluation du danger, l'acceptation d'une aide, la coopération et les stratégies admissibles.

La simplicité reste une valeur de conception. Un modèle n'est retenu que s'il produit une différence de comportement compréhensible, testable et utile au jeu.

## 2. Les cinq couches à ne pas fusionner

| Couche | question | Temporalité | Exemple |
|---|---|---|---|
| personnalité | Qui suis-je de manière relativement stable ? | Longue | Prudence élevée, empathie forte. |
| état psychologique | Comment vais-je sur une durée significative ? | Moyenne | Sentiment de sécurité bas, motivation dégradée. |
| émotion | Que ressens-je en réaction à une situation ? | Courte | Peur après une sirène, soulagement après un soin. |
| besoin | Quel manque ou quelle pression exige une réponse ? | Variable | Soigner une blessure, retrouver sécurité, manger. |
| relation | Que pense-je de cette personne précise ? | Moyenne à longue | confiance faible envers Ivo, gratitude envers le joueur. |

```text
PERSONALITY         module l'évaluation habituelle
PSYCHOLOGICAL STATE modifie durablement la capacité à faire face
EMOTION             réagit à un événement et peut modifier l'urgence
NEED                crée une pression et alimente des Goals
RELATIONSHIP        modifie l'évaluation d'un agent particulier
        |
        v
CHOIX DE STRATÉGIE ET PLANIFICATION
```

### Règle de vocabulaire

« Peureux » ne doit pas automatiquement être un trait. Il peut désigner une personnalité prudente, un état psychologique de sécurité faible, une émotion de peur momentanée, ou une combinaison des trois. La Bible emploiera les termes de couche appropriés.

## 3. Modèle historique audité

Le PDF historique S01 contient plusieurs itérations d'un `EmotionState` et de tables de pondération. Les éléments suivants sont conservés comme généalogie, sans être validés comme design cible.

| Élément historique | Contenu retrouvé | Statut |
|---|---|---|
| Jauges initiales | Stress, confiance envers autrui, confiance en soi, espoir, besoin de sécurité. | 🟠 HISTORIQUE |
| Jauges enrichies | Sécurité physique, soutien social, maîtrise/contrôle, clarté, confort, nouveauté, dignité, sens, moralité, stress. | 🟠 HISTORIQUE |
| Références théoriques | Maslow pour les besoins, Plutchik pour vocabulaire, intensités et diades émotionnelles. | 🟠 HISTORIQUE |
| Chaîne causale | Événement externe -> jauges internes -> émotion -> comportement. | 🟡 WIP, architecture utile à tester. |
| Matrices de pondération | Poids et seuils numériques proposés par Gemini. | 🟠 HISTORIQUE, non calibré et non canonique. |
| Diagramme radar | Outil possible de débogage/équilibrage d'états internes. | 🔵 PROPOSITION. |

### Leçons de l'audit

1. Le modèle historique contient une intuition forte : les événements doivent influencer les décisions via des états intermédiaires, et non déclencher des scripts d'émotion isolés.
2. Il mélange cependant besoins, relations, traits, états psychologiques et émotions dans une même structure. La séparation des couches est nécessaire.
3. Les listes sont très larges. Les adopter sans sélection créerait un système difficile à lire, calibrer et tester.
4. Les valeurs numériques historiques servent d'exemples de discussion, pas de paramètres de production.

## 4. personnalité

### Définition

La personnalité représente les dispositions relativement stables qui modulent la manière dont un agent interprète les situations et valorise les options. Elle ne doit pas déterminer mécaniquement une action, ni empêcher l'évolution du comportement après une expérience marquante.

### Proposition de dimensions continues à étudier

| Dimension | Effet possible sur la stratégie | Archétype émergent, non classé |
|---|---|---|
| Prudence / tolérance au risque | Évalue plus ou moins sévèrement un danger. | « Peureux » si sécurité faible et prudence forte. |
| Empathie | Valorise davantage l'état et le besoin d'autrui. | « Altruiste » si coût de partage perçu faible. |
| Conservation de soi | Priorise fortement les ressources et la survie propres. | « Égoïste » ou prudent selon le contexte. |
| Sociabilité | Recherche ou évite la coordination et le dialogue. | Isolé, communicatif, médiateur. |
| Honnêteté | Rend la tromperie ou la rétention d'information plus ou moins admissible. | Franc, manipulateur, réservé. |
| Curiosité | Valorise exploration, nouveauté et knowledge. | Explorateur, enquêteur. |
| Ambition | Valorise objectifs personnels, statut ou sortie rapide. | Compétitif, meneur. |
| Loyauté | Maintient une promesse ou un groupe malgré un coût. | Fidèle, opportuniste. |

Ces dimensions sont toutes 🔵 PROPOSITION. Le premier slice ne doit en retenir qu'un nombre minimal, par exemple prudence, empathie et conservation de soi, si elles produisent des choix clairement différents.

## 5. état psychologique

### Définition

L'état psychologique représente une condition interne qui évolue plus lentement qu'une émotion et qui module la capacité d'un agent à soutenir un effort, à prendre un risque ou à coopérer. Il peut résulter d'états physiques, d'événements, de relations et de réussite ou d'échec.

### Variables candidates issues de l'audit

| Variable | Sens haut | Sens bas | Effet de jeu à étudier | Statut |
|---|---|---|---|---|
| Sécurité ressentie | À l'abri, capable de faire face. | Menacé, vulnérable. | Évaluation du risque, besoin de refuge. | 🔵 |
| Maîtrise perçue | Capable d'agir et de comprendre. | Impuissant, confus. | Persistance, choix de plans complexes. | 🔵 |
| Motivation / espoir | Anticipe une possibilité de réussite. | Résignation, démobilisation. | Poursuite d'un but, acceptation d'effort. | 🔵 |
| Soutien social ressenti | Aidé, connecté à un groupe. | Isolé, abandonné. | Propension à coopérer, vulnérabilité. | 🔵 |
| Stress durable | Vigilant, sous tension. | Calme, détendu. | Urgence, seuil de peur ou d'irritation. | 🔵 |

Une variable ne doit être ajoutée que si elle modifie une décision ou un feedback. « Espoir » ne vaut pas comme jauge séparée si aucun plan ou comportement ne répond à sa variation.

## 6. Emotions

### Définition

Une émotion est une réaction contextuelle, relativement brève, à un stimulus évalué par l'agent. Elle peut produire un pic de priorité, modifier un coût perçu, changer une expression ou influencer une action immédiate. Elle n'est ni un trait permanent, ni une relation à une personne.

### Vocabulaire historique à étudier

Le PDF S01 recense notamment joie, confiance/acceptation, peur, surprise, tristesse, dégoût, colère et anticipation, avec des variations d'intensité inspirées de Plutchik. Cette liste est 🟠 HISTORIQUE. Elle n'oblige pas EXIT à implémenter une roue complète.

### Proposition de prototype émotionnel

| Émotion ou signal | Déclencheur lisible | Effet possible sur jeu | Statut |
|---|---|---|---|
| Peur | Danger, faible sécurité, surprise menaçante. | Fuite, demande d'aide, évitement d'une route. | 🔵 |
| Soulagement | Danger retiré, soin, aide reçue. | Retour progressif à un plan suspendu. | 🔵 |
| Colère / frustration | Trahison, humiliation, obstacle perçu comme injuste. | Refus, confrontation, changement de trust. | 🔵 |
| Gratitude | Aide coûteuse reçue. | Ouverture à coopération et partage. | 🔵, possiblement relation plutôt qu'émotion. |
| Curiosité / intérêt | Nouveauté, indice, opportunité. | observation et exploration. | 🔵 |

Le premier vertical slice peut se contenter de deux signaux lisibles, par exemple peur et soulagement, plus une modification relationnelle après une aide. L'enjeu est de prouver l'effet comportemental avant d'étendre le vocabulaire.

## 7. Needs

### Définition

Un besoin est une pression qui invite l'agent à chercher une réponse. Il résulte d'un état physique, psychologique, social ou informationnel. Il ne fixe pas la méthode : le planner choisit un but, une stratégie et un plan pour y répondre.

| besoin | Sources possibles | Goals possibles | Statut |
|---|---|---|---|
| Se soigner | Santé basse, blessure, douleur. | Atteindre santé suffisante ; obtenir soin. | 🟡 |
| Se nourrir | Faim élevée. | Obtenir et consommer une ration. | 🟡 |
| Se mettre en sécurité | Danger, peur, sécurité ressentie basse. | Quitter une zone, trouver abri, demander escorte. | 🔵 |
| Obtenir de l'information | Blocage, incertitude, curiosité. | Observer, demander, lire, vérifier. | 🔵 |
| Coopérer | Objectif inaccessible seul, soutien recherché. | Trouver allié, proposer échange, honorer promesse. | 🔵 |
| Préserver une relation | Dette, loyauté, trust ou soutien social. | Secourir, livrer, rassurer, expliquer. | 🔵 |

### Besoins et Maslow

Maslow est conservé comme outil de réflexion historique, non comme système de priorité obligatoire. Dans EXIT, une blessure critique peut dominer la planification, mais un agent peut aussi accepter un risque physique pour sauver un allié, échapper à un danger ou préserver une valeur. Les priorités doivent être contextuelles et modulées, non imposées par une pyramide rigide.

## 8. Chaîne événement -> interprétation -> décision

```text
ÉVÉNEMENT DU MONDE
    « le joueur donne sa dernière ration à Mara »
        |
        v
ÉVALUATION PAR MARA
    besoin satisfait, rareté perçue, coût pour le joueur,
    relation préalable, personnalité et situation
        |
        v
ÉTAT ET RÉACTION
    gratitude ou soulagement ; sécurité et trust possiblement modifiés
        |
        v
GOALS / STRATÉGIE
    accepter l'échange, partager le code, suivre le joueur,
    ou seulement réduire une peur immédiate
```

Ce modèle évite le raccourci « donner une ration = +10 trust ». Un même événement peut avoir une valeur différente selon son coût, l'urgence du besoin, la personnalité du receveur et le contexte social.

## 9. Interaction avec la planification

| Couche | Comment elle affecte le GOAP ou la stratégie |
|---|---|
| personnalité | Modifie coût et admissibilité de certaines voies. |
| état psychologique | Modifie persistance, seuil de risque, urgence et capacité à coopérer. |
| émotion | Peut interrompre ou repondérer un plan localement. |
| besoin | Crée ou augmente la priorité d'un but. |
| relation | Change la probabilité d'accepter une action sociale, un échange ou une promesse. |

Les couches ne doivent pas directement « choisir » une action. Elles modulent l'évaluation ; les préconditions et effets des Tasks restent la source de vérité opérationnelle.

## 10. retour d'information joueur

| État interne | retour d'information compréhensible | À éviter |
|---|---|---|
| Peur | Voix, posture, fuite, refus contextualisé, demande de protection. | Une jauge cachée qui rend un dialogue impossible sans explication. |
| Sécurité accrue | NPC reste près du groupe, accepte une zone plus risquée. | Changement soudain de caractère sans événement. |
| Gratitude / relation améliorée | Mention d'un acte précis, offre, promesse tenue. | Une augmentation de score sans causalité. |
| Désespoir / baisse motivation | Hésitation, besoin de réassurance, abandon d'une route dangereuse. | Immobilisation définitive sans alternative. |
| Curiosité | NPC observe, pose des questions, explore prudemment. | Déplacement omniscient vers une réponse cachée. |

## 11. Critères du premier vertical slice

| Élément à prouver | Critère observable |
|---|---|
| besoin physique | Une blessure ou la faim fait monter un but de soin ou de nourriture. |
| Évaluation contextuelle | Une aide au bon moment a plus d'effet qu'un don sans besoin. |
| État durable minimal | Après aide ou danger, un NPC modifie temporairement son évaluation du risque. |
| Émotion lisible | Peur ou soulagement produit une réaction visible et une conséquence de plan. |
| Relation | Un acte concret affecte une future demande ou un échange. |
| Personnalité minimale | Deux NPC évaluent une même route de danger différemment. |

## 12. Risques et garde-fous

| Risque | Symptôme | Garde-fou |
|---|---|---|
| Simulation encyclopédique | Beaucoup de jauges sans conséquence ludique. | Ajouter une variable seulement pour un comportement et un feedback identifiés. |
| Trait figé | Un NPC « peureux » ne peut jamais changer. | Séparer personnalité stable et état modulable. |
| Émotion décorative | Animation ou dialogue sans effet de décision. | Lier chaque émotion retenue à un coût, but, plan ou relation. |
| Calcul opaque | Le joueur ne comprend pas un refus ou un changement. | Montrer la cause observée et la réaction, pas la formule. |
| Moralisation du joueur | Aider devient toujours optimal ou toujours requis. | Offrir des coûts, alternatives et conséquences nuancées. |
| Théorie surdimensionnée | Maslow ou Plutchik dictent un système non adapté au jeu. | Les utiliser comme références historiques, tester un noyau propre à EXIT. |

## 13. Paramètres à calibrer

- dimensions de personnalité retenues ;
- variables d'état psychologique réellement utiles ;
- émotions présentes dans le prototype ;
- seuils et durée de decay ;
- événements qui déclenchent une évaluation ;
- formule ou règles qualitatives d'évaluation d'un acte social ;
- effet de chaque couche sur le coût et les Goals ;
- niveau de feedback UI et in-world ;
- persistance entre niveaux ;
- politiques de récupération après démotivation, panique ou isolement.

## 14. Décisions de validation demandées

1. Valider les cinq couches et leur séparation de vocabulaire.
2. Sélectionner le noyau du premier slice : 2 à 3 dimensions de personnalité, 1 à 2 états psychologiques, 2 émotions visibles et 2 besoins physiques maximum.
3. Décider si gratitude relève d'une émotion, d'une relation ou des deux à des temporalités différentes.
4. Confirmer que Maslow et Plutchik restent des références historiques, non des prescriptions de système.
5. Décider quand introduire mensonge, violence, dépression, traumatisme ou autres extensions lourdes.

## 15. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Modèle EmotionState avec stress, confiance, espoir et sécurité. | S01 | 🟠 |
| Historique | Tables de facteurs, événements, émotions Plutchik et pondérations. | S01 | 🟠 |
| Actuel | Mandat demande de préserver l'ancien modèle avant toute refonte silencieuse. | S08 | 🟡 |
| Actuel | Séparation personnalité / état psychologique / émotion / besoin / relation retenue comme modèle conceptuel de travail. | S06, S08, Audit Phase 1 | 🟡 |
| Futur | Valeurs numériques et modèle de calcul attendent un vertical slice et des tests d'observation. | Audit Phase 1 | 🔵 |

## Propagation DR-02

**🟢 CANON lexical :** personnalité = dispositions relativement stables ; état psychologique = condition interne évolutive plus persistante ; émotion = réponse affective située/contextuelle. Aucun de ces concepts ne décide seul d'une action. Dimensions, jauges et formules restent hors périmètre.

**🟢 CANON :** besoin = pression interne évaluée par l'IA qui peut modifier les priorités sans imposer une action ; but = état désiré qu'un agent cherche à atteindre ou maintenir.

## Propagation DR-05

**🟢 CANON :** le profil psycho-émotionnel comprend notamment état central, stress, réactions temporaires, personnalité, besoins, mémoire/connaissance et contexte social. L'état central initial comporte sécurité ressentie et confiance en soi ; le stress est réactif avec inertie. Les émotions sont temporaires et issues d'une appréciation, jamais d'un événement appliquant directement une jauge ou une action. Les anciennes jauges restent 🟠 historiques.

La réassurance est une famille d'influences, pas une jauge. L'espoir n'est pas retenu comme jauge persistante. `ÉTAT ≠ DÉCISION` est maintenu ; DR-06 définira l'usage décisionnel.

## Propagation DR-06

**🟢 CANON :** le stress influe sur la temporalité du traitement, sans supprimer les capacités fondamentales, les options connues ni rendre le NPC artificiellement irrationnel. Il reste distinct de la friction décisionnelle, qui exprime la difficulté d'arbitrer entre plusieurs options subjectivement viables.
