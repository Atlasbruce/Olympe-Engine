# Migration du Système de Caméra et Viewport vers une Architecture ECS Complète

## Résumé

La gestion des caméras et viewports reposait sur un état global (`CameraManager`, `activePlayer`) et la distribution d'événements. Cette PR migre vers des composants ECS purs, éliminant l'état global tout en supportant l'écran partagé (1-8 joueurs).

## Modifications

### Composants

- **`Camera_data`** : Position, décalage, zoom, rotation, limites, cible de suivi/vitesse, type/mode de caméra
- **`Viewport_data`** : Rectangle à l'écran, index du viewport, état activé

### Systèmes

- **`CameraSystem`** : Met à jour les positions des caméras, applique un suivi fluide via `vBlend()`, applique les limites
- **`RenderingSystem`** : Modifié pour accepter `SetActiveCameraEntity(cameraEntityID)` au lieu d'interroger l'état global

### Boucle de Rendu

La boucle principale interroge l'ECS pour les entités avec `Camera_data + Viewport_data`, itère sur chaque viewport :

```cpp
for (EntityID cameraEntity : cameraEntities) {
    Viewport_data& viewport = World::Get().GetComponent<Viewport_data>(cameraEntity);
    Camera_data& camera = World::Get().GetComponent<Camera_data>(cameraEntity);
    
    SDL_SetRenderViewport(renderer, &viewport.viewRect);
    renderingSystem->SetActiveCameraEntity(cameraEntity);
    World::Get().Render();
}
```

### Création de Joueur

`VideoGame::AddPlayerEntity()` ajoute maintenant les composants caméra/viewport. `UpdateAllPlayerViewports()` calcule les dispositions de viewport (plein écran, division verticale, grilles 2x2/3x2/4x2) et met à jour les données des composants.

### Utilitaires

- `World::GetSystem<T>()` : Méthode template pour récupérer des instances de systèmes ECS spécifiques
- `VideoGame::UpdateAllPlayerViewports()` : Calcule les rectangles de viewport en fonction du nombre de joueurs, met à jour toutes les entités joueur

## Compatibilité

Le `CameraManager` hérité est conservé comme solution de secours pour les chemins de code non-ECS. `SetActiveCameraFromECS()` synchronise les données ECS vers le manager si nécessaire.

## Critères Satisfaits

✅ **Composants ECS = source de vérité** : Tout l'état de la caméra vit maintenant dans les composants Camera_data et Viewport_data

✅ **Plus d'état global** : Éliminé la dépendance à l'activePlayer global de CameraManager

✅ **Support écran partagé** : Écran partagé 1-8 joueurs entièrement basé sur les données ECS

✅ **Rétrocompatibilité** : CameraManager hérité maintenu comme solution de secours

✅ **Architecture propre** : Séparation claire des préoccupations avec CameraSystem dédié

## Points Techniques Clés

- **CameraSystem** s'exécute dans la phase Process de l'ECS, mettant à jour toutes les caméras
- **Disposition du viewport** calculée dynamiquement (1 joueur = plein écran, 2 = division verticale, 4 = grille 2x2, etc.)
- **Suivi fluide** utilise la fonction `vBlend()` indépendante de la fréquence d'images
- **World::GetSystem<T>()** template ajouté pour récupérer des systèmes spécifiques
- **Validation appropriée** garantit que les entités cibles ont les composants requis

## Chemin de Migration

L'implémentation maintient la rétrocompatibilité :
- Le code hérité utilisant CameraManager continue de fonctionner
- Le nouveau code doit utiliser directement les composants ECS
- Les mécanismes de secours assurent une transition en douceur

## Documentation

Documentation complète disponible dans :
- `Documentation/CAMERA_SYSTEM_ECS.md` (English)
- `Documentation/CAMERA_SYSTEM_ECS_FR.md` (Français)

Les deux guides incluent :
- Description détaillée des composants et systèmes
- Exemples d'utilisation
- Support de l'écran partagé
- Flux de rendu
- Bonnes pratiques
- Améliorations futures possibles
