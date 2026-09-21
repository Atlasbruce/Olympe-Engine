# EXIT Design Bible
## 01 - Vision, identité et expérience EXIT

**Version :** 0.1  
**Statut :** 🟡 WIP - contient des principes 🟢 CANON validés en DR-01  
**Dépendances :** Audit Phase 1, glossaire initial, sources S01 à S08  
**Dernière mise à jour :** 16 septembre 2026

---

## 1. Rôle de cette section

Cette section formule l'identité recherchée pour EXIT et les critères qui guideront les futurs systèmes. Elle ne définit pas encore les règles détaillées du Gestionnaire de quêtes, des NPC ou de la psychologie. Ces détails devront confirmer la promesse ci-dessous, pas la remplacer.

## 2. Définition de travail

> **EXIT est un jeu d'évasion systémique multi-agents dans lequel le joueur cherche une voie vers la sortie en comprenant et en agissant sur un réseau dynamique de dépendances, au sein d'un monde peuplé d'autres agents autonomes confrontés au même problème.**

Le joueur évolue dans un environnement clos, modulaire et surveillé. Il cherche une sortie rarement accessible immédiatement. Pour l'atteindre, il doit explorer, observer, comprendre des relations entre objets, machines, accès, ressources, informations et personnages, puis choisir une manière de progresser.

Les autres personnages ne sont pas des distributeurs de missions. Ils vivent dans le même espace de contraintes : ils perçoivent une partie du monde, possèdent des ressources et des connaissances, ont des besoins, prennent des décisions et peuvent poursuivre leur propre issue. Ils peuvent donc aider, demander, négocier, refuser, se tromper, concurrencer le joueur ou le trahir.

### Statut des affirmations de définition

| Affirmation | Provenance | Statut |
|---|---|---|
| Définition d'EXIT. | DR01-D01 | 🟢 CANON |
| Le problème d'évasion est un réseau de dépendances. | DR01-D05 | 🟢 CANON |
| L'information est une ressource systémique. | DR01-D03 | 🟢 CANON |
| Les NPC sont autonomes, locaux et non omniscients. | DR01-D02 | 🟢 CANON |
| La coopération est située et non arbitraire. | DR01-D04 | 🟢 CANON |
| Les causes et conséquences pertinentes sont interprétables sans révélation obligatoire de solution. | DR01-D06 | 🟢 CANON |
| Observer, comprendre, décider, agir, réévaluer est une boucle d'expérience joueur. | DR01-D07 | 🟢 CANON |
| Le détail de confiance reste hors périmètre. | DR01-D08 | 🔴 À RÉSOUDRE DR-05 |

## 3. Pitch

Réveille-toi dans un complexe expérimental dont les règles restent opaques. La porte EXIT est verrouillée, mais le verrou n'est qu'une conséquence visible d'un système plus vaste : alimentation manquante, clé enfermée, code détenu par un survivant, passage dangereux, ressource rare ou accord fragile.

Explore le complexe. Conserve ce que tu apprends. Décide ce que tu partages. Choisis si tu aides une personne qui peut t'aider en retour, si tu gardes le dernier medikit, ou si tu prends un chemin plus risqué pour ne dépendre de personne. Pendant ce temps, les autres captifs cherchent aussi une issue.

## 4. Promesse joueur

EXIT doit donner au joueur la sensation de résoudre un problème vivant, pas de suivre une liste de serrures prédéfinies.

| Promesse | Ce que le joueur doit vivre | Conséquence de design |
|---|---|---|
| Comprendre le monde | Une découverte rend une situation plus lisible ou ouvre une nouvelle hypothèse. | Les détails visuels, les états de machine et les dialogues deviennent des informations exploitables. |
| Trouver sa voie | Plusieurs approches peuvent mener à une même condition, avec des coûts différents. | Les graphes AND/OR priment sur les chaînes uniques. |
| Peser ses ressources | Un objet utile a souvent plusieurs emplois possibles. | Éviter les objets jetables ou les solutions sans coût d'opportunité. |
| Composer avec les autres | Les NPC ont quelque chose à gagner, à perdre et à apprendre. | Les relations modifient les options sans masquer les règles. |
| Assumer une histoire émergente | Une coopération, un refus ou une perte modifie la partie de manière compréhensible. | Les changements systémiques exigent un feedback causal. |

## 5. Buts du joueur

### But immédiat

Atteindre et franchir la sortie du niveau.

### But opérationnel

Découvrir les conditions de sortie, puis satisfaire les conditions choisies : accès, énergie, clé, code, machine réparée, information vérifiée, aide négociée ou chemin alternatif.

### But de survie

