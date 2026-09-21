# EXIT Design Bible
## 18 - NPC agents, capacités et boucle cognitive

**Version :** 0.1  
**Statut :** 🟡 WIP - en attente de validation  
**Dépendances :** 11 connaissance, 14 quête objectif tâche action, 15 Gestionnaire de quêtes, 16 Solvabilité, 19 GOAP, 20 Psychologie, 21 Relations  
**Dernière mise à jour :** 16 septembre 2026

---

## 1. Définition

Un NPC EXIT est un agent autonome qui vit dans le même problème que le joueur. Il ne constitue pas une serrure humaine, un point de dialogue fixe ou un distributeur de quête. Il peut percevoir, explorer, se souvenir, apprendre, posséder, utiliser, communiquer, choisir un objectif, planifier et agir selon son état et ses capacités.

Le joueur et le NPC partagent autant que possible les mêmes règles fondamentales. La différence ne réside pas dans une capacité secrète du NPC, mais dans son autonomie, ses connaissances locales, ses besoins, son profil et ses objectifs.

## 2. Intention de design

- Produire des rencontres qui changent réellement le problème du niveau.
- Faire de l'information, des ressources et de la confiance des enjeux partagés entre agents.
- Donner une impression de vie sans rendre les NPC imprévisibles ou injustes.
- Créer des histoires émergentes : un NPC trouve une ressource, la garde, l'échange, la consomme, demande de l'aide ou se trompe.
- Permettre au joueur de comprendre les raisons visibles d'un comportement, même si toutes les variables internes ne sont pas affichées.

## 3. Principe d'équivalence

| Règle partagée | Joueur | NPC |
|---|---|---|
| Se déplacer dans la topologie | Contrôle direct. | Navigation autonome. |
| Percevoir et observer | Caméra, UI, Actions d'observation. | Capteurs, portée, Actions d'observation. |
| Acquérir connaissance | Exploration, lecture, dialogue, déduction. | Même sources, selon ce qu'il a perçu ou reçu. |
| Porter et utiliser des objets | Inventaire et interactions. | Inventaire et Actions soumis aux mêmes préconditions. |
| Subir un état physique | Santé, faim et dangers retenus. | Même catégories, paramètres éventuellement différents. |
| Parler, donner, demander, échanger | Choix du joueur. | Décision selon besoins, relations, knowledge et plan. |
| Résoudre un problème | Raisonnement humain assisté par UI. | Stratégie et planification à partir de son état connu. |

### Asymétries admises

Le joueur peut disposer d'un journal, d'une carte annotée et d'une vision synthétique pour compenser l'absence d'automatisation. Ces outils ne doivent pas révéler le État du monde ou le connaissance privé des NPC. À l'inverse, un NPC peut conserver une mémoire formalisée et recalculer un plan plus vite ; cela ne doit pas lui donner une connaissance qu'il n'a pas acquise.

## 4. État conceptuel d'un agent

| Couche | Rôle | Exemple |
|---|---|---|
| état physique | Capacités et vulnérabilités corporelles. | Santé basse, faim élevée, blessure à la jambe. |
| Inventory | Objets actuellement portés ou réservés. | Batterie, bandage, clé. |
| perception | Signaux locaux reçus. | Bruit d'alarme, voyant rouge, appel de Mara. |
| mémoire | Événements et dernières observations. | Ivo a pris le fusible. |
| connaissance | Croyances exploitables et sourcées. | Le générateur exige un fusible F12. |
| Capabilities | Actions possibles, compétences et contraintes. | Peut réparer, ne peut pas franchir une zone contaminée. |
| Needs | Pressions motivantes. | Manger, se soigner, se sentir en sécurité. |
| Goals | États désirés priorisés. | Atteindre l'infirmerie, survivre, quitter le secteur. |
| Relations | État directionnel envers d'autres agents. | confiance envers le joueur, dette envers Mara. |
| personnalité et psychologie | Modulateurs de décision. | Prudence, stress, motivation ; détaillés en Section 20. |

## 5. Boucle cognitive historique consolidée

```text
EXPLORER
    -> DÉCOUVRIR / OBSERVER
    -> MÉMORISER
    -> ANALYSER
    -> STRATÉGIE / CHOIX
    -> PLANIFIER DES TÂCHES
    -> RÉALISER DES ACTIONS
    -> RÉÉVALUER
    -> EXPLORER OU POURSUIVRE
```

### Version simplifiée

```text
PERCEVOIR -> COMPRENDRE -> DÉCIDER -> AGIR -> RÉÉVALUER
```

La première version documente les étapes de design et de diagnostic. La seconde sert de règle de lecture pour le joueur. Un arbre de comportements peut orchestrer des routines d'urgence, d'animation ou de déplacement ; il ne remplace pas la représentation du knowledge, l'évaluation des besoins ni la planification des objectifs.

## 6. Étapes de décision

