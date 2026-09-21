# EXIT — Revue de conception DR-03
## Frontières : Monde, Quête, Gestionnaire de partie et Agent

**Statut :** 🟢 REVIEW COMPLETED  
**Objet :** définir les responsabilités conceptuelles entre Gestionnaire de quêtes, Gestionnaire de partie, Monde/ECS et IA NPC/tableau noir, sans définir l'architecture C++, les composants ECS, GOAP ou l'anti-blocage.

## 1. Point de départ à auditer

```text
GESTIONNAIRE DE QUÊTES : « Je construis le problème. »
        ↓ graphe de quête
GESTIONNAIRE DE PARTIE : « J'orchestre et évalue la partie. »
        ↓ orchestration
MONDE / ECS : « Je matérialise et simule la réalité autoritative. »
        ↓ perception
IA NPC / TABLEAU NOIR : « Je représente localement, j'évalue et j'agis. »
        ↓ actions
MONDE → événements / changements d'état → GESTIONNAIRE DE PARTIE
```

Ce schéma est une hypothèse de travail, pas une architecture canonique déjà adoptée.

## 2. Responsabilités proposées à examiner

| Domaine | Responsabilité principale proposée | Données autoritatives | Ce qu'il ne doit pas décider |
|---|---|---|---|
| Gestionnaire de quêtes | Construire le graphe du problème : quêtes, objectifs, conditions, relations et dépendances. | Graphe de problème généré et ses métadonnées de construction. | Buts, stratégies, plans ou actions des NPC ; comportement de simulation. |
| Gestionnaire de partie | Orchestrer la session via règles et mode de jeu ; suivre progression, branches et fins de partie. | État de session, interprétation des événements selon le mode, résultat de l'évaluation de branche. | Instanciation détaillée, simulation physique, cognition agent. |
| Monde / ECS | Instancier, matérialiser et simuler les entités, états et interactions. | État autoritatif courant du Monde et événements factuels. | Signification de victoire/défaite, buts des agents, structure narrative d'une quête. |
| IA NPC / tableau noir | Percevoir, mémoriser, connaître, évaluer, choisir et exécuter localement. | État interne et représentation locale du NPC. | Vérité globale, décision de fin de partie, modification directe arbitraire du graphe de quête. |

## 3. Matrice conceptuelle de lecture / écriture

`L` = lecture ; `E` = écriture ; `Év.` = reçoit ou produit un événement ; `—` = interdit ou sans responsabilité directe. La matrice est à valider, non à implémenter telle quelle.

| Ressource | Gestionnaire de quêtes | Gestionnaire de partie | Monde / ECS | IA NPC / tableau noir |
|---|---:|---:|---:|---:|
| Graphe de quête | E/L | L | L limitée ou aucune | — par défaut |
| État autoritatif du Monde | L pour construction/validation | L | E/L | L seulement via perception ou interfaces autorisées |
| État de session / mode de jeu | — | E/L | L si règles d'exécution nécessaires | L seulement si exposé au joueur/NPC |
| Tableau noir d'un NPC | — | — ou L diagnostic | — sauf alimentation de signaux | E/L par son NPC |
| Entités et instanciation | Décrit les besoins | Demande / orchestre | E/L | — |
| Événements factuels | — | Reçoit | Produit | Reçoit localement ; produit des tentatives d'action |
| Satisfaisabilité de branche | Décrit les conditions | Évalue / enregistre | Fournit les faits | — ; peut inférer localement |

## 4. Événements et exemples de frontière

| Cas EXIT | Fait du Monde | Lecture attendue |
|---|---|---|
| Deux opérateurs nécessaires, tous les NPC morts | `AvailableOperators < 2` | Le Monde énonce le fait ; le Gestionnaire de partie constate qu'une condition de branche est irréalisable ; le NPC survivant peut l'ignorer. |
| Outils détruits par un feu | Outils absents ou détruits | Le Monde produit l'événement ; le Gestionnaire de partie évalue les branches ; la politique de récupération relève de DR-07. |
| NPC croit le générateur en salle 2 | Générateur réellement en salle 5 | Le tableau noir conserve une connaissance locale erronée ; aucune écriture dans le Monde ne corrige la réalité. |
| NPC franchit EXIT | Franchissement factuel | Le Monde le simule ; le Gestionnaire de partie applique la signification coopérative ou compétitive selon le mode. |

