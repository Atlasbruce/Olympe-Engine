# Guide Olympe Blueprint Editor - Français

> **⚠️ DOCUMENTATION DÉPLACÉE**  
> Ce fichier a été réorganisé. Veuillez consulter la nouvelle structure de documentation à:
> - **Nouvel emplacement**: [Documentation/01-Getting-Started/GUIDE_FRANCAIS.md](Documentation/01-Getting-Started/GUIDE_FRANCAIS.md)
> - **Hub de documentation**: [Documentation/README.md](Documentation/README.md)
>
> Ce fichier est conservé pour la compatibilité mais peut ne pas être mis à jour.

---

## Vue d'ensemble du Projet

Le système Olympe Blueprint Editor permet de créer et éditer des entités de jeu en JSON, sans modifier le code C++. Ce système facilite grandement le travail des game designers et accélère le prototypage.

## Ce qui a été Implémenté (Phase 1 - Terminée)

### 1. Système de Blueprint d'Entité

Un blueprint d'entité est un fichier JSON qui définit complètement une entité avec tous ses composants et leurs propriétés.

**Structure de base:**
```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "NomDeLEntité",
    "description": "Description de l'entité",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 100.0, "y": 200.0, "z": 0.0}
            }
        }
    ]
}
```

### 2. Composants Supportés (Phase 1)

#### Composants de Base
- **Position**: Position 2D/3D de l'entité dans le monde
- **BoundingBox**: Rectangle de collision pour les interactions physiques
- **VisualSprite**: Représentation visuelle (sprite, texture)

#### Composants de Physique
- **Movement**: Direction et vélocité du mouvement
- **PhysicsBody**: Propriétés physiques (masse, vitesse)

#### Composants de Gameplay
- **Health**: Points de vie (actuel et maximum)
- **AIBehavior**: Type de comportement IA (idle, patrol, chase, flee, wander)
- **TriggerZone**: Zone de détection autour de l'entité
- **Inventory**: Liste d'objets possédés par l'entité

#### Composants Visuels/Audio
- **Animation**: Contrôle des animations sprite
- **AudioSource**: Sons et effets sonores
- **FX**: Effets visuels (particules, explosions)

#### Composants d'Entrée
- **Controller**: Gestion des contrôleurs/manettes
- **PlayerController**: États d'entrée du joueur

#### Autres Composants
- **Camera**: Paramètres de caméra
- **NPC**: Données spécifiques aux PNJ

### 3. API C++ pour les Blueprints

**Créer un blueprint:**
```cpp
#include "EntityBlueprint.h"
using namespace Olympe::Blueprint;

// Créer une nouvelle entité
EntityBlueprint entity("MonEntité");
entity.description = "Une entité personnalisée";

// Ajouter des composants avec les helpers
entity.AddComponent("Position", 
    CreatePositionComponent(100.0f, 200.0f).properties);
entity.AddComponent("BoundingBox", 
    CreateBoundingBoxComponent(0, 0, 32, 32).properties);
entity.AddComponent("VisualSprite",
    CreateVisualSpriteComponent("sprite.png", 0, 0, 32, 32).properties);

// Sauvegarder
entity.SaveToFile("Blueprints/mon_entite.json");
```

**Charger et modifier un blueprint:**
```cpp
// Charger depuis un fichier
EntityBlueprint entity = EntityBlueprint::LoadFromFile("Blueprints/mon_entite.json");

// Accéder aux composants
if (auto* pos = entity.GetComponent("Position"))
{
    float x = pos->properties["position"]["x"];
    // Modifier
    pos->properties["position"]["x"] = 500.0f;
}

// Ajouter un nouveau composant
entity.AddComponent("Health", CreateHealthComponent(100, 100).properties);

// Sauvegarder les modifications
entity.SaveToFile("Blueprints/mon_entite.json");
```

## Fichiers Créés

### Code Source
- `OlympeBlueprintEditor/include/EntityBlueprint.h` - Définitions des classes
- `OlympeBlueprintEditor/src/EntityBlueprint.cpp` - Implémentation
- `OlympeBlueprintEditor/src/blueprint_test.cpp` - Programme de test

### Exemples
- `Blueprints/example_entity_simple.json` - Entité simple (3 composants)
- `Blueprints/example_entity_complete.json` - Entité complète (11 composants)

### Documentation
- `Blueprints/README.md` - Documentation technique du système
- `DEVELOPMENT_PLAN.md` - Plan de développement complet (12 semaines, 5 sprints)
- `CLICKUP_TASKS.md` - Tâches formatées pour import dans ClickUp
- `GUIDE_FRANCAIS.md` - Ce guide en français

