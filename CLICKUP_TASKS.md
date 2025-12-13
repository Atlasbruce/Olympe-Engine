# Tâches ClickUp - Olympe Blueprint Editor

Ce fichier contient toutes les tâches formatées pour être importées dans ClickUp.

---

## Sprint 1 - Entity Blueprint System

### 📋 Story 1.1: Définir le schéma JSON pour les entités
- **Type**: Story
- **Points**: 3
- **Priorité**: Haute
- **Status**: ✅ DONE
- **Sprint**: Sprint 1
- **Epic**: Foundation - Entity Blueprint Core
- **Description**: Créer la structure JSON pour représenter une entité avec ses composants
- **Critères d'acceptation**:
  - [x] Schéma JSON documenté avec exemples
  - [x] Support des composants: Position, BoundingBox, VisualSprite
  - [x] Validation de la structure JSON
  - [x] Fichiers d'exemple créés
- **Tags**: foundation, json-schema, documentation

### 📋 Story 1.2: Implémenter la sérialisation JSON
- **Type**: Story
- **Points**: 5
- **Priorité**: Haute
- **Status**: ✅ DONE
- **Sprint**: Sprint 1
- **Epic**: Foundation - Entity Blueprint Core
- **Description**: Créer les fonctions de conversion JSON ↔ C++ pour les blueprints
- **Critères d'acceptation**:
  - [x] Fonction ToJson() pour exporter un blueprint
  - [x] Fonction FromJson() pour importer un blueprint
  - [x] Gestion des erreurs de parsing
  - [x] Tests unitaires de sérialisation/désérialisation
- **Tags**: backend, serialization, cpp

### 📋 Story 1.3: Créer les helpers de création de composants
- **Type**: Story
- **Points**: 3
- **Priorité**: Moyenne
- **Status**: ✅ DONE
- **Sprint**: Sprint 1
- **Epic**: Foundation - Entity Blueprint Core
- **Description**: Fonctions utilitaires pour créer facilement des composants courants
- **Critères d'acceptation**:
  - [x] Helpers pour Position, BoundingBox, VisualSprite
  - [x] API simple et intuitive
  - [x] Documentation des helpers
  - [x] Exemples d'utilisation
- **Tags**: backend, api, utilities

### 📋 Story 1.4: Créer des exemples de blueprints
- **Type**: Story
- **Points**: 2
- **Priorité**: Basse
- **Status**: ✅ DONE
- **Sprint**: Sprint 1
- **Epic**: Foundation - Entity Blueprint Core
- **Description**: Créer plusieurs fichiers d'exemple pour démontrer le système
- **Critères d'acceptation**:
  - [x] Exemple simple (3 composants de base)
  - [x] Exemple complet (tous les composants)
  - [x] Documentation de chaque exemple
  - [x] Validation JSON
- **Tags**: documentation, examples

---

## Sprint 2 - Complete Property System

### 📋 Story 2.1: Mapper tous les composants ECS vers JSON
- **Type**: Story
- **Points**: 5
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 2
- **Epic**: Complete Property System
- **Assigné**: Développeur Backend
- **Description**: Créer la correspondance entre tous les composants C++ et leurs représentations JSON
- **Sous-tâches**:
  - [ ] Mapper Movement, PhysicsBody, Health
  - [ ] Mapper AIBehavior, TriggerZone, Inventory
  - [ ] Mapper Animation, FX, AudioSource
  - [ ] Mapper Controller, PlayerController, Camera, NPC
  - [ ] Créer helpers pour chaque composant
  - [ ] Documenter les propriétés de chaque composant
- **Tags**: backend, ecs, mapping

### 📋 Story 2.2: Implémenter le système de validation des propriétés
- **Type**: Story
- **Points**: 5
- **Priorité**: Moyenne
- **Status**: ⏳ TODO
- **Sprint**: Sprint 2
- **Epic**: Complete Property System
- **Assigné**: Développeur Backend
- **Description**: Créer un système pour valider les valeurs des propriétés
- **Sous-tâches**:
  - [ ] Créer classe PropertyValidator
  - [ ] Implémenter validation des types (int, float, string, bool)
  - [ ] Implémenter validation des plages (min/max)
  - [ ] Créer messages d'erreur descriptifs
  - [ ] Intégrer validation au chargement
  - [ ] Écrire tests de validation
