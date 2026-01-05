# Olympe Blueprint Editor - Plan de Développement

> **⚠️ DOCUMENTATION MOVED**  
> This file has been reorganized. Please see the new documentation structure at:
> - **New Location**: [Documentation/06-Development/DEVELOPMENT_PLAN.md](Documentation/06-Development/DEVELOPMENT_PLAN.md)
> - **Documentation Hub**: [Documentation/README.md](Documentation/README.md)
>
> This file is kept for backwards compatibility but may not be updated.

---

## Vue d'ensemble

Ce document détaille le plan de développement pour l'éditeur Olympe Blueprint, organisé en sprints et tâches pour ClickUp. L'objectif est de créer un éditeur basé sur JSON permettant de définir et éditer des entités avec leurs composants et comportements.

## Architecture du Système

### Composants Principaux

1. **Entity Blueprint** - Définition JSON des entités avec leurs composants
2. **Property Editor** - Système d'édition des propriétés des composants
3. **Behavior System** - Système de comportements/IA prédéfinis
4. **Blueprint Editor UI** - Interface utilisateur pour l'édition visuelle

---

## Sprint 1 : Fondations - Entity Blueprint System (2 semaines)

### Objectif
Créer le système de base pour définir des entités en JSON avec leurs composants (Position, BoundingBox, VisualSprite).

### Milestone
✅ **COMPLETED**: Système de blueprint de base fonctionnel avec sérialisation JSON

### Tâches ClickUp

#### Epic: Foundation - Entity Blueprint Core

**Story 1.1: Définir le schéma JSON pour les entités** ✅ DONE
- **Description**: Créer la structure JSON pour représenter une entité avec ses composants
- **Points**: 3
- **Critères d'acceptation**:
  - [x] Schéma JSON documenté avec exemples
  - [x] Support des composants: Position, BoundingBox, VisualSprite
  - [x] Validation de la structure JSON
  - [x] Fichiers d'exemple créés
- **Livrables**:
  - `EntityBlueprint.h/cpp` - Classes C++ pour les blueprints
  - `example_entity_simple.json` - Exemple simple
  - `Blueprints/README.md` - Documentation du schéma

**Story 1.2: Implémenter la sérialisation JSON** ✅ DONE
- **Description**: Créer les fonctions de conversion JSON ↔ C++ pour les blueprints
- **Points**: 5
- **Critères d'acceptation**:
  - [x] Fonction ToJson() pour exporter un blueprint
  - [x] Fonction FromJson() pour importer un blueprint
  - [x] Gestion des erreurs de parsing
  - [x] Tests unitaires de sérialisation/désérialisation
- **Livrables**:
  - Fonctions de sérialisation dans `EntityBlueprint.cpp`
  - `blueprint_test.cpp` - Programme de test

**Story 1.3: Créer les helpers de création de composants** ✅ DONE
- **Description**: Fonctions utilitaires pour créer facilement des composants courants
- **Points**: 3
- **Critères d'acceptation**:
  - [x] Helpers pour Position, BoundingBox, VisualSprite
  - [x] API simple et intuitive
  - [x] Documentation des helpers
  - [x] Exemples d'utilisation
- **Livrables**:
  - Fonctions `CreateXXXComponent()` dans `EntityBlueprint.cpp`
  - Documentation dans README

**Story 1.4: Créer des exemples de blueprints** ✅ DONE
- **Description**: Créer plusieurs fichiers d'exemple pour démontrer le système
- **Points**: 2
- **Critères d'acceptation**:
  - [x] Exemple simple (3 composants de base)
  - [x] Exemple complet (tous les composants)
  - [x] Documentation de chaque exemple
  - [x] Validation JSON
- **Livrables**:
  - `example_entity_simple.json`
  - `example_entity_complete.json`
  - Documentation dans README

---

## Sprint 2 : Édition Complète des Propriétés (2 semaines)

### Objectif
Étendre le système pour supporter l'édition de TOUS les composants disponibles dans le moteur.