| Étape | Entrées | Sortie | Exemple |
|---|---|---|---|
| Percevoir | Monde proche, signaux sociaux, état propre. | Nouveaux faits ou alertes. | Entend une sirène et voit une porte s'ouvrir. |
| Observer | Cible et capacité d'inspection. | Information interprétée. | Lit l'étiquette « fusible F12 ». |
| Mémoriser | Événement ou observation. | Trace datée et localisée. | Retient que le fusible était dans l'atelier. |
| Analyser | connaissance, État du monde connu, besoins. | options et contraintes. | Déduit que réparer le générateur peut ouvrir l'infirmerie. |
| Évaluer | Besoins, goals, risque, relations, psychologie. | Priorités. | Se soigner devient plus urgent que chercher EXIT. |
| Choisir une stratégie | Alternatives connues. | Approche à haut niveau. | Demander de l'aide plutôt que traverser la zone dangereuse. |
| Planifier | Objectif et Actions disponibles. | plan de Tasks révisable. | Aller à Mara, proposer un échange, obtenir le code. |
| Agir | Préconditions réelles et plan courant. | Effet, échec ou interruption. | Donne un bandage à Mara. |
| Réévaluer | Résultat et événements nouveaux. | Continuer, modifier ou abandonner le plan. | Mara refuse : chercher le terminal. |

## 7. Priorités de haut niveau

La priorité n'est pas une liste fixe de quêtes. Elle organise les réactions lorsqu'un besoin ou un événement exige une décision.

| Priorité WIP | question | Comportements possibles |
|---|---|---|
| Danger immédiat | Suis-je menacé maintenant ? | Fuir, se cacher, stabiliser une blessure, appeler. |
| Besoin critique | Puis-je encore agir si cet état se dégrade ? | Manger, se soigner, se reposer, chercher sécurité. |
| Engagement | Une promesse, un allié ou une dette exige-t-il une réponse ? | Livrer, secourir, expliquer, négocier. |
| but personnel | Puis-je progresser vers mon objectif principal ? | Explorer, récupérer, réparer, demander. |
| Opportunité | Une découverte réduit-elle fortement un coût futur ? | Observer, mémoriser, signaler, prendre. |
| Maintenance | Puis-je améliorer une situation future sans urgence ? | Stocker, cartographier, recharger, partager. |

L'ordre final reste modulé par la personnalité, la psychologie, les relations et la difficulté. Un agent très loyal peut répondre à un allié avant une opportunité personnelle ; un agent gravement blessé doit souvent prioriser sa survie.

## 8. Capabilities et catalogue de comportements

| Famille | Capacités candidates | Fonction systémique |
|---|---|---|
| Déplacement | GoTo, Explore, Follow, Wait, Flee. | Découverte, route, évitement, escorte. |
| observation | Observe, Search, Read, Listen. | Acquisition de knowledge et vérification. |
| Possession | Take, Drop, Store, Carry. | Économie, accès, déplacement d'objets. |
| Utilisation | Use, Connect, Repair, Activate, Unlock, Open. | Transformation du État du monde. |
| Communication | Talk, Ask, Tell, Signal, Refuse. | Partage d'information, coordination, relation. |
| Échange | Give, Exchange, Promise, Request. | Ressource sociale et négociation. |
| Survie | Eat, Drink, Heal, Rest, SeekSafety. | Maintien de capacité et pression. |
| Conflit | Avoid, Threaten, Steal, Restrain, Attack. | WIP ; périmètre à décider avant implémentation. |

Chaque capability doit déclarer ses préconditions, effets, durée, interruptions, feedbacks et impacts potentiels sur le graphe de solvabilité.

## 9. Réévaluation et stabilité comportementale

Un NPC ne doit pas changer de plan à chaque micro-événement. La réévaluation se déclenche lorsque l'information modifie réellement le coût, la possibilité ou l'urgence d'un plan.

| Déclencheur | Exemple | Réaction attendue |
|---|---|---|
| Précondition invalidée | Le fusible recherché a disparu. | Rechercher dernière position, demander, changer de branche. |
| Nouveau knowledge important | Une note révèle un raccourci. | Comparer le nouveau coût au plan courant. |
| Danger immédiat | Gaz dans le couloir. | Interrompre, fuir ou mettre en sécurité. |
| Besoin passe un seuil | Santé critique. | Prioriser une tâche de soin. |
| Relation modifiée | Promesse tenue ou trahison observée. | Réévaluer l'acceptation d'une coopération. |
| Objectif accompli | Générateur réparé. | Sélectionner la prochaine étape ou aider un allié. |

### Garde-fous contre l'oscillation

- seuil minimal de gain avant d'abandonner un plan ;
- délai ou coût de changement de stratégie ;
- engagement temporaire quand une action est presque terminée ;
- mémoire des échecs récents ;
- priorité absolue seulement pour les dangers et besoins critiques.

## 10. Autonomie, lisibilité et liberté du joueur