- **Tags**: backend, validation, error-handling

### 📋 Story 2.3: Créer une bibliothèque de blueprints prédéfinis
- **Type**: Story
- **Points**: 3
- **Priorité**: Basse
- **Status**: ⏳ TODO
- **Sprint**: Sprint 2
- **Epic**: Complete Property System
- **Assigné**: Game Designer
- **Description**: Créer des blueprints réutilisables pour les types d'entités courants
- **Sous-tâches**:
  - [ ] Créer player.json (avec contrôles)
  - [ ] Créer npc_vendor.json (avec IA)
  - [ ] Créer enemy_basic.json (avec combat)
  - [ ] Créer item_collectible.json (objet interactif)
  - [ ] Créer projectile_arrow.json
  - [ ] Documenter chaque prefab
- **Tags**: content, prefabs, game-design

### 📋 Story 2.4: Implémenter le chargement de blueprints dans le moteur
- **Type**: Story
- **Points**: 8
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 2
- **Epic**: Complete Property System
- **Assigné**: Développeur Backend
- **Description**: Intégrer le système de blueprints avec le PrefabFactory du moteur
- **Sous-tâches**:
  - [ ] Créer classe BlueprintLoader
  - [ ] Implémenter conversion JSON → Composants C++
  - [ ] Étendre PrefabFactory pour blueprints
  - [ ] Implémenter hot-reloading
  - [ ] Écrire tests d'intégration
  - [ ] Optimiser performance de chargement
- **Tags**: backend, integration, engine

---

## Sprint 3 - Behavior & Logic System

### 📋 Story 3.1: Définir l'architecture du système de comportements
- **Type**: Story
- **Points**: 5
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 3
- **Epic**: Behavior & Logic System
- **Assigné**: Architecte Logiciel
- **Description**: Concevoir la structure pour les behaviors et leur intégration
- **Sous-tâches**:
  - [ ] Rédiger document d'architecture
  - [ ] Créer diagrammes UML
  - [ ] Définir cycle de vie des behaviors
  - [ ] Définir schéma JSON pour behaviors
  - [ ] Planifier intégration avec ECS
  - [ ] Review avec l'équipe
- **Tags**: architecture, design, documentation

### 📋 Story 3.2: Implémenter les briques de comportement de base
- **Type**: Story
- **Points**: 8
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 3
- **Epic**: Behavior & Logic System
- **Assigné**: Développeur Gameplay
- **Description**: Créer les behaviors fondamentaux réutilisables
- **Sous-tâches**:
  - [ ] Implémenter Move behavior
  - [ ] Implémenter Detect behavior
  - [ ] Implémenter Follow behavior
  - [ ] Implémenter Patrol behavior
  - [ ] Implémenter Flee behavior
  - [ ] Implémenter Attack behavior
  - [ ] Implémenter Idle behavior
  - [ ] Écrire tests pour chaque behavior
  - [ ] Documenter chaque behavior
- **Tags**: gameplay, behaviors, ai

### 📋 Story 3.3: Créer le schéma JSON pour les behavior blueprints
- **Type**: Story
- **Points**: 3
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 3
- **Epic**: Behavior & Logic System
- **Assigné**: Développeur Backend
- **Description**: Définir comment les behaviors sont stockés en JSON
- **Sous-tâches**:
  - [ ] Définir structure BehaviorBlueprint
  - [ ] Implémenter sérialisation JSON
  - [ ] Support composition de behaviors
  - [ ] Système de conditions/déclencheurs
  - [ ] Créer exemples de behaviors
  - [ ] Documenter le schéma
- **Tags**: backend, json-schema, behaviors

