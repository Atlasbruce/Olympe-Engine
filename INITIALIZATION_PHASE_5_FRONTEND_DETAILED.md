# 📊 PHASE 5 DÉTAILLÉE - INITIALISATION FRONTEND (BlueprintEditorGUI)

**Fichier**: `Source/BlueprintEditor/BlueprintEditorGUI.cpp` (lignes 104-160+)

**Responsabilité**: Initialiser tous systèmes frontend ImGui et créer panels

---

## 🌳 HIÉRARCHIE PHASE 5

```
BlueprintEditorGUI::Initialize()
├─ 5.2.1: ImNodes::CreateContext()
├─ 5.2.2: ImNodes::StyleColorsDark()
├─ 5.2.3: ImNodes::GetStyle()
├─ 5.2.4: FontManager::Get().Initialize()
├─ 5.2.5: FontManager::Get().LoadFontAwesome()
├─ 5.2.6: AssetBrowser::Initialize()
├─ 5.2.7: AssetBrowser::SetAssetOpenCallback()
├─ 5.2.8: NodeGraphPanel::Initialize()
├─ 5.2.9: EntitiesPanel::Initialize()
├─ 5.2.10: InspectorPanel::Initialize()
├─ 5.2.11: TemplateBrowserPanel creation + Initialize()
├─ 5.2.12: HistoryPanel creation + Initialize()
├─ 5.2.13: VisualScriptEditorPanel creation + Initialize()
├─ 5.2.14: DebugPanel creation + Initialize()
├─ 5.2.15: ProfilerPanel creation + Initialize()
└─ 5.2.16: Layout configuration loading
```

---

## 📋 APPELS DÉTAILLÉS PHASE 5

### ImGui-Node-Based Editor Setup

#### 5.2.1 `ImNodes::CreateContext()`
- **Ligne**: 107 dans `BlueprintEditorGUI.cpp`
- **Include**: `#include "../third_party/imnodes/imnodes.h"`
- **Responsabilité**: Créer contexte ImNodes global (node editor library)
- **Paramètres**: Aucun
- **Retour**: `ImNodesContext*` (devient contexte courant)
- **État après**: ImNodes context créé, prêt à rendre nodes et connections
- **Note**: ImNodes est built-on top of ImGui, nécessite ImGui::CreateContext() AVANT

#### 5.2.2 `ImNodes::StyleColorsDark()`
- **Ligne**: 108 dans `BlueprintEditorGUI.cpp`
- **Responsabilité**: Appliquer thème ImNodes "Dark"
- **Paramètres**: Aucun
- **État après**: Couleurs nodes/links appliquées (dark theme)

#### 5.2.3 `ImNodes::GetStyle()`
- **Ligne**: 111 dans `BlueprintEditorGUI.cpp`
- **Responsabilité**: Accéder à ImNodesStyle (customization)
- **Paramètres**: Aucun
- **Retour**: `ImNodesStyle&`
- **État après**: Reference `style` prêt à modifier

#### 5.2.3a `style.Flags |= ImNodesStyleFlags_GridLines`
- **Ligne**: 112 dans `BlueprintEditorGUI.cpp`
- **Responsabilité**: Activer affichage grille dans node editor
- **État après**: Grille visible sous nodes

---

### Font & Resource Loading

#### 5.2.4 `FontManager::Get().Initialize()`
- **Ligne**: 115 dans `BlueprintEditorGUI.cpp`
- **Fichier**: `Source/Core/FontManager.h/cpp`
- **Responsabilité**: Initialiser gestionnaire de polices
- **État après**: FontManager singleton prêt à charger fonts
- **Gestion**: Crée mémoire pour texture atlases

#### 5.2.5 `FontManager::Get().LoadFontAwesome("Assets/Fonts/fa-solid-900.otf", 16.0f)`
- **Ligne**: 116 dans `BlueprintEditorGUI.cpp`
- **Responsabilité**: Charger polices Font Awesome (icônes UI)
- **Paramètres**:
  - `"Assets/Fonts/fa-solid-900.otf"`: Path au fichier Font Awesome
  - `16.0f`: Taille (pixels)
