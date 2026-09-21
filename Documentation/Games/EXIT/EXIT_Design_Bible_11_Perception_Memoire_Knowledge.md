# EXIT Design Bible
## 11 - perception, observation, mémoire et knowledge

**Version :** 0.1  
**Statut :** 🟡 WIP - en attente de validation  
**Dépendances :** 01 Vision, glossaire initial, NPC agents, dialogue, GOAP, Gestionnaire de quêtes  
**Dernière mise à jour :** 16 septembre 2026

---

## 1. Définition et intention

EXIT repose sur une distinction fondamentale : le monde possède un état réel, mais aucun agent ne le connaît entièrement. Le joueur et les NPC ne prennent donc pas leurs décisions à partir du `État du monde` complet ; ils agissent selon ce qu'ils ont perçu, retenu, interprété, reçu et vérifié.

Le système existe pour créer de la découverte, de la coopération, de la méfiance et des solutions émergentes. Il doit rendre possible le fait qu'un NPC connaisse un code ignoré du joueur, qu'une information soit partagée contre une ressource, qu'une rumeur soit erronée, ou qu'une observation permette d'éviter une mauvaise décision. Il ne doit pas transformer l'enquête en exercice de gestion de données incompréhensible.

## 2. Expérience joueur

Le joueur doit pouvoir vivre les situations suivantes :

- « J'ai vu un générateur hors service, mais je ne sais pas encore comment le réparer. »
- « Mara affirme connaître le code. Je peux lui faire confiance, la vérifier ou chercher une autre voie. »
- « Le câble relie ce générateur à l'infirmerie : ce détail change mon plan. »
- « J'avais noté qu'il y avait un medikit ici, mais quelqu'un l'a déplacé depuis. »
- « Je n'ai pas besoin de connaître toute la carte pour prendre une bonne décision locale. »

La complexité de simulation doit rester principalement invisible. Le journal, la carte et les dialogues expliquent les conséquences importantes, sans afficher une base de données de croyances.

## 3. Modèle conceptuel

```text
WORLD STATE réel
       |
       v
PERCEPTION locale et imparfaite
       |
       v
OBSERVATION ou interprétation d'un signal
       |
       v
MEMORY événementielle et spatiale
       |
       +-------------------+
       |                   |
       v                   v
KNOWLEDGE              RELATION / réputation de source
       |
       v
ANALYSE, stratégie et planification
       |
       v
ACTION sur le monde
       |
       +--------> réévaluation
```

### Règle de portée

Chaque couche apporte une forme de valeur différente.

| Couche | question à laquelle elle répond | Exemple |
|---|---|---|
| État du monde | Que se passe-t-il réellement ? | Le code réel du coffre est `3812`. |
| perception | Quel signal a atteint l'agent ? | Le joueur voit un papier avec quatre chiffres partiellement effacés. |
| observation | Que l'agent pense avoir constaté ? | « Le papier semble indiquer 3812. » |
| mémoire | Qu'a vécu ou constaté l'agent, et dans quel contexte ? | « J'ai trouvé ce papier dans l'atelier. » |
| connaissance | Quelle proposition l'agent peut-il utiliser pour décider ? | « Le code du coffre est probablement 3812. » |
| Analyse | Quelles conséquences et options découlent de ce savoir ? | « Je peux essayer le coffre ou chercher une confirmation. » |

## 4. Règles de design

1. **Un agent ne reçoit pas une information parce que le joueur ou le Gestionnaire de quêtes la connaît.** Il doit la percevoir, l'observer, la déduire ou la recevoir.
2. **Une information importante doit posséder au moins une source jouable.** Cette source peut être un objet, un lieu, un événement, un dialogue, une machine ou un comportement observable.
3. **Une information peut être utile sans être certaine.** L'incertitude doit être lisible et ne pas servir de piège arbitraire.
4. **La connaissance d'un agent peut devenir obsolète.** Si le monde change, le système doit préserver la source, la date ou un feedback qui explique l'écart.
5. **Le jeu ne demande pas au joueur de mémoriser une valeur essentielle sans support.** Les faits importants peuvent être consignés dans le journal de connaissances.
6. **Le partage d'information est une interaction à part entière.** Donner, cacher, déformer, refuser ou vérifier une information peut produire des effets sociaux et stratégiques.
7. **La vérité n'est pas toujours visible, mais les règles de vérification doivent exister.** Une rumeur peut être confrontée à une source, un terminal, une observation ou un résultat d'action.

