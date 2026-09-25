# EXIT Design Bible
## 08 - État du monde, temps et règles globales

**Statut :** 🟡 WIP  
**Dépendances :** 11 connaissance, 15 Gestionnaire de quêtes, 16 Solvabilité  
**Dernière mise à jour :** 25 septembre 2026

## Définition

Le État du monde est la source de vérité simulée : positions, possessions, portes, machines, dangers, zones et conditions de quête. Il diffère du connaissance de chaque agent, qui n'en est qu'une représentation partielle.

## Domaines d'état

| Domaine | Exemples |
|---|---|
| Spatial | Rooms, connexions, portes, routes, visibilité, zones dangereuses. |
| Physique | Santé, faim, blessure, état d'un objet, énergie. |
| Systémique | Alimentation, alarme, ventilation, pression, état de machine. |
| Possession | Objet dans le monde, porté, stocké, installé, consommé. |
| Social | Relations directionnelles, promesses connues, groupes temporaires. |
| quête | Objectives atteints, branches ouvertes ou fermées, continuité. |
| Narratif | Indices trouvés, événements activés, révélations autorisées. |

## Règles globales

1. Tout changement significatif doit avoir une cause, un effet et un feedback.
2. Les agents agissent sur le État du monde par des Actions avec préconditions et effets.
3. Le État du monde ne divulgue pas ses faits aux agents sans perception, observation ou partage.
4. Le Gestionnaire de quêtes valide les transitions qui touchent les nœuds critiques.
5. Le temps et les dangers modifient l'état, mais aucun changement ne doit invalider silencieusement toutes les voies de progression.

## Temporalité à décider

| Option | Intérêt | question de validation |
|---|---|---|
| Temps réel avec pause | Tension et observation sous pression. | La pause conserve-t-elle l'identité d'EXIT ? |
| Temps réel lent | Flux vivant, accès plus simple. | Suffisant pour planification et UI ? |
| Tour par tour hybride | Lisibilité forte de la simulation. | Compatible avec l'ambiance de fuite ? |

## Questions ouvertes

- Quels événements persistent entre niveaux ?
- Quels paramètres de Recovery, d'évacuation et de présentation de Waiting Room doivent être retenus ?
- Quel modèle temporel permet aux NPC autonomes de rester lisibles ?

## Propagation DR-02

**🟢 CANON :** l'état du monde est l'état autoritatif de la situation simulée dans le Monde. Il ne comprend pas, par principe conceptuel, les représentations privées des agents. Chaque NPC possède un tableau noir IA privé, autoritatif pour son état interne mais potentiellement subjectif ou erroné à propos du Monde. Les responsabilités de lecture, écriture et instanciation sont reportées à DR-03.

## Propagation DR-03

**🟢 CANON :** le Monde possède les opérations d'instanciation et ne lit ni n'écrit directement le tableau noir IA. Les systèmes IA NPC médiatisent toute transformation de données du Monde en état interne d'agent.

## Propagation DR08-09

**🟢 CANON :** le World conserve les corps `UNCONSCIOUS` et `DEAD` dans le Level, expose l'inventaire physique d'un corps mort et produit les transitions physiques telles que `CrossExit START → ESCAPED`. Il ne divulgue jamais ces changements sans mécanisme de perception/communication légitime. Les événements concurrents qui modifient l'Attempt suivent un ordre autoritatif déterministe ; la simultanéité visible ne crée pas de simultanéité logique.
