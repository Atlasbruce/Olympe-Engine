# Tiled Isometric Maps - Olympe Engine Guide

## Vue d'ensemble

Ce document décrit comment Olympe Engine gère les cartes isométriques créées avec Tiled MapEditor, en particulier la conversion des coordonnées des objets.

## Configuration Tiled pour les cartes isométriques

### Création d'une nouvelle carte isométrique

1. **File ? New ? New Map...**
2. Paramètres recommandés :
   - **Orientation**: Isometric
   - **Map size**: Selon vos besoins (ex: 184x128)
   - **Tile size**: Ratio 2:1 recommandé (ex: 58x27, 64x32)
3. **File ? Save As...** ? `Resources/Maps/votre_carte.tmj`

### Taille des tiles

Le format isométrique utilise généralement des tiles avec un ratio largeur:hauteur de 2:1 :
- 58×27 pixels
- 64×32 pixels  
- 128×64 pixels

## Conversion des coordonnées isométriques

### Le problème

Tiled stocke les positions des objets dans un système de coordonnées spécial pour les cartes isométriques. L'utilisation directe de ces coordonnées donne des entités mal alignées.

### La découverte clé

**Tiled mesure BOTH X et Y en unités de `tileHeight` pixels le long des axes isométriques.**

C'est différent des cartes orthogonales où X et Y sont des coordonnées pixel directes.

### Formule de conversion

```cpp
// Étape 1: Convertir les coords pixel TMJ en coords tuile
// CRITIQUE: Les deux X et Y sont divisés par tileHeight (convention Tiled)
tileX = tmjPixelX / tileHeight
tileY = tmjPixelY / tileHeight

// Étape 2: Appliquer la projection isométrique standard
worldX = (tileX - tileY) * (tileWidth / 2)
worldY = (tileX + tileY) * (tileHeight / 2)
```

### Exemple vérifié

**Configuration de la carte:**
- Taille: 184×128 tuiles
- Taille tuile: 58×27 pixels (halfWidth=29, halfHeight=13.5)

**Conversion player_1:**
```
Coordonnées TMJ: (1818.4, 1064.26)
tileX = 1818.4 / 27 = 67.35
tileY = 1064.26 / 27 = 39.42
worldX = (67.35 - 39.42) * 29 = 810
worldY = (67.35 + 39.42) * 13.5 = 1441
Résultat: L'entité s'affiche à la tuile (67, 39) ?
```

## Pourquoi les deux divisés par tileHeight?

Dans la vue isométrique de Tiled, les axes X et Y courent en diagonale. Le mouvement le long de l'un ou l'autre axe couvre la même distance diagonale à l'écran. Tiled normalise cela en utilisant `tileHeight` comme unité pour les DEUX axes, rendant le système de coordonnées uniforme le long des deux directions isométriques.

## Pas de décalage d'origine nécessaire

Le calcul `originX` (mapHeight * halfTileWidth) que Tiled utilise est pour l'**affichage écran uniquement**. Dans notre moteur, les tuiles et les objets utilisent le même système de coordonnées monde où la tuile (0,0) est au monde (0,0). La caméra gère le positionnement écran.

## Utilisation dans le code

### Chargement automatique

La conversion est effectuée automatiquement lors du chargement via `TiledToOlympe`:

```cpp
#include "TiledToOlympe.h"

TiledToOlympe converter;
// La configuration détecte automatiquement l'orientation isométrique
Olympe::Editor::LevelDefinition level;
converter.Convert(map, level);  // Les coordonnées sont converties correctement
```

### Conversion manuelle

```cpp
#include "IsometricProjection.h"

// Coordonnées TMJ de l'objet
float tmjX = 1818.4f;
float tmjY = 1064.26f;

// Configuration de la carte
int tileWidth = 58;
int tileHeight = 27;

// Conversion
float tileX = tmjX / tileHeight;
float tileY = tmjY / tileHeight;
float worldX = (tileX - tileY) * (tileWidth * 0.5f);
float worldY = (tileX + tileY) * (tileHeight * 0.5f);
```

## Utilitaires de projection

Le module `IsometricProjection` fournit des fonctions utilitaires:

```cpp
// Monde ? Écran isométrique
Vec2 screenPos = IsometricProjection::WorldToIso(worldX, worldY, tileWidth, tileHeight);

// Écran isométrique ? Monde  
Vec2 worldPos = IsometricProjection::IsoToWorld(screenX, screenY, tileWidth, tileHeight);

// Écran ? Tuile (pour le picking souris)
int tileX, tileY;
IsometricProjection::ScreenToTile(mouseX, mouseY, tileWidth, tileHeight, tileX, tileY);
```

## Placement d'objets dans Tiled

### Objets point (spawns, waypoints)

1. Sélectionnez le calque d'objets
2. Utilisez l'outil **Insert Point** (I)
3. Cliquez pour placer
4. Définissez le Type (player, npc, enemy, etc.)

### Objets rectangle/polygone (collision, zones)

1. Utilisez **Rectangle** (R) ou **Polygon** (P)
2. Dessinez la forme
3. Définissez le Type approprié

## Débogage

Le système produit des logs de débogage:
```
[TransformObjectPosition] ISO: TMJ(1818.4, 1064.26) -> tile(67.35, 39.42) -> world(810, 1441)
```

## Résumé des conversions

| Étape | Opération | Formule |
|-------|-----------|---------|
| 1 | TMJ ? Tuile | `tileX = tmjX / tileHeight`, `tileY = tmjY / tileHeight` |
| 2 | Tuile ? Monde | `worldX = (tileX - tileY) * halfWidth`, `worldY = (tileX + tileY) * halfHeight` |
| 3 | Offsets | Ajouter offsets de calque (convertis en iso) et offsets de tileset |

## Voir aussi

- [TiledLevelLoader README](../Source/TiledLevelLoader/README.md)
- [Implementation Details](../Source/TiledLevelLoader/IMPLEMENTATION_ISOMETRIC_ENTITY_PLACEMENT_FIX.md)
- [Tiled Templates](../Templates/Tiled/README.md)
- [Examples](../Examples/TiledLoader/README.md)