## 5. Arbitrages DR-03 proposés

### DR03-D01 — Qui déclenche et réalise l'instanciation initiale ?

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Gestionnaire de quêtes instancie directement. | Chemin court. | Le constructeur du problème devient responsable de simulation. |
| B. Gestionnaire de quêtes décrit ; Gestionnaire de partie orchestre ; Monde instancie. | Séparation lisible des intentions, session et réalité. | Contrats de demande à préciser. |
| C. Monde lit directement le graphe et s'instancie. | Moins d'intermédiaires. | Couplage Monde–graphe et responsabilité ambiguë. |

**Recommandation Work : B.**

### DR03-D02 — Qui évalue la satisfaisabilité d'une branche ?

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Gestionnaire de quêtes seul. | Le graphe est à proximité. | Le constructeur devient moniteur permanent de partie. |
| B. Gestionnaire de partie, à partir du graphe et de l'état autoritatif. | Correspond au rôle d'évaluation de session. | Nécessite une interface de faits claire. |
| C. Chaque NPC. | Forte autonomie locale. | Aucun constat objectif de défaite ou de branche perdue. |

**Recommandation Work : B.**

### DR03-D03 — Le Gestionnaire de quêtes modifie-t-il le graphe pendant la partie ?

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Graphe immuable après construction. | Raisonnement et diagnostic simples. | Peu de marge pour des situations dynamiques. |
| B. Graphe modifiable uniquement par des transformations explicitement tracées. | Autorise évolution contrôlée. | Demande une sémantique de mutation. |
| C. Graphe librement modifiable par tout système. | Flexible à court terme. | Perte d'autorité et de débogabilité. |

**Recommandation Work : B, avec journalisation conceptuelle.**

### DR03-D04 — Accès de l'IA NPC à l'état du Monde

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Lecture directe générale. | IA simple. | Omniscience accidentelle ; viole DR01-D02. |
| B. Perception et interfaces bornées ; tableau noir privé. | Respecte connaissance partielle et feedback. | Nécessite de définir les canaux plus tard. |
| C. Copie exhaustive périodique. | Pratique pour un prototype. | Omniscience différée et données obsolètes indistinctes. |

**Recommandation Work : B.**

### DR03-D05 — Qui écrit le tableau noir ?

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Toute IA ou système peut l'écrire directement. | Simple localement. | État interne non traçable, couplage élevé. |
| B. Seul le système du NPC écrit ; des événements et perceptions l'alimentent. | Responsabilité privée et claire. | Adaptateurs à définir. |
| C. Le Monde écrit les données opérationnelles, le NPC écrit les données cognitives. | Proche du schéma de données. | Risque de confusion entre alimentation et autorité. |

**Recommandation Work : B.**

### DR03-D06 — Qui décide de la conséquence d'une branche irréalisable ?

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Le Gestionnaire de partie applique directement une défaite. | Déterministe. | Prémature le contrat d'échec. |
| B. Le Gestionnaire de partie signale l'état ; règles/mode choisissent la conséquence. | Sépare constat et signification. | Contrat à formaliser en DR-07. |
| C. Le Monde répare automatiquement la branche. | Évite certains blocages. | Anti-blocage implicite et monde arbitraire. |

**Recommandation Work : B.**

### DR03-D07 — Le Gestionnaire de quêtes connaît-il les NPC individuellement ?

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Oui, il distribue les rôles et solutions. | Mise en scène directe. | Il devient cerveau des NPC. |
| B. Il décrit les contraintes nécessaires, sans intention individuelle. | Préserve l'autonomie des agents. | Les besoins d'incarnation doivent être transmis indirectement. |
| C. Il ne connaît aucun acteur, même comme contrainte. | Très découplé. | Difficile de construire les problèmes multi-agents. |

**Recommandation Work : B.**

### DR03-D08 — Gestionnaire de partie : lecture directe ou événements seulement ?

| Option | Avantages | Risques / conséquence |
|---|---|---|
| A. Événements seulement. | Découplage fort. | Évaluation peut manquer un fait ou une synchronisation. |
| B. Lecture autoritative ciblée + événements. | Vérification robuste et réaction aux changements. | Limites de lecture à documenter. |
| C. Lecture/écriture générale. | Rapide au départ. | God Object probable. |

