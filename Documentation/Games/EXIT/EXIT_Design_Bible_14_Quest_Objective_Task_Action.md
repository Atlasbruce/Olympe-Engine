# EXIT Design Bible
## 14 - quête, objectif, tâche et action

**Version :** 0.1  
**Statut :** 🟡 WIP - en attente de validation  
**Dépendances :** 11 connaissance, 13 Graphes de dépendances, 15 Gestionnaire de quêtes, 19 GOAP  
**Dernière mise à jour :** 16 septembre 2026

---

## 1. Intention

EXIT doit employer une hiérarchie de termes stable. Sans elle, le même mot finit par désigner une intention de NPC, un état du niveau, une liste de préconditions ou un clic joueur. Cette section propose quatre niveaux : `quête`, `objectif`, `tâche` et `action`.

La hiérarchie ne décrit pas une chaîne nécessairement linéaire. Une quête peut comporter des Objectives alternatifs ; une tâche peut échouer, être révisée ou remplacée ; une action ne garantit jamais son effet si l'état du monde a changé.

## 2. Définitions de travail

| Niveau | Définition | question | Exemple |
|---|---|---|---|
| quête | Problème structuré du niveau, composé d'Objectives et de dépendances. | Quel problème global est posé ? | Obtenir un accès à EXIT. |
| objectif | État testable souhaité, qui contribue à une quête ou à un but d'agent. | Quel résultat faut-il atteindre ? | Le coffre est ouvert. |
| tâche | Opération planifiable, assignable à un acteur, qui vise un effet. | Quelle opération allons-nous tenter ? | Réparer le générateur. |
| action | Capacité atomique effectivement exécutée sur le monde. | Que fait l'acteur maintenant ? | `Use(fusible, générateur)`. |

### Concepts voisins, volontairement séparés

| Terme | Rôle | Exemple |
|---|---|---|
| besoin | Pression ou manque à satisfaire. | Faim élevée, besoin de sécurité. |
| but | État désiré par un agent et priorisé. | Rester en vie, quitter le complexe. |
| stratégie | Approche choisie parmi plusieurs voies. | Négocier avec Mara plutôt que forcer le coffre. |
| plan | Ensemble ordonné ou conditionnel de Tasks pour atteindre un objectif ou but. | Trouver medikit, soigner Mara, demander le code. |

`besoin != but != objectif != tâche != action`. Une faim élevée peut rendre « manger » urgent ; elle ne constitue ni une action, ni le plan complet du NPC.

## 3. Vision globale

```text
QUEST DU NIVEAU
  Ouvrir EXIT
      |
      +-- OBJECTIVE : porte alimentée
      |       |
      |       +-- TASK : réparer le générateur
      |               |
      |               +-- ACTIONS : se déplacer, prendre un fusible, utiliser le fusible, activer
      |
      +-- OBJECTIVE : accès autorisé
              |
              +-- TASK : obtenir le code auprès de Mara
                      |
                      +-- ACTIONS : trouver le medikit, donner le medikit, parler, demander
```

Un agent peut générer un plan sur une partie de cette structure sans connaître la quête entière. Le Gestionnaire de quêtes connaît et valide le problème complet ; l'agent ne choisit que parmi les objectifs et actions qu'il peut raisonnablement inférer depuis ses connaissances.

## 4. quête

### Définition

Une quête est un graphe de problème autoritatif du niveau. Elle exprime un état final, les Objectives qui y contribuent, les branches AND/OR, les ressources, les sources de knowledge, les contraintes et les conséquences. Elle peut être visible au joueur sous une forme très simplifiée ou totalement implicite dans le monde.

### Fiche minimale

| Champ | Description |
|---|---|
| ID | Identifiant stable et versionnable. |
| Intention | Rôle de jeu et rôle narratif. |
| État final | Condition de réussite objectivement testable. |
| Graphe | Objectives, dépendances AND/OR et transitions. |
| Entrées | État initial, ressources, agents, zones, contraintes. |
| Sources d'information | Comment les agents peuvent découvrir le problème. |
| Échecs et transitions | État temporaire, changement de branche, échec terminal annoncé. |
| Continuité | Solutions ou récupérations contre les softlocks. |
| Récompenses et conséquences | Monde, accès, narration, relation, ressource. |

### Exemple

**quête : Ouvrir l'issue du secteur A.**  
État final : `EXIT_A.open == true`.  
Objectives principaux : `EXIT_A.powered == true` AND `EXIT_A.authorized == true`.  
Voies d'autorisation : clé du coffre OR piratage OR passage maintenance.  
Contrainte : la clé, le code et le medikit ne doivent pas devenir simultanément irrécupérables.

## 5. objectif

### Définition

Un objectif décrit un état ou résultat désirable, sans imposer une méthode unique pour l'atteindre. C'est la bonne couche pour représenter les nœuds du graphe de dépendances et la réussite d'une partie de quête.

### Règles