### 📋 Story 3.4: Implémenter le système de conditions et déclencheurs
- **Type**: Story
- **Points**: 5
- **Priorité**: Moyenne
- **Status**: ⏳ TODO
- **Sprint**: Sprint 3
- **Epic**: Behavior & Logic System
- **Assigné**: Développeur Gameplay
- **Description**: Système pour activer/désactiver behaviors selon des conditions
- **Sous-tâches**:
  - [ ] Créer système de conditions booléennes
  - [ ] Implémenter opérateurs logiques (AND, OR, NOT)
  - [ ] Implémenter PropertyCheck condition
  - [ ] Implémenter DistanceCheck condition
  - [ ] Implémenter HealthCheck condition
  - [ ] Implémenter TimerCheck condition
  - [ ] Implémenter EventCheck condition
  - [ ] Écrire tests
- **Tags**: gameplay, conditions, logic

### 📋 Story 3.5: Connecter les behaviors aux propriétés des entités
- **Type**: Story
- **Points**: 5
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 3
- **Epic**: Behavior & Logic System
- **Assigné**: Développeur Backend
- **Description**: Permettre aux behaviors de lire/modifier les propriétés des composants
- **Sous-tâches**:
  - [ ] Créer PropertyBinding system
  - [ ] API pour accéder aux propriétés
  - [ ] Système de binding property → behavior
  - [ ] Validation des accès
  - [ ] Documenter l'API
  - [ ] Créer exemples d'utilisation
- **Tags**: backend, api, data-binding

### 📋 Story 3.6: Créer des exemples de behavior blueprints
- **Type**: Story
- **Points**: 3
- **Priorité**: Basse
- **Status**: ⏳ TODO
- **Sprint**: Sprint 3
- **Epic**: Behavior & Logic System
- **Assigné**: Game Designer
- **Description**: Créer des blueprints de comportements pour cas d'usage courants
- **Sous-tâches**:
  - [ ] Créer guard_patrol.json
  - [ ] Créer enemy_aggressive.json
  - [ ] Créer npc_vendor.json
  - [ ] Créer creature_passive.json
  - [ ] Créer turret_static.json
  - [ ] Documenter chaque behavior
- **Tags**: content, behaviors, examples

---

## Sprint 4 - Visual Blueprint Editor

### 📋 Story 4.1: Concevoir l'UI de l'éditeur
- **Type**: Story
- **Points**: 5
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 4
- **Epic**: Visual Blueprint Editor
- **Assigné**: UI/UX Designer
- **Description**: Créer les maquettes et le design de l'interface utilisateur
- **Sous-tâches**:
  - [ ] Créer wireframes des panneaux principaux
  - [ ] Designer Entity List panel
  - [ ] Designer Component Panel
  - [ ] Designer Property Inspector
  - [ ] Designer Scene Preview
  - [ ] Designer Behavior Graph
  - [ ] Documenter workflow utilisateur
  - [ ] Définir design system
  - [ ] Validation avec stakeholders
- **Tags**: ui-ux, design, mockups

### 📋 Story 4.2: Implémenter l'interface de base avec ImGui
- **Type**: Story
- **Points**: 8
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 4
- **Epic**: Visual Blueprint Editor
- **Assigné**: Développeur Frontend
- **Description**: Créer la fenêtre principale et les panneaux de base
- **Sous-tâches**:
  - [ ] Setup ImGui dans le projet
  - [ ] Créer fenêtre principale
  - [ ] Implémenter système de panneaux
  - [ ] Panneaux redimensionnables
  - [ ] Système de tabs/documents
  - [ ] Appliquer thème visuel
  - [ ] Menu principal
- **Tags**: frontend, imgui, ui

### 📋 Story 4.3: Créer le Property Inspector
- **Type**: Story
- **Points**: 8
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 4
- **Epic**: Visual Blueprint Editor
- **Assigné**: Développeur Frontend
- **Description**: Panel pour éditer les propriétés des composants
- **Sous-tâches**:
  - [ ] Éditeur int, float, string, bool
  - [ ] Éditeur vecteurs (x, y, z)
  - [ ] Éditeur rectangles (x, y, w, h)
  - [ ] Éditeur listes/arrays
  - [ ] Color picker widget
  - [ ] File picker widget
  - [ ] Dropdown widget
  - [ ] Validation en temps réel
  - [ ] Undo/Redo support