| Risque | Effet indésirable | Garde-fou |
|---|---|---|
| NPC trop passif | Il attend le joueur et redevient un donneur de quête. | Goals personnels, exploration limitée et besoins propres. |
| NPC trop efficace | Il résout le niveau sans le joueur. | Budgets de knowledge, capacités complémentaires, intérêt de coordination. |
| NPC trop chaotique | Il déplace les objets sans raison compréhensible. | Plans visibles par contexte, feedback de dernière position, priorités stables. |
| NPC omniscient | Il va directement vers l'objet caché. | Planner limité au knowledge local. |
| NPC destructeur | Il consomme une ressource et bloque le niveau. | Règles de criticité et continuité de la Section 16. |
| NPC manipulable sans coût | Le joueur exploite la même interaction infiniment. | Mémoire d'actes, besoins évolutifs, limites d'échange. |

## 11. Deux exemples de comportement émergent

### Exemple A - Le fusible trouvé avant le joueur

Ivo observe le générateur, lit qu'un fusible F12 est requis, puis trouve un F12 dans l'atelier. Son but immédiat est de rétablir le courant pour accéder à un abri. Il prend le fusible et se dirige vers le générateur. Le joueur peut le suivre, lui demander la pièce, proposer une batterie, l'aider à réparer ou choisir une branche différente. Ivo n'a pas besoin de savoir qu'il sert une quête globale : son action est cohérente avec son knowledge et son but.

### Exemple B - La coopération devient possible

Mara connaît le code du coffre mais sa blessure et son besoin de sécurité la conduisent à refuser de parler à un inconnu. Le joueur lui donne un soin ou la protège durant un événement dangereux. Son état et sa relation évoluent ; elle accepte alors une conversation ou un échange. Le système ne doit pas réduire cela à un seuil caché de « +10 trust » : le joueur voit le besoin, l'acte, la réaction et l'option qui s'ouvre.

## 12. Paramètres à calibrer

- nombre de NPC actifs simultanément ;
- fréquence de perception et de replanification ;
- budgets de mémoire et de knowledge ;
- distance maximale d'exploration autonome ;
- degré de visibilité des plans et intentions de NPC ;
- seuils des besoins critiques ;
- coût du changement de plan ;
- restrictions de possession, de consommation et de déplacement d'objets critiques ;
- niveau de coordination possible entre NPC ;
- périmètre de tromperie, vol et conflit.

## 13. Critères du premier vertical slice

| Capacité à prouver | Critère observable |
|---|---|
| perception locale | Le NPC réagit à un fait qu'il a vu ou entendu, pas à une information globale. |
| connaissance | Il mémorise l'emplacement ou la condition d'un objet utile. |
| but personnel | Il cherche une ressource qui répond à un besoin propre. |
| plan simple | Il enchaîne déplacement, prise et utilisation, avec réévaluation après échec. |
| Interaction sociale | Il peut demander, refuser ou accepter un échange pour une raison lisible. |
| Continuité | Son comportement n'élimine pas toutes les voies de sortie. |
| retour d'information | Le joueur peut interpréter ce qu'il fait et pourquoi. |

## 14. Décisions de validation demandées

1. Valider le principe d'équivalence des capacités joueur/NPC, avec les asymétries UX explicites.
2. Choisir le nombre de NPC du premier vertical slice et leurs Goals initiaux.
3. Valider la boucle cognitive comme structure de référence, sans imposer le arbre de comportements comme architecture universelle.
4. Définir les capabilities réellement nécessaires au premier slice.
5. Décider si les actions de conflit sont exclues, limitées ou incluses dès le prototype.

## 15. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | NPC avec inventaire, santé, faim, émotions et interactions. | S01 | 🟠 |
| Historique | Whiteboard de capacités : exploration, observation, mémoire, stratégie, planification, actions, réévaluation. | S05 | 🟠 |
| Actuel | NPC défini comme autre agent dans le même problème. | S06, S08 | 🟡 |
| Actuel | Séparation Gestionnaire de quêtes/agent et limitation au knowledge local. | S08, Sections 11 et 15 | 🟡 |
| Futur | personnalité, psychologie, émotions et relations seront spécifiés sans les fusionner dans cette section. | Sections 20 et 21 | 🔵 |

## Propagation DR-02

**🟢 CANON :** le NPC entretient un tableau noir IA privé. Ses données opérationnelles et cognitives sont autoritatives pour son état interne, sans être nécessairement vraies concernant le Monde. Il évalue des besoins, poursuit des buts, choisit une stratégie, élabore un plan, agit et réévalue ; aucun état interne ne dicte seul une action.

## Propagation DR-03

**🟢 CANON :** seuls les systèmes IA NPC autorisés lisent les données accessibles du Monde/ECS, les évaluent et écrivent les résultats dans le tableau noir. Une lecture technique n'est pas une connaissance : perception et cognition déterminent ce qui devient exploitable par le NPC.

## Propagation DR-04

**🟢 CANON :** le tableau noir est l'état interne exploitable du NPC et non son système de raisonnement. Les systèmes IA NPC constituent les traitements ; avec le tableau noir, ils forment le cerveau fonctionnel de l'agent. Les besoins sont issus d'une évaluation IA d'états factuels ; `ÉTAT ≠ BESOIN ≠ BUT ≠ ACTION`.