**Recommandation Work : B.**

## 6. Couplages dangereux et garde-fous

| Risque | Symptôme | Garde-fou à valider |
|---|---|---|
| Gestionnaire de quêtes God Object | Génère, simule, planifie et répare tout. | Limiter son mandat au problème et à ses transformations tracées. |
| Gestionnaire de partie God Object | Écrit directement chaque état et chaque décision. | Orchestration, règles de session et évaluation seulement. |
| Monde narrateur | Le Monde attribue victoire, morale ou but. | Il produit les faits ; les règles donnent le sens. |
| IA omnisciente | NPC planifie vers une ressource non perçue. | Canaux de perception et tableau noir privé. |
| Tableau noir partagé implicite | Un NPC apprend ce qu'un autre sait sans transmission. | Propriété agent par agent, partage explicite. |

## 7. Hors périmètre et tensions reportées

- GOAP, classes C++, composants ECS et format de messages.
- Formules de fiabilité, confiance, besoins et psychologie.
- Politique anti-blocage, défaite, récupération et contrat d'échec.
- Génération procédurale complète, tranche verticale et temporalité.
- La distinction exacte entre données Monde/IA et données jeu/cognitives dans le tableau noir devra être affinée sans casser son caractère privé.

## 8. Registre de revue

| Champ | Valeur |
|---|---|
| Date | 17 septembre 2026 |
| Décisions validées | DR03-D01 à DR03-D08 |
| Options écartées | Les options contraires aux décisions consignées dans la section 9 |
| Tensions reportées | Architecture C++, ECS, GOAP, Failure Contract et politiques anti-blocage |
| Documents à propager | 08, 15, 18, 30 à 33 ; DR-04 |

## 9. Décisions validées

### DR03-D01 — Instanciation

**🟢 CANON :** le Gestionnaire de quêtes décrit la situation et ses exigences ; le Gestionnaire de partie orchestre la matérialisation ; le Monde possède et exécute les opérations d'instanciation. Ni le Gestionnaire de quêtes ni le Gestionnaire de partie n'instancient directement une entité du Monde.

### DR03-D02 — Évaluation des conditions

**🟢 CANON :** le Gestionnaire de quêtes définit graphe et conditions. Le Gestionnaire de partie évalue leur état courant à partir du graphe de quête et de l'état autoritatif du Monde, sans réinventer la logique du graphe.

### DR03-D03 — Graphe de quête et correspondance d'exécution

**🟢 CANON :** le graphe de quête est immuable par défaut après génération. Les événements modifient normalement l'état du Monde, puis la satisfaisabilité des conditions. La correspondance d'exécution du Gestionnaire de partie lie les nœuds de condition aux instances concrètes et peut évoluer sans modifier le graphe. Toute transformation dynamique future du graphe doit être justifiée par le game design.

### DR03-D04 / D05 — Monde, systèmes IA NPC et tableau noir

**🟢 CANON :** le Monde ne lit ni n'écrit directement le tableau noir IA. Les systèmes IA NPC lisent les données Monde/ECS auxquelles ils sont autorisés, les évaluent, puis écrivent les résultats dans le tableau noir privé. Une lecture technique du Monde par un système IA ne produit pas automatiquement une connaissance du NPC.

### DR03-D06 — Branche irréalisable

**🟢 CANON :** le Gestionnaire de partie peut constater objectivement une condition ou branche irréalisable. Ce constat n'est pas automatiquement une défaite : règles de jeu, mode de jeu et Failure Contract déterminent la conséquence. Le Failure Contract reste DR-07.

### DR03-D07 — Gestionnaire de quêtes et NPC

**🟢 CANON :** le Gestionnaire de quêtes ne connaît ni ne manipule les instances concrètes de NPC. Il exprime des exigences abstraites — nombre d'agents, possession initiale, connaissance ou capacité requise. Le Gestionnaire de partie effectue le lien initial avec le Monde et les systèmes IA. Il n'attribue jamais ensuite but, stratégie, plan ou action au NPC.

### DR03-D08 — Monitoring hybride

**🟢 CANON :** le Gestionnaire de partie maintient une correspondance d'exécution des conditions pertinentes et évalue par événements/callbacks pertinents ainsi que par contrôle périodique à basse fréquence. Une cadence telle que 400 ms est un exemple de calibration, non une valeur canonique.
