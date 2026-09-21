# EXIT Design Bible
## 12 - Exploration, carte et découverte

**Statut :** 🟡 WIP  
**Dépendances :** 08 État du monde, 11 connaissance, 13 Graphes, 28 UI UX  
**Dernière mise à jour :** 17 septembre 2026

## Intention

Explorer doit produire du savoir, des options et des coûts. Le joueur ne parcourt pas un labyrinthe pour remplir une carte : il cherche des relations entre lieux, objets, machines, dangers et agents.

## Boucle locale

```text
Repérer une zone ou un signal
    -> s'y déplacer ou interroger un agent
    -> observer une propriété ou une connexion
    -> inscrire un fait dans le knowledge
    -> modifier hypothèse, route ou plan
```

## États de connaissance d'un lieu

| État | Ce que le joueur ou NPC sait |
|---|---|
| Inconnu | Aucune information fiable, ou rumeur sans localisation précise. |
| Repéré | Existence ou direction connue, accès incertain. |
| Visité | Géométrie locale et éléments immédiatement visibles. |
| Observé | États, objets ou mécanismes inspectés. |
| Cartographié | Connexions, portes et annotations utiles identifiées. |
| Ancien | Dernière information connue, susceptible d'avoir changé. |

## Règles

- La découverte d'une zone doit apporter une piste, un risque, une ressource, une information ou un choix.
- Les impasses physiques gagnent une fonction : indice, raccourci futur, ressource, récit ou danger à contourner.
- Les informations rapportées par NPC se distinguent de ce que le joueur a vu.
- Une carte révèle ce qui est raisonnablement connu, jamais tous les secrets ni les positions en temps réel.

## Critère du premier slice

Un indice visuel révèle une route ou une condition, une zone contient une information qui modifie le graphe, et une connaissance devient obsolète après un déplacement de ressource.

