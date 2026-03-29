# Documentation Technique Complète : Parsing et Rendu de Cartes au Format Tiled TMX

## Introduction

Ce tutoriel explique comment parser et rendre des cartes créées avec Tiled Map Editor en utilisant le format TMX dans votre propre moteur de jeu. Bien que les exemples utilisent Flash et ActionScript 3, les techniques et concepts peuvent être appliqués à pratiquement n'importe quel environnement de développement de jeux.
source : https://code.tutsplus.com/parsing-and-rendering-tiled-tmx-format-maps-in-your-own-game-engine--gamedev-3104t
---

## 1. Sauvegarde au Format XML

### 1.1 Configuration de Tiled

En utilisant la spécification TMX, nous pouvons stocker les données de plusieurs façons. Pour ce tutoriel, nous sauvegarderons notre carte au format XML.

Pour configurer Tiled afin qu'il sauvegarde en XML :

1. Ouvrez votre carte avec Tiled
2. Sélectionnez **Edit > Preferences...**
3. Dans le menu déroulant "Store tile layer data as:", sélectionnez **XML**

```
┌─────────────────────────────────────┐
│ Tiled Preferences                   │
├─────────────────────────────────────┤
│ Store tile layer data as:           │
│ ┌─────────────────────────────────┐ │
│ │ XML                          ▼  │ │
│ └─────────────────────────────────┘ │
└─────────────────────────────────────┘
```

Maintenant, lorsque vous sauvegardez la carte, elle sera stockée au format XML.

### 1.2 Structure d'un Fichier TMX

Voici un extrait de ce que vous pouvez trouver dans un fichier TMX :

```xml
<?xml version="1.0" encoding="UTF-8"?>
<map version="1.0" orientation="orthogonal" width="20" height="15" tilewidth="32" tileheight="32">
  <tileset firstgid="1" name="grass-tiles-2-small" tilewidth="32" tileheight="32">
    <image source="grass-tiles-2-small.png" width="256" height="288"/>
  </tileset>
  <tileset firstgid="73" name="tree2-final" tilewidth="32" tileheight="32">
    <image source="tree2-final.png" width="32" height="64"/>
  </tileset>
  <layer name="Base" width="20" height="15">
    <data encoding="xml">
      <tile gid="1"/>
      <tile gid="2"/>
      <tile gid="0"/>
      <!-- ... -->
    </data>
  </layer>
  <layer name="Top" width="20" height="15">
    <data encoding="xml">
      <tile gid="0"/>
      <tile gid="0"/>
      <tile gid="73"/>
      <!-- ... -->
    </data>
  </layer>
  <objectgroup name="Collision">
    <object x="64" y="96" width="32" height="32"/>
  </objectgroup>
</map>
```

Comme vous pouvez le voir, le format stocke toutes les informations de la carte dans ce format XML pratique. Les propriétés sont généralement simples, à l'exception du **gid** - nous reviendrons sur ce point plus tard.

Notez l'élément `objectgroup` nommé "Collision". C'est ainsi que sont stockées les zones de collision définies autour des objets comme les arbres. Vous pouvez spécifier des power-ups ou des points d'apparition de joueurs de la même manière.

---

## 2. Plan d'Implémentation

Voici un aperçu de la façon dont nous allons intégrer notre carte dans le jeu :

1. **Lecture du fichier TMX**
2. **Parsing des tilesets**
3. **Chargement des images des tilesets**
4. **Combinaison des tiles en couches**
5. **Ajout des objets (collision, etc.)**
6. **Affichage de la carte**

---

## 3. Lecture du Fichier TMX

Pour votre programme, il s'agit simplement d'un fichier XML. La première chose à faire est de le lire. La plupart des langages disposent d'une bibliothèque XML ; dans le cas d'AS3, nous utiliserons la classe `XML` pour stocker les informations XML et un `URLLoader` pour lire le fichier TMX.

```actionscript
// Lecteur de fichier simple pour "../assets/example.tmx"
private var mapXML:XML;
private var loader:URLLoader;

public function loadMap():void {
    loader = new URLLoader();
    loader.addEventListener(Event.COMPLETE, onMapLoaded);
    loader.load(new URLRequest("../assets/example.tmx"));
}
```

