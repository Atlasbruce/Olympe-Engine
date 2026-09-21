# EXIT Design Bible
## 15 - Gestionnaire de quêtes et génération procédurale de problèmes

**Version :** 0.1  
**Statut :** 🟡 WIP - en attente de validation  
**Dépendances :** 11 connaissance, 13 Graphes de dépendances, 14 quête objectif tâche action, 16 Solvabilité, 19 GOAP  
**Dernière mise à jour :** 16 septembre 2026

---

## 1. Définition

Le Gestionnaire de quêtes construit, instancie et supervise le problème du niveau. Il définit la condition de réussite, les dépendances, les ressources, les contraintes, les alternatives et les garanties de continuité. Il connaît la structure complète du niveau parce qu'il est responsable de sa solvabilité.

Il ne choisit pas la solution à la place des agents. Le joueur et les NPC ne reçoivent que des perceptions, des connaissances, des ressources, des besoins et des capacités locales. Ils doivent découvrir et résoudre le problème par leurs propres stratégies et plans.

> **Le Gestionnaire de quêtes construit le problème. Les agents cherchent comment le résoudre.**

## 2. Intention de design

Le Gestionnaire de quêtes existe pour créer des niveaux variés qui restent cohérents, lisibles et résolubles malgré les actions autonomes des NPC. Il doit permettre de réassembler des ingrédients simples en situations différentes : une information détenue, un outil enfermé, une machine hors service, une personne blessée, une alternative risquée, un raccourci caché.

La génération n'est pas un objectif en soi. Un niveau généré doit produire une situation intéressante, avec un rythme, une découverte, un choix et une continuité, plutôt qu'un graphe fonctionnel mais abstrait.

## 3. Périmètre et frontières

| Gestionnaire de quêtes | Ne relève pas du Gestionnaire de quêtes |
|---|---|
| Choisir ou recevoir une finalité de niveau. | Décider quelle branche un agent préfère. |
| Composer le graphe de dépendances. | Donner à un NPC une connaissance non perçue. |
| Placer les ressources, les sources et les obstacles. | Définir une émotion individuelle à chaque instant. |
| Distribuer les connaissances initiales et les indices. | Forcer une coopération ou un dialogue. |
| Vérifier la solvabilité initiale et dynamique. | Réécrire invisiblement le monde pour sauver un mauvais plan. |
| Observer les transitions du monde et déclencher des réponses prévues. | Exécuter le GOAP, le pathfinding ou les Actions d'un agent. |
| Gérer le statut de quête et les conséquences de niveau. | Rendre un NPC omniscient ou téléporter une solution sans fiction. |

## 4. Entrées de conception

Le Gestionnaire de quêtes ne doit pas générer à partir d'une liste brute d'objets. Il reçoit une intention de niveau et des contraintes de contenu.

| Entrée | Description | Exemple |
|---|---|---|
| Finalité | État final à atteindre ou à éviter. | `EXIT_A.open == true`. |
| Thème de situation | Tension principale recherchée. | « Ressource médicale contre information ». |
| Grammaire de puzzle | Types de verrous autorisés. | Accès, énergie, information, social. |
| Budget de complexité | Profondeur, branches, rooms, NPC, objets et nouveautés. | 2 verrous principaux, 2 alternatives, 2 NPC. |
| Contraintes spatiales | Topologie ou modules disponibles. | Hub, infirmerie, atelier, corridor dangereux. |
| Contraintes narratives | Éléments de fiction, révélations ou exclusions. | Caméras actives, note sur un précédent sujet. |
| Contraintes de sécurité | Règles de solvabilité et d'anti-softlock. | Aucun objet critique détruit sans alternative. |
| État de campagne | Relations, connaissances persistantes, règles déjà apprises. | Le joueur sait réparer un générateur. |

## 5. Sorties attendues

| Sortie | Contenu |
|---|---|
| Graphe de quête instancié | État final, Objectives, branches AND/OR, coûts et dépendances. |
| État initial de monde | Objets, machines, accès, dangers, zones et états actifs. |
| Distribution de knowledge | Sources, rumeurs, indices, détenteurs et informations observables. |
| Population d'agents | NPC, besoins initiaux, capacités, objectifs personnels et relations de départ. |
| Carte et ancrage spatial | Placement des nœuds logiques dans la topologie du niveau. |
| Preuves de validation | Au moins un chemin de réussite, alternatives critiques, risques de softlock détectés. |
| plan de continuité | Réponses prévues aux ressources perdues, NPC indisponibles et changements majeurs. |