- **État après**: Icons chargées, utilisables dans UI (buttons, labels, etc.)
- **Gestion d'erreur**: Si fichier manquant → graceful fallback (texte simple)

---

### Asset Browser & Core Panels

#### 5.2.6 `m_AssetBrowser.Initialize("Blueprints")`
- **Ligne**: 119 dans `BlueprintEditorGUI.cpp`
- **Type**: `AssetBrowser m_AssetBrowser` (member, not pointer)
- **Responsabilité**: Initialiser asset browser panel
- **Paramètres**: `"Blueprints"` (racine répertoire assets)
- **État après**: Asset browser prêt à scanner et afficher fichiers
- **Scan**: Récursivement traverse `Blueprints/` pour trouver graphes

#### 5.2.7 `m_AssetBrowser.SetAssetOpenCallback([this](const std::string& path) { LoadBlueprint(path); })`
- **Ligne**: 122-124 dans `BlueprintEditorGUI.cpp`
- **Responsabilité**: Définir callback exécuté quand utilisateur double-click asset
- **Paramètres**: Lambda fonction capturant `this`
- **État après**: Callback enregistré, appelé lors de sélection asset
- **Effet**: Double-click asset → LoadBlueprint() appelée → Graphe ouvert dans onglet

#### 5.2.8 `m_NodeGraphPanel.Initialize()`
- **Ligne**: 127 dans `BlueprintEditorGUI.cpp`
- **Type**: `NodeGraphPanel m_NodeGraphPanel` (member, not pointer)
- **Responsabilité**: Initialiser panel graphe de nœuds
- **État après**: Panel prêt à rendre nœuds et connections

#### 5.2.9 `m_EntitiesPanel.Initialize()`
- **Ligne**: 128 dans `BlueprintEditorGUI.cpp`
- **Type**: `EntitiesPanel m_EntitiesPanel` (member, not pointer)
- **Responsabilité**: Initialiser panel entités
- **État après**: Panel prêt à afficher liste entités

#### 5.2.10 `m_InspectorPanel.Initialize()`
- **Ligne**: 129 dans `BlueprintEditorGUI.cpp`
- **Type**: `InspectorPanel m_InspectorPanel` (member, not pointer)
- **Responsabilité**: Initialiser panel inspecteur (properties)
- **État après**: Panel prêt à afficher properties entité sélectionnée

---

### Advanced Panels (Heap-Allocated, Lazy Loading Possible)

#### 5.2.11 Template Browser Panel Creation + Initialization
- **Lignes**: 132-133 dans `BlueprintEditorGUI.cpp`
- **Type**: `TemplateBrowserPanel*` (pointer, heap-allocated)
- **Appel 1**: `new TemplateBrowserPanel()`
  - Crée instance sur heap
  - Stockage: `m_TemplateBrowserPanel` (global reference)
- **Appel 2**: `m_TemplateBrowserPanel->Initialize()`
  - Initialise contenu panel
  - Charge templates disponibles

#### 5.2.12 History Panel Creation + Initialization
- **Lignes**: 136-137 dans `BlueprintEditorGUI.cpp`
- **Type**: `HistoryPanel*` (pointer, heap-allocated)
- **Appel 1**: `new HistoryPanel()`
- **Appel 2**: `m_HistoryPanel->Initialize()`

#### 5.2.13 VisualScriptEditorPanel Creation + Initialization
- **Lignes**: 140-141 dans `BlueprintEditorGUI.cpp`
- **Type**: `VisualScriptEditorPanel*` (pointer, heap-allocated)
- **Appel 1**: `new VisualScriptEditorPanel()`
- **Appel 2**: `m_VSEditorPanel->Initialize()`
- **Responsabilité**: Panel spécialisé pour éditer Visual Scripts (ATS graphs)

#### 5.2.14 Debug Panel Creation + Initialization
- **Lignes**: 143-144 dans `BlueprintEditorGUI.cpp`
- **Type**: `DebugPanel*` (pointer, heap-allocated)
- **Appel 1**: `new DebugPanel()`
- **Appel 2**: `m_DebugPanel->Initialize()`