### 3.1 Gestion du Chargement Complet

```actionscript
private var tilesets:Array = [];
private var mapWidth:int;
private var mapHeight:int;
private var tileWidth:int;
private var tileHeight:int;

private function onMapLoaded(e:Event):void {
    mapXML = new XML(e.target.data);
    
    // Parser les attributs de la carte
    mapWidth = mapXML.@width;
    mapHeight = mapXML.@height;
    tileWidth = mapXML.@tilewidth;
    tileHeight = mapXML.@tileheight;
    
    // Parser chaque tileset
    for each (var tilesetNode:XML in mapXML.tileset) {
        var tileset:TileSet = new TileSet();
        
        tileset.firstgid = tilesetNode.@firstgid;
        tileset.name = tilesetNode.@name;
        tileset.tileWidth = tilesetNode.@tilewidth;
        tileset.tileHeight = tilesetNode.@tileheight;
        tileset.imageSource = tilesetNode.image.@source;
        
        // Calculer le lastgid
        var imageWidth:int = tilesetNode.image.@width;
        var imageHeight:int = tilesetNode.image.@height;
        var tilesWide:int = imageWidth / tileset.tileWidth;
        var tilesHigh:int = imageHeight / tileset.tileHeight;
        var tileCount:int = tilesWide * tilesHigh;
        
        tileset.lastgid = tileset.firstgid + tileCount - 1;
        
        tilesets.push(tileset);
    }
    
    // Passer au chargement des tilesets
    loadTilesets();
}
```

---

## 4. Comprendre le "gid" (Global ID)

### 4.1 Concept Fondamental

Pour chaque tile, nous devons l'associer à un tileset et à un emplacement particulier sur ce tileset. C'est le rôle du **gid** (Global ID).

Prenons l'exemple du tileset `grass-tiles-2-small.png` qui contient 72 tiles distinctes :

```
┌────────────────────────────────────────┐
│ grass-tiles-2-small.png (256x288)      │
│ Grille de 8 colonnes × 9 lignes        │
├────┬────┬────┬────┬────┬────┬────┬────┤
│ 1  │ 2  │ 3  │ 4  │ 5  │ 6  │ 7  │ 8  │
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 9  │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │ 16 │
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 17 │ 18 │ 19 │ 20 │ 21 │ 22 │ 23 │ 24 │
├────┼────┼────┼────┼────┼────┼────┼────┤
│ ... jusqu'à 72                         │
└────────────────────────────────────────┘
```

Nous donnons à chacune de ces tiles un **gid** unique de 1 à 72, afin de pouvoir référencer n'importe laquelle avec un seul nombre.

### 4.2 Calcul du GID

Le format TMX ne spécifie que le premier **gid** du tileset (`firstgid`), car tous les autres **gid** peuvent être déduits en connaissant :
- La taille du tileset
- La taille de chaque tile individuelle

**Formule de calcul :**

```
Nombre de tiles par ligne = Largeur de l'image / Largeur de la tile
Nombre de tiles par colonne = Hauteur de l'image / Hauteur de la tile
Nombre total de tiles = tiles par ligne × tiles par colonne

lastgid = firstgid + nombre total de tiles - 1
```

**Exemple :**
```
grass-tiles-2-small.png : 256×288 pixels, tiles de 32×32
- Tiles par ligne : 256 / 32 = 8
- Tiles par colonne : 288 / 32 = 9
- Total : 8 × 9 = 72 tiles
- firstgid = 1
- lastgid = 1 + 72 - 1 = 72
```

### 4.3 Continuité des GID entre Tilesets

Si nous plaçons la tile en bas à droite de ce tileset sur une carte, nous stockerions le **gid 72** à cet emplacement.

**Important :** Le tileset suivant (`tree2-final.png`) a un `firstgid` de **73**. C'est parce que nous continuons à compter les **gid**, et nous ne recommençons pas à 1 pour chaque tileset.

