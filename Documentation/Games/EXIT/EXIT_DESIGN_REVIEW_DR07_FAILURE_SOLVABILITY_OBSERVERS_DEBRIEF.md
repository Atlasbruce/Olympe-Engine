# EXIT — Revue de conception DR-07
## Échec, solvabilité, Observers, débrief et sémantique de relance

**Statut :** 🟢 REVIEW COMPLETED  
**Date :** 20 septembre 2026

## 1. Contrat d'échec et solvabilité

**🟢 CANON.**

```text
Branche invalidée ≠ quête insatisfaisable ≠ fin de partie
Échec de quête ≠ échec du joueur ≠ fin de session
Insatisfaite ≠ insatisfaisable
Existe ≠ accessible ≠ utilisable
Solvabilité du Monde ≠ connaissance des agents
```

La solvabilité est la possibilité systémique de réaliser une quête dans le Monde, non la connaissance subjective d'un agent ni sa probabilité de choisir une voie. Une ignorance, une peur, une défiance ou un refus actuel ne rendent pas seuls une branche impossible. Une destruction irréversible, mort d'un acteur indispensable, information réellement éteinte ou fenêtre définitivement expirée peuvent le faire.

Le Gestionnaire de partie détecte les impossibilités locales démontrables depuis conditions et événements irréversibles. Il ne prétend pas prouver la solvabilité globale : `impossibilité non prouvée ≠ solvabilité prouvée`. Le futur évaluateur de solvabilité/atteignabilité récursif reste 🟡 WIP et ne relève pas actuellement du Gestionnaire de partie.

## 2. Assistance de récupération

Si toutes les branches pertinentes sont prouvées impossibles : assistance désactivée → fin de partie + débrief ; assistance activée → une intervention de récupération peut être autorisée. Sans récupération valide ou sélectionnée, fin de partie + débrief. L'assistance est une possibilité, jamais une intervention automatique.

## 3. Observers / Overseers

**🟢 CANON.** EXIT est une expérience sociale grandeur nature, observée comme un spectacle. Des Observers/Overseers invisibles supervisent l'expérience ; le public peut influencer certains choix par vote, mais ne modifie jamais directement le Monde. Les Observers sélectionnent et interprètent une intervention autorisée.

| Catégorie | Rôle |
|---|---|
| Intervention dynamique | Facilitation, complication, contrainte ou intervention influencée par le public. |
| Activation de condition / contrainte | Active une logique déjà prévue dans la quête. |
| Transformation de quête | Ajoute/modifie réellement le graphe ; pouvoir du Gestionnaire de quêtes. |
| Intervention de récupération | Restaure une possibilité de résolution après une impossibilité issue du gameplay. |

Toute intervention modifiant quête ou Monde passe par une **garde de solvabilité** : elle doit préserver au moins une voie de résolution. Ni Observer ni vote du public ne peut volontairement supprimer la dernière voie. Après intervention, le Gestionnaire de partie réévalue l'état. Le gameplay normal peut ensuite produire un véritable échec.

```text
Monde → Gestionnaire de partie → Observer → demande d'intervention
→ Gestionnaire de partie / Gestionnaire de quêtes → Monde
```

Le Gestionnaire de quêtes construit ou transforme le problème ; le Gestionnaire de partie orchestre et applique les règles ; le Monde matérialise et simule ; l'Observer assure une direction expérientielle/diégétique. Le modèle décisionnel des Observers est 🟡 WIP.

## 4. Parité d'information et d'expérience

**🟢 CANON — Parité d'information.** Les NPC sont, autant que possible, soumis aux mêmes règles d'accès, de découverte, de mémorisation et de conservation de l'information que le joueur humain. Ils ne reçoivent pas arbitrairement une information inaccessible au joueur et ne perdent pas artificiellement celle qu'un joueur conserverait naturellement entre tentatives. Cela n'implique jamais une connaissance partagée.

**🟢 CANON — Parité expérientielle.** Lorsqu'une expérience d'un joueur humain survivrait naturellement à une relance comme connaissance, souvenir, émotion mémorisée ou histoire relationnelle, les NPC bénéficient autant que possible de la même continuité.

> EXIT réinitialise l'épreuve, pas ceux qui l'ont vécue.

## 5. Tentative, relance, rechargement et sortie

| Terme | Définition CANON |
|---|---|
| Tentative | Exécution d'un niveau entre son démarrage et sa terminaison ou sa relance. |
| Relancer le niveau | Réinitialise exactement l'épreuve et sa configuration initiale, pas l'expérience persistante des mêmes participants. |
| Recharger le niveau | Décharge/réinstancie entièrement la simulation ; les nouvelles instances NPC repartent de leur configuration initiale, sans expérience de tentatives antérieures. |
| Quitter le niveau | Quitte l'expérience/niveau ; interface et navigation hors périmètre. |