## 6. Pipeline conceptuel

```text
1. Choisir une finalité et une intention de situation
2. Composer un graphe logique de base
3. Ajouter les dépendances AND et les alternatives OR
4. Allouer objets, informations, machines et agents
5. Ancrer les nœuds dans une topologie spatiale
6. Ajouter indices, feedback et coûts de branche
7. Valider la solvabilité initiale
8. Simuler les perturbations pertinentes
9. Instancier le niveau
10. Superviser les transitions et la continuité pendant la partie
```

Chaque étape produit un état inspectable par le designer. Cette traçabilité est indispensable : une génération opaque rend les bugs de solvabilité impossibles à comprendre.

## 7. Étape 1 - Finalité et situation

La finalité est l'état final du niveau ; la situation est le type de problème humain et systémique qui doit être vécu pour l'atteindre.

| Finalité | Situation possible | Expérience visée |
|---|---|---|
| Ouvrir EXIT | Un survivant possède une information mais a besoin d'aide. | Coopération négociée. |
| Évacuer un secteur | L'alimentation d'urgence sert aussi à sauver un NPC. | Dilemme de ressource. |
| Atteindre un terminal | Plusieurs routes, dont une zone contaminée plus rapide. | Risque contre temps. |
| Désactiver une alarme | La seule personne compétente se méfie du joueur. | Confiance et information. |

Une situation ne doit pas être réduite à un thème visuel. Elle définit un conflit de choix que le graphe rend jouable.

## 8. Étape 2 - Composer le graphe logique

Le Gestionnaire de quêtes part d'une finalité puis décompose les conditions nécessaires. Il ajoute ensuite des alternatives pour les nœuds critiques et des coûts qui différencient les routes.

```text
OBJECTIVE : EXIT ouverte
    AND
    |- alimentation disponible
    |    OR
    |    |- générateur réparé
    |    |- batterie d'urgence installée
    |
    |- accès autorisé
         OR
         |- clé obtenue
         |- verrou piraté
         |- passage maintenance accessible
```

### Règles de composition

1. Commencer par la finalité, pas par les objets disponibles.
2. Ajouter une dépendance seulement si elle introduit une décision, une découverte ou une tension identifiable.
3. Ajouter une alternative OR aux nœuds dont la disparition pourrait bloquer le niveau.
4. Relier chaque condition à une ou plusieurs sources de knowledge.
5. Prévoir le coût ou le risque de chaque branche dès sa création.
6. Respecter le budget de complexité du niveau ; éviter d'empiler les verrous de même nature.

## 9. Étape 3 - Allouer les ingrédients

L'allocation donne une existence physique, informationnelle et narrative aux nœuds du graphe.

| Nœud logique | Allocation possible | Justification fictionnelle |
|---|---|---|
| Fusible F12 | Atelier, NPC technicien, tiroir de maintenance. | Pièce stockée près des équipements. |
| Code de coffre | Note, terminal, mémoire de Mara, observation d'un panneau. | Procédure, souvenir ou trace d'un protocole. |
| Medikit | Infirmerie, trousse d'urgence, inventaire d'un NPC. | Ressource médicale localisée de manière plausible. |
| Passage maintenance | Grille, plan du complexe, ventilation, bruit d'air. | Infrastructure réelle, pas raccourci arbitraire. |

### Règle d'orthogonalité

Les propriétés doivent se combiner. Un fusible est une ressource d'énergie ; il peut alimenter une porte, une infirmerie ou un terminal selon compatibilité. Une batterie peut servir de source d'énergie, de monnaie d'échange ou de coût de branche. La variété vient des relations entre propriétés, non d'une prolifération d'objets uniques.

## 10. Étape 4 - Distribution de knowledge et indices

Un graphe solvable mais inconnu ne produit pas nécessairement une expérience solvable pour le joueur. Le Gestionnaire de quêtes distribue donc des chemins de découverte.

