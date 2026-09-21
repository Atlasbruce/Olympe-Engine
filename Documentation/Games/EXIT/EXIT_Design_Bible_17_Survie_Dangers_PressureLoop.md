# EXIT Design Bible
## 17 - Survie, dangers et pressure loop

**Version :** 0.1  
**Statut :** 🟡 WIP - périmètre de prototype à valider  
**Dépendances :** 01 Vision, 13 Graphes, 16 Solvabilité, 18 NPC Agents, 20 Besoins, 21 Relations  
**Dernière mise à jour :** 17 septembre 2026

---

## 1. Intention

La survie et les dangers donnent un coût au temps, aux ressources et à la coopération. Ils empêchent le joueur de résoudre un niveau comme un puzzle abstrait hors du monde. Une ration peut être mangée, donnée ou échangée. Une batterie peut alimenter une porte, un terminal ou une solution d'urgence. Un medikit peut sauver le joueur ou rendre un NPC coopératif.

La pression doit créer des décisions, jamais interrompre mécaniquement la pensée. Chaque danger important doit être perceptible, lisible, évitable ou négociable à un coût. Une difficulté correcte augmente les tensions entre options ; elle ne masque pas les solutions.

## 2. Pressure loop

```text
EXPLORATION ET RÉSOLUTION
    -> consommation de temps et exposition
    -> besoins, dangers ou ressources deviennent pressants
    -> priorités et coûts des plans changent
    -> le joueur/NPC arbitre : survivre, aider, accélérer, contourner
    -> le monde et les relations évoluent
    -> nouvelle exploration et réévaluation
```

| Source de pression | Exemple | Décision attendue |
|---|---|---|
| État physique | Santé faible, faim. | Chercher un soin, manger, demander une ressource ou prendre un risque. |
| Danger environnemental | Gaz, eau montante, lasers, secteur instable. | Attendre, désactiver, contourner, traverser vite ou coopérer. |
| Temps | Compte à rebours, alarme, événement évolutif. | Choisir une route plus rapide ou plus sûre. |
| Ressource rare | Batterie, medikit, ration, outil. | Conserver, investir, échanger ou partager. |
| Autres agents | NPC concurrent, blessé, inquiet ou opportuniste. | Négocier, aider, éviter, observer ou modifier le plan. |

## 3. Règles de design

1. Une variable de survie doit modifier une décision, une capacité ou une relation ; sinon elle n'a pas de raison d'exister.
2. Le joueur reçoit un signal avant que la pression ne devienne critique.
3. Toute ressource critique consommable entre dans le contrôle de solvabilité.
4. Les effets de seuil sont graduels et compréhensibles, sauf événement terminal explicitement annoncé.
5. Une pression peut être déplacée, transformée ou partagée ; elle ne doit pas toujours être seulement subie.
6. Les NPC utilisent les mêmes familles de pression que le joueur, sous réserve des asymétries explicitement documentées.
7. Les dangers doivent servir le graphe de dépendances, le rythme ou la fiction ; ils ne sont pas ajoutés pour remplir le niveau.

## 4. État physique minimal

### Santé

La santé représente la capacité physique à continuer d'agir. Elle est explicitement présente dans les sources historiques et constitue le noyau minimal recommandé.

| Niveau | Conséquence de jeu à étudier | retour d'information |
|---|---|---|
| Stable | Actions normalement disponibles. | HUD discret, animation normale. |
| Blessé | Certains coûts ou risques augmentent. | Posture, vitesse, dialogue, état visible. |
| Critique | Besoin de soin prioritaire ; certaines routes deviennent trop risquées. | Signal clair, feedback médical, appel d'aide possible. |
| Incapacité ou mort | Transition à définir : échec, secours, perte temporaire, conséquence de campagne. | Jamais un état ambigu ou silencieux. |

### Faim

La faim est explicitement souhaitée dans S01. Elle peut rendre une ration utile au joueur, au NPC ou à l'échange.

| Niveau | Conséquence de jeu à étudier | retour d'information |
|---|---|---|
| Satisfait | Pas de contrainte. | HUD léger. |
| Faim | besoin de nourriture augmente, attention et plans de ravitaillement. | Commentaire, animation, journal. |
| Faim forte | Coût de certaines Actions, priorisation de survie. | Signal soutenu mais non intrusif. |
| Épuisement | À décider seulement si cela crée une transition juste. | Avertissements graduels. |

### Variables reportées

Soif, fatigue, sommeil, température, stress physique et munitions sont présents comme propositions ou exemples historiques. Ils sont 🔵 PROPOSITION. Le premier slice ne doit pas les ajouter sans rôle distinct de la santé et de la faim.