### Milestone
Système complet de propriétés éditable pour tous les composants ECS

### Tâches ClickUp

#### Epic: Complete Property System

**Story 2.1: Mapper tous les composants ECS vers JSON**
- **Description**: Créer la correspondance entre tous les composants C++ et leurs représentations JSON
- **Points**: 5
- **Assigné**: Développeur Backend
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Tous les composants de `ECS_Components.h` supportés
  - [ ] Schéma JSON pour chaque type de composant
  - [ ] Documentation des propriétés de chaque composant
  - [ ] Validation des types de données
- **Composants à mapper**:
  - Movement, PhysicsBody, Health
  - AIBehavior, TriggerZone, Inventory
  - Animation, FX, AudioSource
  - Controller, PlayerController, Camera, NPC
- **Livrables**:
  - Extension de `EntityBlueprint.h/cpp`
  - Helpers pour tous les composants
  - Documentation complète dans README

**Story 2.2: Implémenter le système de validation des propriétés**
- **Description**: Créer un système pour valider les valeurs des propriétés
- **Points**: 5
- **Assigné**: Développeur Backend
- **Priorité**: Moyenne
- **Critères d'acceptation**:
  - [ ] Validation des types (int, float, string, bool)
  - [ ] Validation des plages de valeurs (min/max)
  - [ ] Messages d'erreur descriptifs
  - [ ] Validation automatique au chargement
- **Livrables**:
  - Classe `PropertyValidator`
  - Règles de validation par composant
  - Tests de validation

**Story 2.3: Créer une bibliothèque de blueprints prédéfinis**
- **Description**: Créer des blueprints réutilisables pour les types d'entités courants
- **Points**: 3
- **Assigné**: Game Designer
- **Priorité**: Basse
- **Critères d'acceptation**:
  - [ ] Blueprint pour Player (avec contrôles)
  - [ ] Blueprint pour NPC (avec IA)
  - [ ] Blueprint pour Ennemi (avec combat)
  - [ ] Blueprint pour Objet interactif
  - [ ] Blueprint pour Projectile
- **Livrables**:
  - `Blueprints/prefabs/player.json`
  - `Blueprints/prefabs/npc_vendor.json`
  - `Blueprints/prefabs/enemy_basic.json`
  - `Blueprints/prefabs/item_collectible.json`
  - `Blueprints/prefabs/projectile_arrow.json`

**Story 2.4: Implémenter le chargement de blueprints dans le moteur**
- **Description**: Intégrer le système de blueprints avec le PrefabFactory du moteur
- **Points**: 8
- **Assigné**: Développeur Backend
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Fonction pour charger un blueprint et créer une entité ECS
  - [ ] Conversion automatique JSON → Composants C++
  - [ ] Support du hot-reloading des blueprints
  - [ ] Tests d'intégration avec le moteur
- **Livrables**:
  - Extension de `PrefabFactory` pour les blueprints
  - `BlueprintLoader` class
  - Tests d'intégration

---

## Sprint 3 : Système de Comportements (3 semaines)

### Objectif
Créer un système de comportements/IA prédéfinis (briques de comportement) qui peuvent être attachés aux entités.

### Milestone
Système de behavior blueprints fonctionnel avec briques prédéfinies

### Tâches ClickUp

#### Epic: Behavior & Logic System

**Story 3.1: Définir l'architecture du système de comportements**
- **Description**: Concevoir la structure pour les behaviors et leur intégration avec les entités
- **Points**: 5
- **Assigné**: Architecte Logiciel
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Document d'architecture du système
  - [ ] Schéma de classes pour les behaviors
  - [ ] Définition du cycle de vie des behaviors
  - [ ] Plan d'intégration avec ECS
- **Livrables**:
  - Document `BEHAVIOR_ARCHITECTURE.md`
  - Diagrammes UML
  - Schéma JSON pour behaviors