## 5. État du monde et état connu

### État du monde

Le `État du monde` contient les faits autoritatifs du niveau : objets et propriétaires, portes et préconditions, états de machines, zones accessibles, dangers actifs, positions, santé et inventaires. Le Gestionnaire de quêtes l'utilise pour créer et vérifier le problème. Les agents n'y ont pas accès directement.

### État connu par un agent

Chaque agent conserve une représentation partielle, datée et révisable du monde. Elle n'est pas nécessairement fausse lorsqu'elle diffère du réel : elle peut être incomplète ou obsolète.

| Situation | État du monde | connaissance de l'agent | Effet de jeu |
|---|---|---|---|
| Objet déplacé | Le medikit est porté par Ivo. | « Un medikit était dans l'infirmerie il y a dix minutes. » | Recherche possible, mais non garantie. |
| Code reçu | Le code est 3812. | « Mara dit que le code est 3812. » | L'agent peut tenter le code ou évaluer Mara. |
| Danger masqué | Le couloir est contaminé. | « J'entends une alarme et vois un voyant rouge. » | L'agent doit interpréter, observer ou contourner. |
| Porte réparée | La porte est désormais alimentée. | « Le générateur est réparé. » | Cette croyance ouvre une nouvelle branche de plan. |

## 6. perception et observation

### perception

La perception est la réception d'un signal : une silhouette, un son, une lumière, une odeur, un texte, une trace, une parole ou un changement de machine. Elle peut être passive et limitée par la distance, l'orientation, l'occlusion, l'éclairage, le bruit ou l'état physique.

### observation

L'observation est une action ou une attention dirigée qui transforme un signal en fait interprété. Elle peut demander du temps, de la proximité, un outil, une capacité ou un risque.

| Signal | perception possible | observation possible | Exemple d'effet |
|---|---|---|---|
| Porte | Voir son voyant rouge. | Examiner son panneau et lire « alimentation absente ». | Crée le knowledge d'une précondition. |
| NPC | Voir une blessure ou entendre une plainte. | Parler ou examiner la blessure. | Révèle un besoin, une information ou un état de relation. |
| Machine | Entendre un ronronnement irrégulier. | Suivre les câbles ou ouvrir le panneau. | Révèle une panne ou une connexion. |
| Objet | Apercevoir une forme dans une vitrine. | S'approcher, lire l'étiquette, prendre l'objet. | Révèle type, état, propriété et usage possible. |
| Environnement | Remarquer une grille ou un courant d'air. | Fouiller, écouter ou mesurer. | Fait émerger une route, un danger ou un indice. |

### Paramètres à calibrer

- portée visuelle et auditive ;
- occlusion, transparence de murs et caméra isométrique ;
- durée et coût d'une observation active ;
- niveau de précision délivré par défaut ;
- différences de perception entre NPC ;
- sources d'erreur voulues et lisibles.

## 7. mémoire

La mémoire est une trace locale d'un événement ou d'une expérience. Elle répond à « qu'est-il arrivé, quand, où et avec qui ? ». Elle sert à alimenter la connaissance, les relations et la réévaluation des plans.

### Types de mémoire de travail

| Type | Contenu | Exemple | Usage |
|---|---|---|---|
| Épisodique | Événement contextualisé. | « Bob m'a agressé près du générateur. » | Relation, évitement, dialogue, interprétation. |
| Spatiale | Lieu ou chemin connu. | « L'infirmerie est derrière la porte bleue. » | Exploration et planification de trajet. |
| Objet | État ou dernière position d'une ressource. | « Le badge était dans le casier A. » | Recherche, partage, vérification. |
| Sociale | Promesse, échange, aide, grief. | « Alice m'a donné sa ration. » | confiance, dette, coopération. |
| Procédurale | Méthode ou règle apprise. | « Le terminal exige une batterie chargée. » | Résolution de puzzle et choix d'action. |

### WIP : oubli et obsolescence

Le système n'a pas besoin de simuler l'oubli humain complet au début. Le premier slice peut conserver les souvenirs importants mais marquer les observations de monde mutable comme « dernière information connue ». L'oubli, la confusion, la mémoire fausse et le traumatisme sont des extensions à étudier seulement s'ils créent des choix intéressants et lisibles.

## 8. connaissance comme donnée de première classe

