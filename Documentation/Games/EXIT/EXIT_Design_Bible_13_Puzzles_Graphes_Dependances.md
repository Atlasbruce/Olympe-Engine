# EXIT Design Bible
## 13 - Puzzles et graphes de dépendances AND OR

**Version :** 0.1  
**Statut :** 🟡 WIP - en attente de validation  
**Dépendances :** 01 Vision, 11 perception mémoire connaissance, 14 quête objectif tâche action, 15 Gestionnaire de quêtes, 16 Solvabilité  
**Dernière mise à jour :** 16 septembre 2026

---

## 1. Définition

Un puzzle EXIT est une transformation du monde qui rend une condition souhaitée atteignable. Le niveau relie ces conditions dans un graphe de dépendances : une sortie verrouillée dépend d'un accès, d'une alimentation, d'une information, d'une machine ou d'une coopération. Les objets, NPC et informations sont des moyens possibles de satisfaire les conditions, jamais de simples décorations.

Cette représentation sert simultanément à la conception des niveaux, au Gestionnaire de quêtes, à la planification des agents et à la validation anti-softlock. Elle ne doit pas imposer au joueur une seule séquence de résolution.

## 2. Intention de design

- Créer des moments de compréhension : le joueur découvre pourquoi un verrou existe et ce qui peut le lever.
- Permettre des stratégies distinctes : chercher, réparer, négocier, contourner, observer, échanger ou prendre un risque.
- Donner une valeur systémique aux informations et aux NPC.
- Préserver la lisibilité malgré un niveau vivant et procédural.
- Rendre la coopération avantageuse sans enfermer le joueur dans une seule relation obligatoire.

## 3. Grammaire du graphe

### Nœuds

| Type de nœud | question | Exemples |
|---|---|---|
| État final | Quelle situation doit devenir vraie ? | `EXIT ouverte`, `secteur évacué`. |
| Condition | Qu'est-ce qui doit être vrai pour agir ? | `porte alimentée`, `code connu`, `NPC stabilisé`. |
| Ressource | Quel élément est consommé, possédé ou installé ? | clé, fusible, badge, ration, batterie. |
| Information | Quelle proposition est nécessaire ou utile ? | code, emplacement, identité, routine de sécurité. |
| action | Quelle transformation peut être tentée ? | réparer, entrer un code, demander, forcer. |
| Agent | Qui possède une capacité, une ressource ou une connaissance ? | Mara connaît un code, Ivo peut réparer. |
| Risque ou coût | Que faut-il accepter pour emprunter une voie ? | alarme, ressource rare, danger, dette, temps. |

### Arêtes

| Relation | Sens | Exemple |
|---|---|---|
| `nécessite` | Le nœud cible exige la source. | Ouvrir EXIT nécessite une clé. |
| `permet` | Le nœud source rend le cible réalisable. | Alimenter l'infirmerie permet d'obtenir le medikit. |
| `contient` | Une ressource est localisée ou détenue. | Le coffre contient la clé. |
| `révèle` | Une action ou information rend un fait connu. | Lire une note révèle un code. |
| `consomme` | Une action retire ou dégrade une ressource. | Forcer une porte consomme un outil. |
| `modifie` | Une action change un état. | Réparer le générateur active le courant. |
| `bloque` | Un danger ou état empêche une route. | Gaz toxique bloque le couloir. |

## 4. Opérateurs AND et OR

### AND : exigences cumulatives

Toutes les conditions doivent être satisfaites.

```text
EXIT ouverte
    AND
    |- clé physique obtenue
    |- alimentation de la porte rétablie
```

Le recours à AND augmente la profondeur d'un problème. Il doit rester lisible : le joueur peut savoir que la porte exige une clé et du courant, même si les moyens d'obtenir ces deux éléments restent à découvrir.

### OR : voies de résolution substituables

Une seule branche valide suffit.

```text
Coffre ouvert
    OR
    |- code confirmé
    |- terminal piraté
    |- outil de forçage, avec alarme
```

Le recours à OR soutient l'improvisation, la coopération volontaire et la robustesse. Une branche alternative n'a pas besoin d'être équivalente : elle peut coûter du temps, une ressource, une réputation, une alarme ou une exposition au danger.

### AND/OR : forme attendue d'un niveau EXIT

