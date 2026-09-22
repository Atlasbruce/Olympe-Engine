# EXIT Design Bible
## 31 - Glossaire canonique de travail

**Statut :** 🟢 Définitions DR-02 enregistrées ; extensions signalées WIP  
**Dernière mise à jour :** 17 septembre 2026

| Terme | Définition de travail |
|---|---|
| action | Capacité atomique tentée dans le monde. |
| but | État désiré et priorisé par un agent. |
| connaissance | Croyance exploitable avec source, certitude et fraîcheur. |
| mémoire | Trace locale d'un événement vécu ou observé. |
| besoin | Pression ou manque qui alimente des Goals. |
| objectif | État testable contribuant à une quête ou un but. |
| perception | Signal local reçu depuis le monde ou un agent. |
| plan | Ensemble ordonné ou conditionnel de Tasks vers un but. |
| quête | Graphe de problème du niveau, comprenant des Objectives et dépendances. |
| Gestionnaire de quêtes | Système qui construit et valide le problème du niveau. |
| relation | État directionnel d'un agent envers un autre. |
| stratégie | Approche à haut niveau choisie parmi des compromis. |
| tâche | Opération planifiable avec préconditions, coût et effets attendus. |
| confiance | Anticipation de fiabilité et d'intentions coopératives d'un agent envers un autre. |
| État du monde | État réel, autoritatif et simulé du monde. |

## Ajouts CANON DR-02

| Terme | Définition canonique |
|---|---|
| Monde | Entités, éléments instanciés et systèmes nécessaires à la simulation et à l'interactivité. |
| Tableau noir IA | État privé d'un NPC, autoritatif pour son état interne mais possiblement subjectif à propos du Monde. |
| Information | Contenu susceptible d'être perçu, acquis ou transmis. |
| Inférence | Processus de production ou révision d'une connaissance à partir des données disponibles. |
| Condition | État ou relation à satisfaire pour qu'un objectif ou une autre condition soit satisfait. |
| Tâche | 🟠 Terme historique de l'ancien modèle de quête ; non retenu entre plan et action. |

## Ajouts CANON DR-03

| Terme | Définition canonique |
|---|---|
| Correspondance d'exécution | Liaison maintenue par le Gestionnaire de partie entre conditions abstraites et instances concrètes du Monde. |
| Systèmes IA NPC | Systèmes qui lisent les données Monde/ECS autorisées, les évaluent et mettent à jour le tableau noir privé. |
| Satisfaisabilité | État objectif indiquant qu'une condition ou branche demeure réalisable au regard du graphe et du Monde courant. |

## Ajouts CANON DR-04

| Terme | Définition canonique |
|---|---|
| Mémoire | Banque sélective, structurée et réactualisable de connaissances mémorisées utiles au gameplay. |
| Mode d'acquisition | Manière dont une information est acquise ; distinct de sa source. |
| Source | Origine identifiable et pertinente d'une information ; recommandée, non obligatoire. |
| Connaissance CURRENT | Version actuellement pertinente d'une connaissance pour les systèmes IA. |
| Connaissance SUPERSEDED | Version historique remplacée par un changement sémantique pertinent. |
| Grammaire systémique | Règles communes d'usage des éléments interactifs, accessibles au joueur et aux systèmes IA. |

## Ajouts CANON DR-05

| Terme | Définition canonique |
|---|---|
| Profil psycho-émotionnel | Configuration interne d'un NPC influençant ressenti, interprétation et appréciation subjective, sans décider une action. |
| Sécurité ressentie | Sentiment global, subjectif et intrinsèque de sécurité ; ni danger objectif ni probabilité de survie. |
| Confiance en soi | Sentiment global de pouvoir comprendre, affronter et surmonter les difficultés ; distinct de la capacité réelle. |
| Stress | État réactif avec inertie, modulé par pressions et influences rassurantes. |
| Réaction émotionnelle | Réaction temporaire et contextuelle, potentiellement intense, régulée dans le temps. |
| Empreinte émotionnelle | Trace mémorisée et contextualisée d'une expérience émotionnellement significative, neutre en valence. |
| Réassurance | Famille d'influences qui font percevoir des moyens de comprendre, contrôler ou affronter une situation ; pas une jauge. |
| Profil de régulation | Paramètres conceptuels propres à une réaction : décroissance, maintien, renforcement et apaisement. |
| Appréciation psycho-émotionnelle | Évaluation subjective située qui précède une évolution d'état ou une réaction ; elle ne décide pas directement l'action. |