Un élément de `connaissance` est une proposition sur le monde, pas un simple texte dans un inventaire.

### Fiche conceptuelle minimale

| Champ | Rôle | Exemple |
|---|---|---|
| Proposition | Ce qui est tenu pour vrai. | « Le coffre médical exige le code 3812. » |
| Sujet | Entité ou relation concernée. | Coffre médical. |
| Valeur | Valeur, état ou condition. | Code = 3812. |
| Source | Origine du savoir. | Mara, note, terminal, observation. |
| Date ou fraîcheur | Moment de l'acquisition ou de la dernière vérification. | Obtenu il y a deux minutes. |
| Certitude | Confiance dans la proposition. | Confirmé, probable, rumeur, démenti. |
| Portée | Qui connaît l'information. | Joueur seul, Mara seule, groupe, public. |
| Vérification | État de contrôle. | Non vérifié, confirmé, invalidé. |

### Exemples

| Proposition | Source | Certitude initiale | Vérification possible |
|---|---|---|---|
| « La clé est dans le coffre. » | Ivo l'a vue. | Probable | Ouvrir le coffre ou regarder par une fente. |
| « Mara connaît le code. » | Mara elle-même. | Possible | La soigner puis tester sa réponse. |
| « Le générateur alimente l'infirmerie. » | Câbles observés. | Forte | Activer le générateur et observer les voyants. |
| « La sortie secondaire est ouverte. » | Rumeur d'un NPC. | Faible | Explorer, consulter une caméra ou trouver un plan. |

## 9. Partage, rétention et contradiction

### Actions informationnelles à étudier

| action | Intention | Exemple | Statut |
|---|---|---|---|
| Informer | Transmettre une proposition connue. | Donner un code à un allié. | 🟡 |
| Demander | Obtenir une connaissance d'un autre agent. | Demander l'emplacement d'un badge. | 🟡 |
| Échanger | Lier information et ressource ou promesse. | Une carte contre une ration. | 🟡 |
| Vérifier | Confronter une proposition à une source. | Tester un code ou consulter le terminal. | 🟡 |
| Retenir | Ne pas transmettre une information pertinente. | Garder un raccourci secret. | 🔵 |
| Déformer ou mentir | Transmettre une proposition que l'agent sait ou soupçonne fausse. | Donner un mauvais code pour détourner quelqu'un. | 🔵 |
| Signaler | Rendre un fait visible à plusieurs agents. | Marquer une zone dangereuse. | 🔵 |

### Règles de lisibilité sociale

- Une information reçue conserve sa source dans le journal.
- Une contradiction n'établit pas automatiquement un mensonge : elle peut signaler une erreur, une information datée ou une intention hostile.
- Le joueur doit disposer d'au moins une manière concrète de vérifier un élément qui conditionne fortement sa progression.
- Le mensonge ne sera introduit que si les NPC peuvent en avoir une raison lisible et si le joueur possède des outils de réponse.

## 10. Interactions avec les autres systèmes

| Système | Ce que connaissance lui apporte | Ce qu'il renvoie |
|---|---|---|
| Exploration | Cibles de recherche, zones connues, hypothèses. | Nouvelles perceptions et observations. |
| Inventaire | Informations sur possessions, usages et emplacements. | Objets qui servent de source ou de support de knowledge. |
| Gestionnaire de quêtes | Conditions et sources à distribuer dans le niveau. | Problème réel et chemins potentiels de vérification. |
| GOAP | État connu pour choisir une action et estimer ses préconditions. | Résultats qui confirment, infirment ou enrichissent le knowledge. |
| Needs et goals | Moyens de reconnaître une opportunité ou une urgence. | Priorité accordée à une information. |
| Relations | Historique de source, promesse et aide reçue. | Évaluation de confiance et volonté de partager. |
| UI/UX | Journal, carte, provenance et marqueurs de certitude. | retour d'information compréhensible sans omniscience. |

## 11. Risques et edge cases

