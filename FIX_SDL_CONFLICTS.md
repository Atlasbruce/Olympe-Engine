# ?? FIX: Conflits de build SDL_AppEvent

## ?? Problème identifié

Le fichier `Olympe Engine.sln` contient **à la fois** :
- Le projet principal (`Olympe Engine.vcxproj`)
- Les projets éditeurs (`OlympeBlueprintEditor.vcxproj`, `OlympeTilemapEditor.vcxproj`)

**Résultat :** Conflits de symboles SDL lors de la compilation !

```
Error LNK2005: SDL_AppEvent déjà défini(e) dans OlympeEngine.obj
Error LNK2005: SDL_AppInit déjà défini(e) dans OlympeEngine.obj
Error LNK2005: SDL_main déjà défini(e) dans OlympeEngine.obj
```

## ?? Solution

**Chaque projet doit avoir sa propre solution `.sln` :**

```
Olympe-Engine/
??? Olympe Engine.sln           ? Compile UNIQUEMENT le jeu
??? OlympeBlueprintEditor.sln   ? Compile UNIQUEMENT l'éditeur de blueprints
??? OlympeTilemapEditor.sln     ? Compile UNIQUEMENT l'éditeur de tilemap
```

## ? Application du fix (MÉTHODE RECOMMANDÉE)

### Option 1 : Script PowerShell automatique

```powershell
# Depuis la racine du projet
.\Fix-OlympeEngineSolution.ps1
```

Ce script va :
1. ? Créer un backup de `Olympe Engine.sln`
2. ? Retirer `OlympeBlueprintEditor` du fichier
3. ? Retirer `OlympeTilemapEditor` du fichier
4. ? Supprimer toutes les configurations de build associées

### Option 2 : Remplacement manuel

1. **Fermer Visual Studio** complètement
2. **Sauvegarder** `Olympe Engine.sln` :
   ```powershell
   Copy-Item "Olympe Engine.sln" "Olympe Engine.sln.backup"
   ```
3. **Supprimer** le fichier actuel :
   ```powershell
   Remove-Item "Olympe Engine.sln"
   ```
4. **Renommer** le fichier fixé :
   ```powershell
   Rename-Item "Olympe Engine_FIXED.sln" "Olympe Engine.sln"
   ```

## ? Vérification

Après avoir appliqué le fix :

1. **Ouvrir** `Olympe Engine.sln` dans Visual Studio
2. **Vérifier** Solution Explorer :
   ```
   Solution 'Olympe Engine' (1 project)
   ??? Olympe Engine  ? (seul projet visible)
   ```
3. **Build** ? Build Solution (Ctrl+Shift+B)
4. **Résultat attendu** : ? Compilation réussie sans conflits SDL

## ?? Structure correcte après fix

### Olympe Engine.sln
```xml
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "Olympe Engine", "Olympe Engine.vcxproj", "{GUID}"
EndProject
```
? **UN SEUL PROJET** : Le jeu principal

### OlympeBlueprintEditor.sln
```xml
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "OlympeBlueprintEditor", "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj", "{GUID}"
EndProject
```
? **UN SEUL PROJET** : L'éditeur de blueprints

### OlympeTilemapEditor.sln
```xml
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "OlympeTilemapEditor", "OlympeTilemapEditor\OlympeTilemapEditor.vcxproj", "{GUID}"
EndProject
```
? **UN SEUL PROJET** : L'éditeur de tilemap

## ?? Workflow de compilation

### Pour compiler le JEU :
```powershell
start "Olympe Engine.sln"
# Build ? Build Solution
# Résultat : Olympe Engine_d.exe
```

### Pour compiler l'éditeur de BLUEPRINTS :
```powershell
start "OlympeBlueprintEditor.sln"
# Build ? Build Solution
# Résultat : OlympeBlueprintEditor_d.exe
```

### Pour compiler l'éditeur de TILEMAP :
```powershell
start "OlympeTilemapEditor.sln"
# Build ? Build Solution
# Résultat : OlympeTilemapEditor_d.exe
```