## Plan de Développement Complet

Le développement est organisé en 5 sprints sur 12 semaines:

### Sprint 1 (2 semaines) - ✅ TERMINÉ
**Fondations - Système de Blueprint d'Entité**
- Schéma JSON pour entités
- Sérialisation/désérialisation
- Helpers de création de composants
- Exemples et tests

**Points d'histoire**: 13 points
**Statut**: ✅ 100% complété

### Sprint 2 (2 semaines) - 📋 À FAIRE
**Système Complet de Propriétés**
- Mapper TOUS les composants ECS vers JSON
- Système de validation des propriétés
- Bibliothèque de blueprints prédéfinis (Player, NPC, Ennemi, etc.)
- Intégration avec le PrefabFactory du moteur

**Points d'histoire**: 21 points
**Tâches clés**:
- 5 points: Mapper tous les composants ECS
- 5 points: Système de validation
- 3 points: Bibliothèque de prefabs
- 8 points: Intégration moteur

### Sprint 3 (3 semaines) - 📋 À FAIRE
**Système de Comportements/IA**
- Architecture du système de behaviors
- Briques de comportement prédéfinies:
  - **Move**: Déplacement vers position/direction
  - **Detect**: Détection d'entités
  - **Follow**: Suivre une cible
  - **Patrol**: Patrouille entre points
  - **Flee**: Fuir
  - **Attack**: Attaquer
  - **Idle**: Repos/attente
- Schéma JSON pour behaviors
- Système de conditions et déclencheurs
- Connexion behaviors ↔ propriétés d'entités

**Points d'histoire**: 29 points
**Tâches clés**:
- 5 points: Architecture du système
- 8 points: Implémentation des behaviors de base
- 3 points: Schéma JSON behaviors
- 5 points: Système de conditions
- 5 points: Property binding
- 3 points: Exemples de behaviors

### Sprint 4 (3 semaines) - 📋 À FAIRE
**Interface d'Édition Visuelle**
- Design UI/UX de l'éditeur
- Interface de base avec ImGui
- Property Inspector (édition des propriétés)
- Component Panel (ajout/suppression de composants)
- Éditeur graphique de behaviors (avec ImNodes)
- Preview visuel de l'entité
- Import/Export de fichiers

**Points d'histoire**: 47 points
**Tâches clés**:
- 5 points: Design UI/UX
- 8 points: Interface de base
- 8 points: Property Inspector
- 5 points: Component Panel
- 13 points: Éditeur graphique behaviors
- 5 points: Preview entité
- 3 points: Import/Export

### Sprint 5 (2 semaines) - 📋 À FAIRE
**Intégration et Polish**
- Tests d'intégration complets
- Documentation utilisateur
- Amélioration de l'expérience utilisateur
- Optimisation et correction de bugs
- Préparation du release

**Points d'histoire**: 26 points

## Utilisation Actuelle (Phase 1)

### Compiler le Programme de Test

```bash
cd /chemin/vers/Olympe-Engine
g++ -std=c++17 -I. -I./Source/third_party \
    -o blueprint_test \
    OlympeBlueprintEditor/src/EntityBlueprint.cpp \
    OlympeBlueprintEditor/src/blueprint_test.cpp
```

### Exécuter les Tests

```bash
./blueprint_test
```

Le programme teste:
1. Création et sauvegarde de blueprints
2. Chargement et modification de blueprints
3. Affichage d'un blueprint complet avec tous les composants

### Créer votre Premier Blueprint

1. **Copier un exemple**:
   ```bash
   cp Blueprints/example_entity_simple.json Blueprints/mon_entite.json
   ```

2. **Éditer le JSON**:
   ```json
   {
       "schema_version": 1,
       "type": "EntityBlueprint",
       "name": "MonHeros",
       "description": "Le personnage principal du jeu",
       "components": [
           {
               "type": "Position",
               "properties": {
                   "position": {"x": 0.0, "y": 0.0, "z": 0.0}
               }
           },
           {
               "type": "BoundingBox",
               "properties": {
                   "boundingBox": {"x": 0, "y": 0, "w": 32, "h": 48}
               }
           },
           {
               "type": "VisualSprite",
               "properties": {
                   "spritePath": "Resources/hero.png",
                   "srcRect": {"x": 0, "y": 0, "w": 32, "h": 48},
                   "hotSpot": {"x": 16, "y": 24}
               }
           },
           {
               "type": "Health",
               "properties": {
                   "currentHealth": 100,
                   "maxHealth": 100
               }
           },
           {
               "type": "PlayerController",
               "properties": {
                   "isJumping": false,
                   "isShooting": false,
                   "isRunning": false
               }
           }
       ]
   }
   ```