| Risque | Cas | Garde-fou de design |
|---|---|---|
| Savoir devenu faux | Un NPC a déplacé le dernier medikit. | Stocker une dernière localisation et une date ; l'agent réévalue en cas d'échec. |
| Source indisponible | Le détenteur d'un code est blessé, enfermé ou mort. | Prévoir une alternative, un relais, une trace ou une méthode de récupération. |
| Rumeur punitive | Le joueur suit une fausse piste sans signal ni possibilité de la vérifier. | Marquer la confiance, fournir des indices contradictoires ou une sortie de route. |
| Spam d'informations | Le journal se remplit de détails sans valeur. | Conserver uniquement les propositions exploitables ou les compacter par sujet. |
| Omniscience accidentelle | Un NPC planifie vers un objet qu'il n'a jamais perçu. | Le planner n'utilise que l'état connu de l'agent. |
| Charge cognitive | Trop de sources, de dates et de degrés de certitude. | UI qualitative par défaut : confirmé, rapporté, ancien, contradictoire. |

## 12. Paramètres à calibrer

- granularité des connaissances : fait, relation, emplacement, état, intention ;
- classes de certitude affichées ;
- conditions qui font vieillir ou invalider une information ;
- règles de mémorisation automatique et de prise de note manuelle ;
- coût temporel et risque de l'observation ;
- portée du partage : bilatéral, groupe, signal dans le monde ;
- visibilité des sources et des contradictions ;
- premier périmètre du mensonge, de l'erreur et de l'oubli.

## 13. Décisions de validation demandées

1. Valider la séparation `État du monde -> perception -> observation -> mémoire -> connaissance`.
2. Valider connaissance comme donnée de première classe, avec au minimum source, fraîcheur et état de vérification.
3. Confirmer que le journal joueur doit distinguer un fait observé, rapporté et confirmé.
4. Décider si le premier vertical slice inclut seulement des informations exactes à source variable, ou déjà une information contradictoire / erronée.
5. Décider si les NPC conservent toutes les mémoires utiles au prototype, sans oubli simulé.

## 14. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Inventaire physique et inventaire de connaissances. | S01 | 🟠 |
| Historique | « Pile » mêlant découvertes, besoins, objectifs et intentions. | S01 | 🟠 |
| Audit | Séparation nécessaire entre knowledge, memory, state, need, goal, plan et action. | S08, Audit Phase 1 | 🟡 |
| Actuel | connaissance est proposé comme ressource traçable et exploitable. | S06, S08 | 🟡 |
| Futur | Rumeurs, mensonge, oubli et contradiction attendent un prototype et une décision de périmètre. | Audit Phase 1 | 🔵 |

## Propagation DR-02

**🟢 CANON :** perception = réception locale d'un signal ; observation = acquisition attentive, dirigée ou située d'une information exploitable. Ni l'une ni l'autre ne révèle automatiquement la vérité globale.

**🟢 CANON :** information = contenu perceptible, acquis ou transmissible ; mémoire = trace contextualisée d'expérience ; connaissance = représentation locale, mémorisée, exploitable et révisable concernant le Monde ; inférence = processus de production ou révision d'une connaissance. La croyance n'est pas une structure indépendante ; mémoire et connaissance sont distinctes et interactives.

**🟡 WIP :** distinguer fiabilité subjective de l'acquisition et validité temporelle actuelle, sans arrêter de variable ni de formule.

## Propagation DR-04

**🟢 CANON :** la mémoire est une banque sélective, structurée et réactualisable de connaissances mémorisées ; elle n'est ni un snapshot du Monde, ni un journal exhaustif, ni une étape obligatoire vers la connaissance. Perception/observation ne produisent une connaissance mémorisée qu'après identification, qualification, évaluation et décision de pertinence par les systèmes IA.

La consolidation est `CREATE / IGNORE / VERSION` sur une connaissance `CURRENT` correspondante. Une répétition identique n'écrit rien, ne réécrit pas le timestamp et ne renforce pas automatiquement la fiabilité. Un changement sémantique crée une nouvelle version `CURRENT` et rend l'ancienne `SUPERSEDED`.

## Propagation DR-06

**🟢 CANON :** la mémoire doit proposer des associations structurées — NPC, lieu, objet, événement, interaction, besoin, but, situation, relation et importance émotionnelle — permettant un rappel contextuel sans balayage exhaustif. Ce rappel alimente une appréciation ; il ne décide pas l'action.

## Propagation DR-08

**🟢 CANON :** entendre, mémoriser, croire et répondre sont distincts. Une déclaration est une information sourcée concernant ce qu'un agent affirme ; elle ne modifie pas la vérité du Monde. Déclarer posséder une information ne divulgue pas son contenu. Répéter une même affirmation par la même source ne renforce pas automatiquement sa fiabilité.
