# EXIT Design Bible
## 19 - GOAP, stratégie et planification agent side

**Version :** 0.1  
**Statut :** 🟡 WIP - en attente de validation  
**Dépendances :** 11 connaissance, 14 quête objectif tâche action, 15 Gestionnaire de quêtes, 18 NPC Agents, 20 Psychologie, 21 Relations  
**Dernière mise à jour :** 17 septembre 2026

---

## 1. Définition

GOAP signifie but Oriented action Planning. Dans EXIT, il désigne le raisonnement local d'un agent qui cherche une suite de Tasks et d'Actions capable de transformer son état connu vers un but prioritaire.

Le GOAP n'écrit pas la quête et ne possède pas la vérité complète sur le niveau. Le Gestionnaire de quêtes a construit le problème global ; le planner d'un agent ne voit que ses perceptions, son connaissance, son inventaire, son état physique, ses capacités, ses besoins, ses relations et les effets qu'il estime possibles.

## 2. Intention de design

Le GOAP doit permettre à deux agents placés dans le même niveau de formuler des plans différents sans script spécifique : l'un demande un medikit, l'autre cherche un chemin sûr, un troisième garde une ressource pour lui-même. Cette différence vient de leurs connaissances, buts, coûts perçus, relations et états, non d'un traitement de faveur du Gestionnaire de quêtes.

Le système doit rester débogable et lisible. Un plan est une hypothèse révisable, pas une promesse de comportement parfait. Le joueur doit pouvoir comprendre l'intention générale d'un NPC par son contexte, ses paroles, ses déplacements et les effets qu'il produit.

## 3. Frontière essentielle

| Gestionnaire de quêtes | GOAP de l'agent |
|---|---|
| Connaît le graphe complet et les ressources réelles. | Connaît seulement des croyances et opportunités locales. |
| Vérifie la solvabilité du problème. | Cherche un chemin vers son but à partir de son état connu. |
| Place des alternatives et des sources de knowledge. | Compare les alternatives qu'il a découvertes. |
| Protège la continuité du niveau. | Peut échouer, se tromper, abandonner ou modifier son plan. |
| Ne décide pas de la stratégie individuelle. | Choisit sa stratégie, ses Tasks et ses Actions. |

> Le Gestionnaire de quêtes garantit qu'un problème existe et reste jouable. Le GOAP détermine comment un agent essaie de le résoudre.

## 4. Chaîne de décision

```text
WORLD PERÇU
    -> memory et knowledge
    -> évaluation des besoins et goals
    -> choix de stratégie
    -> GOAP : recherche d'un plan de Tasks
    -> exécution d'Actions
    -> résultat / interruption / nouvel événement
    -> mise à jour knowledge et réévaluation
```

### Deux niveaux de décision

| Niveau | Rôle | Exemple |
|---|---|---|
| Stratégie | Choisir une approche globale et ses compromis. | « Je vais négocier avec Mara plutôt que forcer le coffre. » |
| Planification GOAP | Trouver les Tasks capables de réaliser cette approche. | Trouver medikit, atteindre Mara, donner le soin, demander le code. |

La stratégie évite de laisser le planner optimiser uniquement une distance ou un nombre d'Actions. Elle introduit les préférences de risque, de coopération et de coût. Le GOAP traduit ensuite la stratégie en un plan concret et vérifiable.

## 5. Entrées du planner

| Entrée | Rôle | Exemple |
|---|---|---|
| État connu | Faits tenus pour vrais par l'agent. | « Le générateur a besoin d'un fusible F12. » |
| but prioritaire | État désiré sélectionné. | « Santé au-dessus du seuil. » |
| Tasks candidates | Opérations dont l'agent connaît les préconditions et effets. | Réparer, demander, chercher, utiliser. |
| Capabilities | Ce que l'agent peut réellement tenter. | Peut porter une batterie, ne sait pas pirater. |
| Inventaire et état physique | Ressources et limites actuelles. | Bandage porté, santé faible. |
| Coûts perçus | Temps, risque, ressource, relation, exposition. | Traverser une zone contaminée est coûteux. |
| Relations et psychologie | Modulateurs de choix et d'acceptation sociale. | confiance faible envers le joueur ; peur élevée. |
| Contraintes de stratégie | Routes admissibles ou préférées. | Éviter violence, privilégier sécurité. |