1. Un objectif se formule comme un état testable, pas comme une action verbale imposée.
2. Il peut être satisfait par plusieurs Tasks ou stratégies.
3. Il est indépendant de l'agent qui le poursuit, même si certaines voies demandent une capacité ou une relation.
4. Il décrit son succès, ses préconditions de validité et sa contribution au graphe.

| Mauvaise formulation | Pourquoi | Formulation recommandée |
|---|---|---|
| « Demander le code à Mara » | Impose une méthode et un agent. | « Obtenir un code valide pour le coffre ». |
| « Trouver une clé » | Ne précise pas la clé, ni son usage. | « Posséder une clé qui autorise EXIT ». |
| « Réparer le générateur » | Peut rester une tâche si la méthode est imposée. | « Rétablir l'alimentation de l'infirmerie ». |

### Fiche minimale

| Champ | Description |
|---|---|
| État désiré | Proposition testable, ex. `medicalWing.powered == true`. |
| Importance | Contribution à quête, but, survie ou narration. |
| Dépendances | AND/OR, contraintes et alternatives. |
| Visibilité | Connu du joueur, connu d'un NPC, caché mais déductible. |
| Récompense | Nouvelles options, accès, information ou changement d'état. |
| Échec | Conséquence, replanification ou état terminal annoncé. |

## 6. tâche

### Définition

Une tâche est une opération planifiable de granularité moyenne. Elle associe un acteur, une intention opérationnelle, une cible, des préconditions, un coût et des effets attendus. Le planificateur peut sélectionner une tâche car ses effets participent à un objectif.

### Contrat conceptuel

| Champ | Rôle | Exemple |
|---|---|---|
| Actor | Agent prévu ou catégorie capable. | Ivo, joueur, agent avec compétence technique. |
| Verbe | Opération planifiable. | Réparer. |
| Target | Cible principale. | Générateur A. |
| Preconditions | État et ressources requis. | Fusible F12 possédé, accès au panneau, danger géré. |
| Cost | Ressource, temps, risque, relation, exposition. | 30 secondes, risque d'alarme. |
| Effets attendus | Changements attendus. | `generatorA.repaired = true`. |
| Failure modes | Raisons possibles d'échec. | Fusible incompatible, interruption, état déjà modifié. |
| retour d'information | Signaux avant, pendant et après exécution. | Panneau, son, câble éclairé, journal. |

### Granularité

Une tâche doit être assez compacte pour être planifiée et replanifiée, mais pas si petite qu'elle devienne un simple mouvement d'animation. `Obtenir le medikit` est souvent une tâche ou sous-plan. `Déplacer la main vers la poignée` ne l'est jamais : c'est de l'exécution. `Guérir Mara et obtenir le code` est un plan ou une stratégie, pas une tâche atomique.

## 7. action

### Définition

Une action est la capacité atomique effectivement exécutée dans le monde. Elle reçoit des cibles et des paramètres. Elle peut réussir, échouer, être interrompue ou produire un effet partiel selon le État du monde du moment.

### Catalogue initial à auditer

| Famille | Actions candidates | Statut |
|---|---|---|
| Mouvement | Move, GoTo, Follow, Wait | 🟡 |
| Exploration | Explore, Search, Observe, Read | 🟡 |
| Possession | Take, Drop, Give, Exchange | 🟡 |
| Utilisation | Use, Connect, Activate, Repair, Unlock, Open | 🟡 |
| Social | Talk, Ask, Tell, Promise, Refuse | 🔵 |
| Survie | Eat, Drink, Heal, Rest | 🟡 / 🔵 |
| Conflit | Threaten, Steal, Attack, Restrain | 🔵, périmètre non validé |

Ce tableau est un inventaire de candidature. Il ne définit pas encore une API, une liste de commandes UI ou un scope de prototype.

### Résultat d'action

| État | Signification | Exemple |
|---|---|---|
| Success | Les effets attendus sont appliqués. | La clé est prise. |
| Failed | Les préconditions ou l'exécution empêchent l'effet. | Le coffre requiert un code. |
| Partial | Une partie de l'effet a eu lieu. | Panneau ouvert, mais réparation interrompue. |
| Interrupted | Un événement ou un agent met fin à l'action. | Une alarme force la fuite. |
| Invalid | L'action ne peut plus être tentée dans cet état. | Objet absent ou cible détruite. |

## 8. Règles de passage entre les niveaux

| Passage | Condition | Exemple |
|---|---|---|
| quête -> objectif | L'objectif est un nœud nécessaire ou alternatif du graphe. | EXIT requiert une alimentation. |
| objectif -> tâche | La tâche produit ou aide à produire l'état désiré. | Réparer produit une alimentation. |
| tâche -> action | Les Actions exécutent l'opération avec les paramètres réels. | Prendre le fusible, l'installer, activer. |
| action -> World | Le monde accepte, refuse ou modifie l'effet. | Le générateur démarre ou révèle une panne différente. |
| World -> plan | L'agent met à jour knowledge et réévalue. | La batterie est absente : choisir une autre tâche. |

