# 📊 GRAPHE D'APPELS COMPLET - INITIALISATION BLUEPRINT EDITOR

**Objective**: Tracer TOUS les appels de fonctions, méthodes, classes AVANT l'entrée dans la boucle principale (SDL_AppIterate)

**Scope**: De `SDL_AppInit()` jusqu'à la dernière ligne avant `return SDL_APP_CONTINUE` dans `SDL_AppInit()`

**Format**: Structure hiérarchique avec descriptions, paramètres, et données initialisées

---

## 📍 POINT D'ENTRÉE: `SDL_AppInit()` 

**Fichier**: `Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp` (lignes 29-110)

**Responsabilité**: Initialiser SDL3, ImGui, et les systèmes de l'éditeur Blueprint

**Retour**: `SDL_APP_CONTINUE` → Lance `SDL_AppIterate()` boucle principale

---

## 🌳 HIÉRARCHIE D'APPELS COMPLÈTE

```
SDL_AppInit() [ENTRY POINT]
├─ Phase 1: INITIALISATION SDL3 & RENDERING
├─ Phase 2: INITIALISATION ImGui CONTEXT
├─ Phase 3: INITIALISATION IMGUI BACKENDS
├─ Phase 4: INITIALISATION BACKEND (BlueprintEditor)
├─ Phase 5: INITIALISATION FRONTEND (BlueprintEditorGUI)
└─ Phase 6: RETURN → SDL_AppIterate() [MAIN LOOP]
```

---

## 📋 APPELS DÉTAILLÉS PAR PHASE

### ⚙️ PHASE 1: INITIALISATION SDL3 & RENDERING
**Lignes**: 38-65 (SDL_AppInit)

#### 1.1 `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)`
- **Ligne**: 38 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Initialiser subsystèmes SDL3 (vidéo + événements)
- **Paramètres**: 
  - `SDL_INIT_VIDEO`: Initialise le système vidéo/renderer
  - `SDL_INIT_EVENTS`: Initialise la queue d'événements
- **Retour**: `bool` (true = succès, false = erreur)
- **État après**: SDL3 système vidéo prêt, event loop prêt à recevoir événements
- **Gestion d'erreur**: If fails → log + `SDL_Quit()` + return `SDL_APP_FAILURE`

#### 1.2 `SDL_CreateWindow(...)`
- **Ligne**: 40-43 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Créer fenêtre SDL3 pour l'éditeur
- **Paramètres**:
  ```cpp
  SDL_CreateWindow(
    "Olympe Blueprint Editor Standalone - WYSIWYG Editor",  // title
    1920,                     // width (pixels)
    1080,                     // height (pixels)
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED  // flags
  )
  ```
- **Retour**: `SDL_Window*` (pointer ou nullptr si erreur)
- **État après**: Fenêtre créée (1920x1080 resizable, maximized)
- **Stockage**: `g_BlueprintEditorWindow` (global variable)
- **Gestion d'erreur**: If nullptr → log + `SDL_Quit()` + return `SDL_APP_FAILURE`

#### 1.3 `SDL_CreateRenderer(g_BlueprintEditorWindow, nullptr)`
- **Ligne**: 48-49 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Créer renderer SDL3 pour dessiner sur fenêtre
- **Paramètres**:
  - `g_BlueprintEditorWindow`: Fenêtre cible
  - `nullptr`: Auto-select renderer (GPU si possible)
- **Retour**: `SDL_Renderer*` (pointer ou nullptr si erreur)
- **État après**: Renderer prêt à rendre 2D/3D
- **Stockage**: `g_BlueprintEditorRenderer` (global variable)
- **Gestion d'erreur**: If nullptr → log + `SDL_DestroyWindow()` + `SDL_Quit()` + return `SDL_APP_FAILURE`

---

### 🎨 PHASE 2: INITIALISATION ImGui CONTEXT
**Lignes**: 57-76 (SDL_AppInit)

#### 2.1 `IMGUI_CHECKVERSION()`
- **Ligne**: 57 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Vérifier compatibilité entre headers ImGui compilés et version runtime
- **Paramètres**: Aucun
- **Retour**: `void` (assertion si version mismatch)
- **État après**: Validation de compatibilité ImGui