La relance restaure Monde, exécution de quête, topologie, objets, ressources, machines, portes, positions/états physiques/inventaires NPC, modifications runtime Observer, conditions/branches runtime, buts/stratégies/plans/actions courants et autres états temporaires de tentative. Les mêmes NPC conservent mémoire/connaissance, empreintes émotionnelles, relations, confiance et histoire vécue.

Après relance : stress régulé vers un état adapté ; confiance en soi restaurée vers une valeur de nouvelle tentative ; sécurité ressentie réévaluée depuis le Monde restauré et l'expérience persistante. Valeurs et algorithmes : 🟡 WIP. Le rechargement ne prétend pas effacer la connaissance réelle du joueur humain.

## 6. Événements, trace et débrief

```text
Monde / ECS, IA NPC, objets, quête, systèmes de jeu
→ messages/événements → QueueManager → Gestionnaire de partie
→ sélection d'événements significatifs → trace d'événements de jeu → débrief
```

Le Gestionnaire de partie écoute, sans devenir propriétaire des systèmes sources. Événement capturé ≠ événement retenu. La trace conserve les éléments significatifs, factuels, chronologiques et cliniques : timestamp, type, sujets, lieu, états avant/après, acteur factuel, contexte factuel, système/condition liés lorsque pertinents.

Le débrief offre : (1) rapport de résultat final ; (2) chronologie d'expérience. Il n'attribue pas encore responsabilité, jugement moral, causalité complexe, opportunités manquées ou chemins alternatifs. Une convergence future avec la trace de décision subjective NPC est 🔵 proposition.

## 7. Modes, fin et temporisateurs

Le mode coopératif ne rend jamais la coopération comportementale obligatoire : chaque participant demeure autonome. Il modifie les règles et les conditions de résultat, non le modèle de décision.

En mode coopératif, l'activation de la sortie peut ouvrir une fenêtre limitée. Chaque participant doit la franchir physiquement avant expiration ; les sortants sont classés par ordre. Les participants restants perdent et sont considérés morts pour le résultat. Valeurs du temporisateur : 🟡 WIP.

En mode compétitif, le premier participant satisfaisant l'évasion gagne selon les règles du mode ; la session peut alors se terminer.

Trois temporisateurs sont distincts : temporisateur de quête (disponibilité de condition/branche), de session/sortie (règles de jeu), de danger (évolution du Monde). Un danger arrivé à échéance déclenche ses conséquences dans le Monde, puis le Gestionnaire de partie réévalue : il n'appelle pas automatiquement une fin de partie.

## 8. Mort du participant

La mort du joueur humain ne termine pas automatiquement l'expérience. Il est un participant soumis autant que possible aux mêmes règles fondamentales que les NPC. Si d'autres participants peuvent poursuivre, simulation et expérience continuent. Si tous les NPC meurent alors que le joueur peut fuir, la quête peut rester solvable. À l'inverse, si le joueur ne peut plus fuir mais qu'un NPC le peut, la solvabilité globale peut persister. L'interface du joueur mort est 🟡 WIP.

## 9. Décisions enregistrées

| ID | Décision | Statut |
|---|---|---|
| DR07-D01 | Contrat d'échec : branche, quête, joueur et session distincts. | 🟢 CANON |
| DR07-D02 | Solvabilité locale démontrable, sans prétention de preuve globale. | 🟢 CANON |
| DR07-D03 | Assistance de récupération optionnelle. | 🟢 CANON |
| DR07-D04 | Observers, public indirect et garde de solvabilité. | 🟢 CANON |
| DR07-D05 | Parité d'information. | 🟢 CANON |
| DR07-D06 | Parité expérientielle. | 🟢 CANON |
| DR07-D07 | Relancer ≠ recharger ; persistance définie. | 🟢 CANON |
| DR07-D08 | Tentative, rechargement et sortie de niveau. | 🟢 CANON |
| DR07-D09 | Trace d'événements factuelle et débrief clinique. | 🟢 CANON |
| DR07-D10 | Modes, temporisateurs et mort de participant. | 🟢 CANON |

## 10. WIP reporté

Évaluateur global/récursif de solvabilité, décisions/votes Observer, valeurs des timers, régulation exacte au restart, UI post-mort et navigation Restart/Reload/Quit, débrief causal enrichi, convergence trace de jeu/trace de décision.

## 11. Revue finale

Les ambiguïtés historiques « relancer efface la mémoire NPC » et « Gestionnaire de quêtes surveille/résout toute solvabilité » sont remplacées par ce contrat CANON et restent 🟠 historiques dans les sources qui les portent. Aucune contradiction bloquante détectée. DR‑08 n'est pas ouverte.