| Fait à découvrir | Source principale | Source secondaire ou indice | Risque à éviter |
|---|---|---|---|
| La porte manque de courant | Voyant et panneau de la porte. | Câble visible, message de terminal. | Précondition invisible. |
| Le fusible requis est F12 | Étiquette du générateur. | Manuel ou NPC compétent. | Objets incompatibles indifférenciables. |
| Mara connaît le code | Dialogue avec Mara. | Note mentionnant son rôle, comportement protecteur. | NPC critique sans piste. |
| Passage de maintenance existe | Grille et courant d'air. | plan ou rumeur. | Alternative non découvrable. |

Les sources sont des nœuds de gameplay. Elles doivent être placées avec le même soin que les ressources physiques.

## 11. Étape 5 - Ancrage spatial et rythme

L'ancrage spatial crée la séquence d'exploration, les coûts de trajet, les rencontres et les révélations. Il ne doit pas transformer le graphe en succession de pièces obligatoires.

| Règle | Effet recherché |
|---|---|
| Placer la finalité ou son signe dans un repère initial. | Donner une direction sans révéler la solution. |
| Séparer condition et moyen par une exploration significative. | Créer une raison de cartographier et d'observer. |
| Placer les alternatives dans des zones de coût différent. | Donner un sens au choix de route. |
| Prévoir des hubs, raccourcis et retours. | Réduire la répétition et rendre les connexions lisibles. |
| Faire correspondre la fonction d'une salle à son langage visuel. | Permettre une déduction avant texte. |

## 12. Étape 6 - Validation

La validation est une responsabilité du Gestionnaire de quêtes, décrite en détail dans la Section 16. À ce stade, le système doit au minimum vérifier :

- qu'un chemin existe entre l'état initial et l'état final ;
- que les préconditions de chaque action de branche sont atteignables ;
- que les ressources critiques restent accessibles ;
- que la connaissance de la solution peut être obtenue ;
- que les NPC ne peuvent pas consommer ou déplacer silencieusement l'unique moyen de réussite ;
- que les alternatives annoncées sont réellement indépendantes à un niveau suffisant.

## 13. Supervision dynamique

Après instanciation, le Gestionnaire de quêtes observe les changements pertinents du monde. Il ne « triche » pas pour garantir la réussite ; il active des mécanismes de continuité prévus par le graphe et rend ces transitions fictionnellement visibles.

| Événement | Diagnostic | Réponse acceptable | Réponse à éviter |
|---|---|---|---|
| NPC critique blessé ou absent | La voie sociale devient incertaine. | Activer une trace, un relais ou une branche alternative prévue. | Téléporter le NPC ou donner le code sans cause. |
| Ressource critique consommée | Une branche perd sa précondition. | Signaler la consommation et préserver une voie OR. | Déclarer la quête résolue ou impossible silencieusement. |
| Dangers modifient une zone | Topologie et coût changent. | Mettre à jour les feedbacks, ouvrir une route cohérente ou maintenir un chemin. | Fermer toutes les routes vers une condition. |
| plan du joueur échoue | Échec local ou knowledge incorrect. | Expliquer l'état, permettre réévaluation et vérification. | Réinitialiser le monde sans conséquence. |

## 14. Génération : authored, procédural et hybride

| Approche | Avantage | Limite | Usage recommandé |
|---|---|---|---|
| Authored | Rythme, narration et lisibilité très contrôlés. | Rejouabilité limitée. | Niveau de référence et tutoriels. |
| Assemblage de gabarits | Variété contrôlable, plus simple à vérifier. | Peut devenir répétitif. | Premier système procédural. |
| Graphe procédural | Grande combinatoire des conditions et alternatives. | Exige validation solide, risque de graphes sans intérêt. | Après gabarits stabilisés. |
| Simulation forte | Histoires émergentes avec NPC autonomes. | Débogage et continuité complexes. | Extension progressive, pas prérequis du vertical slice. |

La recommandation de travail est un modèle hybride : intentions et gabarits authored, instanciation de ressources, branches et agents sous contraintes, puis validation explicite.

## 15. Budget de complexité de départ

Pour un premier niveau de référence, proposition à valider :