```text
EXIT ouverte
    AND
    |- porte alimentée
    |     OR
    |     |- générateur réparé
    |     |- alimentation d'urgence activée
    |
    |- accès autorisé
          OR
          |- clé obtenue
          |     AND
          |     |- coffre ouvert
          |     |- code obtenu ou déduit
          |
          |- verrou piraté
          |- issue de maintenance découverte
```

## 5. Graphe logique et topologie spatiale

Le graphe logique répond à « que faut-il rendre vrai ? ». La topologie répond à « où se trouvent les choses et par quels chemins y accéder ? ». Ils doivent être connectés, mais ne sont pas le même système.

| Élément | Graphe logique | Topologie spatiale |
|---|---|---|
| Générateur | Condition pour alimenter une porte. | Salle technique accessible via un couloir. |
| Medikit | Ressource pour stabiliser Mara. | Infirmerie derrière une porte non alimentée. |
| Mara | Source potentielle du code. | Zone de repos, immobilisée par sa blessure. |
| Sortie | État final. | Porte au hub ou au bord du niveau. |

Cette séparation évite deux problèmes : une carte qui ressemble à une ligne de tâches, et une quête logique qui ignore les coûts de déplacement, de danger et de visibilité.

## 6. Forme d'un verrou

Chaque verrou conçu ou généré doit recevoir une fiche minimale.

| Champ | Description |
|---|---|
| ID et type | Accès, énergie, information, mécanisme, sécurité, social, survie, espace. |
| État bloqué visible | Ce que l'agent constate avant de résoudre. |
| Condition de réussite | L'état final qui lève le verrou. |
| Voie principale | Chemin le plus direct et le plus lisible. |
| Alternatives | Branches OR, leurs préconditions et leurs coûts. |
| Sources de connaissance | Où comprendre l'état, les conditions et les alternatives. |
| Conséquences | Monde, relation, ressource, danger ou narration modifiés. |
| Continuité | Ce qui garantit la progression si un élément ou agent devient indisponible. |

## 7. Exemple de graphe de référence

### Situation

La sortie exige une clé et l'alimentation de la porte. La clé se trouve dans un coffre électronique. Mara connaît le code mais refuse ou ne peut pas aider tant qu'elle est gravement blessée. Le medikit est dans l'infirmerie, elle-même non alimentée.

```text
EXIT ouverte
AND
|- porte EXIT alimentée
|    OR
|    |- générateur réparé
|    |    AND
|    |    |- fusible compatible obtenu
|    |    |- action Réparer réussie
|    |
|    |- alimentation d'urgence activée, coût : batterie rare
|
|- clé EXIT obtenue
     OR
     |- coffre électronique ouvert
     |    OR
     |    |- code obtenu auprès de Mara
     |    |    AND
     |    |    |- Mara soignée
     |    |    |- relation ou échange acceptable
     |    |
     |    |- code lu dans un journal de maintenance
     |    |- coffre forcé, coût : alarme et outil
     |
     |- accès de maintenance découvert, risque : zone dangereuse
```

### Ce que cet exemple doit prouver

- La sortie conserve deux conditions cumulatives simples à lire.
- Le code n'est pas une « serrure humaine » : Mara est une voie, pas l'unique clé.
- Soigner Mara peut être un acte de coopération, un coût de ressource et un moyen d'information.
- La batterie peut résoudre l'urgence ou servir à une autre condition ; ce choix crée un dilemme.
- Le forçage et la maintenance permettent une autonomie coûteuse sans invalider l'intérêt de la coopération.

## 8. Règles de construction

1. Chaque état final possède une expression de réussite explicite.
2. Tout nœud critique possède au moins une source de connaissance accessible avant son exigence forte.
3. Une branche OR doit modifier réellement le coût, le risque, le temps, la relation ou l'espace ; elle ne peut pas être une copie esthétique.
4. Une dépendance sociale critique doit être doublée par une voie de continuité, une récupération ou une alternative validée.
5. Les objets utilisés comme solution doivent avoir une raison fictionnelle et une affordance lisible.
6. Une route dangereuse doit annoncer son danger avant le point de non-retour.
7. Un niveau doit disposer de respirations : toutes les étapes ne peuvent pas introduire simultanément une nouvelle règle, un nouveau danger et une nouvelle relation.
8. Les coûts consommables critiques sont traités comme des nœuds de solvabilité, non comme un simple butin.

## 9. Lisibilité et feedback