**Story 3.2: Implémenter les briques de comportement de base**
- **Description**: Créer les behaviors fondamentaux réutilisables
- **Points**: 8
- **Assigné**: Développeur Gameplay
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] **Move**: Déplacement vers une position/direction
  - [ ] **Detect**: Détection d'entités dans un rayon
  - [ ] **Follow**: Suivre une entité cible
  - [ ] **Patrol**: Patrouille entre points
  - [ ] **Flee**: Fuir une entité/position
  - [ ] **Attack**: Attaquer une cible
  - [ ] **Idle**: Attente/repos
  - [ ] Tests pour chaque behavior
- **Behaviors à implémenter**:
  
  **Move Behavior**:
  ```json
  {
      "type": "Move",
      "properties": {
          "targetPosition": {"x": 100, "y": 200},
          "moveSpeed": 50.0,
          "stopDistance": 5.0
      }
  }
  ```
  
  **Detect Behavior**:
  ```json
  {
      "type": "Detect",
      "properties": {
          "detectionRadius": 100.0,
          "targetTags": ["player", "enemy"],
          "onDetect": "TriggerEvent_EnemySpotted"
      }
  }
  ```
  
  **Patrol Behavior**:
  ```json
  {
      "type": "Patrol",
      "properties": {
          "waypoints": [
              {"x": 100, "y": 100},
              {"x": 200, "y": 100},
              {"x": 200, "y": 200}
          ],
          "looping": true,
          "waitTime": 2.0
      }
  }
  ```

- **Livrables**:
  - Classes C++ pour chaque behavior
  - Système de sérialisation JSON
  - Tests unitaires
  - Documentation

**Story 3.3: Créer le schéma JSON pour les behavior blueprints**
- **Description**: Définir comment les behaviors sont stockés en JSON
- **Points**: 3
- **Assigné**: Développeur Backend
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Schéma JSON pour behavior blueprints
  - [ ] Support de la composition de behaviors
  - [ ] Système de conditions/déclencheurs
  - [ ] Exemples de behaviors complexes
- **Structure proposée**:
  ```json
  {
      "schema_version": 1,
      "type": "BehaviorBlueprint",
      "name": "GuardPatrol",
      "description": "Guard patrols and attacks on sight",
      "behaviors": [
          {
              "type": "Patrol",
              "priority": 1,
              "conditions": [],
              "properties": {
                  "waypoints": [...],
                  "looping": true
              }
          },
          {
              "type": "Detect",
              "priority": 2,
              "conditions": [],
              "properties": {
                  "detectionRadius": 150.0,
                  "targetTags": ["player"]
              }
          },
          {
              "type": "Attack",
              "priority": 3,
              "conditions": [
                  {"type": "EntityDetected", "value": true}
              ],
              "properties": {
                  "damage": 10,
                  "attackRange": 30.0
              }
          }
      ]
  }
  ```
- **Livrables**:
  - `BehaviorBlueprint.h/cpp`
  - Documentation du schéma
  - Exemples de behavior blueprints

**Story 3.4: Implémenter le système de conditions et déclencheurs**
- **Description**: Système pour activer/désactiver behaviors selon des conditions
- **Points**: 5
- **Assigné**: Développeur Gameplay
- **Priorité**: Moyenne
- **Critères d'acceptation**:
  - [ ] Système de conditions booléennes
  - [ ] Support des opérateurs logiques (AND, OR, NOT)
  - [ ] Conditions sur les propriétés d'entité
  - [ ] Conditions sur l'état du jeu
- **Types de conditions**:
  - `PropertyCheck`: Vérifie une propriété d'entité
  - `DistanceCheck`: Vérifie la distance à une entité
  - `HealthCheck`: Vérifie les points de vie
  - `TimerCheck`: Vérifie un timer
  - `EventCheck`: Vérifie qu'un événement s'est produit
- **Livrables**:
  - Système de conditions
  - Évaluateur de conditions
  - Tests

**Story 3.5: Connecter les behaviors aux propriétés des entités**
- **Description**: Permettre aux behaviors de lire/modifier les propriétés des composants
- **Points**: 5
- **Assigné**: Développeur Backend
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] API pour accéder aux propriétés depuis un behavior
  - [ ] System de binding property → behavior
  - [ ] Validation des accès aux propriétés
  - [ ] Documentation de l'API