Le planner utilise l'état **connu**, et non l'état réel. Si un agent croit qu'un medikit est encore dans l'infirmerie, son plan peut être raisonnable même si l'objet a été déplacé ; l'échec met alors à jour sa mémoire et déclenche une réévaluation.

## 6. Structure de tâche exploitable par le planner

| Champ | Fonction dans la planification | Exemple |
|---|---|---|
| Preconditions | Ce qui doit être cru ou observé vrai avant tentative. | Posséder un fusible compatible ; accéder au panneau. |
| Effects | Changements attendus si réussite. | Générateur réparé ; courant disponible. |
| Cost model | Coût estimé selon l'agent et le contexte. | Temps + risque de danger + perte d'une batterie. |
| Capability requirement | Quel agent peut accomplir la tâche. | Réparer exige compétence ou outil. |
| Targets | Cibles possibles et contraintes de sélection. | Générateur A, coffre médical, Mara. |
| Failure modes | Ce qui invalide ou interrompt l'hypothèse. | Objet absent, cible verrouillée, refus de NPC. |
| connaissance requirement | Ce que l'agent doit connaître pour considérer la tâche. | Savoir que Mara peut connaître le code. |

## 7. Planification conceptuelle

Le planner cherche une série de Tasks dont les effets satisfont un but ou un objectif. Il peut raisonner à rebours depuis l'état souhaité vers les préconditions, puis ordonner les Tasks réalisables.

```text
but : obtenir un code valide
    <- tâche : demander le code à Mara
         preconditions : Mara accessible, Mara coopérative, dialogue possible
    <- tâche : stabiliser Mara
         preconditions : posséder un soin, atteindre Mara
    <- tâche : obtenir un soin
         preconditions : savoir où chercher ou explorer une zone
```

Le même but peut produire un autre plan : lire le journal de maintenance, consulter le terminal ou forcer le coffre. Ces routes ne deviennent candidates que si l'agent les connaît, les juge admissibles et peut en satisfaire les préconditions.

## 8. Modèle de coût et choix de stratégie

La distance la plus courte n'est pas toujours le meilleur plan. Le coût perçu dépend de l'agent et de son état.

| Dimension | Questions | Effet possible |
|---|---|---|
| Temps | Le délai compromet-il un besoin ou un événement ? | plan urgent, raccourci risqué, attente refusée. |
| Ressource | L'objet a-t-il un autre usage plus important ? | Réserver une batterie, demander un échange. |
| Danger | L'agent peut-il traverser la zone sans se mettre hors d'état ? | Éviter, se protéger, demander escorte. |
| Social | L'autre agent acceptera-t-il, et à quel prix ? | Négocier, promettre, renoncer ou chercher seul. |
| Moral ou identité | Cette voie est-elle compatible avec la personnalité ou l'état psychologique ? | Refuser vol ou violence, privilégier l'entraide. |
| Incertitude | L'information est-elle assez fiable pour justifier la route ? | Vérifier d'abord, explorer une alternative. |

### Proposition de règle

Une stratégie peut filtrer les plans inacceptables avant comparaison. Exemple : un NPC peut classer les routes de violence comme interdites sauf si son danger immédiat dépasse un seuil. Cette proposition reste WIP : elle doit être testée avec un périmètre réduit de personnalité et de conflit.

## 9. Réévaluation du plan

Le plan est réévalué lorsqu'une hypothèse importante devient fausse ou qu'une opportunité change sensiblement le coût attendu.