#### 2.2 `ImGui::CreateContext()`
- **Ligne**: 58 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Créer context ImGui global
- **Paramètres**: Aucun (uses default AllocatorFuncs)
- **Retour**: `ImGuiContext*` (global context pointer)
- **État après**: ImGui context créé, devient context courant
- **Stockage**: Implicite (ImGui::GetCurrentContext())
- **Effet**: Tous les appels ImGui ultérieurs utilisent ce context

#### 2.3 `ImGui::GetIO()`
- **Ligne**: 59 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Accéder au structure ImGuiIO (input/output)
- **Paramètres**: Aucun
- **Retour**: `ImGuiIO&` (reference au IO struct du context courant)
- **État après**: Reference `io` prêt à configurer

#### 2.4 `io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard`
- **Ligne**: 60 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Activer navigation au clavier ImGui
- **Paramètres**: Aucun (flag bitwise OR)
- **État après**: ImGui accepte input clavier (Tab, Arrows, Enter)
- **Alternative désactivée**: `ImGuiConfigFlags_DockingEnable` (pas disponible dans cette version)

#### 2.5 `io.IniFilename = nullptr`
- **Ligne**: 63 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Désactiver sauvegarde automatique ImGui layout (imgui.ini)
- **État après**: ImGui n'écrira pas de fichier .ini (config gérée par JSON custom)

#### 2.6 `ImGui::StyleColorsDark()`
- **Ligne**: 65 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Appliquer thème ImGui "Dark"
- **Paramètres**: Aucun
- **État après**: Couleurs par défaut dark appliquées

#### 2.7 `ImGui::GetStyle()`
- **Ligne**: 67 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Accéder à ImGuiStyle (couleurs, sizes, spacing)
- **Paramètres**: Aucun
- **Retour**: `ImGuiStyle&` (reference à style du context)
- **État après**: Reference `style` prêt à customizer

#### 2.8 `style.Colors[ImGuiCol_*] = ImVec4(...)`
- **Lignes**: 68-70 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Customiser couleurs ImGui (window bg, title, active title)
- **État après**: Couleurs personnalisées appliquées:
  - `WindowBg`: (0.12, 0.12, 0.14, 1.0) → Dark navy
  - `TitleBg`: (0.15, 0.15, 0.18, 1.0) → Dark gray
  - `TitleBgActive`: (0.20, 0.40, 0.60, 1.0) → Blue active

---

### 🔗 PHASE 3: INITIALISATION ImGui BACKENDS
**Lignes**: 72-74 (SDL_AppInit)

#### 3.1 `ImGui_ImplSDL3_InitForSDLRenderer(g_BlueprintEditorWindow, g_BlueprintEditorRenderer)`
- **Ligne**: 72 dans `BlueprintEditorStandaloneMain.cpp`
- **Fichier**: `../third_party/imgui/backends/imgui_impl_sdl3.h`
- **Responsabilité**: Initialiser backend SDL3 pour ImGui
- **Paramètres**:
  - `g_BlueprintEditorWindow`: Fenêtre SDL3
  - `g_BlueprintEditorRenderer`: Renderer SDL3
- **État après**: ImGui SDL3 backend prêt (event handling + render target)
- **Configuration**: Polling events, clipboard support, mouse/keyboard input

#### 3.2 `ImGui_ImplSDLRenderer3_Init(g_BlueprintEditorRenderer)`
- **Ligne**: 73 dans `BlueprintEditorStandaloneMain.cpp`
- **Fichier**: `../third_party/imgui/backends/imgui_impl_sdlrenderer3.h`
- **Responsabilité**: Initialiser backend SDLRenderer3 pour ImGui rendering
- **Paramètres**: `g_BlueprintEditorRenderer` (renderer cible)
- **État après**: ImGui SDLRenderer3 backend prêt (draw lists → SDL3 rendering)
- **Configuration**: Texture upload, draw command translation

---

### 🎯 PHASE 4: INITIALISATION BACKEND (BlueprintEditor Singleton)
**Lignes**: 76-82 (SDL_AppInit)