## Ajouts CANON DR-06

| Terme | Définition canonique |
|---|---|
| Option connue | Possibilité connue ou perçue par le NPC, distincte de toute possibilité objective du Monde. |
| Faisabilité perçue | Estimation subjective de la possibilité d'une option, influencée notamment par capacité, état physique et confiance en soi. |
| Rappel contextuel | Récupération structurée d'éléments pertinents de mémoire pour alimenter l'appréciation d'une option. |
| Association | Facette structurée reliant une mémoire à un NPC, lieu, objet, événement, interaction, besoin, but, situation, relation ou importance émotionnelle. |
| Friction décisionnelle | Difficulté émergente d'arbitrage entre options viables et dimensions importantes ; distincte du stress. |
| Valeur morale | Principe stable ou lentement évolutif contribuant à l'alignement d'une option, distinct de personnalité et profil psycho-émotionnel. |

## Ajouts CANON DR-07

| Terme | Définition canonique |
|---|---|
| Tentative | Exécution d'un niveau comprise entre son démarrage et sa terminaison ou sa relance. |
| Relancer le niveau | Réinitialisation de l'épreuve et du runtime, sans effacer l'expérience persistante des mêmes participants. |
| Recharger le niveau | Déchargement et réinstanciation de la simulation ; nouveaux NPC sans expérience des tentatives précédentes. |
| Observer / Overseer | Superviseur diégétique qui sélectionne une intervention autorisée, sans modifier directement le Monde. |
| Garde de solvabilité | Vérification préalable garantissant qu'une intervention préserve au moins une voie de résolution. |
| Trace d'événements de jeu | Chronologie factuelle et significative de l'expérience, source du débrief clinique. |

## Ajouts CANON DR-08

| Terme | Définition canonique |
|---|---|
| Exposé | Information, besoin ou contenu explicitement rendu accessible dans le Monde. |
| Interne | Connaissance, état ou contenu non directement accessible à autrui. |
| Affirmation | Information attribuée à une source décrivant ce qu'elle déclare, distincte de la vérité du Monde. |
| Déclaration | Communication signalant la possession d'une information sans nécessairement en révéler le contenu. |
| Accord social | Attente sociale mémorisable née d'une proposition et acceptation ; ni contrat du Monde ni condition de quête. |

## Ajouts CANON DR08-06

| Terme | Définition canonique |
|---|---|
| Acte de communication | Message composé d'émetteur, destinataires, intention, contenu et mode de diffusion. |
| Engagement | Attente sociale active issue d'un accord, distincte d'un but, plan ou action et sans autorité d'exécution. |
| État d'engagement | État social d'un engagement connu/accepté/actif, distinct de son exécution courante. |
| Mode de diffusion | Manière de transmettre un acte, par exemple parler, crier ou broadcast Observer. |

## Ajouts CANON DR08-07

| Terme | Définition canonique |
|---|---|
| Appraisal | Appréciation subjective multidimensionnelle des possibilités connues afin d'arbitrer ce qui mérite d'être tenté. |
| Faisabilité objective | Ce que le Monde permet réellement. |
| Faisabilité perçue | Ce qu'un participant estime réalisable depuis ses connaissances et son état subjectif. |
| Volonté de tenter | Disposition subjective à essayer une possibilité, distincte de sa faisabilité perçue. |
| Effort acceptable | Niveau de coût, risque ou effort qu'un participant estime acceptable dans ce contexte. |
| Réévaluation | Nouveau processus d'appréciation qui peut conserver ou modifier stratégie/plan. |

**WIP explicite :** les variables éventuelles de fiabilité et de fraîcheur d'une connaissance, ainsi que leurs formules, ne sont pas définies.

Les définitions détaillées et les confusions à éviter figurent dans l'audit et les sections 11, 14, 20 et 21.

## Usages validés par DR-01

| Expression | Statut | Limite |
|---|---|---|
| Information comme ressource systémique. | 🟢 CANON | Ne valide pas encore Information, connaissance, croyance, mémoire ou leurs propriétés comme définitions distinctes. |
| NPC autonome avec connaissance partielle. | 🟢 CANON | Ne valide pas le modèle Agent détaillé. |
| Réseau de dépendances. | 🟢 CANON | Ne valide pas le format AND/OR ou une implémentation. |
| Coopération systémique. | 🟢 CANON | Ne valide pas confiance, les relations ou les règles de négociation. |
