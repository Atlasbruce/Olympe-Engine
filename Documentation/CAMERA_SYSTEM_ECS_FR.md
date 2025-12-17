# Système de Caméra et Viewport - Architecture ECS

## Vue d'ensemble

Le système de caméra et de viewport a été migré vers une architecture ECS (Entity Component System) complète. Chaque entité joueur possède maintenant sa propre caméra et viewport gérés via des composants ECS, éliminant le besoin d'état global.

## Composants

### Camera_data

Stocke tous les paramètres runtime de la caméra :

```cpp
struct Camera_data
{
    Vector position;        // Position de la caméra dans l'espace monde
    Vector offset;          // Décalage par rapport à la cible (ou centre écran)
    float zoomLevel;        // Niveau de zoom (par défaut : 1.0)
    float rotation;         // Angle de rotation en radians
    SDL_Rect bounds;        // Limites de la caméra pour restreindre le mouvement
    EntityID targetEntity;  // Entité suivie par la caméra
    bool followTarget;      // Activer le suivi de la caméra
    float followSpeed;      // Vitesse de suivi fluide (0-1, plus élevé = plus rapide)
    
    enum class Type { Type_2D, Type_2_5D, Type_Isometric } type;
    enum class Mode { 
        Standard_Fixed,   // Position fixe sur le rendu principal
        Standard_Follow,  // Suit une entité sur le rendu principal
        Viewport_Fixed,   // Position fixe dans le viewport
        Viewport_Follow   // Suit une entité dans le viewport
    } mode;
};
```

### Viewport_data

Stocke le rectangle du viewport et les informations de rendu :

```cpp
struct Viewport_data
{
    SDL_FRect viewRect;    // Rectangle du viewport à l'écran
    int viewportIndex;     // Index de ce viewport (0-7)
    bool enabled;          // Indique si ce viewport est actif
};
```

## Systèmes

### CameraSystem

Traite toutes les entités avec les composants `Camera_data` :

- Met à jour les positions des caméras lors du suivi de cibles
- Applique un mouvement de caméra fluide via `vBlend()`
- Applique les limites de la caméra
- S'exécute pendant la phase Process de l'ECS

**Fonctionnalités principales :**
- Suivi automatique fluide lorsque `followTarget` est activé
- Mouvement indépendant de la fréquence d'images utilisant `vBlend()`
- Nettoyage automatique lorsque l'entité cible est détruite

### RenderingSystem

Effectue le rendu des entités avec des composants visuels :

- Accepte une entité caméra active via `SetActiveCameraEntity()`
- Utilise la position de la caméra pour décaler les entités rendues
- Revient au `CameraManager` hérité si aucune caméra ECS n'est définie

## Configuration des Entités Joueur

Chaque entité joueur créée via `VideoGame::AddPlayerEntity()` reçoit :

1. **Camera_data** - Configuré pour suivre l'entité joueur elle-même
2. **Viewport_data** - Avec rectangle de viewport calculé

Exemple d'initialisation :
```cpp
Camera_data camera;
camera.targetEntity = playerEntityID;
camera.followTarget = true;
camera.offset = Vector(-screenWidth/2.f, -screenHeight/2.f, 0.0f);
camera.mode = Camera_data::Mode::Viewport_Follow;
world.AddComponent<Camera_data>(playerEntityID, camera);
```

## Support de l'Écran Partagé

La disposition du viewport est automatiquement calculée en fonction du nombre de joueurs :

| Joueurs | Disposition |
|---------|-------------|
| 1 | Plein écran |
| 2 | Division verticale (2 colonnes) |
| 3 | Division verticale (3 colonnes) |
| 4 | Grille 2x2 |
| 5-6 | Grille 3x2 |
| 7-8 | Grille 4x2 |

La disposition est calculée par `VideoGame::UpdateAllPlayerViewports()`, qui :
- Calcule les rectangles de viewport en fonction de la taille d'écran et du nombre de joueurs
- Met à jour `Viewport_data.viewRect` pour chaque entité joueur
- Met à jour les décalages de caméra pour correspondre aux dimensions du viewport

## Flux de Rendu

La boucle de rendu principale dans `OlympeEngine.cpp` :

1. Interroge toutes les entités avec `Camera_data` + `Viewport_data`
2. Pour chaque entité caméra :
   - Définit le viewport SDL sur le `viewRect` de l'entité
   - Définit l'entité caméra active dans `RenderingSystem`
   - Appelle `World::Render()` pour rendre toutes les entités visuelles

```cpp
for (EntityID cameraEntity : cameraEntities)
{
    Viewport_data& viewport = World::Get().GetComponent<Viewport_data>(cameraEntity);
    Camera_data& camera = World::Get().GetComponent<Camera_data>(cameraEntity);
    
    // Définir le viewport SDL
    SDL_SetRenderViewport(renderer, &viewportRect);
    
    // Définir la caméra active pour le rendu
    RenderingSystem* renderSys = World::Get().GetSystem<RenderingSystem>();
    renderSys->SetActiveCameraEntity(cameraEntity);
    
    // Rendre le monde pour ce viewport
    World::Get().Render();
}
```

## Rétrocompatibilité

Le `CameraManager` hérité est maintenu pour la rétrocompatibilité :

- Traite toujours les événements de caméra pour le code non-ECS
- Utilisé comme solution de secours lorsqu'aucune caméra ECS n'est active
- Peut être synchronisé à partir des données ECS via `SetActiveCameraFromECS()`

**Chemin de migration :** Le nouveau code doit utiliser les composants ECS. Le code hérité continuera de fonctionner via le mécanisme de secours.

## Ajouter une Nouvelle Caméra

Pour créer une nouvelle entité caméra :

```cpp
// Créer l'entité
EntityID cameraEntity = World::Get().CreateEntity();

// Ajouter le composant caméra
Camera_data camera;
camera.position = Vector(0, 0, 0);
camera.offset = Vector(-400, -300, 0);
camera.followTarget = true;
camera.targetEntity = playerEntity;
camera.followSpeed = 0.75f;
World::Get().AddComponent<Camera_data>(cameraEntity, camera);

// Ajouter le composant viewport
Viewport_data viewport;
viewport.viewRect = {0.f, 0.f, 800.f, 600.f};
viewport.viewportIndex = 0;
viewport.enabled = true;
World::Get().AddComponent<Viewport_data>(cameraEntity, viewport);
```

## Bonnes Pratiques

1. **Toujours utiliser les composants ECS** pour les nouvelles fonctionnalités de caméra
2. **Appeler UpdateAllPlayerViewports()** lors de l'ajout/suppression de joueurs
3. **Définir followTarget=true** pour activer le suivi automatique de la caméra
4. **Utiliser les limites de caméra** pour empêcher la caméra de sortir des limites du niveau
5. **Maintenir followSpeed** dans la plage 0.1-1.0 pour un mouvement fluide

## Améliorations Futures

Améliorations futures possibles :
- Effets de tremblement de caméra via décalage de position temporaire
- Transitions de zoom de caméra
- Plusieurs caméras par joueur (picture-in-picture)
- Zone morte de caméra (déplacer la caméra uniquement lorsque le joueur dépasse le seuil)
- Chemins/rails de caméra personnalisés
- Animations de transition de caméra lors du changement de cibles