#### 4.1 `BlueprintEditor::Get()`
- **Ligne**: 76 dans `BlueprintEditorStandaloneMain.cpp`
- **Fichier**: `Source/BlueprintEditor/blueprinteditor.h/cpp`
- **Responsabilité**: Accéder singleton BlueprintEditor (instance unique globale)
- **Paramètres**: Aucun
- **Retour**: `BlueprintEditor&` (reference au singleton)
- **État après**: Reference singleton prêt pour Initialize()

#### 4.2 `BlueprintEditor::Initialize()`
- **Lignes**: 72-107 dans `Source/BlueprintEditor/blueprinteditor.cpp`
- **Responsabilité**: Initialiser tout le système backend
- **Paramètres**: Aucun

**Appels internes (4.2 expansion):**

##### 4.2.1 État initial
```cpp
m_IsActive = false;
m_HasUnsavedChanges = false;
m_CurrentBlueprint = Blueprint::EntityBlueprint();
m_CurrentFilepath = "";
m_AssetTreeRoot = nullptr;
m_LastError.clear();
```
- Initialise membres backend à état vierge
- `m_IsActive`: false (sera set à true après initialisation complète)
- `m_HasUnsavedChanges`: false (aucun changement au démarrage)
- `m_CurrentBlueprint`: EntityBlueprint vide
- `m_AssetTreeRoot`: nullptr (assets pas encore scannés)

##### 4.2.2 `EnumCatalogManager::Get().Initialize()`
- **Responsabilité**: Initialiser catalogue d'énumérations
- **État après**: Enums chargées depuis fichier config (color types, component types, etc.)
- **Fichier**: `Source/BlueprintEditor/EnumCatalogManager.h/cpp`
- **Données**: `std::map<string, std::vector<string>>` d'énumérations

##### 4.2.3 `NodeGraphManager::Get().Initialize()`
- **Responsabilité**: Initialiser gestionnaire de graphes de nœuds
- **État après**: ID counter réinitialisé, gestionnaire prêt à créer nœuds
- **Fichier**: `Source/NodeGraphCore/NodeGraphManager.h/cpp`
- **Données**: NodeID counter, default node templates

##### 4.2.4 `EntityInspectorManager::Get().Initialize()`
- **Responsabilité**: Initialiser inspecteur d'entités (synchronisation avec World)
- **État après**: Gestionnaire prêt à tracker entités sélectionnées
- **Fichier**: `Source/BlueprintEditor/EntityInspectorManager.h/cpp`
- **Données**: Entity selection state, property cache

##### 4.2.5 `TemplateManager::Get().Initialize()`
- **Responsabilité**: Initialiser gestionnaire de templates de graphes
- **État après**: Templates chargés depuis `Gamedata/Templates/`
- **Fichier**: `Source/BlueprintEditor/TemplateManager.h/cpp`
- **Données**: `std::vector<GraphTemplate>` de templates disponibles

##### 4.2.6 `new Olympe::Blueprint::CommandStack()`
- **Ligne**: 97 dans `blueprinteditor.cpp`
- **Responsabilité**: Créer stack de commandes pour undo/redo
- **État après**: Stack vide, prêt pour enregistrer commandes
- **Stockage**: `m_CommandStack` (heap-allocated)
- **Destruction**: Dans `Shutdown()` (ligne 139)

##### 4.2.7 `InitializePlugins()`
- **Ligne**: 100 dans `blueprinteditor.cpp`
- **Responsabilité**: Charger et enregistrer plugins de type de graphe
- **État après**: Tous plugins chargés, types disponibles pour TabManager
- **Détails**: Chargement depuis `Gamedata/Plugins/` (si implémenté)

##### 4.2.8 `LoadConfig("blueprint_editor_config.json")`
- **Ligne**: 103 dans `blueprinteditor.cpp`
- **Responsabilité**: Charger configuration éditeur depuis fichier JSON
- **État après**: Layout, zoom levels, recent files, préférences chargés
- **Gestion d'erreur**: Si fichier manquant → defaults utilisés

##### 4.2.9 `RefreshAssets()`
- **Ligne**: 106 dans `blueprinteditor.cpp`
- **Responsabilité**: Scanner disque pour assets (graphes, entités, ressources)
- **État après**: Asset tree construit, utilisé par Asset Browser
- **Performance**: Peut être lent pour gros repos (100+ fichiers)