- **Exemple d'utilisation**:
  ```json
  {
      "type": "Move",
      "properties": {
          "targetPosition": {
              "bindTo": "entity.targetEntity.Position.position"
          },
          "moveSpeed": {
              "bindTo": "entity.PhysicsBody.speed"
          }
      }
  }
  ```
- **Livrables**:
  - `PropertyBinding` system
  - Documentation de l'API
  - Exemples de bindings

**Story 3.6: Créer des exemples de behavior blueprints**
- **Description**: Créer des blueprints de comportements pour cas d'usage courants
- **Points**: 3
- **Assigné**: Game Designer
- **Priorité**: Basse
- **Critères d'acceptation**:
  - [ ] Behavior pour garde qui patrouille
  - [ ] Behavior pour ennemi qui chasse le joueur
  - [ ] Behavior pour PNJ marchand
  - [ ] Behavior pour créature qui fuit
  - [ ] Behavior pour tourelle statique
- **Livrables**:
  - `Blueprints/behaviors/guard_patrol.json`
  - `Blueprints/behaviors/enemy_aggressive.json`
  - `Blueprints/behaviors/npc_vendor.json`
  - `Blueprints/behaviors/creature_passive.json`
  - `Blueprints/behaviors/turret_static.json`

---

## Sprint 4 : Interface d'Édition Visuelle (3 semaines)

### Objectif
Créer une interface graphique pour éditer les blueprints visuellement (non-programmeur friendly).

### Milestone
Éditeur visuel fonctionnel pour créer/modifier des blueprints

### Tâches ClickUp

#### Epic: Visual Blueprint Editor

**Story 4.1: Concevoir l'UI de l'éditeur**
- **Description**: Créer les maquettes et le design de l'interface utilisateur
- **Points**: 5
- **Assigné**: UI/UX Designer
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Maquettes Figma/wireframes
  - [ ] Workflow utilisateur documenté
  - [ ] Design system défini
  - [ ] Validation avec les stakeholders
- **Panneaux principaux**:
  - **Entity List**: Liste des entités dans le projet
  - **Component Panel**: Liste des composants disponibles
  - **Property Inspector**: Éditeur de propriétés
  - **Scene Preview**: Aperçu visuel de l'entité
  - **Behavior Graph**: Vue graphique des behaviors
- **Livrables**:
  - Maquettes UI
  - Document de workflow
  - Guide de style

**Story 4.2: Implémenter l'interface de base avec ImGui**
- **Description**: Créer la fenêtre principale et les panneaux de base
- **Points**: 8
- **Assigné**: Développeur Frontend
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Fenêtre principale avec menu
  - [ ] Panneaux redimensionnables
  - [ ] Système de tabs/documents
  - [ ] Thème visuel cohérent
- **Livrables**:
  - Structure UI dans `BlueprintEditor.cpp`
  - Système de panneaux
  - Menu principal

**Story 4.3: Créer le Property Inspector**
- **Description**: Panel pour éditer les propriétés des composants
- **Points**: 8
- **Assigné**: Développeur Frontend
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Éditeurs pour types de base (int, float, string, bool)
  - [ ] Éditeurs pour vecteurs (x, y, z)
  - [ ] Éditeurs pour rectangles (x, y, w, h)
  - [ ] Éditeurs pour listes/arrays
  - [ ] Validation en temps réel
  - [ ] Undo/Redo support
- **Widgets à implémenter**:
  - Drag float pour nombres
  - Color picker pour couleurs
  - File picker pour assets
  - Dropdown pour énumérations
  - Text input multi-ligne
- **Livrables**:
  - `PropertyInspector` class
  - Widgets custom ImGui
  - Tests de l'UI