- **Tags**: frontend, property-editor, widgets

### 📋 Story 4.4: Implémenter le Component Panel
- **Type**: Story
- **Points**: 5
- **Priorité**: Moyenne
- **Status**: ⏳ TODO
- **Sprint**: Sprint 4
- **Epic**: Visual Blueprint Editor
- **Assigné**: Développeur Frontend
- **Description**: Panel pour ajouter/supprimer des composants
- **Sous-tâches**:
  - [ ] Liste des composants disponibles
  - [ ] Système de filtrage/recherche
  - [ ] Bouton "Add Component"
  - [ ] Menu déroulant composants
  - [ ] Drag & drop pour ajouter
  - [ ] Bouton de suppression
  - [ ] Réorganisation par drag & drop
- **Tags**: frontend, components, ui

### 📋 Story 4.5: Créer l'éditeur graphique de behaviors (ImNodes)
- **Type**: Story
- **Points**: 13
- **Priorité**: Moyenne
- **Status**: ⏳ TODO
- **Sprint**: Sprint 4
- **Epic**: Visual Blueprint Editor
- **Assigné**: Développeur Frontend
- **Description**: Interface graphique pour composer des behaviors
- **Sous-tâches**:
  - [ ] Intégrer ImNodes dans le projet
  - [ ] Afficher behaviors en nœuds
  - [ ] Système de connexions
  - [ ] Éditeur de conditions visuel
  - [ ] Système de priorités
  - [ ] Zoom/pan de la vue
  - [ ] Sélection multiple
  - [ ] Copy/paste de nœuds
  - [ ] Sérialisation graph → JSON
  - [ ] Désérialisation JSON → graph
- **Tags**: frontend, imnodes, graph-editor

### 📋 Story 4.6: Implémenter la preview de l'entité
- **Type**: Story
- **Points**: 5
- **Priorité**: Basse
- **Status**: ⏳ TODO
- **Sprint**: Sprint 4
- **Epic**: Visual Blueprint Editor
- **Assigné**: Développeur Frontend
- **Description**: Aperçu visuel en temps réel de l'entité éditée
- **Sous-tâches**:
  - [ ] Rendering du sprite
  - [ ] Affichage bounding box
  - [ ] Affichage trigger zone
  - [ ] Transformation interactive (move, rotate, scale)
  - [ ] Grid/rulers
  - [ ] Zoom in/out
  - [ ] Snap to grid
- **Tags**: frontend, preview, rendering

### 📋 Story 4.7: Ajouter les fonctionnalités d'import/export
- **Type**: Story
- **Points**: 3
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 4
- **Epic**: Visual Blueprint Editor
- **Assigné**: Développeur Frontend
- **Description**: Charger et sauvegarder des blueprints depuis l'UI
- **Sous-tâches**:
  - [ ] Menu File (New, Open, Save, Save As)
  - [ ] Dialog de sélection de fichier
  - [ ] Auto-save optionnel
  - [ ] Liste des fichiers récents
  - [ ] Gestion non sauvegardés
  - [ ] Confirmation avant fermeture
- **Tags**: frontend, file-io, ui

---

## Sprint 5 - Integration & Polish

### 📋 Story 5.1: Tests d'intégration avec le moteur
- **Type**: Story
- **Points**: 5
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 5
- **Epic**: Integration & Polish
- **Assigné**: QA Engineer
- **Description**: Valider que les blueprints fonctionnent correctement dans le moteur
- **Sous-tâches**:
  - [ ] Test création d'entités depuis blueprints
  - [ ] Test tous les types de composants
  - [ ] Test tous les behaviors
  - [ ] Test hot-reload
  - [ ] Test de charge (100+ entités)
  - [ ] Test de performance
  - [ ] Rédiger rapport de bugs
  - [ ] Vérifier corrections
