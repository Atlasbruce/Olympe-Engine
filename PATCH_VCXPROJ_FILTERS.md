# Modifications à appliquer manuellement dans "Olympe Engine.vcxproj.filters"

## 1. Ajouter le filtre TiledLevelLoader

Trouvez la section qui se termine par :
```xml
    <Filter Include="Fichiers d'en-tête\Engine Rendering\SDL3">
      <UniqueIdentifier>{e56c9465-90fe-4575-b9a0-6de136b72f41}</UniqueIdentifier>
    </Filter>
  </ItemGroup>
```

Remplacez par :
```xml
    <Filter Include="Fichiers d'en-tête\Engine Rendering\SDL3">
      <UniqueIdentifier>{e56c9465-90fe-4575-b9a0-6de136b72f41}</UniqueIdentifier>
    </Filter>
    <Filter Include="TiledLevelLoader">
      <UniqueIdentifier>{a1b2c3d4-5e6f-7890-ab12-cd34ef567890}</UniqueIdentifier>
    </Filter>
  </ItemGroup>
```

## 2. Ajouter les fichiers .cpp dans le filtre

Trouvez une ligne avec un ClCompile (par exemple après World.cpp) et ajoutez :

```xml
    <ClCompile Include="Source\TiledLevelLoader\src\TiledLevelLoader.cpp">
      <Filter>TiledLevelLoader</Filter>
    </ClCompile>
    <ClCompile Include="Source\TiledLevelLoader\src\TiledToOlympe.cpp">
      <Filter>TiledLevelLoader</Filter>
    </ClCompile>
```

## Alternative : Ajouter via Visual Studio

1. Dans Visual Studio, après avoir ajouté les fichiers au projet
2. Ils apparaîtront automatiquement dans l'Explorateur de solutions
3. Vous pouvez ensuite les glisser-déposer dans un nouveau dossier "TiledLevelLoader"
4. Visual Studio mettra à jour le .vcxproj.filters automatiquement