**Story 4.4: Implémenter le Component Panel**
- **Description**: Panel pour ajouter/supprimer des composants
- **Points**: 5
- **Assigné**: Développeur Frontend
- **Priorité**: Moyenne
- **Critères d'acceptation**:
  - [ ] Liste filtrée des composants disponibles
  - [ ] Bouton "Add Component" avec menu déroulant
  - [ ] Drag & drop pour ajouter des composants
  - [ ] Bouton de suppression par composant
  - [ ] Réorganisation par drag & drop
- **Livrables**:
  - `ComponentPanel` class
  - UI d'ajout/suppression

**Story 4.5: Créer l'éditeur graphique de behaviors (ImNodes)**
- **Description**: Interface graphique pour composer des behaviors
- **Points**: 13
- **Assigné**: Développeur Frontend
- **Priorité**: Moyenne
- **Critères d'acceptation**:
  - [ ] Affichage des behaviors en nœuds
  - [ ] Connexions entre behaviors
  - [ ] Éditeur de conditions visuellement
  - [ ] Système de priorités visible
  - [ ] Zoom/pan de la vue
- **Livrables**:
  - `BehaviorGraphEditor` class
  - Intégration avec ImNodes
  - Sérialisation graph → JSON

**Story 4.6: Implémenter la preview de l'entité**
- **Description**: Aperçu visuel en temps réel de l'entité éditée
- **Points**: 5
- **Assigné**: Développeur Frontend
- **Priorité**: Basse
- **Critères d'acceptation**:
  - [ ] Affichage du sprite
  - [ ] Affichage de la bounding box
  - [ ] Affichage de la trigger zone
  - [ ] Transformation interactive (move, rotate, scale)
  - [ ] Grid/rulers pour positionnement
- **Livrables**:
  - `ScenePreview` class
  - Rendering dans une texture ImGui

**Story 4.7: Ajouter les fonctionnalités d'import/export**
- **Description**: Charger et sauvegarder des blueprints depuis l'UI
- **Points**: 3
- **Assigné**: Développeur Frontend
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Menu File → New, Open, Save, Save As
  - [ ] Dialog de sélection de fichier
  - [ ] Auto-save optionnel
  - [ ] Liste des fichiers récents
  - [ ] Gestion des fichiers non sauvegardés
- **Livrables**:
  - Système de fichiers dans l'éditeur
  - Dialogs ImGui

---

## Sprint 5 : Intégration et Polish (2 semaines)

### Objectif
Finaliser l'intégration avec le moteur et polir l'expérience utilisateur.

### Milestone
Éditeur complet et prêt pour la production

### Tâches ClickUp

#### Epic: Integration & Polish

**Story 5.1: Tests d'intégration avec le moteur**
- **Description**: Valider que les blueprints fonctionnent correctement dans le moteur
- **Points**: 5
- **Assigné**: QA Engineer
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Création d'entités depuis blueprints
  - [ ] Tous les composants fonctionnent correctement
  - [ ] Behaviors s'exécutent comme prévu
  - [ ] Hot-reload des blueprints fonctionne
  - [ ] Performance acceptable
- **Tests à effectuer**:
  - Charger 100+ entités depuis blueprints
  - Modifier un blueprint et recharger
  - Tester tous les types de composants
  - Tester tous les behaviors
  - Test de stress/performance
- **Livrables**:
  - Suite de tests d'intégration
  - Rapport de bugs
  - Documentation des problèmes

**Story 5.2: Créer la documentation utilisateur**
- **Description**: Guide complet pour utiliser l'éditeur
- **Points**: 5
- **Assigné**: Technical Writer
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Guide de démarrage rapide
  - [ ] Tutoriel pas-à-pas
  - [ ] Documentation de référence
  - [ ] Vidéos de démonstration
  - [ ] FAQ et troubleshooting
- **Livrables**:
  - `USER_GUIDE.md`
  - `TUTORIAL.md`
  - `REFERENCE.md`
  - Vidéos tutoriels