Préserver un état physique suffisant pour continuer à agir. La faim et la santé sont les variables explicitement présentes dans les sources. Toute autre pression, comme la soif, la fatigue, le temps ou des dangers environnementaux, reste à sélectionner pour le vertical slice.

### But narratif

Comprendre graduellement la situation : qui a construit le complexe, pourquoi les personnages y sont enfermés et ce que signifie réellement « sortir ». La réponse n'est pas définie à ce stade.

## 6. Expérience émotionnelle recherchée

EXIT ne vise pas une tension uniforme. Le rythme doit alterner entre lecture, incertitude, déduction, préparation, prise de risque et conséquence.

| Moment | État recherché chez le joueur | Moyen de design |
|---|---|---|
| Arrivée dans une zone | Curiosité et légère vulnérabilité | Espace lisible localement, fonction globale inconnue, traces d'activité. |
| Première découverte | « Ceci pourrait servir. » | Objet, message ou état de machine avec une fonction partiellement révélée. |
| Mise en relation | « Ah, je comprends ce qui bloque. » | Indices recoupables, flux visibles, dialogue ou observation qui complète un manque. |
| Dilemme | « Je peux progresser, mais à quel prix ? » | Ressource rare, aide demandée, temps, risque ou promesse à tenir. |
| Exécution du plan | Concentration et tension juste | Préconditions visibles, coût annoncé, danger lisible, possibilité de réévaluer. |
| Conséquence sociale | Responsabilité et attachement | NPC qui se souvient, change de comportement ou réoriente son propre plan. |
| Ouverture de la sortie | Soulagement et questionnement | Résolution systémique claire, puis élément narratif qui relance le mystère. |

## 7. Piliers de design

### Pilier 1 - Le niveau est un problème, pas un couloir de puzzles

Chaque verrou, objet, machine, information ou personne importante doit s'inscrire dans une logique de dépendance. Une porte peut exiger une clé et du courant. La clé peut venir d'un coffre. Le coffre peut être ouvert grâce à une information, à une compétence, à un autre accès ou à un choix risqué.

**Test de cohérence :** le level designer explique quelle condition un élément sert, quelles alternatives existent, et ce qui change si l'élément disparaît.

### Pilier 2 - L'information a une valeur jouable

Un code, un emplacement, une identité, l'état d'une machine ou l'intention d'un NPC peuvent être plus importants qu'un objet physique. L'information peut être découverte, mémorisée, vérifiée, conservée, échangée, retenue, erronée ou mensongère selon le périmètre validé.

**Test de cohérence :** découvrir une information utile modifie au moins une décision possible du joueur ou d'un agent.

### Pilier 3 - Les NPC font partie du système

Un NPC possède une perception locale, un inventaire, une mémoire, des connaissances, des besoins, des capacités et des objectifs. Il peut découvrir une ressource avant le joueur et l'utiliser pour lui-même. Une relation avec lui augmente ou réduit des options, mais ne doit pas devenir une serrure sociale opaque.

**Test de cohérence :** une rencontre transforme le problème, même sans dialogue de quête scripté.

### Pilier 4 - La coopération est choisie, jamais décorative

La coopération résulte de la situation et des agents. Elle peut être facultative, avantageuse ou nécessaire lorsque le problème impose une contrainte intelligible. Elle ne peut être un verrou social arbitraire reposant sur un seuil caché.

**Test de cohérence :** le joueur comprend pourquoi un NPC accepte, hésite ou refuse.

### Pilier 5 - La pression rend les choix significatifs

La survie, les dangers et les contraintes de temps créent des coûts. Ils ne doivent pas confisquer la résolution du problème de façon silencieuse. Une pression est juste lorsqu'elle est perceptible, anticipable et liée à une réponse possible.

**Test de cohérence :** le joueur explique après coup pourquoi son plan a échoué, changé ou coûté cher.

### Pilier 6 - Une complexité interne lisible de l'extérieur

La simulation peut calculer des croyances, des priorités et des relations. Le joueur ne doit pas lire un tableau de coefficients pour comprendre le monde. Il doit pouvoir interpréter une porte non alimentée, une promesse non tenue, une blessure, une absence ou un objet déplacé, sans recevoir nécessairement la précondition exacte ou la solution.

**Test de cohérence :** chaque effet systémique important dispose d'au moins un signal in-world, UI ou narratif intelligible.

## 8. Ce qui différencie EXIT d'un escape game classique