## 5. Ressources à emplois concurrents

| Ressource | Emploi personnel | Emploi systémique ou social | Dilemme |
|---|---|---|---|
| Medikit | Restaurer sa santé. | Soigner un NPC qui détient un code ou devient allié. | Sécurité personnelle contre accès ou coopération. |
| Ration | Réduire sa faim. | Échanger ou aider un NPC affamé. | Besoin futur contre trust ou information. |
| Batterie | Alimenter un équipement portatif. | Activer porte, terminal ou alimentation d'urgence. | Information, accès ou sécurité. |
| Outil | Réparer un mécanisme. | Prêter ou échanger contre une autre capacité. | Gain immédiat contre autonomie. |
| Temps | Explorer ou se reposer. | Secourir, attendre, contourner un danger. | Connaissance, relation ou route rapide. |

Une bonne ressource crée au moins deux usages significatifs. Elle doit aussi être lisible : le joueur peut comprendre ce qu'il sacrifie ou rend possible avant une consommation majeure.

## 6. Dangers et hazards

### Taxonomie

| Famille | Exemples historiques | Rôle de design potentiel | Statut |
|---|---|---|---|
| Environnemental | Eau montante, gaz, poison, pluie acide. | Route temporairement coûteuse, urgence, modification spatiale. | 🟠 / 🔵 |
| Mécanique | Compression de murs, trappes, pics, flèches. | Lecture de timing, verrou temporaire, danger local. | 🟠 / 🔵 |
| Énergétique | Lasers, barrières électriques, circuits. | Dépendance énergie, observation des états, détournement. | 🟡 |
| Surveillance | Caméras, capteurs, alarmes, haut-parleurs. | Pression d'information, alarme, changement de comportement NPC. | 🟡 |
| Entité | Sentinelles meurtrières. | Menace mobile, fuite ou désactivation. | 🟠 / 🔴 périmètre à trancher |
| Social | Vol, menace, trahison, conflit. | Coût relationnel et redirection de plan. | 🟡 / 🔴 périmètre à trancher |

### Règles de hazard

- Le danger est signalé par la scène, le son, une observation ou un événement préalable.
- Le joueur peut identifier au moins une réponse : éviter, désactiver, protéger, contourner, attendre ou accepter le coût.
- Un hazard peut modifier la topologie ou le coût, mais doit respecter la continuité des voies critiques.
- Les pièges aveugles et les décès sans possibilité de lecture ne correspondent pas à la promesse de puzzle systémique.
- La dangerosité n'est pas nécessairement mortelle : ralentir, alarmer, blesser, séparer, révéler ou consommer une ressource peut suffire.

## 7. Temps et événements évolutifs

Le temps reste une dimension à valider. Il peut être global, local, déclenché par une action ou lié à une zone. Le modèle temporel du jeu doit être décidé avant de calibrer les comptes à rebours.

| Forme | Exemple | Avantage | Risque |
|---|---|---|---|
| Pression locale | Une salle se remplit d'eau après activation d'une valve. | Lisible et contenue. | Peut être résolue comme routine si répétée. |
| Pression de quête | Alarme qui augmente le coût de certaines routes. | Lie temps et graphe. | Doit préserver une voie de continuité. |
| Pression globale | Purge du complexe après un délai. | Tension forte et rythme de run. | Peut nuire à l'observation et la planification. |
| Événement réactif | Une alarme suit le forçage d'un coffre. | Conséquence claire d'un choix. | Ne doit pas devenir une punition dominante. |

## 8. Survie et coopération

La survie crée des besoins visibles qui peuvent devenir des demandes sociales : « As-tu de la nourriture ? », « Peux-tu me soigner ? », « Peux-tu m'escorter ? ». Le joueur reste libre de répondre, mais son choix change la valeur des relations et les routes disponibles.

| Situation | Choix possible | Conséquence attendue |
|---|---|---|
| NPC blessé avec information utile | Donner medikit, chercher une autre voie, négocier, laisser l'agent. | Rapport, accès au knowledge, coût personnel ou social. |
| Joueur affamé avec une ration | Manger, échanger, donner, conserver. | Survie contre information, trust ou dette. |
| Zone dangereuse | Traverser seul, demander escorte, désactiver, contourner. | Coût de temps, santé, relation ou ressource. |
| NPC inquiet | Rassurer, informer, ignorer, exploiter sa peur. | Coopération, refus, déplacement ou évolution relationnelle. |

