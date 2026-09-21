# EXIT Design Bible
## 16 - Solvabilité et anti-softlock

**Version :** 0.1  
**Statut :** 🟡 WIP - en attente de validation  
**Dépendances :** 13 Graphes AND/OR, 14 quête objectif tâche action, 15 Gestionnaire de quêtes, 18 NPC Agents  
**Dernière mise à jour :** 16 septembre 2026

---

## 1. Intention

EXIT cherche à faire vivre des conséquences, des choix coûteux et des agents autonomes. Cette ambition ne justifie pas un état où le joueur ne peut plus progresser sans le comprendre ou sans possibilité de réaction. La solvabilité protège la promesse du jeu : le niveau peut devenir difficile, changer de forme ou exiger une stratégie nouvelle, mais il conserve une voie intelligible vers une résolution ou vers un échec terminal explicitement annoncé.

La solvabilité concerne à la fois la génération initiale et l'évolution du niveau pendant la partie. Elle relève du Gestionnaire de quêtes, avec l'aide de règles de possession, de consommation, de feedback et de continuité.

## 2. Définitions

| Terme | Définition |
|---|---|
| Solvabilité initiale | Au lancement, au moins un chemin valide relie l'état initial à la condition de réussite. |
| Solvabilité dynamique | Après les actions des agents et les changements du monde, au moins un chemin valide reste disponible ou une transition explicite redéfinit l'issue. |
| Softlock | État non terminal où la progression requise est impossible, sans que le jeu ne l'annonce comme une défaite ou une transition. |
| Échec récupérable | Échec local qui ferme une branche mais préserve ou révèle une autre voie. |
| Échec terminal | État de fin intentionnel, annoncé et cohérent avec les règles du jeu. |
| Nœud critique | Condition, ressource, information, agent ou transition indispensable à toute voie actuellement valide. |
| Continuité | Ensemble des mécanismes qui empêchent ou réparent un softlock de manière visible et fictionnellement cohérente. |

## 3. Principe de travail

> **Tout élément indispensable doit disposer d'une redondance, d'une méthode alternative de récupération, ou d'une transition de quête explicitement conçue.**

Ce principe est WIP. Il ne signifie pas que chaque objet doit être dupliqué ni que toute erreur doit être annulée. Il signifie que le système doit connaître les éléments critiques et prévoir ce qui arrive lorsqu'ils deviennent indisponibles.

## 4. Types de softlock à prévenir

| Type | Exemple | Protection attendue |
|---|---|---|
| Ressource consommée | La seule batterie sert à un terminal alors qu'elle était nécessaire à la porte. | Alternative OR, batterie rechargeable, autre source d'énergie, avertissement de coût. |
| Objet détruit ou perdu | La clé tombe dans une zone inaccessible ou est détruite. | Objet récupérable, voie de contournement, reproduction cohérente ou clé non destructible. |
| NPC indisponible | Mara meurt, fuit, devient hostile ou est enfermée alors qu'elle connaît l'unique code. | Autre source du code, trace, relais, voie de piratage ou forçage. |
| Information perdue | Le joueur oublie une valeur critique ou une note disparaît. | Journal persistant, source relisible, autre indice, connaissance sauvegardée. |
| Accès fermé | Un événement ferme toutes les routes vers l'atelier. | Itinéraire secondaire, moyen de rouvrir, changement de quête annoncé. |
| État irréversible opaque | Le joueur active une purge qui neutralise une ressource sans comprendre la conséquence. | Prévisualisation de risque, confirmation contextualisée, solution de récupération. |
| Simulation émergente | Un NPC prend, consomme ou cache une ressource nécessaire. | Droits de réservation, mémoire de dernière position, alternatives et replanification. |

## 5. Invariants de solvabilité

Ces invariants sont des règles à implémenter conceptuellement avant tout algorithme.

1. **Chemin initial :** une route complète existe depuis l'état initial vers la réussite.
2. **Connaissabilité :** chaque verrou critique possède au moins une piste accessible permettant de comprendre sa précondition ou une branche.
3. **Ressource critique protégée :** une ressource unique nécessaire à toutes les routes ne peut pas être définitivement perdue sans continuité prévue.
4. **Agent critique protégé :** un NPC qui porte une information ou capacité indispensable possède une relève, une trace ou une alternative.
5. **Dynamique surveillée :** tout changement affectant un nœud critique déclenche une vérification de voies restantes.
6. **Échec lisible :** lorsqu'une branche se ferme, le joueur peut en identifier la cause et découvrir ou poursuivre une autre branche.
7. **Fin explicite :** si aucune continuité n'est voulue, la partie doit devenir un échec terminal déclaré, non un monde silencieusement insoluble.

## 6. Classification des éléments par criticité

| Classe | Définition | Traitement |
|---|---|---|
| Décoratif | N'affecte aucune voie de progression. | Libre à la simulation. |
| Utile | Facilite une voie mais n'est pas nécessaire. | Peut disparaître avec feedback léger. |
| Important | Ouvre une branche notable ou réduit un coût majeur. | Suivre dernière position et conséquences. |
| Critique de branche | Indispensable à une voie, mais une autre voie existe. | Empêcher la disparition opaque, mettre à jour le graphe. |
| Critique global | Indispensable à toutes les voies valides. | Réserver, dupliquer logiquement ou fournir une récupération/transition. |

