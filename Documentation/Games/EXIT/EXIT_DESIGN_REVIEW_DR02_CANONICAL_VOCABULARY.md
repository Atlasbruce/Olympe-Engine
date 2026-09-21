# EXIT — Revue de conception DR-02 : vocabulaire canonique

**Statut :** 🟢 REVIEW COMPLETED  
**Date :** 17 septembre 2026  
**Participants :** Nicolas, ChatGPT  
**Objet :** fixer les frontières lexicales nécessaires avant DR-03, sans fixer l'architecture C++, les formules, les jauges ou les algorithmes.

## 1. Décisions validées

### DR02-D01 — État du monde et tableau noir IA

**🟢 CANON.** L'**État du monde** est l'état autoritatif de la situation simulée dans le **Monde**. Le Monde possède les entités, éléments instanciés et systèmes nécessaires à leur simulation et leur interactivité.

Chaque NPC possède un **tableau noir IA privé**. Il est autoritatif pour l'état interne de cet agent, sans rendre ses données automatiquement vraies dans le Monde.

```text
TABLEAU NOIR IA DU NPC
├── Données Monde / IA : santé, position, position désirée, cible courante,
│   navigation et données opérationnelles
└── Données de jeu / cognitives : connaissance, mémoire, besoins, buts,
    personnalité, état psychologique, émotions, relations et confiance
```

Exemple : `Generator.location = Room5` dans le Monde ; le NPC peut posséder la connaissance locale, fortement fiable à ses yeux, que le générateur se trouve en `Room2`. Son état interne est alors vrai pour le NPC, sa représentation du Monde est factuellement erronée.

### DR02-D02 — Perception et observation

**🟢 CANON.** La **perception** est la réception locale d'un signal accessible à l'agent. L'**observation** est une acquisition attentive, dirigée ou située qui produit une information exploitable. Aucune ne donne automatiquement accès à la vérité globale du Monde. Une expérience directe procure habituellement une forte fiabilité subjective, jamais une certitude absolue.

### DR02-D03 / D04 — Information, mémoire, connaissance et inférence

**🟢 CANON.**

- **Information** : contenu susceptible d'être perçu, acquis ou transmis.
- **Mémoire** : trace contextualisée d'une expérience passée.
- **Connaissance** : représentation locale, mémorisée, exploitable et révisable qu'un agent possède concernant le Monde.
- **Inférence** : processus qui produit ou révise une connaissance à partir d'informations, observations, mémoires ou connaissances disponibles.

Une connaissance peut être correcte, incorrecte, incomplète, incertaine, obsolète ou contradictoire avec le Monde. Elle porte assez de contexte pour que l'agent évalue subjectivement sa fiabilité. **Croyance (`Belief`) n'est pas introduite comme structure conceptuelle indépendante à ce stade.**

```text
Vérité objective       → Monde
Fiabilité subjective   → agent
```

Une vérification peut renforcer ou contester une connaissance, modifier son statut de vérification et, le cas échéant, faire évoluer la relation/confiance envers sa source.

**🟡 Question conservée :** comment distinguer la fiabilité de l'acquisition et la validité temporelle actuelle ? Une observation directe à `t0` peut être fiable, mais obsolète à `t1` si l'objet a été déplacé. Les variables éventuelles `Reliability` et `Freshness`, ainsi que toute formule, restent WIP.

### DR02-D05 — Besoin et but

**🟢 CANON.** Un **besoin** est une pression interne évaluée par l'IA, qui peut modifier les priorités sans imposer directement une action. Un **but** est un état désiré qu'un agent cherche à atteindre ou maintenir.

```text
État physique bas → évaluation de situation → besoin accru
→ but possible : obtenir un soin → stratégie → plan → actions
```

`ÉTAT ≠ DÉCISION` demeure WIP : besoins, relations et contexte modulent l'évaluation sans décider seuls de l'action.

### DR02-D06 — Quête, objectif et condition

**🟢 CANON.** La structure cible du problème est :

```text
QUÊTE → OBJECTIFS → CONDITIONS
```

- **Quête** : problème systémique structuré correspondant à la situation d'évasion.
- **Objectif** : résultat ou état requis par une quête.
- **Condition** : état ou relation qui doit être satisfait pour permettre un objectif ou une autre condition.

Une condition exprime ce qui doit être vrai ; elle n'instruit jamais un agent. `Tâche` reste **🟠 HISTORIQUE** pour l'ancien modèle `Quête → Objectifs → Tâches` et n'est pas introduite dans la planification agent.

**Principe :** le Gestionnaire de quêtes construit le problème ; les agents cherchent comment le résoudre.

### DR02-D07 — Stratégie, plan et action

**🟢 CANON.** Côté agent :

- **Stratégie** : approche générale choisie pour poursuivre un but.
- **Plan** : organisation ordonnée ou conditionnelle des moyens et actions envisagés.
- **Action** : opération exécutable produisant ou tentant une transition.

`Tâche` n'est pas introduite entre plan et action. GOAP reste hors périmètre algorithmique.

### DR02-D08 — Personnalité, état psychologique et émotion

**🟢 CANON.** La **personnalité** regroupe les dispositions relativement stables ; l'**état psychologique** est une condition interne plus persistante et évolutive ; l'**émotion** est une réponse affective située ou contextuelle. Aucun de ces termes ne détermine seul une action.

### DR02-D09 — Relation et confiance

**🟢 CANON.** `Relation(A→B)` est l'état directionnel du lien de A envers B, distinct de `Relation(B→A)`. `Confiance(A→B)` est l'estimation contextuelle par A de la fiabilité de B, notamment comme source, partenaire, acteur ou engagement.

La confiance n'est ni affection, coopération, sécurité, acceptation, permission, ni seuil comportemental. Une forte confiance ne force pas l'acceptation ; une faible confiance ne force pas le refus.

## 2. Frontières alimentant DR-03

- Le Gestionnaire de quêtes construit le graphe du problème, sans attribuer buts, stratégies, plans ou actions aux NPC.
- Le Gestionnaire de partie orchestre la partie selon les règles et le mode de jeu, observe le Monde, évalue la satisfaisabilité des branches et applique les conséquences de partie.
- Le Monde matérialise et simule la réalité autoritative ; il instancie les éléments requis et produit des événements.
- L'IA NPC perçoit localement, entretient son tableau noir, évalue ses besoins, priorise ses buts, choisit une stratégie, planifie, agit et réévalue.

Le Gestionnaire de partie peut confronter le graphe de quête à l'état autoritatif courant et établir objectivement si une branche est réalisable ; un NPC peut ignorer cette réalité ou se tromper. La politique anti-blocage, la défaite et le contrat d'échec restent DR-07.

Un même événement du Monde peut avoir une signification différente selon les règles et le mode de jeu. Exemple : un NPC franchissant EXIT peut constituer une victoire coopérative ou une défaite compétitive. Les systèmes complets de score, classement, matchmaking et multijoueur restent hors périmètre.

## 3. Éléments explicitement reportés

- Architecture C++, composants ECS et méthode concrète d'instanciation.
- Algorithme GOAP et formule de planification.
- Variables, formule et affichage de fiabilité/fraîcheur des connaissances.
- Formule de confiance, dimensions sociales et jauges psychologiques.
- Politique anti-blocage, contrat d'échec, procéduralité et tranche verticale.

## 4. Registre de la revue

| Champ | Valeur |
|---|---|
| Décisions approuvées | DR02-D01 à DR02-D09 |
| Documents de suivi | Propagation minimale ; DR-03 Frontières Monde/Quête/Gestionnaire de partie/Agent |
| Statut | 🟢 REVIEW COMPLETED |