| État de puzzle | retour d'information recommandé | À éviter |
|---|---|---|
| Verrou inconnu | Silhouette, signalétique, son, zone visible mais inaccessible. | Une porte sans indication ni piste. |
| Précondition manquante | Panneau, terminal, dialogue, câblage ou journal qui explique le manque. | « Impossible » sans précision. |
| Branche disponible | Indice local, comportement NPC, route visible, outil reconnaissable. | Alternative cachée sans moyen de la découvrir. |
| Conséquence | Changement in-world, journal, son, réaction de NPC. | Modification silencieuse de l'état global. |
| Échec récupérable | Explication de la cause et nouvelle piste ou état persistant. | Réinitialisation sans compréhension. |

## 10. Interactions avec les agents

Un graphe définit le problème. Un agent choisit une solution à partir de ses connaissances, ressources, besoins et relations.

| Situation | Gestionnaire de quêtes / monde | Agent |
|---|---|---|
| Mara détient le code | Connaissance source placée dans le graphe. | Ne peut l'utiliser que s'il sait que Mara est une source et choisit de la contacter. |
| Fusible dans l'atelier | Ressource et emplacement réels. | Peut l'obtenir s'il connaît ou explore l'atelier. |
| Batterie rare | Ressource avec plusieurs arcs de consommation. | Évalue selon son but, sa peur, son plan et la valeur des alternatives. |
| Coffre forcé | Branche OR avec coût et conséquences. | Décide si l'alarme et l'outil valent l'accès immédiat. |

Le Gestionnaire de quêtes ne doit pas favoriser la branche « narrative ». Toute branche validée doit être traitée comme une possibilité réelle ; la personnalité et les connaissances des agents déterminent simplement laquelle devient probable.

## 11. Paramètres à calibrer

- profondeur moyenne des graphes ;
- nombre de verrous principaux par niveau ;
- ratio AND/OR ;
- nombre et nature des alternatives par verrou critique ;
- distance spatiale entre condition, ressource et source d'information ;
- coût des branches risquées ;
- visibilité initiale de la sortie et des sous-objectifs ;
- quantité d'objets polyvalents par niveau ;
- budget de nouveaux concepts introduits par niveau.

## 12. Risques et edge cases

| Risque | Symptôme | Garde-fou |
|---|---|---|
| Chaîne unique déguisée | Les branches OR dépendent toutes de la même ressource perdue. | Vérifier les ressources et préconditions effectives de chaque chemin. |
| Explosion combinatoire | Trop de branches empêchent conception, test et lecture. | Limiter la profondeur et proposer des alternatives significatives, pas exhaustives. |
| Alternative dominante | Une voie est toujours moins coûteuse et moins risquée. | Donner à chaque route un contexte, coût ou contrainte propre. |
| NPC critique indisponible | Mort, fuite ou hostilité rend le puzzle impossible. | Alternative, redondance, relais, récupération ou changement de but. |
| Faux puzzle | Le joueur possède déjà tout mais ne peut pas savoir quoi faire. | Affordances, feedback de préconditions et sources de connaissance. |
| Coût irréversible opaque | Consommation d'une batterie bloque une autre condition sans avertissement. | Signaler les usages concurrents et tester la continuité. |

## 13. Questions à valider

1. Valider que les graphes AND/OR deviennent le format conceptuel principal des puzzles EXIT.
2. Définir le nombre minimal d'alternatives pour un verrou critique : une voie de secours suffit-elle, ou faut-il une redondance plus forte ?
3. Définir quels coûts peuvent rendre une branche valable : temps, santé, alarme, ressource, trust, exposition, réputation.
4. Déterminer si les branches de violence ou de vol appartiennent au premier prototype.
5. Choisir un budget de complexité pour le premier niveau de référence.

## 14. Historique et décisions

| Version | Évolution | Source / raison | Statut |
|---|---|---|---|
| Historique | EXIT verrouillée par clé, code, énergie, coffre et NPC. | S01, S03, S04 | 🟠 |
| Historique | Décomposition linéaire GOAP `Find Exit -> key -> safe -> password`. | S02 | 🟠 |
| Actuel | Formalisation AND/OR pour sortir de la chaîne unique. | S06, S08, Audit Phase 1 | 🟡 |
| Actuel | Distinction entre graphe logique et topologie spatiale. | S08, Audit Phase 1 | 🟡 |
| Futur | Les règles chiffrées de génération et de validation relèvent du Gestionnaire de quêtes et de la solvabilité. | Sections 15 et 16 | 🔵 |