La classification n'est pas figée par type d'objet. Une batterie peut être simplement utile dans un niveau, mais critique globale dans un autre, selon le graphe instancié.

## 7. Validation initiale du graphe

### Vérifications nécessaires

| Vérification | question |
|---|---|
| Reachability | Un chemin relie-t-il l'état initial à l'état final ? |
| Preconditions | Chaque action de chaque voie dispose-t-elle de préconditions atteignables ? |
| Resource flow | Une ressource consommée est-elle disponible au bon moment et à un seul endroit logique ? |
| connaissance flow | Les agents peuvent-ils découvrir la nécessité et la localisation des éléments importants ? |
| Agent availability | Les détenteurs de ressources et connaissances sont-ils accessibles ou récupérables ? |
| Spatial reachability | Les rooms, routes et dangers permettent-ils physiquement chaque voie validée ? |
| Alternative independence | Deux branches partagent-elles en réalité un même nœud critique ? |
| Cost feasibility | Les coûts de temps, santé et survie restent-ils supportables avec les ressources du niveau ? |

### Forme de résultat attendue

La validation doit produire davantage qu'un booléen « solvable ». Elle doit pouvoir fournir :

- les chemins validés ;
- les nœuds critiques globaux et de branche ;
- les préconditions non satisfaites ;
- les alternatives dépendantes d'une même ressource ;
- les sources de knowledge absentes ;
- les hypothèses retenues sur l'état et la disponibilité des NPC.

Cette trace permet au designer de corriger une situation plutôt que d'accepter une génération opaque.

## 8. Solvabilité dynamique

La validation initiale ne suffit pas : le niveau change parce que le joueur et les NPC agissent. À chaque événement susceptible de toucher une branche critique, le Gestionnaire de quêtes évalue le graphe restant.

```text
Événement du monde
    -> élément critique touché ?
    -> branches affectées identifiées
    -> chemin valide restant ?
         oui : mettre à jour états et feedback
         non : activer une continuité prévue ou déclencher une transition explicitement conçue
```

### Événements de surveillance

| Événement | Exemple | action de supervision |
|---|---|---|
| Changement de possession | Ivo prend le fusible. | Suivre l'objet, mettre à jour les agents et vérifier son accessibilité. |
| Consommation | Batterie installée dans le terminal. | Marquer la branche porte affectée et tester les alternatives. |
| Destruction | Incendie détruit une note. | Vérifier la redondance de knowledge. |
| Incapacité NPC | Mara est blessée ou enfermée. | Vérifier les voies dépendantes de son information. |
| Modification spatiale | Couloir inondé, porte scellée. | Recalculer les routes vers les nœuds critiques. |
| Changement relationnel | Un NPC refuse désormais d'aider. | Fermer la voie sociale correspondante, sans confondre refus et disparition de toute solution. |

## 9. Mécanismes de continuité

| Mécanisme | Description | Quand l'utiliser | Coût ou risque |
|---|---|---|---|
| Branche OR préexistante | Une solution différente est déjà dans le graphe. | Solution privilégiée. | Coût de route normal. |
| Redondance | Deux sources indépendantes d'une même ressource ou information. | Connaissances et ressources fragiles. | Peut réduire la tension si excessive. |
| Récupération | L'objet est récupérable après perte apparente. | Objets physiques déplaçables. | Temps, danger ou détour. |
| Relais | Un autre NPC, document ou terminal transmet l'information. | Connaissances détenues par NPC. | Peut demander exploration ou échange. |
| Conversion | Une ressource différente peut accomplir le même rôle. | Énergie, accès, outils polyvalents. | Coût supérieur ou conséquence. |
| Transition de quête | Le problème évolue vers une finalité cohérente. | Événement narratif majeur conçu à l'avance. | Nouveau risque, temps ou perte explicite. |
| Échec terminal | La partie se conclut de façon annoncée. | Seulement si le contrat de jeu l'autorise. | Fin de run, jamais silence. |

La continuité n'est pas une gratuité. Une solution de récupération peut coûter une alarme, une dette, une zone dangereuse ou un objectif secondaire, tant que son existence est compréhensible.

## 10. Propriété, réservation et autonomie NPC

Un NPC autonome ne doit pas être gelé dès qu'il approche d'un objet important. Toutefois, son autonomie doit rester compatible avec la continuité du niveau.

| Règle WIP | Effet recherché |
|---|---|
| Réserver logiquement les éléments critiques globaux. | Empêcher une consommation ou destruction irréversible par un agent non informé. |
| Autoriser possession et déplacement, conserver dernière position et porteur. | L'objet reste une source de jeu et de négociation. |
| Autoriser consommation seulement si une voie de continuité existe. | Préserver les conséquences sans softlock. |
| Laisser les NPC planifier à partir de leur knowledge local. | Ne pas leur révéler qu'un objet est critique pour la quête globale. |
| Produire un feedback quand un NPC agit sur une ressource importante. | Permettre au joueur d'enquêter, poursuivre ou négocier. |

