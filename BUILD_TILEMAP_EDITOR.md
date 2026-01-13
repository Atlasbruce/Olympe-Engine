# Olympe Tilemap Editor - Build Instructions

## ?? IMPORTANT - Éviter les conflits de compilation

Le projet **OlympeTilemapEditor** est un **exécutable standalone séparé** (comme OlympeBlueprintEditor).

### ? Configuration correcte

**Utilisez UNIQUEMENT la solution dédiée :**

```
OlympeTilemapEditor.sln  ? Ouvrir CETTE solution
```

**PAS** `Olympe Engine.sln` (qui compile le jeu principal)

### ?? Problème courant : Conflit SDL_AppEvent

Si vous voyez cette erreur :
```
LNK2005: SDL_AppEvent déjà défini(e) dans OlympeEngine.obj
LNK2005: SDL_main déjà défini(e) dans OlympeEngine.obj
```

**Cause :** Vous essayez de compiler le TilemapEditor depuis `Olympe Engine.sln`

**Solution :** Fermez `Olympe Engine.sln` et ouvrez `OlympeTilemapEditor.sln`

## ?? Build depuis PowerShell

```powershell
# Aller à la racine du projet
cd "C:\Users\Nico\source\repos\Atlasbruce\Olympe-Engine"

# Ouvrir la bonne solution
start OlympeTilemapEditor.sln

# Dans Visual Studio :
# 1. Configuration : Debug | x64
# 2. Build ? Build Solution (Ctrl+Shift+B)
```

## ?? Structure des solutions

```
Olympe-Engine/
??? Olympe Engine.sln           ? Compile le jeu (OlympeEngine.exe)
??? OlympeBlueprintEditor.sln   ? Compile l'éditeur de blueprints
??? OlympeTilemapEditor.sln     ? Compile l'éditeur de tilemap ?
```

## ?? Projets générés

| Solution                      | Exécutable généré                         |
|-------------------------------|-------------------------------------------|
| `Olympe Engine.sln`           | `Olympe Engine_d.exe` (jeu)              |
| `OlympeBlueprintEditor.sln`   | `OlympeBlueprintEditor_d.exe` (éditeur)  |
| `OlympeTilemapEditor.sln`     | `OlympeTilemapEditor_d.exe` (éditeur)    |

## ? Vérification

Après ouverture de `OlympeTilemapEditor.sln`, vérifiez dans Visual Studio :

1. **Solution Explorer** : Un seul projet visible (`OlympeTilemapEditor`)
2. **Fichiers inclus** :
   - `Source/OlympeTilemapEditor/src/*.cpp` ?
   - `Source/third_party/imgui/*.cpp` ?
   - **PAS** de fichiers `OlympeEngine.cpp` ?

## ?? Troubleshooting

### Erreur : "SDL_AppEvent already defined"

**Symptôme :** Conflits de linking avec symboles SDL

**Diagnostic :**
```powershell
# Vérifier quelle solution est ouverte dans VS
Get-Process devenv | Select-Object MainWindowTitle
```

Si vous voyez "Olympe Engine.sln", c'est le problème !

**Solution :**
1. Fermer Visual Studio
2. Ouvrir `OlympeTilemapEditor.sln`
3. Rebuild

### Erreur : "Cannot find SDL3.lib"

**Cause :** Chemins relatifs des includes

**Solution :** Les includes SDL sont relatifs depuis `OlympeTilemapEditor/` vers `SDL/`

Vérifiez que cette structure existe :
```
Olympe-Engine/
??? OlympeTilemapEditor/
?   ??? OlympeTilemapEditor.vcxproj
??? SDL/
    ??? lib/
        ??? SDL3.lib
```

### Erreur de compilation dans LevelManager.cpp

**Cause :** Utilisation de la lib JSON custom

**Solution :** Déjà corrigée dans la version actuelle :
- Utilise `json::parse()` au lieu de `operator>>`
- Itère sur arrays avec `size()` et `operator[]`

## ?? Checklist avant de compiler

- [ ] Fermé `Olympe Engine.sln`
- [ ] Ouvert `OlympeTilemapEditor.sln`
- [ ] Configuration : Debug | x64
- [ ] Un seul projet dans Solution Explorer
- [ ] Build ? Build Solution

## ?? Lancement après compilation

```powershell
# Debug build
.\OlympeTilemapEditor_d.exe

# Ou depuis le répertoire de sortie
.\bin\TilemapEditor\Debug_x64\OlympeTilemapEditor_d.exe
```

## ?? Pour les développeurs

### Ajouter de nouveaux fichiers

Si vous ajoutez des fichiers au TilemapEditor :

1. Créer le fichier dans `Source/OlympeTilemapEditor/`
2. Ouvrir `OlympeTilemapEditor.sln`
3. Clic droit sur le projet ? Add ? Existing Item
4. Sélectionner votre nouveau fichier
5. Rebuild

### Modifier les dépendances

Le projet dépend de :
- SDL3 (windowing, events)
- ImGui (UI)
- nlohmann/json custom (sérialisation)

**Ne pas** ajouter de dépendances vers :
- OlympeEngine.cpp ?
- GameEngine.cpp ?
- VideoGame.cpp ?

## ?? Références

- Documentation complète : `Source/OlympeTilemapEditor/README.md`
- Pull Request template : `.github/PULL_REQUEST_TEMPLATE_TILEMAP_EDITOR.md`
- Projet similaire : OlympeBlueprintEditor

---

**En cas de doute : Toujours utiliser `OlympeTilemapEditor.sln` !**