| Escape game classique | EXIT visé |
|---|---|
| Puzzles principalement fixes et séquentiels | Réseau de dépendances pouvant comporter des alternatives AND/OR. |
| Joueur seul face au décor | Plusieurs agents qui explorent, accumulent et dépensent des ressources. |
| Indice généralement public et stable | Information locale, possédée, partagée, retenue ou à vérifier. |
| NPC principalement narratif ou fonctionnel | NPC autonome, social et susceptible de modifier l'état du niveau. |
| Solution attendue unique | Voies différenciées par coût, risque, relation et conséquence. |
| Échec local souvent binaire | Réévaluation, plan B et continuité du problème à privilégier. |

## 9. Frontière de systèmes essentielle

```text
QUESTMANAGER
Construit et vérifie le problème du niveau :
conditions, dépendances, ressources, alternatives, solvabilité.

AGENT JOUEUR OU NPC
Perçoit une partie du monde, forme des connaissances,
évalue ses besoins et choisit une stratégie puis un plan.
```

Cette frontière protège l'émergence : le Gestionnaire de quêtes ne résout pas le puzzle à la place des agents, et un agent n'obtient jamais par magie la connaissance de la structure complète.

## 10. Non-objectifs actuels

- Simuler exhaustivement la psychologie humaine ou toutes les émotions de Plutchik.
- Construire un jeu de combat complet avant de prouver le puzzle social et informationnel.
- Générer procéduralement l'intégralité de la fiction, des niveaux et des comportements dès le départ.
- Remplacer la lecture du monde par un tableau de quêtes omniscient.
- Multiplier les jauges de survie si elles ne créent pas de choix intéressants.

## 11. Premiers critères du vertical slice

Le premier niveau de référence doit démontrer les piliers sans dépendre d'une IA ou d'une génération procédurale complète.

| Élément à prouver | Critère observable |
|---|---|
| Dépendance | La sortie requiert au moins deux conditions différentes. |
| Alternative | Au moins une condition dispose de deux voies réellement distinctes. |
| Information | Une information détenue ou découverte change le plan possible. |
| NPC autonome | Un NPC poursuit un objectif simple et peut déplacer une ressource ou en parler. |
| Coopération | Aider ou échanger avec le NPC rend une voie plus accessible sans être la seule solution. |
| Pression | Une ressource de survie ou un danger rend l'ordre des décisions important. |
| retour d'information | Le joueur comprend les préconditions manquantes et la cause d'un refus ou d'un changement. |

## 12. Risques de design à surveiller

| Risque | Symptôme | Garde-fou |
|---|---|---|
| Graphe illisible | Le joueur ne sait plus quelle information manque ni pourquoi. | Rendre les états et flux lisibles, fournir des indices recoupables. |
| Autonomie frustrante | Un NPC prend un élément critique sans solution visible. | Garanties de continuité, mémoire de localisation, alternatives et feedback. |
| Social opaque | Le joueur subit des refus qu'il ne peut pas interpréter. | Motifs observables, actes vérifiables et dimensions relationnelles limitées. |
| Simulation disproportionnée | Le comportement paraît aléatoire ou impossible à tester. | Commencer par peu de variables et d'actions. |
| Pression punitive | La survie interrompt la résolution plutôt que de créer un choix. | Coûts annoncés, cadence maîtrisée, voies de récupération. |
| Procéduralité sans intention | Les niveaux produisent des chaînes fonctionnelles mais sans dilemme ni rythme. | Gabarits de situations, validation de variété et tension authored. |

## 13. Questions à valider avant promotion CANON

Les décisions de vision sont clôturées par DR-01. Perspective, temporalité, survie, psychologie, confiance, hazards, violence, mortalité, vertical slice et architecture restent hors périmètre canonique de DR-01.

## 14. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | Sortie verrouillée, objets, codes, énergie et échanges entre personnages. | S01, S03 | 🟠 |
| Historique | Décomposition GOAP de `Find Exit`. | S02 | 🟠 |
| Historique | Boucle cognitive et capacités communes des agents. | S05 | 🟠 |
| Actuel | Séparation souhaitée entre problème construit par Gestionnaire de quêtes et résolution recherchée par agents. | S08 | 🟡 |
| Actuel | Cette section consolide la vision sans promouvoir les propositions assistant au statut CANON. | Audit Phase 1 | 🟡 |

## Propagation DR-07

**🟢 CANON :** EXIT est aussi une expérience sociale grandeur nature, observée comme un spectacle. Des Observers/Overseers invisibles peuvent orienter l'expérience par des interventions autorisées ; le public ne modifie jamais directement le Monde. Toute intervention préserve une voie de résolution via la garde de solvabilité.

**🟢 CANON :** parité d'information et parité expérientielle : autant que possible, NPC et joueur humain partagent les règles d'acquisition, conservation et continuité naturelle de l'information et de l'expérience. EXIT réinitialise l'épreuve, pas ceux qui l'ont vécue.
