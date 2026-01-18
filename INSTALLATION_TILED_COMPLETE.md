# Instructions pour finaliser l'intégration Tiled MapEditor

## ? Modifications déjà appliquées

1. **TiledToOlympe.h** - Ajout de la méthode `LoadPrefabMapping`
2. **TiledToOlympe.cpp** - Implémentation complète de `LoadPrefabMapping`
3. **World.cpp** - Ajout des includes et correction de l'affichage MapOrientation
4. **TiledStructures.h** - Fonction `GetTileId()` déjà présente
5. **Config/tiled_prefab_mapping.json** - Fichier déjà existant

## ?? Action requise : Ajouter les fichiers au projet Visual Studio

### Méthode 1 : Via Visual Studio (RECOMMANDÉ)

1. Ouvrez le projet **Olympe Engine** dans Visual Studio
2. Dans l'**Explorateur de solutions**, cliquez droit sur le projet
3. Sélectionnez **Add ? Existing Item...**
4. Naviguez vers `Source/TiledLevelLoader/src/`
5. Sélectionnez les 2 fichiers :
   - `TiledLevelLoader.cpp`
   - `TiledToOlympe.cpp`
6. Cliquez sur **Add**
7. **Rebuild** le projet (Ctrl+Shift+B)

### Méthode 2 : Édition manuelle du fichier .vcxproj

Si vous préférez éditer manuellement :

1. Fermez Visual Studio
2. Ouvrez `Olympe Engine.vcxproj` dans un éditeur de texte
3. Trouvez la ligne contenant `<ClCompile Include="Source\third_party\imnodes\imnodes.cpp" />`
4. Ajoutez juste après ces 2 lignes :

```xml
    <ClCompile Include="Source\TiledLevelLoader\src\TiledLevelLoader.cpp" />
    <ClCompile Include="Source\TiledLevelLoader\src\TiledToOlympe.cpp" />
```

5. Sauvegardez le fichier
6. Rouvrez Visual Studio
7. Rebuild le projet

### Méthode 3 : Script PowerShell automatique

Exécutez ce script PowerShell dans le répertoire racine du projet :

```powershell
# Backup du fichier
Copy-Item "Olympe Engine.vcxproj" "Olympe Engine.vcxproj.backup"

# Lire le fichier ligne par ligne
$lines = Get-Content "Olympe Engine.vcxproj"
$newLines = @()

foreach ($line in $lines) {
    $newLines += $line
    # Après la ligne imnodes.cpp, ajouter nos fichiers
    if ($line -match 'imnodes\.cpp') {
        $newLines += '    <ClCompile Include="Source\TiledLevelLoader\src\TiledLevelLoader.cpp" />'
        $newLines += '    <ClCompile Include="Source\TiledLevelLoader\src\TiledToOlympe.cpp" />'
    }
}

# Sauvegarder
$newLines | Set-Content "Olympe Engine.vcxproj"
Write-Host "Fichiers ajoutés au projet avec succès!"
```

## ?? Vérification

Après l'ajout des fichiers, vous devriez voir dans Visual Studio :
- `TiledLevelLoader.cpp` dans l'Explorateur de solutions
- `TiledToOlympe.cpp` dans l'Explorateur de solutions

## ?? Build et Test

1. **Rebuild** le projet complet
2. Les erreurs de liaison (LNK2019) devraient disparaître
3. Testez le chargement d'une map Tiled :

```cpp
World& world = World::Get();
bool success = world.LoadLevelFromTiled("Resources/Maps/level1.tmj");
if (success) {
    SYSTEM_LOG << "Level loaded successfully!\n";
}
```

## ?? Structure finale attendue

```
Source/
??? TiledLevelLoader/
?   ??? include/
?   ?   ??? TiledLevelLoader.h
?   ?   ??? TiledToOlympe.h
?   ?   ??? TiledStructures.h
?   ??? src/
?       ??? TiledLevelLoader.cpp    ? DOIT être dans le projet
?       ??? TiledToOlympe.cpp       ? DOIT être dans le projet
??? World.cpp                        ? Modifié ?
??? World.h

Config/
??? tiled_prefab_mapping.json       ? Existe ?
```

## ?? Fonctionnalités implémentées

? Chargement de maps Tiled (.tmj)
? Conversion vers LevelDefinition Olympe
? Mapping d'objets Tiled ? Prefabs Olympe
? Support des patrol paths (polylines)
? Support des propriétés custom
? Gestion des layers de collision
? Support des tilesets externes
? Déchargement de niveau (UnloadCurrentLevel)

## ?? Dépannage

### Erreur LNK2019 persiste ?
? Vérifiez que les fichiers .cpp sont bien dans le projet VS

### Erreur C2079 'LevelDefinition' non défini ?
? Vérifiez que `#include "OlympeTilemapEditor/include/LevelManager.h"` est présent dans World.cpp

### Prefab mapping ne charge pas ?
? Vérifiez que `Config/tiled_prefab_mapping.json` existe et est valide JSON

### Les entités ne sont pas créées ?
? Vérifiez que les prefabs existent dans PrefabFactory et correspondent aux noms dans le JSON

## ?? Documentation

Consultez `Documentation/TILED_INTEGRATION.md` pour :
- Guide d'utilisation complet
- Format des fichiers JSON
- Exemples de maps Tiled
- Workflow de création de niveau