#### 4.3 `BlueprintEditor::InitializeStandaloneEditor()`
- **Lignes**: 116-125 dans `Source/BlueprintEditor/blueprinteditor.cpp`
- **Responsabilité**: Configuration spécifique mode Standalone (full CRUD, pas de runtime)
- **Paramètres**: Aucun

**Appels internes (4.3 expansion):**

##### 4.3.1 `EditorContext::Get().InitializeStandalone()`
- **Responsabilité**: Initialiser contexte d'édition en mode Standalone
- **État après**: EditorContext configuré pour full CRUD (no runtime restrictions)
- **Fichier**: `Source/BlueprintEditor/EditorContext.h/cpp`
- **Flags**: 
  - `m_EditorMode = EditorMode::Standalone`
  - Full CRUD operations enabled
  - No runtime validation (design-time only)

##### 4.3.2 `PreloadATSGraphs()`
- **Responsabilité**: Pré-charger tous graphes ATS (Visual Scripts) trouvés
- **État après**: Graphes validés, prêts pour Asset Browser
- **Données**: Scanne `Blueprints/` et `Gamedata/` pour fichiers `.ats` et `.json`
- **Performance**: Peut prendre plusieurs secondes pour gros repos
- **Validation**: Chaque graphe validé pour erreurs de schéma/cycle

#### 4.4 `BlueprintEditor::Get().SetActive(true)`
- **Ligne**: 82 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Activer le backend (marquer comme active)
- **État après**: `m_IsActive = true` → Frontend commence à rendre

---

### 🖼️ PHASE 5: INITIALISATION FRONTEND (BlueprintEditorGUI Singleton)
**Lignes**: 84-86 (SDL_AppInit)

#### 5.1 `new Olympe::BlueprintEditorGUI()`
- **Ligne**: 84 dans `BlueprintEditorStandaloneMain.cpp`
- **Fichier**: `Source/BlueprintEditor/BlueprintEditorGUI.cpp`
- **Responsabilité**: Créer instance frontend GUI
- **Constructeur** (lignes 32-69 dans BlueprintEditorGUI.cpp):

**État après constructeur:**
```cpp
m_SelectedComponentIndex = -1
m_NextNodeId = 0
m_ShowDemoWindow = false
m_ShowAddComponentDialog = false
m_ShowAboutDialog = false
m_SelectedComponentType = 0

// Panels visibility flags
m_ShowAssetBrowser = true       // Panel main 1
m_ShowInspector = true          // Panel main 3
m_ShowNodeGraph = true          // Panel main 2
m_ShowTemplateBrowser = false   // Advanced panel
m_ShowHistory = false           // Advanced panel
m_ShowDebugger = false          // Advanced panel
m_ShowProfiler = false          // Advanced panel
m_ShowVSEditor = false          // Advanced panel
m_ShowPreferences = false       // Dialog
m_ShowShortcuts = false         // Dialog

// Layout parameters
m_AssetBrowserWidth = 400.0f
m_InspectorWidth = 400.0f
m_MinPanelWidth = 200.0f
m_SplitterSize = 8.0f
m_LeftPanelSplitHeight = 0.0f
m_InspectorPanelHeight = 0.0f
m_VerificationLogsPanelHeight = 0.0f

// All panel pointers = nullptr (lazy loaded)
m_TemplateBrowserPanel = nullptr
m_HistoryPanel = nullptr
m_VSEditorPanel = nullptr
m_DebugPanel = nullptr
m_ProfilerPanel = nullptr

// String buffers
m_NewBlueprintNameBuffer[0] = '\0'
m_FilepathBuffer[0] = '\0'
```

- **Stockage**: `g_BlueprintEditorGUI` (global pointer)

#### 5.2 `BlueprintEditorGUI::Initialize()`
- **Ligne**: 85 dans `BlueprintEditorStandaloneMain.cpp`
- **Fichier**: `Source/BlueprintEditor/BlueprintEditorGUI.cpp` (línea de inicio desconocida, need get_file)
- **Responsabilité**: Initialiser tous systèmes frontend ImGui

