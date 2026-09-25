# EXIT Design Bible
## 30 - Architecture conceptuelle et frontières d'implémentation

**Statut :** 🟡 WIP  
**Dernière mise à jour :** 17 septembre 2026

## Architecture de référence

```text
QUESTMANAGER
  finalité, graphe, allocation, validation, continuité
      |
      v
WORLD STATE <----> TOPOLOGIE / OBJETS / MACHINES / DANGERS
      |
      v
AGENTS JOUEUR ET NPC
  perception -> memory -> knowledge -> needs/goals
  -> stratégie -> GOAP/plan -> actions
      |
      +-- personality / psychologie / relations modulent les choix
      |
      v
UI ET FEEDBACK
  rendent le monde, les préconditions et les conséquences intelligibles
```

## Frontières

| Concept | Responsabilité conceptuelle |
|---|---|
| État du monde | Vérité simulée et transitions autorisées. |
| Gestionnaire de quêtes | Construction et contrôle de continuité du problème. |
| Agent | Décision locale et exécution à partir du knowledge. |
| GOAP | Recherche de plan agent-side. |
| arbre de comportements | Orchestration d'exécution et routines. |
| UI | Traduction lisible des états visibles, sans omniscience. |

Cette section n'est pas une architecture Olympe Engine, ni une spécification de code. Elle protège les responsabilités avant toute implémentation.

## Propagation DR-02

**🟢 CANON :** le Gestionnaire de quêtes construit le problème ; le Gestionnaire de partie orchestre et évalue la partie ; le Monde matérialise et simule la réalité autoritative ; l'agent perçoit une partie de cette réalité et décide comment agir. Les responsabilités précises, droits de lecture/écriture et événements relèvent de DR-03.

## Propagation DR-03

```text
Gestionnaire de quêtes : définition abstraite du problème
Gestionnaire de partie : correspondance d'exécution + évaluation de session
Monde / ECS : réalité simulée autoritative
Systèmes IA NPC : évaluation et médiation
Tableau noir IA : état privé + représentation locale
```

Le suivi de conditions est hybride (événements pertinents et contrôle périodique basse fréquence). Les fréquences, interfaces et composants ne sont pas définis ici.

## Propagation DR08-08

**🟢 CANON :** entre le Plan cognitif et le World autoritatif, l'Action Runtime distingue Action tentée, Execution courante et conséquences World. Il peut informer la cognition de changements pertinents sans décider à sa place. Les mutations d'inventaire atomiques sont possédées par le système autoritatif d'inventaire/échange au moment de l'**Inventory Commit**. Les classes, API, structures de séquence et lifecycle technique restent 🟡 WIP.

## Propagation DR08-09

**🟢 CANON :** le World produit les transitions physiques telles que `CrossExit START → ESCAPED`; le GameManager et le Game Mode en interprètent les outcomes d'Attempt. Le timer d'évacuation est représenté dans le World avant perception/connaissance du participant. Les événements concurrents sont ordonnés de façon autoritative et déterministe ; aucune simultanéité visuelle ne donne accès à une vérité concurrente.

## Propagation DR-04

**🟢 CANON :** le Monde ne met jamais à jour directement le tableau noir. Les systèmes IA transforment des données Monde accessibles en état interne exploitable. Le tableau noir reste privé au domaine IA ; sa structure détaillée demeure hors périmètre.

## Propagation DR-07

**🟢 CANON :** QueueManager transporte les événements/messages des systèmes vers le Gestionnaire de partie. Celui-ci sélectionne les événements significatifs pour une trace factuelle et un débrief ; il ne produit pas les événements, ne génère pas de puzzle et ne fournit pas de preuve globale de solvabilité. Observer demande une intervention, le Gestionnaire de quêtes transforme le problème si nécessaire, le Monde matérialise.