#### 5.2.15 Profiler Panel Creation + Initialization
- **Lignes**: 146-147 dans `BlueprintEditorGUI.cpp`
- **Type**: `ProfilerPanel*` (pointer, heap-allocated)
- **Appel 1**: `new ProfilerPanel()`
- **Appel 2**: `m_ProfilerPanel->Initialize()`

---

### Configuration Loading

#### 5.2.16 Layout Configuration Loading
- **Lignes**: 149-160 dans `BlueprintEditorGUI.cpp`
- **Responsabilité**: Charger configuration de layout depuis backend

**Appels détaillés:**

##### 5.2.16a `BlueprintEditor::Get()`
- Accéder singleton backend

##### 5.2.16b `backend.GetConfig()`
- Retourne `nlohmann::json` config object

##### 5.2.16c `config["layout"]` & `layout["asset_browser_width"]` etc.
- Parse JSON pour layout settings
- Restaure dimensions panels depuis session précédente
- Si manquant → use defaults

**Paramètres JSON chargés:**
```json
{
  "layout": {
    "asset_browser_width": 400.0,
    "inspector_width": 400.0,
    "min_panel_width": 200.0,
    // ... autres settings
  }
}
```

---

## 📊 RÉSUMÉ PHASE 5

| Groupe | Appels | État après |
|--------|--------|-----------|
| ImNodes Setup | 3 | Node editor context + styling ready |
| Font Loading | 2 | Icons loaded, fonts ready for rendering |
| Core Panels | 3 | Asset browser, node graph, entities, inspector ready |
| Advanced Panels | 10 | All 5 advanced panels created + initialized |
| Configuration | 1 | Layout settings loaded from config |
| **TOTAL** | **19** | **Frontend fully initialized** |

---

## 🔗 DÉPENDANCES PHASE 5

```
ImGui context (Phase 2) ✓
  ↓
ImGui backends (Phase 3) ✓
  ↓
BlueprintEditor backend (Phase 4) ✓
  ↓
BlueprintEditorGUI::Initialize()
  ├─ ImNodes::CreateContext() → Requires ImGui context
  ├─ FontManager::Get().Initialize() → Independent
  ├─ AssetBrowser::Initialize() → Requires ImGui
  ├─ Panels::Initialize() → All require ImGui
  └─ Layout loading → Requires BlueprintEditor backend config
  ↓
Frontend fully operational ✓
```

---

## 📝 OBSERVATIONS IMPORTANTES

1. **Deux types de panels:**
   - **Stack-allocated** (m_AssetBrowser, m_NodeGraphPanel, etc.):
     - Créés automatiquement avec BlueprintEditorGUI instance
     - Toujours présents en mémoire
     - Inicializados dans Initialize()
   
   - **Heap-allocated** (m_TemplateBrowserPanel, m_HistoryPanel, etc.):
     - Créés avec `new` dans Initialize()
     - Deleted dans destructeur (~BlueprintEditorGUI)
     - Candidats pour lazy loading

2. **Lazy loading opportunity:**
   - Advanced panels (TemplateBrowser, History, Debug, Profiler) pourraient être créés à demande
   - VSEditorPanel créé toujours (?) - vérifier si nécessaire au démarrage

3. **Asset Browser callback:**
   - Important: Callback défini APRÈS Initialize()
   - Permet à asset browser de déclencher LoadBlueprint() lors de selection

4. **Configuration persistence:**
   - Layout settings sauvegardés lors shutdown
   - Restaurés lors next startup
   - Permet expérience utilisateur cohérente

---

## 🎯 ÉTAT APRÈS PHASE 5

```
✓ SDL3 window + renderer running
✓ ImGui context + backends operational
✓ ImNodes context + styling ready
✓ All fonts loaded (Font Awesome icons)
✓ Asset browser ready (scanning Blueprints/)
✓ All core panels (NodeGraph, Entities, Inspector) initialized
✓ All advanced panels created
✓ Layout configuration restored
✓ Backend managers running
✓ EditorContext in Standalone mode
✓ ATS graphs pre-loaded and validated

→ READY FOR FIRST RENDER (SDL_AppIterate)
```