## 9. Exemple complet

| Niveau | Formulation | Explication |
|---|---|---|
| quête | Ouvrir EXIT du secteur A. | Problème global du niveau. |
| objectif | Obtenir une autorisation de sortie. | État atteignable par clé, piratage ou maintenance. |
| stratégie | Passer par la clé et éviter l'alarme. | Choix d'approche selon le risque et les connaissances. |
| plan | Stabiliser Mara, obtenir le code, ouvrir le coffre. | Ensemble de Tasks, révisable. |
| tâche | Donner un medikit à Mara. | Opération ciblée, avec besoin de possession et proximité. |
| Actions | GoTo Mara ; Give(medikit, Mara) ; Talk(Mara) ; Ask(code). | Exécution concrète et résultats vérifiables. |

## 10. Interactions avec les systèmes

| Système | Rôle dans la hiérarchie |
|---|---|
| Graphes AND/OR | Porte la structure des Quests et Objectives. |
| Gestionnaire de quêtes | Construit, instancie, valide et suit les Quests. |
| connaissance | Détermine ce qu'un agent sait des Objectives, Tasks et préconditions. |
| GOAP | Sélectionne ou compose un plan de Tasks vers un but à partir de l'état connu. |
| NPC | Possède des Goals, exécute des Actions et peut contribuer ou nuire à une quête. |
| Relations | Modulent l'acceptation des Tasks sociales et le coût des Actions de dialogue ou d'échange. |
| UI | Présente les Objectives compris, les interactions possibles et le résultat des Actions. |

## 11. Risques et garde-fous

| Risque | Symptôme | Garde-fou |
|---|---|---|
| Objectif déguisé en solution | Le jeu impose une méthode malgré des alternatives annoncées. | Formuler les Objectives comme états ; séparer stratégies et Tasks. |
| Tâches trop haut niveau | Impossible de replanifier après un changement local. | Décomposer jusqu'à une opération avec préconditions et effets clairs. |
| Actions trop fines | Plans gigantesques et fragiles, difficilement lisibles. | Garder l'animation et le pathfinding hors de la couche de tâche. |
| Quête omnisciente | Le joueur reçoit un plan complet avant d'avoir découvert le problème. | N'afficher que les Objectives connus ou déduits. |
| Confusion agent/Gestionnaire de quêtes | Le système force un NPC vers la « bonne » solution. | Le Gestionnaire de quêtes définit le problème ; le planner agent-side choisit. |
| Échec sans réévaluation | Un plan continue malgré une précondition devenue fausse. | États d'action explicites, mise à jour knowledge et replanification. |

## 12. Paramètres à calibrer

- granularité de tâche acceptable pour le GOAP ;
- nombre d'Actions de base du vertical slice ;
- quelles Actions sont accessibles au joueur, aux NPC, ou aux deux ;
- temps, risque et ressources attachés aux Tasks ;
- visibilité des Objectives à chaque stade de découverte ;
- règles d'interruption et de replanification ;
- traitement des Tasks sociales et promesses ;
- périmètre des Actions de conflit.

## 13. Décisions de validation demandées

1. Valider les définitions et la séparation des quatre niveaux.
2. Valider que quête appartient au problème du niveau et qu'un plan appartient à l'agent.
3. Choisir les Actions du premier vertical slice.
4. Décider si `Explore` est une action autonome, une stratégie de déplacement, ou les deux selon le contexte.
5. Définir si une tâche sociale comme négocier est générique ou reste une composition de `Talk`, `Ask`, `Give` et `Promise`.

## 14. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | `Find Exit` est décomposé en clé, coffre, mot de passe et interactions. | S02 | 🟠 |
| Historique | Verbes proposés : Move, Take, Use, Repair, Talk, Ask, Tell, Exchange, Heal et autres. | S01, S05 | 🟠 |
| Actuel | Distinction quête/objectif/tâche/action demandée par le mandat. | S08 | 🟡 |
| Actuel | Separation but/objectif et stratégie/plan établie comme vocabulaire de travail. | Audit Phase 1 | 🟡 |
| Futur | Contrat exact des Actions et implémentation Olympe Engine restent hors de cette Bible conceptuelle. | S08 | 🔵 |

## Propagation DR-02

**🟢 CANON :** la structure du problème est `Quête → Objectifs → Conditions`. Une condition décrit un état ou une relation à satisfaire ; elle n'adresse aucune instruction à un agent. `Tâche` est 🟠 historique pour l'ancien modèle `Quête → Objectifs → Tâches`.

Côté agent, le vocabulaire canonique est `But → Stratégie → Plan → Actions`. Une stratégie est une approche générale, un plan organise les moyens et actions envisagés, et une action est une opération exécutable qui tente une transition. GOAP et toute responsabilité distincte future de « tâche » restent à examiner.
