# EXIT Design Bible
## 24 - Level design, topologie et génération spatiale

**Statut :** 🟡 WIP  
**Dépendances :** 13 Graphes, 15 Gestionnaire de quêtes, 17 Pression, 28 UI UX  
**Dernière mise à jour :** 17 septembre 2026

## Intention

Le level design transforme le graphe logique en espace exploré. Les salles, corridors, hubs, raccourcis et zones à risque créent le rythme, la lisibilité et les coûts de déplacement. La topologie spatiale ne doit pas devenir une copie de la chaîne de quête.

## Grammaire spatiale

| Espace | Fonction |
|---|---|
| Hub | Repère, rencontre, redistribution de routes et ressources. |
| Corridor | Connexion, tension, écoute, visibilité limitée. |
| Salle système | Puzzle local, machine, câblage, lecture de préconditions. |
| Zone ressource | Choix de possession, stockage, échange ou conflit potentiel. |
| Zone danger | Raccourci, coût, changement d'état ou pression. |
| Point narratif | Indice, mémoire du complexe, révélation ou dilemme. |
| Raccourci | Récompense d'une découverte ou d'une transformation du monde. |

## Règles de lisibilité isométrique

- Une salle indique sa fonction par silhouette, lumière, son, mobilier et flux visibles.
- Câbles, conduites, signaux et portes rendent les relations techniques déductibles.
- Les murs ou objets ne masquent jamais durablement une cible critique ; transparence contextuelle ou découpe de caméra requise.
- Une zone dangereuse annonce son risque avant l'engagement.
- La sortie, ou au moins son signe, sert de repère de but tôt dans le niveau.

## Génération spatiale hybride

1. Choisir ou générer une topologie à partir de modules fiables.
2. Ancrer le graphe de quête dans les rooms selon coût, visibilité et rythme.
3. Vérifier l'accessibilité physique de chaque branche logique.
4. Ajouter raccourcis, alternatives, points de repos et storytelling.
5. Revalider après placement des NPC et dangers.

## Budget de départ

Un niveau de référence peut utiliser : un hub, trois à cinq salles fonctionnelles, une zone de danger, un raccourci potentiel et deux NPC. La lisibilité prime sur l'étendue.