**Story 5.3: Améliorer l'expérience utilisateur**
- **Description**: Polish de l'interface et ajout de fonctionnalités de confort
- **Points**: 5
- **Assigné**: Développeur Frontend
- **Priorité**: Moyenne
- **Critères d'acceptation**:
  - [ ] Keyboard shortcuts
  - [ ] Tooltips informatifs
  - [ ] Messages d'erreur clairs
  - [ ] Templates de blueprints
  - [ ] Système de recherche/filtrage
- **Fonctionnalités**:
  - Ctrl+S pour save
  - Ctrl+Z/Y pour undo/redo
  - F2 pour rename
  - Delete pour supprimer
  - Tooltips sur tous les composants
  - Templates: Player, Enemy, NPC, Item, etc.
- **Livrables**:
  - Amélirations UI
  - Documentation des shortcuts

**Story 5.4: Optimisation et correction de bugs**
- **Description**: Optimiser les performances et corriger les bugs connus
- **Points**: 8
- **Assigné**: Toute l'équipe
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Tous les bugs critiques corrigés
  - [ ] Temps de chargement < 1s pour blueprints moyens
  - [ ] Pas de memory leaks
  - [ ] UI réactive (60 FPS min)
- **Livrables**:
  - Code optimisé
  - Bugs corrigés
  - Rapport de performance

**Story 5.5: Préparation pour le release**
- **Description**: Finaliser pour la mise en production
- **Points**: 3
- **Assigné**: Release Manager
- **Priorité**: Haute
- **Critères d'acceptation**:
  - [ ] Build de release stable
  - [ ] Documentation complète
  - [ ] Exemples et templates inclus
  - [ ] Changelog rédigé
  - [ ] Installer/packaging
- **Livrables**:
  - Build de release
  - Package d'installation
  - `CHANGELOG.md`
  - `RELEASE_NOTES.md`

---

## Vue d'ensemble des Sprints

| Sprint | Durée | Focus | Livrables Clés |
|--------|-------|-------|----------------|
| Sprint 1 | 2 sem | Entity Blueprints | Système JSON de base, sérialisation |
| Sprint 2 | 2 sem | Propriétés complètes | Support de tous les composants ECS |
| Sprint 3 | 3 sem | Behaviors/IA | Système de behaviors prédéfinis |
| Sprint 4 | 3 sem | UI Visuelle | Éditeur graphique complet |
| Sprint 5 | 2 sem | Integration & Polish | Release finale |
| **Total** | **12 sem** | **3 mois** | **Éditeur complet** |

## Estimation des Points

- **Total Story Points**: ~140 points
- **Vélocité estimée**: 12 points/semaine
- **Durée totale**: 12 semaines (3 mois)
- **Équipe suggérée**: 3-4 développeurs

## Dépendances et Risques

### Dépendances Techniques
- nlohmann/json (déjà intégré)
- ImGui (pour l'UI)
- ImNodes (pour le graph editor)
- Intégration avec le système ECS existant

### Risques Identifiés
1. **Complexité du système de behaviors**: Peut nécessiter plus de temps que prévu
2. **Performance**: Chargement de nombreux blueprints peut être lent
3. **UI/UX**: Interface complexe, risque d'être peu intuitive
4. **Intégration**: Possible incompatibilité avec le moteur existant

### Mitigation
- Prototypage rapide des features complexes
- Tests de performance réguliers
- Sessions de feedback utilisateur
- Tests d'intégration continus

## Prochaines Étapes

1. ✅ Valider le plan avec l'équipe
2. ✅ Créer les tâches dans ClickUp
3. ⏳ Commencer Sprint 1
4. ⏳ Setup de l'environnement de développement
5. ⏳ Kick-off meeting

---

## Notes

- Ce plan est flexible et peut être ajusté selon les retours et les découvertes durant le développement
- Les points d'histoire sont des estimations et peuvent varier
- La priorité des features peut être réorganisée selon les besoins
- Des sprints de review/retrospective peuvent être ajoutés si nécessaire

---

## Contacts et Responsabilités

- **Product Owner**: [À définir]
- **Scrum Master**: [À définir]
- **Lead Developer**: [À définir]
- **UI/UX Designer**: [À définir]
- **QA Lead**: [À définir]