3. **Charger dans votre code**:
   ```cpp
   EntityBlueprint hero = EntityBlueprint::LoadFromFile("Blueprints/mon_entite.json");
   // Utiliser le blueprint pour créer une entité dans le moteur
   ```

## Exemples de Cas d'Usage

### Créer un Ennemi Simple

```json
{
    "name": "GoblinBasic",
    "description": "Ennemi goblin de base",
    "components": [
        {"type": "Position", "properties": {"position": {"x": 0, "y": 0, "z": 0}}},
        {"type": "BoundingBox", "properties": {"boundingBox": {"x": 0, "y": 0, "w": 24, "h": 32}}},
        {"type": "VisualSprite", "properties": {
            "spritePath": "Resources/goblin.png",
            "srcRect": {"x": 0, "y": 0, "w": 24, "h": 32},
            "hotSpot": {"x": 12, "y": 16}
        }},
        {"type": "Health", "properties": {"currentHealth": 50, "maxHealth": 50}},
        {"type": "AIBehavior", "properties": {"behaviorType": "patrol"}},
        {"type": "Movement", "properties": {
            "direction": {"x": 1, "y": 0},
            "velocity": {"x": 0, "y": 0}
        }},
        {"type": "PhysicsBody", "properties": {"mass": 1.0, "speed": 30.0}}
    ]
}
```

### Créer un PNJ Marchand

```json
{
    "name": "Marchand",
    "description": "PNJ marchand qui vend des objets",
    "components": [
        {"type": "Position", "properties": {"position": {"x": 500, "y": 300, "z": 0}}},
        {"type": "BoundingBox", "properties": {"boundingBox": {"x": 0, "y": 0, "w": 32, "h": 48}}},
        {"type": "VisualSprite", "properties": {
            "spritePath": "Resources/merchant.png",
            "srcRect": {"x": 0, "y": 0, "w": 32, "h": 48}
        }},
        {"type": "NPC", "properties": {"npcType": "vendor"}},
        {"type": "Inventory", "properties": {
            "items": ["potion_health", "potion_mana", "sword_iron"]
        }},
        {"type": "TriggerZone", "properties": {
            "radius": 50.0,
            "triggered": false
        }}
    ]
}
```

## Prochaines Étapes

### Pour les Développeurs

1. **Sprint 2 - Système Complet**:
   - Implémenter les helpers pour TOUS les composants ECS
   - Créer le système de validation
   - Intégrer avec le PrefabFactory

2. **Sprint 3 - Behaviors**:
   - Implémenter les briques de comportement
   - Créer le système de graph pour les behaviors
   - Permettre la composition de behaviors complexes

3. **Sprint 4 - UI Visuelle**:
   - Créer l'éditeur graphique avec ImGui
   - Implémenter le drag & drop
   - Ajouter la preview en temps réel

### Pour les Game Designers

1. **Maintenant** (avec Phase 1):
   - Créer des blueprints JSON manuellement
   - Tester différentes configurations de composants
   - Documenter les entités du jeu

2. **Sprint 2**:
   - Utiliser la bibliothèque de prefabs
   - Créer des variations d'entités
   - Tester les behaviors prédéfinis

3. **Sprint 4**:
   - Utiliser l'éditeur visuel
   - Créer des behaviors complexes graphiquement
   - Itérer rapidement sur le design

## Import dans ClickUp

Le fichier `CLICKUP_TASKS.md` contient toutes les tâches formatées pour ClickUp:

1. Ouvrir ClickUp
2. Créer une nouvelle Liste "Olympe Blueprint Editor"
3. Pour chaque Story dans `CLICKUP_TASKS.md`:
   - Créer une nouvelle tâche
   - Copier le titre et la description
   - Définir les points d'histoire
   - Assigner au sprint correspondant
   - Ajouter les tags
   - Créer les sous-tâches (checklist)

## Support et Questions

Pour toute question ou problème:
1. Consulter `Blueprints/README.md` pour la documentation technique
2. Consulter `DEVELOPMENT_PLAN.md` pour le plan détaillé
3. Examiner les exemples dans `Blueprints/example_*.json`
4. Exécuter `blueprint_test` pour voir le système en action

## Licence et Crédits

Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

Ce système de blueprint fait partie du moteur Olympe Engine V2.

---

**Version**: 1.0 (Phase 1 complétée)
**Date**: Décembre 2025
**Statut**: Système de base fonctionnel, prêt pour Sprint 2