Le choix « humain » ne doit pas toujours optimiser la sortie, et la solution la plus rapide ne doit pas toujours être la meilleure pour survivre. Cette tension constitue une promesse WIP forte d'EXIT.

## 9. Interactions avec les systèmes

| Système | Contribution de la pression | Effet en retour |
|---|---|---|
| Needs et Goals | Les états physiques créent une urgence ou reclassent des Goals. | Le plan peut réduire l'exposition et restaurer une capacité. |
| Graphes de dépendances | Les ressources de survie deviennent des nœuds à usages concurrents. | Les alternatives donnent des réponses au danger. |
| Gestionnaire de quêtes | Place dangers, ressources et contraintes avec validation de coût. | Surveille les changements qui pourraient produire un softlock. |
| NPC Agents | Évalue le risque et poursuit sa survie selon son knowledge local. | Déplace ressources et modifie le contexte social. |
| Relations | L'aide satisfait un besoin et peut changer une relation. | La relation affecte la probabilité d'aide future. |
| UI/UX | Rend seuils et risques compréhensibles. | Le joueur peut planifier plutôt que subir. |

## 10. Critères du premier vertical slice

| Élément à prouver | Critère observable |
|---|---|
| Santé | Blessure du joueur ou NPC qui justifie une recherche de soin. |
| Faim ou ressource sociale | Une ration a un usage personnel et un usage relationnel. |
| Hazard lisible | Un danger local possède un signal et au moins deux réponses. |
| Coût de branche | Une route rapide est plus risquée ou plus chère qu'une route sûre. |
| Continuité | La consommation d'une ressource critique ne bloque pas le niveau. |
| retour d'information | Le joueur sait pourquoi une capacité, un NPC ou une route change. |

## 11. Risques et garde-fous

| Risque | Symptôme | Garde-fou |
|---|---|---|
| Gestion de jauges sans choix | Le joueur consomme automatiquement dès qu'une barre baisse. | Introduire des usages concurrents et des seuils non triviaux. |
| Punition de l'exploration | Explorer suffisamment fait perdre par famine ou chrono. | Cadence généreuse, ressources, zones sûres et informations utiles. |
| Danger arbitraire | Piège impossible à prévoir ou éviter. | Télégraphie, observation, réponse alternative. |
| Surcharge de systèmes | Santé, faim, soif, fatigue et stress se recouvrent. | Prototype avec santé + une seule pression complémentaire. |
| Coût anti-social obligatoire | Aider est toujours optimal, ou toujours suicidaire. | Équilibrer les alternatives et contextualiser les besoins. |
| Softlock de consommation | La meilleure décision locale ferme la sortie. | Validation de Section 16 et avertissement de coût. |

## 12. Paramètres à calibrer

- variables de survie retenues par acte et niveau ;
- vitesse de consommation et seuils ;
- durée des dangers ;
- coût de déplacement, repos, observation et réparation ;
- rareté des medikits, rations, batteries et outils ;
- visibilité des risques et prévisions UI ;
- fréquence des événements temporels ;
- dégâts, incapacité, mort et récupération ;
- difficulté selon la progression ;
- règles de partage et de stockage des ressources.

## 13. Décisions de validation demandées

1. Confirmer que santé et faim constituent le noyau de survie du premier slice.
2. Décider si un danger local non mortel est préférable à une pression globale de temps au prototype.
3. Sélectionner un premier hazard : énergie, zone contaminée, alarme ou autre.
4. Définir le comportement d'incapacité et de mort pour joueur et NPC.
5. Confirmer que chaque ressource critique doit avoir au moins deux usages significatifs et une vérification anti-softlock.

## 14. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Santé, faim, nourriture, medikits et ressources de survie. | S01 | 🟠 |
| Historique | Catalogue de dangers : eau, gaz, pièges, sentinelles, surveillance et événements cruels. | S01 | 🟠 |
| Actuel | Pressure loop par survie, temps, dangers, ressources et agents. | S06, S07, S08 | 🟡 |
| Actuel | Survie comme choix et monnaie sociale, pas collection de jauges. | Audit Phase 1 | 🟡 |
| Futur | Modèle temporel, combat, mortalité et catalogue de hazards attendent une décision de périmètre. | registre des sujets à traiter DB-04, DB-08, DB-11 | 🔴 |

## Propagation DR-07

**🟢 CANON :** temporisateur de quête, de session/sortie et de danger sont distincts. Un temporisateur de danger déclenche les conséquences dans le Monde ; il ne produit pas automatiquement une fin de partie. Le Gestionnaire de partie réévalue ensuite selon les règles/mode de jeu.
