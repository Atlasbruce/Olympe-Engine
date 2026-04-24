# 🎯 GRAPHE D'APPELS COMPLET - VUE SYNTHÉTIQUE

Document récapitulatif avec tous les appels de fonction/méthode/classe AVANT la boucle principale

---

## 📊 GRAPHE HIÉRARCHIQUE COMPLET

```
╔════════════════════════════════════════════════════════════════════════════╗
║                    SDL_AppInit() - ENTRY POINT                             ║
║                                                                             ║
║  BlueprintEditorStandaloneMain.cpp :: SDL_AppResult SDL_AppInit(...)      ║
║  Ligne 29-110                                                               ║
╚════════════════════════════════════════════════════════════════════════════╝
                                    │
                ┌───────────────────┼───────────────────┐
                │                   │                   │
                ▼                   ▼                   ▼
    ┌──────────────────┐ ┌─────────────────┐ ┌──────────────────┐
    │  PHASE 1         │ │  PHASE 2        │ │  PHASE 3         │
    │  SDL3 INIT       │ │  ImGui CONTEXT  │ │  ImGui BACKENDS  │
    │                  │ │                 │ │                  │
    │  3 appels        │ │  8 appels       │ │  2 appels        │
    └──────────────────┘ └─────────────────┘ └──────────────────┘
          │                     │                     │
          ▼                     ▼                     ▼
   ┌─────────────────┐  ┌──────────────────┐  ┌──────────────────┐
   │SDL_Init()       │  │IMGUI_CHECKVER()  │  │ImGui_ImplSDL3    │
   │SDL_CreateWindow │  │ImGui::CreateCtx()│  │_InitForSDLRender │
   │SDL_CreateRender │  │ImGui::GetIO()    │  │ImGui_ImplSDL     │
   │                 │  │ConfigFlags|=Nav  │  │Renderer3_Init()  │
   │                 │  │IniFilename=null  │  │                  │
   │                 │  │StyleColorsDark() │  │                  │
   │                 │  │GetStyle() cfg    │  │                  │
   └─────────────────┘  └──────────────────┘  └──────────────────┘
          │                     │                     │
          └─────────────────────┼─────────────────────┘
                                │
                    ┌───────────┴───────────┐
                    │                       │
                    ▼                       ▼
        ┌──────────────────────────┐  ┌─────────────────────┐
        │  PHASE 4                 │  │ PHASE 5             │
        │  BACKEND INIT            │  │ FRONTEND INIT       │
        │                          │  │                     │
        │  10+ appels              │  │ 19 appels           │
        └──────────────────────────┘  └─────────────────────┘
                    │                       │
                    ▼                       ▼
        ┌──────────────────────────┐  ┌─────────────────────┐
        │BlueprintEditor::Get()    │  │ImNodes::CreateCtx() │
        │  .Initialize()           │  │ImNodes::StyleDark() │
        │    ├─ EnumCatalogMgr     │  │FontManager init     │
        │    ├─ NodeGraphMgr       │  │LoadFontAwesome()    │
        │    ├─ EntityInspectorMgr │  │AssetBrowser init    │
        │    ├─ TemplateMgr        │  │ SetAssetCallback    │
        │    ├─ CommandStack       │  │NodeGraphPanel init  │
        │    ├─ InitializePlugins()│  │EntitiesPanel init   │
        │    ├─ LoadConfig()       │  │InspectorPanel init  │
        │    └─ RefreshAssets()    │  │TemplateBrowserPanel│
        │                          │  │HistoryPanel        │
        │  .InitializeStandalone() │  │ VSEditorPanel       │
        │    ├─ EditorCtx.Init()   │  │ DebugPanel          │
        │    └─ PreloadATSGraphs() │  │ ProfilerPanel       │
        │                          │  │LoadLayoutConfig()   │
        │  .SetActive(true)        │  │                     │
        └──────────────────────────┘  └─────────────────────┘
                    │                       │
                    └───────────┬───────────┘
                                │
                    ┌───────────┴──────────┐
                    │                      │
                    ▼                      ▼
        ┌─────────────────────┐   ┌──────────────────┐
        │  PHASE 6            │   │  MAIN LOOP ENTRY │
        │  RETURN TO SDL3     │   │                  │
        │                     │   │  return          │
        │return SDL_APP_      │   │  SDL_APP_        │
        │CONTINUE             │   │  CONTINUE        │
        │                     │   │                  │
        │→ SDL_AppIterate()   │   │→ 60 FPS callback │
        │→ SDL_AppEvent()     │   │→ User events     │
        │→ SDL_AppQuit()      │   │→ Clean shutdown  │
        └─────────────────────┘   └──────────────────┘
```