La « réservation » est une contrainte du monde et de la simulation, pas une connaissance magique donnée au NPC. Un NPC peut vouloir prendre une batterie ; le système détermine simplement si son emploi définitif fermerait toutes les routes et impose alors une règle de continuité cohérente.

## 11. Exemple : Mara et la clé

### Mauvaise configuration

```text
EXIT -> clé -> coffre -> code -> Mara
Mara est blessée ; le seul medikit est consommable ; Mara peut mourir.
```

Si le joueur consomme le medikit, ou si Mara meurt, aucune route ne reste. C'est un softlock si la partie continue sans statut terminal.

### Configuration robuste

```text
EXIT -> clé -> coffre
coffre -> code OR forçage OR terminal
code -> Mara soignée OR journal de maintenance
Mara soignée -> medikit OR bandage + temps
```

Mara garde son intérêt social : elle offre une voie lisible, humaine et potentiellement économique. Le journal, le terminal ou le forçage préservent l'autonomie du joueur si sa situation devient impossible. Le medikit conserve un choix car le bandage et le temps ont un coût différent.

## 12. UI et feedback de continuité

Le système ne doit pas exposer ses graphes internes, mais ses effets doivent être visibles.

| Situation | retour d'information recommandé |
|---|---|
| Objet important déplacé | Journal « dernière localisation connue » ou information obtenue par observation. |
| Branche fermée | Le monde explique la cause : porte scellée, NPC refuse, outil cassé. |
| Alternative découverte | Nouvel indice, dialogue, route visible ou propriété d'objet révélée. |
| Consommation à enjeu | Description des usages concurrents et conséquence probable avant validation. |
| Échec terminal | Signal clair, cause compréhensible, option de recommencer ou de poursuivre selon le modèle de campagne. |

## 13. Tests de conception recommandés

| Test | Scénario | Résultat attendu |
|---|---|---|
| Objet déplacé | Un NPC prend une ressource critique de branche. | L'objet est traçable et une route reste. |
| Objet consommé | Le joueur emploie une batterie pour une option secondaire. | La sortie conserve une voie ou l'impact était explicitement terminal. |
| NPC absent | Le détenteur d'un code devient inaccessible. | Une source ou méthode alternative est identifiée. |
| Information erronée | Une rumeur mène à une impasse. | Le joueur peut vérifier, corriger ou changer de branche. |
| Route fermée | Un danger bloque un couloir. | La topologie offre une route, un moyen de rouvrir ou une transition de quête. |
| Stress du système | Plusieurs NPC poursuivent simultanément leurs objectifs. | Aucun comportement ne supprime silencieusement toutes les voies. |

## 14. Paramètres à calibrer

- définition exacte de « critique global » ;
- degré de redondance selon difficulté ;
- quelles actions peuvent détruire, consommer ou rendre inaccessible un objet ;
- conditions de réservation et de libération d'une ressource ;
- fréquence de vérification dynamique ;
- coût acceptable d'une solution de récupération ;
- tolérance aux échecs terminaux ;
- politiques de mort, incapacité et fuite des NPC ;
- traitement de l'information contradictoire ou devenue obsolète.

## 15. Décisions de validation demandées

1. Valider le principe de continuité comme règle WIP centrale.
2. Décider quels échecs doivent être récupérables et lesquels peuvent être terminaux.
3. Valider la réservation logique des ressources critiques, sans priver les NPC de leur autonomie apparente.
4. Définir le premier contrat de mort/incapacité des NPC.
5. Définir si le joueur reçoit un avertissement explicite avant toute consommation qui peut fermer une branche majeure.

## 16. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Chaînes de dépendances avec ressources, NPC blessé, informations et objets de soin. | S01 | 🟠 |
| Actuel | Solvabilité initiale et dynamique demandée explicitement pour la génération. | S08 | 🟡 |
| Actuel | Redondance ou récupération des éléments indispensables proposée comme règle. | S08 | 🟡 |
| Actuel | V0 anticipait ces principes ; l'audit les maintient WIP en attente de validation. | S07, Audit Phase 1 | 🟡 |
| Futur | Méthodes de recherche de chemin, algorithmes et tests automatisés relèvent de la spécification fonctionnelle et de l'implémentation. | Mandat S08 | 🔵 |

## Propagation DR-07

**🟢 CANON :** branche invalidée ≠ quête insatisfaisable ≠ fin de partie ; insatisfaite ≠ insatisfaisable ; existe ≠ accessible ≠ utilisable ; solvabilité du Monde ≠ connaissance des agents. Le Gestionnaire de partie détecte les impossibilités locales démontrables depuis graphe, conditions et événements irréversibles. `Impossibilité non prouvée ≠ solvabilité prouvée`.

Le futur évaluateur global/récursif de solvabilité reste 🟡 WIP. Si toutes les branches pertinentes sont prouvées impossibles, l'assistance de récupération est optionnelle ; sans récupération valide, fin de partie + débrief.