| Élément | Budget proposé | Raison |
|---|---|---|
| Finalité | 1 | Garder l'issue lisible. |
| Verrous principaux | 2 | Créer une structure AND sans surcharge. |
| Sous-objectifs | 2 à 4 | Donner de la profondeur contrôlée. |
| Alternatives OR | 1 à 2 | Prouver la liberté sans explosion combinatoire. |
| NPC actifs | 2 | Tester coopération et concurrence sans simulation de foule. |
| Objets significatifs | 6 à 10 | Permettre polyvalence et lisibilité. |
| Informations critiques | 3 à 5 | Construire une enquête simple et vérifiable. |
| Nouveaux systèmes introduits | 1 par niveau | Maintenir l'apprentissage. |

## 16. Paramètres à calibrer

- catégories de gabarits de quête ;
- profondeur et largeur de graphes ;
- probabilité et nature des alternatives ;
- règles d'allocation spatiale ;
- règles de possession initiale par NPC ;
- degré de redondance de knowledge ;
- coûts des branches ;
- fréquence de reconfiguration dynamique ;
- limites de génération procédurale par acte ou difficulté ;
- outils de visualisation et traces pour déboguer un graphe généré.

## 17. Risques et garde-fous

| Risque | Symptôme | Garde-fou |
|---|---|---|
| Génération fonctionnelle mais terne | Les graphes marchent sans créer de découverte ni dilemme. | Générer depuis une intention de situation et tester les moments de jeu. |
| Réparation invisible | Le monde s'ajuste sans que le joueur comprenne pourquoi. | Toute continuité doit avoir une cause, un signal et un coût lisibles. |
| Gestionnaire de quêtes omnipotent | Il guide indirectement les NPC vers le bon plan. | Séparer strictement état global et knowledge agent-side. |
| Dépendance de source | Toutes les alternatives supposent la même information ou ressource. | Vérifier l'indépendance effective des branches critiques. |
| Explosion combinatoire | Test et débogage deviennent impraticables. | Budgets de profondeur, gabarits et contraintes de variété. |
| Narration incohérente | Un fusible ou un code apparaît sans raison. | Exiger une justification fictionnelle et spatiale à chaque allocation. |

## 18. Décisions de validation demandées

1. Valider la frontière Gestionnaire de quêtes/Agent présentée dans cette section.
2. Valider une progression hybride : gabarits authored avant génération de graphes libres.
3. Valider le budget de complexité du premier niveau de référence, ou le corriger.
4. Décider si le Gestionnaire de quêtes peut déclencher des réponses de continuité pendant une partie, et sous quelles règles de lisibilité.
5. Définir les premiers gabarits de situations à produire : information contre soin, énergie à arbitrer, accès alternatif, évacuation sous danger, ou autre.

## 19. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Énigmes en cascade : clé, coffre, générateur, code, NPC blessé, medikit. | S01 | 🟠 |
| Historique | Décomposition de `Find Exit` dans le prototype GOAP. | S02 | 🟠 |
| Actuel | Gestionnaire de quêtes différencié de l'agent : construction du problème contre recherche de solution. | S08 | 🟡 |
| Actuel | Génération AND/OR, placement, knowledge et solvabilité comme responsabilités du Gestionnaire de quêtes. | S06, S08 | 🟡 |
| Futur | Algorithmes, structures de données et intégration Olympe Engine restent hors de cette spécification. | Mandat S08 | 🔵 |

## Propagation DR-02

**🟢 CANON :** le Gestionnaire de quêtes construit le problème sous forme de graphe de quêtes, objectifs, conditions, relations et dépendances. Il ne prescrit pas aux NPC leurs buts, stratégies, plans ou actions de résolution. Son rapport au Gestionnaire de partie, au Monde et à la satisfaisabilité des branches sera traité en DR-03.

## Propagation DR-03

**🟢 CANON :** le graphe est immuable par défaut après génération. Le Gestionnaire de quêtes exprime des exigences abstraites, sans connaître les NPC concrets. Le Gestionnaire de partie lie les conditions aux instances du Monde et évalue graphe + état autoritatif ; une branche irréalisable ne constitue pas automatiquement une défaite.