- **Tags**: qa, testing, integration

### 📋 Story 5.2: Créer la documentation utilisateur
- **Type**: Story
- **Points**: 5
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 5
- **Epic**: Integration & Polish
- **Assigné**: Technical Writer
- **Description**: Guide complet pour utiliser l'éditeur
- **Sous-tâches**:
  - [ ] Rédiger guide de démarrage rapide
  - [ ] Créer tutoriel pas-à-pas
  - [ ] Rédiger documentation de référence
  - [ ] Enregistrer vidéos de démonstration
  - [ ] Créer FAQ
  - [ ] Section troubleshooting
  - [ ] Exemples de cas d'usage
- **Tags**: documentation, user-guide, tutorial

### 📋 Story 5.3: Améliorer l'expérience utilisateur
- **Type**: Story
- **Points**: 5
- **Priorité**: Moyenne
- **Status**: ⏳ TODO
- **Sprint**: Sprint 5
- **Epic**: Integration & Polish
- **Assigné**: Développeur Frontend
- **Description**: Polish de l'interface et ajout de fonctionnalités de confort
- **Sous-tâches**:
  - [ ] Implémenter keyboard shortcuts
  - [ ] Ajouter tooltips informatifs
  - [ ] Améliorer messages d'erreur
  - [ ] Créer templates de blueprints
  - [ ] Système de recherche/filtrage
  - [ ] Documenter shortcuts
  - [ ] Help intégré
- **Tags**: frontend, ux, polish

### 📋 Story 5.4: Optimisation et correction de bugs
- **Type**: Story
- **Points**: 8
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 5
- **Epic**: Integration & Polish
- **Assigné**: Toute l'équipe
- **Description**: Optimiser les performances et corriger les bugs connus
- **Sous-tâches**:
  - [ ] Corriger bugs critiques
  - [ ] Optimiser chargement JSON
  - [ ] Optimiser rendering UI
  - [ ] Détecter memory leaks
  - [ ] Profiling de performance
  - [ ] Optimisations ciblées
  - [ ] Tests de régression
- **Tags**: optimization, bug-fixing, performance

### 📋 Story 5.5: Préparation pour le release
- **Type**: Story
- **Points**: 3
- **Priorité**: Haute
- **Status**: ⏳ TODO
- **Sprint**: Sprint 5
- **Epic**: Integration & Polish
- **Assigné**: Release Manager
- **Description**: Finaliser pour la mise en production
- **Sous-tâches**:
  - [ ] Build de release
  - [ ] Tests finaux
  - [ ] Rédiger CHANGELOG
  - [ ] Rédiger RELEASE_NOTES
  - [ ] Créer package d'installation
  - [ ] Préparer exemples/templates
  - [ ] Validation finale
- **Tags**: release, packaging, deployment

---

## Résumé des Points par Sprint

| Sprint | Points | Durée |
|--------|--------|-------|
| Sprint 1 | 13 | 2 semaines |
| Sprint 2 | 21 | 2 semaines |
| Sprint 3 | 29 | 3 semaines |
| Sprint 4 | 47 | 3 semaines |
| Sprint 5 | 26 | 2 semaines |
| **Total** | **136** | **12 semaines** |

---

## Instructions d'Import dans ClickUp

1. Copier chaque Story dans une nouvelle tâche ClickUp
2. Définir le Type = Story
3. Attribuer les Points d'histoire
4. Définir la Priorité
5. Assigner au Sprint approprié
6. Créer les Sous-tâches (checklist)
7. Ajouter les Tags
8. Lier à l'Epic correspondant

## Codes Couleur Suggérés pour les Tags

- 🔵 **backend**: Bleu
- 🟢 **frontend**: Vert
- 🟡 **documentation**: Jaune
- 🔴 **priority-high**: Rouge
- 🟣 **ui-ux**: Violet
- 🟠 **testing**: Orange
- ⚫ **architecture**: Noir