| Déclencheur | Exemple | Réponse du planner |
|---|---|---|
| Précondition absente | Le fusible n'est plus dans l'atelier. | Retrouver le porteur, demander, explorer une autre source. |
| tâche échouée | Mara refuse de parler. | Réévaluer relation, offrir un échange, trouver un journal. |
| Nouveau knowledge | Une grille révèle un passage maintenance. | Comparer la nouvelle route au plan actuel. |
| Besoin critique | Santé tombe sous le seuil. | Suspendre le plan de sortie et chercher un soin. |
| Monde modifié | La zone est inondée. | Rechercher une route sûre ou un moyen de modifier l'état. |
| but atteint | Générateur réparé. | Sélectionner le prochain objectif pertinent. |

### Stabilité

Le planner ne change pas de plan pour un gain insignifiant. Un seuil de gain, un engagement temporaire dans une action, une mémoire des échecs récents et des priorités d'urgence empêchent l'oscillation.

## 10. GOAP et arbre de comportements

| Composant | Usage approprié | Usage à éviter |
|---|---|---|
| GOAP / planner | Choisir ou construire un plan vers un but à partir de préconditions et effets. | Contrôler chaque animation ou réaction triviale. |
| arbre de comportements | Orchestrer l'exécution, les interruptions, les routines et les priorités immédiates. | Contenir à lui seul toutes les connaissances, motivations et combinaisons de plans. |
| Système de besoins | Produire et hiérarchiser des Goals. | Définir directement chaque mouvement. |
| Relations et psychologie | Modifier coûts, objectifs admissibles et réponses sociales. | Remplacer la logique de préconditions et effets. |

Le whiteboard historique peut donc être conservé comme une boucle de capacité et d'orchestration. Il ne force pas la totalité de l'intelligence NPC à résider dans un arbre de comportements unique.

## 11. Exemple comparatif

### Situation commune

Le générateur est hors service. Un fusible est dans l'atelier. Le couloir direct est dangereux. Le joueur connaît un passage plus sûr, mais Ivo ne le connaît pas encore.

| Agent | État pertinent | Stratégie probable | plan possible |
|---|---|---|---|
| Ivo | Connaît le fusible ; santé bonne ; forte tolérance au risque. | Prendre le couloir direct. | GoTo atelier, Take fusible, GoTo générateur, Repair. |
| Mara | Connaît un ancien passage ; santé faible ; peur élevée. | Éviter la zone et demander une escorte. | Tell passage, Ask aide, Follow joueur, attendre sécurité. |
| Joueur | Connaît les deux routes ; batterie rare. | Arbitrer risque, temps et coopération. | Aider Mara, informer Ivo, ou prendre la branche urgence. |

La divergence n'est pas scriptée comme « Ivo est courageux ». Elle résulte d'informations, états, coûts perçus et modulateurs distincts.

## 12. Critères du premier vertical slice

| Élément à prouver | Critère |
|---|---|
| but local | Un NPC sélectionne un but à partir d'un besoin ou d'une opportunité connue. |
| plan de Tasks | Il construit ou suit une courte chaîne de 2 à 4 Tasks. |
| État connu | Il échoue raisonnablement lorsqu'une croyance est dépassée, puis se met à jour. |
| Alternative | Il peut sélectionner une seconde route s'il connaît une autre méthode. |
| Coût | Une différence de danger ou de relation change la sélection de stratégie. |
| Réévaluation | Une précondition invalidée déclenche un changement interprétable de plan. |
| Frontière | Le NPC ne reçoit jamais de réponse provenant du Gestionnaire de quêtes sans perception ou knowledge. |

## 13. Risques et garde-fous