---

## 📋 LISTE COMPLÈTE DES APPELS (24+ APPELS)

### PHASE 1: SDL3 INITIALIZATION (3 appels)
```
1. SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)
   ├─ Initialise subsystèmes SDL3
   ├─ Activation: système vidéo + event loop
   └─ Retour: bool (true = succès)

2. SDL_CreateWindow(title, 1920, 1080, flags)
   ├─ Crée fenêtre 1920x1080 resizable + maximized
   ├─ Stockage: g_BlueprintEditorWindow
   └─ Retour: SDL_Window* (nullptr = erreur)

3. SDL_CreateRenderer(window, nullptr)
   ├─ Crée renderer SDL3 pour dessiner
   ├─ Stockage: g_BlueprintEditorRenderer
   └─ Retour: SDL_Renderer* (nullptr = erreur)
```

### PHASE 2: ImGui CONTEXT & CONFIGURATION (8 appels)
```
4. IMGUI_CHECKVERSION()
   ├─ Vérifie compatibilité headers/runtime
   └─ Assertion si version mismatch

5. ImGui::CreateContext()
   ├─ Crée contexte ImGui global
   └─ Devient contexte courant

6. ImGui::GetIO()
   ├─ Accès à structure ImGuiIO
   └─ Retour: ImGuiIO& reference

7. io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard
   ├─ Active navigation clavier
   └─ ImGui accepte Tab, Arrows, Enter

8. io.IniFilename = nullptr
   ├─ Désactive sauvegarde imgui.ini
   └─ Configuration via JSON custom

9. ImGui::StyleColorsDark()
   ├─ Applique thème "Dark"
   └─ Couleurs par défaut

10. ImGui::GetStyle()
    ├─ Accès à ImGuiStyle
    └─ Retour: ImGuiStyle& reference

11. style.Colors[ImGuiCol_WindowBg] = ImVec4(...)
    ├─ Customise couleur window background
    └─ Couleur: (0.12, 0.12, 0.14, 1.0)

12. style.Colors[ImGuiCol_TitleBg] = ImVec4(...)
    ├─ Customise couleur title bar
    └─ Couleur: (0.15, 0.15, 0.18, 1.0)

    + style.Colors[ImGuiCol_TitleBgActive]
    └─ Couleur: (0.20, 0.40, 0.60, 1.0)
```

### PHASE 3: ImGui BACKENDS (2 appels)
```
13. ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)
    ├─ Initialise backend SDL3 pour ImGui
    ├─ Event handling + render target setup
    └─ Fichier: imgui_impl_sdl3.h

14. ImGui_ImplSDLRenderer3_Init(renderer)
    ├─ Initialise backend SDLRenderer3
    ├─ Draw lists → SDL3 rendering
    └─ Fichier: imgui_impl_sdlrenderer3.h
```

### PHASE 4: BACKEND INITIALIZATION (10+ appels)
```
15. BlueprintEditor::Get()
    ├─ Accès singleton BlueprintEditor
    └─ Retour: BlueprintEditor& reference

16. BlueprintEditor::Initialize()
    ├─ APPELS INTERNES:
    │
    ├─ 16.1: État initial
    │   ├─ m_IsActive = false
    │   ├─ m_HasUnsavedChanges = false
    │   ├─ m_CurrentBlueprint = EntityBlueprint()
    │   ├─ m_CurrentFilepath = ""
    │   ├─ m_AssetTreeRoot = nullptr
    │   └─ m_LastError.clear()
    │
    ├─ 16.2: EnumCatalogManager::Get().Initialize()
    │   └─ Charge enums depuis config
    │
    ├─ 16.3: NodeGraphManager::Get().Initialize()
    │   └─ Setup node ID counter
    │
    ├─ 16.4: EntityInspectorManager::Get().Initialize()
    │   └─ Setup inspecteur d'entités
    │
    ├─ 16.5: TemplateManager::Get().Initialize()
    │   └─ Charge templates depuis disque
    │
    ├─ 16.6: new Olympe::Blueprint::CommandStack()
    │   └─ Crée heap-allocated command stack
    │
    ├─ 16.7: InitializePlugins()
    │   └─ Charge plugins de type de graphe
    │
    ├─ 16.8: LoadConfig("blueprint_editor_config.json")
    │   └─ Charge configuration depuis fichier
    │
    └─ 16.9: RefreshAssets()
        └─ Scanner assets sur disque

17. BlueprintEditor::Get().InitializeStandaloneEditor()
    ├─ APPELS INTERNES:
    │
    ├─ 17.1: EditorContext::Get().InitializeStandalone()
    │   └─ Mode Standalone (full CRUD)
    │
    └─ 17.2: PreloadATSGraphs()
        └─ Pré-charge Visual Scripts

18. BlueprintEditor::Get().SetActive(true)
    ├─ m_IsActive = true
    └─ Frontend commence à rendre
```