```
┌──────────────────────────────────────────┐
│ Tileset 1: grass-tiles-2-small.png       │
│ firstgid = 1, lastgid = 72               │
└──────────────────────────────────────────┘
                    ↓
┌──────────────────────────────────────────┐
│ Tileset 2: tree2-final.png               │
│ firstgid = 73, lastgid = 74              │
└──────────────────────────────────────────┘
```

### 4.4 Conversion GID vers Coordonnées

Pour convertir un **gid** en coordonnées (x, y) dans le tileset :

```actionscript
function gidToTileCoords(gid:int, tileset:TileSet):Point {
    var localId:int = gid - tileset.firstgid;
    var tilesPerRow:int = tileset.imageWidth / tileset.tileWidth;
    
    var tileX:int = localId % tilesPerRow;
    var tileY:int = Math.floor(localId / tilesPerRow);
    
    return new Point(tileX, tileY);
}
```

**Exemple visuel :**
```
GID 27 dans le tileset avec firstgid=1
localId = 27 - 1 = 26
tilesPerRow = 8

tileX = 26 % 8 = 2
tileY = 26 / 8 = 3

┌────┬────┬────┬────┬────┬────┬────┬────┐
│ 1  │ 2  │ 3  │ 4  │ 5  │ 6  │ 7  │ 8  │  y=0
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 9  │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │ 16 │  y=1
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 17 │ 18 │ 19 │ 20 │ 21 │ 22 │ 23 │ 24 │  y=2
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 25 │ 26 │[27]│ 28 │ 29 │ 30 │ 31 │ 32 │  y=3
└────┴────┴────┴────┴────┴────┴────┴────┘
  x=0  x=1  x=2  x=3
```

---

## 5. Chargement des Tilesets

### 5.1 Classe TileSet

```actionscript
package {
    import flash.display.BitmapData;
    
    public class TileSet {
        public var firstgid:int;
        public var lastgid:int;
        public var name:String;
        public var tileWidth:int;
        public var tileHeight:int;
        public var imageSource:String;
        public var imageWidth:int;
        public var imageHeight:int;
        public var bitmapData:BitmapData;
        
        public function TileSet() {
        }
    }
}
```

### 5.2 Loader Personnalisé

Pour associer facilement un loader à son tileset, nous créons une classe étendue :

```actionscript
package {
    import flash.display.Loader;
    
    public class TileSetLoader extends Loader {
        public var tileset:TileSet;
        
        public function TileSetLoader(tileset:TileSet) {
            super();
            this.tileset = tileset;
        }
    }
}
```

### 5.3 Chargement des Images

```actionscript
private var loadedTilesets:int = 0;

private function loadTilesets():void {
    for each (var tileset:TileSet in tilesets) {
        var loader:TileSetLoader = new TileSetLoader(tileset);
        
        loader.contentLoaderInfo.addEventListener(Event.COMPLETE, onTilesetLoaded);
        loader.contentLoaderInfo.addEventListener(ProgressEvent.PROGRESS, onTilesetProgress);
        
        loader.load(new URLRequest("../assets/" + tileset.imageSource));
    }
}

private function onTilesetProgress(e:ProgressEvent):void {
    var percentLoaded:Number = e.bytesLoaded / e.bytesTotal;
    trace("Chargement: " + Math.round(percentLoaded * 100) + "%");
    // Ici vous pouvez mettre à jour une barre de progression
}

private function onTilesetLoaded(e:Event):void {
    var loader:TileSetLoader = e.target.loader as TileSetLoader;
    
    // Stocker le BitmapData avec le tileset
    loader.tileset.bitmapData = Bitmap(loader.content).bitmapData;
    
    loadedTilesets++;
    
    // Quand tous les tilesets sont chargés
    if (loadedTilesets == tilesets.length) {
        createMapBitmaps();
        addTileBitmapData();
    }
}
```

---

## 6. Création des Bitmaps de Carte

### 6.1 Initialisation des Couches

```actionscript
private var baseLayer:BitmapData;
private var topLayer:BitmapData;

private function createMapBitmaps():void {
    var mapPixelWidth:int = mapWidth * tileWidth;
    var mapPixelHeight:int = mapHeight * tileHeight;
    
    //