**À examiner**: Implementation détaillée de `BlueprintEditorGUI::Initialize()` (need more get_file)

---

### ✅ PHASE 6: RETURN → MAIN LOOP
**Ligne**: 109 dans `BlueprintEditorStandaloneMain.cpp`

#### 6.1 `return SDL_APP_CONTINUE`
- **Ligne**: 109 dans `BlueprintEditorStandaloneMain.cpp`
- **Responsabilité**: Signaler à SDL3 que l'app est prête, lancer event loop
- **État après**: SDL3 passe au mode callback:
  ```
  SDL_AppIterate() → appelé 60 FPS pour render
  SDL_AppEvent() → appelé sur événement utilisateur
  SDL_AppQuit() → appelé à fermeture app
  ```

---

## 📊 RÉSUMÉ DES APPELS

| Phase | Appels | Responsabilité |
|-------|--------|-----------------|
| 1 | 3 | SDL3 initialization (window, renderer) |
| 2 | 8 | ImGui context + style + IO configuration |
| 3 | 2 | ImGui backends (SDL3 + SDLRenderer3) |
| 4 | 10+ | Blueprint Editor backend (managers, config, plugins) |
| 5 | 1+ | Blueprint Editor frontend (GUI initialization) |
| **TOTAL** | **24+** | **Complete pre-loop initialization** |

---

## 🔗 DÉPENDANCES CRITIQUES

```
SDL3 (window + renderer)
  ↓
ImGui context
  ↓
ImGui backends (SDL3, SDLRenderer3)
  ↓
BlueprintEditor::Get().Initialize()
  ├─ EnumCatalogManager
  ├─ NodeGraphManager
  ├─ EntityInspectorManager
  ├─ TemplateManager
  ├─ CommandStack
  ├─ InitializePlugins()
  ├─ LoadConfig()
  └─ RefreshAssets()
  ↓
BlueprintEditor::Get().InitializeStandaloneEditor()
  ├─ EditorContext::InitializeStandalone()
  └─ PreloadATSGraphs()
  ↓
BlueprintEditor::Get().SetActive(true)
  ↓
BlueprintEditorGUI::Initialize()
  ├─ ImGui::CreateContext() [déjà fait]
  ├─ ImNodes::CreateContext()
  ├─ FontManager::Initialize()
  ├─ Create panels (lazy loaded)
  └─ LoadEditorConfig()
  ↓
SDL_APP_CONTINUE → SDL_AppIterate() [MAIN LOOP]
```

---

## 📝 NOTES IMPORTANTES

1. **Ordre critique**: Chaque phase DOIT compléter avant la suivante
   - SDL3 AVANT ImGui
   - ImGui context AVANT ImGui backends
   - ImGui backends AVANT Blueprint Editor

2. **Gestion d'erreurs**: Chaque appel SDL3 a check + cleanup + return failure

3. **Singletons**: Tous les managers utilisent pattern singleton `Get()`
   - `EnumCatalogManager::Get()`
   - `NodeGraphManager::Get()`
   - `EntityInspectorManager::Get()`
   - `TemplateManager::Get()`
   - `EditorContext::Get()`

4. **Lazy loading**: Panels créés à demande (sauf core panels)
   - `m_VSEditorPanel`: Créé quand clique sur tab VisualScript
   - `m_DebugPanel`: Créé quand ouvre debug
   - Etc.

5. **Configuration**: Chargée depuis JSON
   - `blueprint_editor_config.json` (backend config)
   - Editor layout config
   - Recent files
   - Presets utilisateur

6. **Standalone mode**: Pas de restrictions runtime
   - Full CRUD operations enabled
   - No world synchronization needed
   - Design-time only validation

---

## 🎯 PROCHAINES ÉTAPES

Pour créer la **base neuve et simple**:

1. **Simplifier Phase 2-3**: ImGui init peut être condensé
2. **Consolidate Phase 4**: Managers peuvent être regroupés
3. **Defer Phase 5**: Lazy load frontend panels
4. **Plugin system**: Rendre extensible sans hard-code types
5. **Dependency injection**: Remplacer singletons par DI framework