### PHASE 5: FRONTEND INITIALIZATION (19 appels)
```
19. ImNodes::CreateContext()
    ├─ Crée contexte ImNodes global
    └─ Devient contexte courant

20. ImNodes::StyleColorsDark()
    ├─ Applique thème "Dark"
    └─ Couleurs nodes/links

21. ImNodes::GetStyle()
    ├─ Accès à ImNodesStyle
    └─ Retour: ImNodesStyle& reference

22. style.Flags |= ImNodesStyleFlags_GridLines
    ├─ Active grille visuelle
    └─ Grille affichée sous nodes

23. FontManager::Get().Initialize()
    ├─ Initialise gestionnaire de polices
    └─ Textures prêtes pour render

24. FontManager::Get().LoadFontAwesome(path, 16.0f)
    ├─ Charge polices Font Awesome
    ├─ Icons utilisables dans UI
    └─ Paramètres: path + taille (16px)

25. m_AssetBrowser.Initialize("Blueprints")
    ├─ Initialise asset browser
    ├─ Scanner répertoire Blueprints/
    └─ Affichage fichiers

26. m_AssetBrowser.SetAssetOpenCallback(lambda)
    ├─ Callback sur selection asset
    ├─ Double-click → LoadBlueprint()
    └─ Lambda: [this](const string& path) { LoadBlueprint(path); }

27. m_NodeGraphPanel.Initialize()
    ├─ Initialise panel graphe de nœuds
    └─ Prêt à rendre

28. m_EntitiesPanel.Initialize()
    ├─ Initialise panel entités
    └─ Prêt à rendre

29. m_InspectorPanel.Initialize()
    ├─ Initialise panel inspecteur
    └─ Prêt à afficher properties

30. new TemplateBrowserPanel()
    ├─ Crée instance sur heap
    └─ Stockage: m_TemplateBrowserPanel

31. m_TemplateBrowserPanel->Initialize()
    ├─ Initialise contenu panel
    └─ Charge templates

32. new HistoryPanel()
    ├─ Crée instance sur heap
    └─ Stockage: m_HistoryPanel

33. m_HistoryPanel->Initialize()
    ├─ Initialise contenu panel
    └─ Prêt pour affichage historique

34. new VisualScriptEditorPanel()
    ├─ Crée instance sur heap
    └─ Stockage: m_VSEditorPanel

35. m_VSEditorPanel->Initialize()
    ├─ Initialise Visual Script editor
    └─ Prêt pour édition ATS graphs

36. new DebugPanel()
    ├─ Crée instance sur heap
    └─ Stockage: m_DebugPanel

37. m_DebugPanel->Initialize()
    ├─ Initialise debug panel
    └─ Prêt pour debugging

38. new ProfilerPanel()
    ├─ Crée instance sur heap
    └─ Stockage: m_ProfilerPanel

39. m_ProfilerPanel->Initialize()
    ├─ Initialise profiler panel
    └─ Prêt pour profiling

40. BlueprintEditor::Get().GetConfig()
    ├─ Accès configuration backend
    └─ Retour: nlohmann::json& reference

41. config["layout"] parsing
    ├─ Parse layout settings
    ├─ Restaure dimensions panels
    └─ JSON: asset_browser_width, inspector_width, min_panel_width
```

---

## 📊 STATISTIQUES COMPLÈTES

| Phase | Appels | Types | Responsabilité |
|-------|--------|-------|-----------------|
| 1 | 3 | SDL3 API | Window + Renderer creation |
| 2 | 8 | ImGui API | Context + Styling + IO |
| 3 | 2 | ImGui Backends | SDL3 + SDLRenderer3 |
| 4 | 10+ | Singletons + Managers | Backend initialization |
| 5 | 19 | ImNodes + Panels | Frontend initialization |
| 6 | 1 | Return code | Main loop entry |
| **TOTAL** | **43+** | **Mixed** | **Complete pre-loop init** |

