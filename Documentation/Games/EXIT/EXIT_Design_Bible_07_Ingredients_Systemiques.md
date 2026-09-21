# EXIT Design Bible
## 07 - Ingrédients systémiques

**Statut :** 🟡 WIP  
**Dépendances :** 13 Graphes, 17 Survie, 18 Agents  
**Dernière mise à jour :** 17 septembre 2026

## Intention

Les ingrédients d'EXIT sont des briques simples et combinables. Leur intérêt vient de leurs propriétés, de leur état, de leur emplacement et de leurs relations avec le graphe de dépendances, non d'une longue liste d'objets à usage unique.

## Taxonomie

| Famille | Exemples | Propriétés utiles |
|---|---|---|
| Accès | Clé, badge, carte, code, serrure. | Autorisation, compatibilité, état, propriétaire. |
| Énergie | Batterie, fusible, générateur, câble. | Charge, compatibilité, entrée/sortie, consommation. |
| Outil | Pince, tournevis, levier, outil de forçage. | Tags d'usage, état, risque, durabilité si retenue. |
| Médical | Bandage, medikit, stimulant. | Effet de soin, quantité, urgence, valeur sociale. |
| Survie | Ration, eau, abri. | Satisfaction d'un besoin, rareté, échangeabilité. |
| Information | Code, emplacement, indice, carte, identité. | Source, certitude, fraîcheur, portée, vérification. |
| Contenant | Coffre, casier, vitrine, tiroir. | Accès, contenu, visibilité, état, protection. |
| Machine | Générateur, terminal, pompe, ascenseur. | États, préconditions, entrées, sorties, pannes. |
| Contrôle | Levier, bouton, console, vanne. | Cible, effet, état, délai, danger. |
| Surveillance | Caméra, capteur, haut-parleur, alarme. | Zone, détection, signal, conséquence. |
| Obstacle | Porte, grille, barrière, zone contaminée. | Type de verrou, voie de réponse, coût. |

## Fiche d'ingrédient

| Champ | Description |
|---|---|
| Identité | Type, tags et variante fictionnelle. |
| État | Intact, vide, actif, cassé, verrouillé, consommé, installé. |
| Localisation | Monde, contenant, porteur, stockage, installation. |
| Affordance | Ce qui suggère son usage au joueur. |
| Actions compatibles | Prendre, observer, donner, connecter, réparer, ouvrir. |
| Contribution | Quel nœud du graphe il sert, et avec quelles alternatives. |
| Risque / coût | Ressource consommée, alarme, poids, temps, relation. |
| Provenance | Justification spatiale et fictionnelle. |

## Règles

- Un ingrédient important possède au moins une affordance lisible et une source de knowledge.
- Un objet polyvalent est préférable à une clé arbitraire s'il crée de vraies décisions.
- Les consommables pouvant fermer une branche relèvent de la validation anti-softlock.
- Un ingrédient critique ne peut pas dépendre d'une logique de possession opaque.

## Critère du premier slice

Inclure un ingrédient d'accès, un d'énergie, un médical, une ressource sociale et une information traçable. Cela suffit à prouver la combinatoire de base.

## Questions ouvertes

- Les objets ont-ils une masse, une durabilité ou seulement des états fonctionnels ?
- Le crafting simple apporte-t-il des stratégies réelles ou seulement une couche de friction ?
- Quelles compatibilités doivent être explicites par tags plutôt que conçues au cas par cas ?