| Risque | Symptôme | Garde-fou |
|---|---|---|
| plan parfait artificiel | Le NPC connaît toujours la meilleure route. | Limiter le planner au connaissance local et à l'incertitude. |
| plan inhumain | Le NPC choisit systématiquement l'option la plus courte malgré danger ou relation. | Modèle de coût contextualisé et stratégie. |
| Explosion de recherche | Trop de Tasks et d'états rendent le planificateur opaque ou lent. | Catalogue minimal, hiérarchie objectif/tâche, budgets de profondeur. |
| Boucle de replanification | L'agent hésite sans agir. | Seuil de changement, mémoire d'échecs, engagement temporaire. |
| Social magique | Le planner suppose qu'un NPC acceptera sans évaluer relation ou demande. | Actions sociales avec préconditions, coût et résultat explicites. |
| Injustice pour le joueur | Un plan NPC annule une ressource critique. | Continuité et solvabilité dynamique de la Section 16. |

## 14. Paramètres à calibrer

- Goals retenus pour le prototype ;
- fréquence de planification et de réévaluation ;
- profondeur maximale de plan ;
- nombre de Tasks candidates par but ;
- dimensions du coût et leurs poids relatifs ;
- seuil de changement de stratégie ;
- durée d'engagement dans une action ;
- règles d'incertitude et de vérification ;
- rôle exact du arbre de comportements ;
- traces de débogage nécessaires pour expliquer un choix NPC.

## 15. Décisions de validation demandées

1. Valider GOAP comme modèle de planification agent-side, distinct du Gestionnaire de quêtes.
2. Valider la séparation stratégie / planification des Tasks.
3. Définir les Goals et Tasks minimum du premier vertical slice.
4. Choisir les dimensions de coût nécessaires au prototype : temps, danger, ressource, social, incertitude.
5. Décider quelle part de l'orchestration reste dans un arbre de comportements.

## 16. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Prototype GOAP avec `Find Exit`, clé, coffre, mot de passe et Actions intermédiaires. | S02 | 🟠 |
| Historique | Boucle analyser, stratégie, planifier, réaliser, réévaluer. | S05 | 🟠 |
| Actuel | GOAP explicitement rattaché au raisonnement de l'agent. | S08 | 🟡 |
| Actuel | connaissance, besoins, relations et état modulent la stratégie puis la planification. | Sections 11 et 18 | 🟡 |
| Futur | Algorithme concret, format des états et intégration avec Olympe Engine restent à spécifier après validation de ce modèle. | Mandat S08 | 🔵 |

## Propagation DR-02

**🟢 CANON lexical :** stratégie = approche générale poursuivant un but ; plan = organisation ordonnée ou conditionnelle des moyens et actions ; action = opération exécutable tentant une transition. `Tâche` n'est pas retenue entre plan et action. GOAP demeure hors périmètre algorithmique.

## Propagation DR-06

**🟢 CANON :** un agent identifie les options qu'il connaît et estime possibles, puis les apprécie subjectivement avant de filtrer, comparer et décider. But, stratégie et plan ont des responsabilités distinctes. L'appréciation est multidimensionnelle et non un score unique imposé ; stress et friction décisionnelle contribuent à la latence, non à une irrationalité ni à une action forcée.

## Propagation DR08-06

**🟢 CANON :** une communication, proposition, menace ou engagement ne commande jamais directement but, stratégie, plan ou action. Un engagement actif peut être temporairement non exécuté ; engagements concurrents participent à l'appréciation. Une réévaluation peut conserver le plan courant ou conclure qu'aucun plan utile n'est disponible, sans blocage cognitif.

## Propagation DR08-07

**🟢 CANON :** l'Appraisal arbitre seulement des possibilités connues depuis un contexte subjectif. Importance et urgence sont distinctes ; faisabilité objective ≠ faisabilité perçue. Une possibilité peu probable peut rester tentable selon bénéfice, urgence, alternatives et effort acceptable. La réévaluation compare au plan courant et peut produire `KEEP` ou `CHANGE` ; réévaluation ≠ changement.
