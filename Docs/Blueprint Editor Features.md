# Blueprint Editor - Fonctionnalités CRUD

Ce document décrit les fonctionnalités CRUD (Create, Read, Update, Delete) disponibles dans l'éditeur Blueprint de l'Olympe Engine.

## Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Opérations sur les nœuds](#opérations-sur-les-nœuds)
3. [Opérations sur les liens](#opérations-sur-les-liens)
4. [Raccourcis clavier](#raccourcis-clavier)
5. [Système de permissions](#système-de-permissions)

---

## Vue d'ensemble

L'éditeur Blueprint permet de créer et modifier visuellement des graphes de comportement (Behavior Trees) pour l'IA. Les opérations CRUD sont accessibles via :
- **Menus contextuels** (clic droit)
- **Raccourcis clavier**
- **Double-clic** pour l'édition
- **Barre d'outils** pour la sauvegarde

Toutes les opérations utilisent le système de commandes avec support de l'**undo/redo**.

---

## Opérations sur les nœuds

### Créer un nœud (Create)

#### Via le menu contextuel
1. **Clic droit** sur une zone vide du canvas
2. Sélectionner un type de nœud dans le menu :
   - **Sequence** : Nœud composite qui exécute ses enfants séquentiellement
   - **Selector** : Nœud composite qui exécute ses enfants jusqu'à réussite
   - **Action** : Nœud feuille qui exécute une action spécifique
   - **Condition** : Nœud feuille qui évalue une condition
   - **Decorator** : Nœud qui modifie le comportement d'un enfant
   - **Comment Box** : Zone de commentaire pour annoter le graphe

#### Via le drag & drop
1. Faire glisser un type de nœud depuis le panneau de palette
2. Déposer sur le canvas à l'emplacement souhaité

#### Recherche filtrée
- Le menu de création dispose d'un champ de recherche
- Tapez pour filtrer les types de nœuds disponibles
- Particulièrement utile pour les Atomic Tasks

**Permissions requises** : `CanCreate()`

---

### Lire/Afficher un nœud (Read)

#### Visualisation
- Les nœuds affichent automatiquement :
  - **Icône** du type de nœud
  - **Nom** du nœud
  - **Pins d'entrée/sortie** (triangles pour le flux d'exécution)
  - **Type spécifique** (ActionType, ConditionType, etc.)

#### Inspection détaillée
- **Simple clic** : Sélectionne le nœud
- Le nœud sélectionné affiche son ID dans la console
- Les informations détaillées apparaissent dans le panneau d'inspection

---

### Modifier un nœud (Update)

#### Édition des propriétés

##### Via double-clic
1. **Double-cliquer** sur un nœud
2. Une fenêtre modale s'ouvre avec :
   - **Champ de nom** : Modifier le nom du nœud
   - **Paramètres** : Selon le type de nœud
3. Cliquer **OK** pour valider ou **Cancel** pour annuler

##### Via menu contextuel
1. **Clic droit** sur un nœud
2. Sélectionner **"Edit Properties"**
3. Même fenêtre modale que le double-clic

#### Déplacement
- **Glisser-déposer** le nœud sur le canvas
- Les positions sont **automatiquement sauvegardées**
- Option **Snap-to-Grid** disponible (Ctrl+G)

#### Duplication
1. **Clic droit** sur un nœud
2. Sélectionner **"Duplicate"** (ou Ctrl+D)
3. Un nouveau nœud identique est créé à proximité

**Raccourci** : `Ctrl+D`

**Permissions requises** : `CanEdit()` et `CanCreate()` pour la duplication

---

### Supprimer un nœud (Delete)

#### Via menu contextuel
1. **Clic droit** sur un nœud
2. Sélectionner **"Delete"**
3. Le nœud et ses liaisons sont supprimés

#### Via raccourci clavier
1. **Sélectionner** un nœud (simple clic)
2. Appuyer sur la touche **Delete**

**Raccourci** : `Delete` ou `Del`

**Permissions requises** : `CanDelete()`

⚠️ **Attention** : La suppression d'un nœud supprime également toutes ses liaisons entrantes et sortantes.

---

## Opérations sur les liens

### Créer un lien (Create)

1. **Cliquer** sur un pin de sortie (à droite d'un nœud)
2. **Glisser** vers un pin d'entrée (à gauche d'un autre nœud)
3. **Relâcher** pour créer la liaison

**Validation automatique** :
- Vérifie que la connexion est valide (pas de cycle, types compatibles)
- Affiche un message d'erreur si la connexion est invalide

**Permissions requises** : `CanLink()`

---

### Afficher les informations d'un lien (Read)

#### Survol
- Survolez un lien pour le mettre en surbrillance

#### Sélection
- **Cliquer** sur un lien pour le sélectionner
- Le lien sélectionné change d'apparence

#### Menu contextuel
- **Clic droit** sur un lien
- Le menu affiche :
  ```
  Link: NomSource -> NomDestination
  ─────────────────────────────
  Delete Link     Del
  ```

---

### Supprimer un lien (Delete)

#### Via menu contextuel
1. **Clic droit** sur un lien
2. Sélectionner **"Delete Link"**

#### Via raccourci clavier
1. **Sélectionner** un lien (clic sur le lien)
2. Appuyer sur la touche **Delete**

**Raccourci** : `Delete` ou `Del`

**Permissions requises** : `CanDelete()`

---

## Raccourcis clavier

| Action | Raccourci | Description |
|--------|-----------|-------------|
| **Éditer un nœud** | Double-clic | Ouvre la fenêtre d'édition des propriétés |
| **Dupliquer un nœud** | `Ctrl+D` | Crée une copie du nœud sélectionné |
| **Supprimer** | `Delete` ou `Del` | Supprime le nœud ou lien sélectionné |
| **Sauvegarder** | `Ctrl+S` | Sauvegarde le graphe (si un chemin est défini) |
| **Annuler** | `Ctrl+Z` | Annule la dernière action |
| **Refaire** | `Ctrl+Y` | Refait l'action annulée |
| **Snap-to-Grid** | `Ctrl+G` | Active/désactive l'alignement sur la grille |
| **Minimap** | `Ctrl+M` | Affiche/masque la minimap |

---

## Système de permissions

Les opérations CRUD respectent le système de permissions de l'`EditorContext` :

### Permissions disponibles

| Permission | Description | Opérations affectées |
|------------|-------------|---------------------|
| **CanCreate()** | Autorisation de créer de nouveaux éléments | - Créer des nœuds<br>- Dupliquer des nœuds |
| **CanEdit()** | Autorisation de modifier les éléments existants | - Éditer les propriétés<br>- Déplacer les nœuds<br>- Dupliquer (avec CanCreate) |
| **CanDelete()** | Autorisation de supprimer des éléments | - Supprimer des nœuds<br>- Supprimer des liens |
| **CanLink()** | Autorisation de créer des connexions | - Créer des liens entre nœuds |

### Comportement

- Les options non autorisées sont **désactivées** dans les menus
- Les raccourcis clavier sont **ignorés** si la permission n'est pas accordée
- Un **tooltip** explique pourquoi une option est désactivée (au survol)

**Exemple** : En mode lecture seule, seule l'option "Edit Properties" est disponible (pour visualisation uniquement).

---

## Fonctionnalités avancées

### Sauvegarde automatique des positions

- Les positions des nœuds sont **automatiquement enregistrées**
- Déclenchement de la sauvegarde :
  - Au déplacement d'un nœud
  - Lors de la sauvegarde manuelle (`Ctrl+S`)
  - Via l'autosave (toutes les 1.5 secondes après modification)

### Système de commandes

Toutes les opérations CRUD utilisent le **Command Pattern** :
- **Undo/Redo** complet
- **Historique** des actions
- **Annulation** de modifications complexes

### Validation

- Validation automatique avant sauvegarde
- Messages d'erreur explicites en cas de graphe invalide
- Empêche la création de cycles dans les Behavior Trees

---

## Architecture technique

### Classes principales

- **NodeGraphPanel** : Interface utilisateur principale
  - `RenderGraph()` : Rendu du graphe avec ImNodes
  - `HandleNodeInteractions()` : Gestion des interactions utilisateur
  - `SyncNodePositionsFromImNodes()` : Synchronisation des positions
  
- **NodeGraphManager** : Gestionnaire de graphes
  - Gestion des graphes multiples
  - Sérialisation/Désérialisation JSON
  
- **Command classes** :
  - `DeleteNodeCommand` : Suppression de nœud
  - `UnlinkNodesCommand` : Suppression de lien
  - `DuplicateNodeCommand` : Duplication de nœud (via BlueprintAdapter)

### Format de sauvegarde

Les graphes sont sauvegardés au format **JSON** incluant :
- Positions des nœuds (x, y)
- Types et propriétés
- Connexions (liens parent-enfant)
- Métadonnées de l'éditeur (zoom, scroll, sélection)

**Exemple de structure JSON** :
```json
{
  "schemaVersion": 2,
  "type": "BehaviorTree",
  "data": {
    "nodes": [
      {
        "id": 1,
        "type": "Sequence",
        "name": "Root Sequence",
        "position": { "x": 100.0, "y": 150.0 },
        "children": [2, 3]
      }
    ]
  }
}
```

---

## Bonnes pratiques

### Organisation du graphe

1. **Nommer clairement les nœuds** : Utilisez des noms descriptifs
2. **Utiliser les Comment Box** : Documentez les sections complexes
3. **Grouper logiquement** : Organisez visuellement les sous-arbres
4. **Snap-to-Grid** : Activez pour un alignement propre

### Performance

- **Autosave optimisé** : La sérialisation se fait sur le thread UI, l'écriture en arrière-plan
- **Positions synchronisées** : Mise à jour uniquement si changement détecté
- **Dirty flag** : Sauvegarde uniquement si le graphe a été modifié

### Workflow recommandé

1. **Créer la structure** : Ajoutez les nœuds principaux
2. **Connecter** : Créez les liens logiques
3. **Configurer** : Éditez les propriétés de chaque nœud
4. **Documenter** : Ajoutez des commentaires
5. **Sauvegarder** : Ctrl+S régulièrement

---

## Dépannage

### Le menu contextuel ne s'affiche pas
- Vérifiez que vous faites un **clic droit** sur un élément valide
- Assurez-vous que les permissions sont accordées

### Impossible de créer un lien
- Vérifiez `CanLink()` dans l'EditorContext
- Assurez-vous qu'aucun cycle ne serait créé
- Vérifiez la compatibilité des types de pins

### Les positions ne sont pas sauvegardées
- Vérifiez que le graphe a un filepath défini
- Utilisez "Save As..." pour définir un chemin
- La synchronisation est automatique lors de la sauvegarde

### La duplication ne fonctionne pas
- Nécessite à la fois `CanEdit()` ET `CanCreate()`
- Vérifiez les permissions de l'EditorContext

---

## Liens utiles

- **Architecture NodeGraph** : `Docs/Developer/NodeGraph_Unified_Architecture.md`
- **Commandes Blueprint** : `Source/BlueprintEditor/BPCommandSystem.h`
- **Tests** : `Docs/Archive/BT_EDITOR_TESTING_CHECKLIST.md`

---

**Dernière mise à jour** : Mars 2026  
**Version de l'éditeur** : 2.0  
**Auteur** : Olympe Engine Team