---

## 🔗 DÉPENDANCES CRITIQUES

```
┌─────────────────────────────────┐
│  SDL3_INIT_VIDEO | SDL_INIT_EVENTS
│  • Fenêtre (1920x1080)
│  • Renderer SDL3
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│  ImGui::CreateContext()
│  • IO Configuration
│  • Style Customization
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│  ImGui_ImplSDL3 + SDLRenderer3
│  • Event handling
│  • Render target
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│  BlueprintEditor::Initialize()
│  • 4 Managers
│  • Command Stack
│  • Plugins + Config + Assets
└─────────────────┬───────────────┘
                  │
         ┌────────┴────────┐
         │                 │
         ▼                 ▼
┌──────────────┐  ┌──────────────┐
│InitStandalone│  │SetActive()   │
│• EditorCtx   │  │Ready for UI  │
│• PreloadATS  │  │              │
└──────┬───────┘  └──────┬───────┘
       │                 │
       └────────┬────────┘
                │
                ▼
┌─────────────────────────────────┐
│  BlueprintEditorGUI::Initialize()
│  • ImNodes Context + Styling
│  • FontManager + FontAwesome
│  • Asset Browser
│  • 7 Panels (3 core, 4 advanced)
│  • Layout Configuration
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│  return SDL_APP_CONTINUE
│  → SDL3 launches event loop
│  → SDL_AppIterate() called 60x/sec
└─────────────────────────────────┘
```

---

## ✅ ÉTAT SYSTÈME À CHAQUE PHASE

```
Avant Phase 1:
  • SDL3 pas initialisé
  • Fenêtre inexistante
  • Renderer inexistant
  └─ ✗ Impossible de rendre

Après Phase 1:
  • SDL3 active
  • Fenêtre créée (1920x1080)
  • Renderer créé
  └─ ✓ Peut créer contexte ImGui

Après Phase 2:
  • ImGui context créé
  • IO configuré (clavier enabled)
  • Style Dark appliqué
  └─ ✓ Peut initialiser backends

Après Phase 3:
  • ImGui SDL3 backend prêt
  • ImGui SDLRenderer3 backend prêt
  • Event handling setup
  └─ ✓ Peut initialiser Blueprint Editor

Après Phase 4:
  • 4 Managers chargés
  • Command stack créé
  • Plugins enregistrés
  • Assets scannés
  • Contexte Standalone configuré
  • Backend active
  └─ ✓ Peut initialiser Frontend

Après Phase 5:
  • ImNodes context créé
  • Fonts chargées
  • Asset Browser prêt
  • 7 Panels créés + initialisés
  • Layout restauré
  └─ ✓ PRÊT POUR MAIN LOOP

Après Phase 6:
  • SDL_APP_CONTINUE retourné
  • SDL3 lance event loop
  • SDL_AppIterate() appelé 60x/sec
  • User peut interagir
  └─ ✓ APPLICATION OPÉRATIONNELLE
```

---

## 🎯 POUR LA NOUVELLE BASE PROPRE

### À CONSERVER:
- ✅ Phase 1-3: SDL3 + ImGui foundation (core graphics layer)
- ✅ Phase 4 structure: Manager pattern (but peut être simplifié)
- ✅ Phase 5 principle: Panel initialization (but lazy loading recommended)

### À AMÉLIORER:
- ⚠️ Phase 4: Manager singletons → DI framework
- ⚠️ Phase 4: Hard-coded type switching → Plugin system
- ⚠️ Phase 5: Eager panel creation → Lazy loading
- ⚠️ Phase 5: Heap allocation → Smart pointers (unique_ptr)

### À AJOUTER:
- ➕ Proper error handling with recovery
- ➕ Initialization logging (state transitions)
- ➕ Dependency validation
- ➕ Initialization timeout handling
- ➕ Graceful degradation on load failure

---

## 📝 NOTES POUR REFACTORISATION

1. **Ordre invariant**: Phases 1-3 DOIVENT rester dans cet ordre (SDL3 → ImGui → Backends)
2. **Phase 4 simplification**: Réduire 10+ appels à ~5 appels via DI/factory
3. **Phase 5 lazy loading**: Créer panels seulement quand utilisateur les ouvre
4. **Error handling**: Ajouter recovery paths pour chaque phase
5. **Logging**: Ajouter traces pour débugage phase-by-phase