## ?? Pourquoi ce conflit ?

### Callbacks SDL3 en double

Tous ces fichiers définissent les mêmes fonctions SDL3 :

**`Source/OlympeEngine.cpp` :**
```cpp
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) { ... }
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) { ... }
SDL_AppResult SDL_AppIterate(void* appstate) { ... }
void SDL_AppQuit(void* appstate, SDL_AppResult result) { ... }
```

**`Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp` :**
```cpp
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) { ... }
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) { ... }
SDL_AppResult SDL_AppIterate(void* appstate) { ... }
void SDL_AppQuit(void* appstate, SDL_AppResult result) { ... }
```

**`Source/OlympeTilemapEditor/src/main.cpp` :**
```cpp
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) { ... }
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) { ... }
SDL_AppResult SDL_AppIterate(void* appstate, SDL_AppResult result) { ... }
void SDL_AppQuit(void* appstate, SDL_AppResult result) { ... }
```

### Quand plusieurs projets sont dans la même solution

Le **linker** voit ces symboles **plusieurs fois** et génère l'erreur `LNK2005`.

### Solution : Projets séparés

En compilant chaque projet dans sa propre solution, le linker ne voit qu'**une seule définition** de chaque callback SDL3. ?

## ?? Checklist de résolution

- [ ] Fermer Visual Studio
- [ ] Exécuter `Fix-OlympeEngineSolution.ps1` OU remplacer manuellement
- [ ] Vérifier que `Olympe Engine.sln` ne contient qu'un projet
- [ ] Rouvrir `Olympe Engine.sln` dans VS
- [ ] Build ? Build Solution
- [ ] ? Compilation réussie !
- [ ] Tester : `Olympe Engine_d.exe` se lance correctement
- [ ] Pour les éditeurs : utiliser leurs solutions dédiées

## ?? Si le problème persiste

### Symptômes
```
Error LNK2005: SDL_AppEvent déjà défini(e) dans OlympeEngine.obj
```

### Diagnostic

1. **Vérifier la solution ouverte dans VS** :
   ```powershell
   Get-Process devenv | Select-Object MainWindowTitle
   ```
   Si vous voyez "OlympeBlueprintEditor.sln" ou "OlympeTilemapEditor.sln", c'est normal.
   Si vous voyez "Olympe Engine.sln" avec plusieurs projets, c'est le problème !

2. **Vérifier le contenu de la solution** :
   ```powershell
   Select-String -Path "Olympe Engine.sln" -Pattern "OlympeBlueprintEditor|OlympeTilemapEditor"
   ```
   Si cette commande retourne des résultats, le fix n'a pas été appliqué correctement.

### Solutions de secours

1. **Nettoyer complètement** :
   ```powershell
   # Fermer VS
   Remove-Item -Recurse -Force bin/
   Remove-Item -Recurse -Force .vs/
   ```

2. **Rebuilder** depuis zéro :
   ```powershell
   start "Olympe Engine.sln"
   # Build ? Rebuild Solution
   ```

3. **Vérifier les chemins d'include** dans `Olympe Engine.vcxproj` :
   - Ne doit PAS inclure `Source/OlympeTilemapEditor/`
   - Ne doit PAS inclure `Source/BlueprintEditorStandalone/`

## ?? Références

- **Documentation Tilemap Editor** : `BUILD_TILEMAP_EDITOR.md`
- **README Tilemap Editor** : `Source/OlympeTilemapEditor/README.md`
- **Pull Request Template** : `.github/PULL_REQUEST_TEMPLATE_TILEMAP_EDITOR.md`

## ? Résumé

| Avant (? MAUVAIS) | Après (? BON) |
|-------------------|--------------|
| `Olympe Engine.sln` contient 3 projets | `Olympe Engine.sln` contient 1 projet |
| Conflits SDL_AppEvent | Aucun conflit |
| Build échoue avec LNK2005 | Build réussit |
| Confusion entre projets | Séparation claire |

---

**Après ce fix, chaque solution compile son propre exécutable sans conflits !** ??
