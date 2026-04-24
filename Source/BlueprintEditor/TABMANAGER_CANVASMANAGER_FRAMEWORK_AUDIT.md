# AUDIT COMPLET: TabManager + Framework Canvas/Renderer
**Date**: 2026-04-17  
**Phase**: Framework Evolution Planning (Post-Animation)  
**Auteur**: Architecture Audit  
**Status**: 📋 COMPREHENSIVE AUDIT COMPLETE - READY FOR DESIGN PHASE

---

## 🎯 EXECUTIVE SUMMARY

**Constat**: Le framework actuel fonctionne mais est **fragmenté**:
- TabManager centralise la gestion des onglets et du chargement
- Chaque renderer implémente séparément: grille, pan, zoom, save, modales, sélection
- Aucune abstraction commune pour les features transversales
- Code dupliqué: 60-80% du code est identique entre VisualScript, BehaviorTree, EntityPrefab

**Opportunité**: Créer une **architecture unifiée IGraphEditor** qui:
1. Centralise 80% des features communes (grid, pan, zoom, save, modales, menus, shortcuts)
2. Permet aux types de graphe d'étendre pour leurs features spécifiques (20%)
3. Réduit de 40-50% la duplication de code
4. Standardise le flux de chargement/sauvegarde
5. Crée un système de plugins pour la vérification et les outils

**Timeline**: 3-4 phases, 2-3 semaines, 1.5 FTE

---

## 1. ÉTAT ACTUEL - INVENTAIRE COMPLET

### 1.1 Architecture Actuelle (5 Niveaux)

```
┌─────────────────────────────────────────────────────────────────┐
│ TIER 1: Application UI Layer                                   │
│  - BlueprintEditorGUI: Menu bar, layout principal, orchestration
│  - TabManager: Gestion des onglets, détection type, caching   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 2: Document Abstraction Layer                             │
│  - IGraphDocument: Interface (Load/Save/IsDirty/GetRenderer)   │
│  - 3 Implementations:                                          │
│    • VisualScriptGraphDocument (Adapter pattern)               │
│    • BehaviorTreeGraphDocument (Adapter pattern)               │
│    • EntityPrefabGraphDocument (Direct pattern)                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 3: Renderer Layer                                         │
│  - IGraphRenderer: Interface (Render/Load/Save/IsDirty)        │
│  - 3 Implementations (AVEC DUPLICATION MASSIVE):              │
│    • VisualScriptEditorPanel                                  │
│    • BehaviorTreeRenderer                                     │
│    • EntityPrefabRenderer                                     │
│  - Chaque renderer implémente INDÉPENDAMMENT:                 │
│    • Grid rendering (CanvasGridRenderer)                      │
│    • Pan/Zoom management                                      │
│    • Save/SaveAs modales                                      │
│    • Toolbar avec boutons                                     │
│    • Menu contextuels                                         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 4: Canvas Abstraction                                     │
│  - ICanvasEditor: Interface (Pan/Zoom/Grid/Coordinates)        │
│  - 2 Implementations:                                          │
│    • ImNodesCanvasEditor (pour VS & BT avec imnodes)          │
│    • CustomCanvasEditor (pour EntityPrefab)                   │
│    • [FUTURE] TimelineCanvasEditor (pour Animation)           │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 5: Utilities & Rendering Helpers                          │
│  - CanvasGridRenderer: Grille visuelle                         │
│  - CanvasMinimapRenderer: Minimap (Phase 37)                  │
│  - ComponentPalettePanel: Drag-drop nodes (EntityPrefab)      │
│  - PropertyEditorPanel: Édition propriétés (EntityPrefab)     │
│  - NodeSearchPanel: Recherche nodes                           │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 TabManager - Structure Détaillée

**Responsabilités**:
- ✅ Gestion collection EditorTab
- ✅ Détection type de graphe (JSON inspection)
- ✅ Création tabs vides (CreateNewTab)
- ✅ Ouverture fichiers (OpenFileInTab)
- ✅ Caching fichiers ouverts (Phase 51)
- ✅ Navigation tabs (SetActiveTab, GetActiveTab)
- ✅ Fermeture tabs avec dialog "unsaved changes"
- ✅ Gestion modales Save/SaveAs
- ✅ Rendu tab bar
- ✅ Delegation rendu canvas au renderer

**Data Structure**:
```cpp
class TabManager {
    std::vector<EditorTab> m_tabs;              // Collection tabs
    std::string m_activeTabID;                  // Tab actif
    int m_nextTabNum;                           // Counter pour noms
    int m_nextTabIDNum;                         // Counter pour IDs
    std::string m_pendingSelectTabID;           // Tab à activer (deferred)
    std::string m_pendingCloseTabID;            // Tab à fermer (deferred)
    bool m_showSaveAsDialog;                    // Modal SaveAs
    char m_saveAsBuffer[512];                   // Buffer filename
    std::string m_saveAsTabID;                  // Tab en cours de SaveAs
    std::map<std::string, std::string> m_loadedFilePaths;  // Cache (Phase 51)
}

struct EditorTab {
    std::string tabID;          // "tab_N"
    std::string displayName;    // Filename ou "Untitled-N"
    std::string filePath;       // Full path (empty = unsaved)
    std::string graphType;      // "VisualScript", "BehaviorTree", "EntityPrefab"
    bool isDirty;               // Unsaved changes?
    bool isActive;              // Currently selected?
    IGraphRenderer* renderer;   // Owned pointer
    IGraphDocument* document;   // Owned pointer (conditional cleanup Phase 56-57)
}
```

**Flux Principal** (OpenFileInTab):
1. Vérifier si fichier déjà ouvert (caching) → Activer tab existante
2. Détecter type graphe (DetectGraphType) → Inspection JSON
3. Créer EditorTab avec type détecté
4. Créer document IGraphDocument (polymorphe)
5. Créer renderer IGraphRenderer (polymorphe)
6. Appeler document->Load(filePath)
7. Retourner tabID
8. TabManager ajoutera tab à m_tabs

**Problèmes Identifiés**:
1. ❌ **Pas de standardisation des features** - Chaque renderer réimplémente grid, pan, zoom
2. ❌ **Duplication modale** - Save/SaveAs modales dans TabManager + renderers
3. ❌ **Pas de plugin system** - Impossible d'ajouter features de vérification custom
4. ❌ **Coordination faible** - Peu de synchro entre TabManager et renderers pour state
5. ❌ **Pas de abstraction "node palette/property panel"** - Harcoded dans EntityPrefab

---

## 2. AUDIT DES FEATURES EXISTANTES

### 2.1 Feature Matrix: Ce qui existe vs. ce qui manque

```
╔═══════════════════════════════════════╦════════════╦═════════════╦═════════════╦══════════╗
║ Feature                               ║ VisualScrpt║ BehaviorTree║ EntityPrefab║ Animation║
╠═══════════════════════════════════════╬════════════╬═════════════╬═════════════╬══════════╣
║ CORE RENDERING FEATURES               ║            ║             ║             ║          ║
║ ├─ Grid Display                       ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Pan (drag canvas)                  ║ ✅ ImNodes ║ ✅ ImNodes  ║ ✅ Custom   ║ ⏳ Needed║
║ ├─ Zoom (mouse wheel)                 ║ ❌ Fixed   ║ ❌ Fixed    ║ ✅ 0.1-3.0x ║ ⏳ Needed║
║ ├─ Minimap                            ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Grid Toggle (on/off)               ║ ✅ Toolbar ║ ✅ Toolbar  ║ ✅ Toolbar  ║ ⏳ Needed║
║ ├─ Snap-to-Grid                       ║ ✅ ImNodes ║ ✅ ImNodes  ║ ❌ No       ║ ⏳ Maybe  ║
║                                       ║            ║             ║             ║          ║
║ FILE OPERATIONS                       ║            ║             ║             ║          ║
║ ├─ Save (Ctrl+S)                      ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ SaveAs (Ctrl+Shift+S)              ║ ✅ Modal   ║ ✅ Modal    ║ ✅ Modal    ║ ⏳ Needed║
║ ├─ Close + "unsaved?" dialog          ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Dirty flag tracking                ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Tab title update on save           ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║                                       ║            ║             ║             ║          ║
║ USER INTERACTION                      ║            ║             ║             ║          ║
║ ├─ Single node selection              ║ ✅ Click   ║ ✅ Click    ║ ✅ Click    ║ ⏳ Needed║
║ ├─ Multi-select with Ctrl+Click       ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Rectangle selection                ║ ⚠️ Partial ║ ⚠️ Partial  ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Delete selected (Del key)          ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Node creation                      ║ ✅ Palette ║ ❌ Menu     ║ ✅ Palette  ║ ⏳ Needed║
║ ├─ Node connection                    ║ ✅ Drag    ║ ✅ Drag     ║ ✅ Drag     ║ ⏳ Needed║
║ ├─ Connection deletion                ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Right-Ctx║ ⏳ Needed║
║                                       ║            ║             ║             ║          ║
║ EDIT TOOLS & PANELS                   ║            ║             ║             ║          ║
║ ├─ Node Palette                       ║ ❌ Menu    ║ ❌ Menu     ║ ✅ Panel    ║ ⏳ Needed║
║ ├─ Property Editor                    ║ ✅ Partial ║ ✅ BT Param ║ ✅ Panel    ║ ⏳ Needed║
║ ├─ Local Variables                    ║ ✅ Panel   ║ ❌ No       ║ ❌ No       ║ ⏳ Maybe  ║
║ ├─ Global Variables                   ║ ✅ Panel   ║ ❌ No       ║ ❌ No       ║ ⏳ Maybe  ║
║ ├─ Search/Find nodes                  ║ ❌ No      ║ ❌ No       ║ ❌ No       ║ ⏳ Needed║
║                                       ║            ║             ║             ║          ║
║ CONTEXT MENUS                         ║            ║             ║             ║          ║
║ ├─ Right-click on node                ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Right-click on connection          ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Right-click on empty space         ║ ⚠️ Partial ║ ⚠️ Partial  ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Copy/Paste                         ║ ❌ No      ║ ❌ No       ║ ❌ No       ║ ❌ No    ║
║ ├─ Undo/Redo                          ║ ❌ No      ║ ❌ No       ║ ❌ No       ║ ❌ No    ║
║                                       ║            ║             ║             ║          ║
║ VERIFICATION & TOOLS                  ║            ║             ║             ║          ║
║ ├─ Graph Verification                 ║ ❌ No      ║ ✅ Toolbar  ║ ❌ No       ║ ⏳ Needed║
║ ├─ Simulation/Execution Trace         ║ ❌ No      ║ ✅ Output   ║ ❌ No       ║ ⏳ Needed║
║ ├─ Error Highlighting                 ║ ❌ No      ║ ✅ Yes      ║ ❌ No       ║ ⏳ Needed║
║ ├─ Output Panel (logs)                ║ ❌ No      ║ ✅ Yes      ║ ❌ No       ║ ⏳ Needed║
║ ├─ Breakpoints                        ║ ❌ No      ║ ✅ Yes      ║ ❌ No       ║ ❌ No    ║
║                                       ║            ║             ║             ║          ║
║ TOOLBAR BUTTONS                       ║            ║             ║             ║          ║
║ ├─ Grid toggle                        ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Pan/Zoom reset                     ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Minimap toggle                     ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Verify button                      ║ ❌ No      ║ ✅ Yes      ║ ❌ No       ║ ⏳ Needed║
║ ├─ Run/Execute button                 ║ ❌ No      ║ ✅ Yes      ║ ❌ No       ║ ⏳ Needed║
║                                       ║            ║             ║             ║          ║
║ STATE MANAGEMENT                      ║            ║             ║             ║          ║
║ ├─ Persist pan/zoom on tab switch     ║ ✅ Yes     ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Persist canvas state (SaveCanvasState)║✅ Yes  ║ ✅ Yes      ║ ✅ Yes      ║ ⏳ Needed║
║ ├─ Selection persistence              ║ ✅ Partial ║ ✅ Partial  ║ ✅ Yes      ║ ⏳ Needed║
╚═══════════════════════════════════════╩════════════╩═════════════╩═════════════╩══════════╝
```

### 2.2 Analyse Détaillée par Type de Graphe

#### 2.2.1 VisualScript

**Status**: ✅ Production (Phase 1+)  
**Canvas**: ImNodesCanvasEditor (imnodes native, fixed 1.0x zoom)  
**Pattern**: Adapter (VisualScriptGraphDocument wraps VisualScriptEditorPanel)

**Features Actuelles**:
```
✅ Grid rendering (CanvasGridRenderer.h)
✅ Pan (ImNodes native via EditorContext)
❌ Zoom (fixed 1.0x limitation imnodes)
✅ Minimap (ImNodes::MiniMap native, Phase 37)
✅ Grid toggle (toolbar)
✅ Dirty flag tracking
✅ Save/SaveAs modales (TabManager + OnSaveClicked)
✅ Node creation (via right-click "create node" menu)
✅ Node selection (click)
✅ Multi-select (Ctrl+Click, rectangle drag)
✅ Delete (Del key)
✅ Connection drag
✅ Local Variables panel (VisualScriptEditorPanel::RenderLocalVariablesPanel)
✅ Global Variables panel (VisualScriptEditorPanel::RenderGlobalVariablesPanel)
✅ Preset Conditions panel (Phase 24)
✅ Context menus (partial)
⚠️ Rectangle selection (works but via imnodes, not custom)
❌ Copy/Paste
❌ Undo/Redo
❌ Search nodes
❌ Verification tools
❌ Plugin system for custom panels
```

**Code Duplication Issues**:
- Grid rendering: Duplicated logic in RenderContent() for grid setup
- Pan logic: Wrapped by imnodes, renderer doesn't need to reimplement
- Zoom: N/A (fixed 1.0x)
- Save/SaveAs: Modal logic partially in TabManager, partially in renderer (OnSaveClicked)

**Panel Architecture** (RenderContent in VisualScriptEditorPanel):
```cpp
// Main layout: 3 sections
─ Top: Toolbar (Save, SaveAs, Verify, Run, Grid toggle, minimap controls)
└─ Left: Node graph (ImNodes)
└─ Right: Tabbed panels
   ├─ Tab 0: Preset Bank (Condition Presets)
   ├─ Tab 1: Local Variables (Local Blackboard)
   └─ Tab 2: Global Variables (Global Blackboard)
```

#### 2.2.2 BehaviorTree

**Status**: ✅ Production (Phase 27+, Phase 61 ImNodes adapter)  
**Canvas**: ImNodesCanvasEditor (with BehaviorTreeImNodesAdapter from Phase 61)  
**Pattern**: Adapter (BehaviorTreeGraphDocument via NodeGraphManager)

**Features Actuelles**:
```
✅ Grid rendering (CanvasGridRenderer)
✅ Pan (ImNodes native)
❌ Zoom (fixed 1.0x)
✅ Minimap (ImNodes::MiniMap, Phase 37)
✅ Grid toggle
✅ Dirty flag tracking
✅ Save/SaveAs modales
✅ Node creation (via NodeGraphPanel right-click menu)
✅ Node selection (click)
✅ Multi-select (Ctrl+Click, rectangle drag)
✅ Delete (Del key)
✅ Connection drag
❌ Local/Global Variables (not applicable to BT)
✅ BehaviorTreeImNodesAdapter (Phase 61) - routing, coordinate transforms
✅ Node parameters editing (BT-specific)
✅ Verification button (Verify Graph in toolbar)
✅ Simulation/Execution trace (Output panel with logs)
✅ Context menus (right-click on nodes)
❌ Copy/Paste
❌ Undo/Redo
❌ Search nodes
❌ Plugin system
```

**Code Duplication Issues**:
- Same grid rendering logic as VS
- Same pan logic (ImNodes)
- Same toolbar setup (grid toggle, minimap, pan/zoom reset)
- Similar modal handling (TabManager Save/SaveAs)

**Unique BehaviorTree Features**:
- NodeGraphPanel integration (legacy panel)
- BehaviorTreeImNodesAdapter (Phase 61) for data routing
- Verification logic (specific to BT execution model)
- Simulation trace output

**Panel Architecture** (BehaviorTreeRenderer::Render):
```cpp
─ Top: Toolbar (BT-specific: Verify, Run/Execute, Grid, Minimap)
├─ Left: Node graph (ImNodes with BehaviorTreeImNodesAdapter)
└─ Right: BT Parameters panel (type-specific)
```

#### 2.2.3 EntityPrefab

**Status**: ✅ Production (Phase 27+)  
**Canvas**: CustomCanvasEditor (custom zoom 0.1x-3.0x, pan via offset)  
**Pattern**: Direct (EntityPrefabGraphDocument owns data)

**Features Actuelles**:
```
✅ Grid rendering (CanvasGridRenderer, custom zoom-aware)
✅ Pan (custom m_canvasOffset, ImGui drag)
✅ Zoom (0.1x - 3.0x via mouse wheel)
✅ Minimap (CanvasMinimapRenderer overlay, Phase 37)
✅ Grid toggle
✅ Dirty flag tracking
✅ Save/SaveAs modales (TabManager)
✅ Node creation (ComponentPalettePanel drag-drop, Phase 29)
✅ Node selection (click, visual glow highlight)
✅ Multi-select (Ctrl+Click, rectangle selection Phase 31)
✅ Rectangle selection (Phase 31)
✅ Delete (Del key, RenderContextMenu)
✅ Connection drag (Phase 30)
✅ Connection visual feedback (hover highlight Phase 30)
✅ Component Palette panel (Phase 29 - drag-drop component list)
✅ Property Editor panel (Phase 31 - edit selected node properties)
✅ Context menus (right-click on nodes/connections/empty space Phase 30)
✅ Coordinate transforms verified (Phase 29 fixes)
✅ Connection hit detection (GetDistanceToConnection Bezier sampling)
❌ Copy/Paste nodes
❌ Undo/Redo
❌ Search/Filter components
❌ Verification tools
```

**Code Architecture** (EntityPrefabRenderer):
```cpp
─ Top: Toolbar (Grid toggle, Pan/Zoom reset, Minimap controls)
├─ Left: Node graph (PrefabCanvas)
│  └─ Custom ImGui rendering with Bezier curves
│  └─ Multi-selection with rectangle
│  └─ Context menus on right-click
└─ Right: Tabbed panels
   ├─ Tab 0: Component Palette (drag-drop component list)
   └─ Tab 1: Property Editor (edit node properties)
```

**Advantages vs. ImNodes**:
- Full zoom capability (0.1x-3.0x)
- Custom interaction handling (rectangle selection, context menus)
- Simpler for simpler graph types (prefabs less complex than BT)

**Custom Canvas Code** (PrefabCanvas):
- OnMouseMove, OnMouseDown, OnMouseUp, OnMouseScroll (input handlers)
- ScreenToCanvas, CanvasToScreen (coordinate transforms Phase 29)
- RenderGrid, RenderNodes, RenderConnections
- SelectNodesInRectangle (Phase 31)
- Context menu rendering

---

## 3. PROBLÈMES IDENTIFIÉS

### 3.1 Duplication de Code (60-80%)

| Feature | VisualScript | BehaviorTree | EntityPrefab | Duplication |
|---------|--------------|--------------|--------------|-------------|
| Grid rendering | ✅ | ✅ | ✅ | 100% (CanvasGridRenderer) |
| Pan implementation | ✅ ImNodes | ✅ ImNodes | ✅ Custom | 50% similar |
| Zoom handling | ❌ N/A | ❌ N/A | ✅ Custom | 0% |
| Minimap | ✅ | ✅ | ✅ | 100% (CanvasMinimapRenderer) |
| Save/SaveAs modal | 🔄 Partial | 🔄 Partial | 🔄 Partial | 80% |
| Toolbar setup | ✅ Duped | ✅ Duped | ✅ Duped | 75% |
| Node selection | ✅ Duped | ✅ Duped | ✅ Duped | 85% |
| Context menus | ✅ Duped | ✅ Duped | ✅ Duped | 80% |
| Dirty tracking | ✅ Duped | ✅ Duped | ✅ Duped | 100% |
| **TOTAL** | | | | **~70% avg** |

### 3.2 Absence d'Abstraction Commune

**Features Manquantes**:
1. **IGraphEditorBase** (unifying abstraction)
   - Pas de classe mère commune pour renderers
   - Chaque renderer implémente IGraphRenderer de manière isolée
   - Pas de contract pour features communes (grid, toolbar, modales)

2. **INodePalette** (abstraction for node creation)
   - Only EntityPrefab has panel-based palette (ComponentPalettePanel)
   - VisualScript & BehaviorTree use menu-based system
   - Impossible to swap implementations
   - No common interface for type-specific palettes

3. **IPropertyPanel** (abstraction for property editing)
   - Only EntityPrefab has dedicated PropertyEditorPanel
   - VisualScript has inline properties
   - BehaviorTree has parameter editing spread across
   - No standardized way to add new property panels

4. **IVerificationTool** (abstraction for graph validation)
   - Only BehaviorTree has verification
   - No interface for custom verifiers
   - Verification logic hardcoded in renderer
   - Cannot add type-specific verification tools

5. **IOutputPanel** (abstraction for logs/traces)
   - Only BehaviorTree has output panel
   - Cannot share verification output system
   - No standardized tracing mechanism

### 3.3 Problèmes Spécifiques

**TabManager**:
- ❌ Toute la logique Save/SaveAs en une place (inefficace pour 3+ types)
- ❌ Aucune séparation entre features communes et type-spécifiques
- ❌ Modal coordination difficile à suivre (Phase 45 fix needed proper frame timing)
- ❌ Pas de plugin system pour ajouter nouvelles features

**Renderers** (VS, BT, Prefab):
- ❌ Chacun réimplémente ~200-300 lignes de code pour toolbar
- ❌ Grid setup code dupliqué (même si CanvasGridRenderer existe)
- ❌ Pas d'interface commune pour toolbar buttons
- ❌ Context menus complètement custom à chaque type
- ❌ Selection logic dupliqué (single, multi, rectangle)

**Canvas Editors** (ImNodesCanvasEditor vs CustomCanvasEditor):
- ✅ Bien séparés (bon)
- ⚠️ Mais pas de interface commune pour grid/minimap/toolbar options
- ❌ Impossible d'avoir des options canvas uniformes

---

## 4. ARCHITECTURE PROPOSÉE - UNIFIED FRAMEWORK

### 4.1 Nouvelle Hiérarchie (6 Niveaux Proposés)

```
┌──────────────────────────────────────────────────────────────────────┐
│ TIER 1: Application Layer (unchanged)                               │
│  - BlueprintEditorGUI: Main UI orchestrator                         │
│  - TabManager: Tab collection + life cycle management               │
└──────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────────┐
│ TIER 2: Document Abstraction Layer (unchanged)                      │
│  - IGraphDocument: Interface (Load/Save/IsDirty)                    │
│  - 3 Concrete Implementations (existing)                            │
│  - [FUTURE] AnimationGraphDocument                                  │
└──────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────────┐
│ TIER 3: UNIFIED RENDERER BASE ✨ NEW                                │
│  - IGraphEditor (new base interface)                               │
│    • Render() + RenderToolbar()                                     │
│    • PanZoom management (abstracted)                                │
│    • Selection management (common)                                  │
│    • Context menus (framework support)                              │
│    • Dirty flag + save flow                                         │
│  - GraphEditorBase (abstract base class) ✨ NEW                    │
│    • Common implementation of 80% features:                         │
│      - Toolbar rendering (grid, pan/zoom reset, minimap)           │
│      - Grid management (on/off toggle, style)                       │
│      - Pan/zoom state persistence                                   │
│      - Selection management (single, multi-select, rectangle)       │
│      - Context menu framework                                       │
│      - Dirty flag coordination                                      │
│      - Common keyboard shortcuts (Ctrl+S, Del, Ctrl+A, etc.)       │
│      - Modal dialogs (Save/SaveAs/Unsaved)                         │
│      - File operations delegation                                   │
│    • Template methods for type-specific overrides                   │
│  - 3 Concrete Implementations (refactored from existing):           │
│    • VisualScriptEditorPanel : GraphEditorBase                    │
│    • BehaviorTreeRenderer : GraphEditorBase                       │
│    • EntityPrefabRenderer : GraphEditorBase                       │
│  - [FUTURE] AnimationGraphRenderer : GraphEditorBase              │
└──────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────────┐
│ TIER 4: Canvas Abstraction Layer (minimal changes)                  │
│  - ICanvasEditor (existing, refined)                               │
│  - ImNodesCanvasEditor (existing)                                  │
│  - CustomCanvasEditor (existing)                                   │
│  - [FUTURE] TimelineCanvasEditor (for Animation)                  │
└──────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────────┐
│ TIER 5: PLUGIN SYSTEM ✨ NEW                                        │
│  - IEditorTool (interface for custom tools) ✨ NEW                 │
│    • Verification tools (IGraphVerifier)                            │
│    • Output panel tools (IOutputPanel)                              │
│    • Analysis tools (IGraphAnalyzer)                                │
│  - Concrete implementations:                                        │
│    • BehaviorTreeVerifier : IGraphVerifier                         │
│    • VisualScriptVerifier : IGraphVerifier ✨ NEW                  │
│    • EntityPrefabVerifier : IGraphVerifier ✨ NEW                  │
│    • TraceOutputPanel : IOutputPanel ✨ NEW                        │
│    • ErrorOutputPanel : IOutputPanel ✨ NEW                        │
├─ Type-Specific Panels (via plugin framework):                       │
│    • NodePalettePanel (with INodePaletteProvider interface)        │
│    • PropertyEditorPanel                                            │
│    • CustomPanels (per type)                                        │
└──────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────────┐
│ TIER 6: Utilities (mostly unchanged)                                │
│  - CanvasGridRenderer                                               │
│  - CanvasMinimapRenderer                                            │
│  - Coordinate transforms utilities                                  │
│  - [REFACTOR] Common selection utilities (ISelectionManager)       │
│  - [REFACTOR] Common shortcut handler (IShortcutManager)           │
└──────────────────────────────────────────────────────────────────────┘
```

### 4.2 GraphEditorBase - Core Implementation

```cpp
// === NEW FILE: Source/BlueprintEditor/Framework/GraphEditorBase.h ===

class GraphEditorBase : public IGraphRenderer
{
public:
    virtual ~GraphEditorBase() = default;

    // --- IGraphRenderer Implementation ---
    virtual void Render() override final;           // FINAL - calls template methods
    virtual bool Load(const std::string& path) override = 0;
    virtual bool Save(const std::string& path) override = 0;
    virtual bool IsDirty() const override = 0;
    virtual std::string GetGraphType() const override = 0;
    virtual std::string GetCurrentPath() const override = 0;

    // --- Canvas State Management ---
    virtual void SaveCanvasState() override;
    virtual void RestoreCanvasState() override;

    // --- Framework Modals ---
    virtual void RenderFrameworkModals() override;

    // --- PROTECTED TEMPLATE METHODS (override in subclasses) ---

    /** Render the main graph/canvas area (nodes, links, etc.) */
    virtual void RenderGraphContent() = 0;

    /** Render type-specific toolbar buttons (optional, can be empty) */
    virtual void RenderTypeSpecificToolbar() {}

    /** Render type-specific panels (left/right side panels) */
    virtual void RenderTypePanels() {}

    /** Handle type-specific keyboard shortcuts (optional) */
    virtual void HandleTypeSpecificShortcuts() {}

    /** Get type-specific context menu options (optional) */
    virtual std::vector<std::string> GetTypeContextMenuOptions() { return {}; }

    /** Handle context menu item selection (optional) */
    virtual void OnContextMenuSelected(const std::string& option) {}

    // --- COMMON PROTECTED UTILITIES (available to subclasses) ---

    // Pan/Zoom
    void ApplyPanZoomTransform();
    void RenderCommonToolbar();
    void HandlePanZoomInput();

    // Selection
    void UpdateSelection(ImVec2 mousePos, bool ctrlPressed, bool dragStarted);
    void RenderSelectionRectangle();
    void SelectAll();
    void DeselectAll();

    // Context menus
    void RenderContextMenu();

    // Shortcuts
    void HandleCommonShortcuts();

    // State
    void MarkDirty();
    void ClearDirty();

    // --- MEMBER VARIABLES (protected, accessible to subclasses) ---

    ICanvasEditor* m_canvas;               // Pan/zoom/grid management
    ImVec2 m_canvasOffset;                 // Pan offset
    float m_canvasZoom;                    // Zoom level (1.0 = 100%)
    bool m_gridVisible;                    // Grid toggle state
    bool m_minimapVisible;                 // Minimap toggle state
    bool m_showContextMenu;                // Context menu state
    ImVec2 m_contextMenuPos;               // Where to show context menu

    // Selection state
    std::vector<int> m_selectedNodeIds;    // Multi-select
    bool m_isDrawingSelectionRect;
    ImVec2 m_selectionRectStart;
    ImVec2 m_selectionRectEnd;

    // Keyboard tracking
    bool m_ctrlPressed;
    bool m_shiftPressed;
    bool m_altPressed;

    // Save As dialog
    bool m_showSaveAsDialog;
    char m_saveAsBuffer[512];

    // Canvas state persistence
    json m_savedCanvasState;
};
```

### 4.3 Plugin System - IEditorTool Interface

```cpp
// === NEW FILES ===

// IEditorTool.h (base interface for any tool)
class IEditorTool
{
public:
    virtual ~IEditorTool() = default;

    virtual std::string GetToolName() const = 0;
    virtual void Initialize(IGraphDocument* document, GraphEditorBase* editor) = 0;
    virtual void Shutdown() = 0;
};

// IGraphVerifier.h
class IGraphVerifier : public IEditorTool
{
public:
    struct VerificationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    virtual VerificationResult Verify() = 0;
    virtual void HighlightError(const std::string& errorId) = 0;
};

// IOutputPanel.h
class IOutputPanel : public IEditorTool
{
public:
    virtual void RenderPanel() = 0;
    virtual void AddLog(const std::string& message, LogLevel level) = 0;
    virtual void Clear() = 0;
    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool visible) = 0;
};

// INodePaletteProvider.h
class INodePaletteProvider : public IEditorTool
{
public:
    virtual void RenderPalette() = 0;
    virtual bool AcceptDragDrop(const std::string& componentType, ImVec2 pos) = 0;
};

// Concrete implementations:
// BehaviorTreeVerifier : IGraphVerifier (existing logic extracted)
// VisualScriptVerifier : IGraphVerifier (verify script connections)
// EntityPrefabVerifier : IGraphVerifier (verify component compatibility)
```

### 4.4 Refactoring Strategy (Non-Breaking)

**Phase 1**: Create GraphEditorBase + plugin infrastructure
- Keep existing renderers unchanged
- Add side-by-side new implementations
- No forced migration

**Phase 2**: Refactor VisualScript
- Migrate VisualScriptEditorPanel → VisualScriptEditorPanel : GraphEditorBase
- Extract common code into base class
- Add VisualScriptVerifier plugin

**Phase 3**: Refactor BehaviorTree
- Migrate BehaviorTreeRenderer → BehaviorTreeRenderer : GraphEditorBase
- Extract NodeGraphPanel logic into plugins
- Leverage IGraphVerifier for existing verification

**Phase 4**: Refactor EntityPrefab
- Migrate EntityPrefabRenderer → EntityPrefabRenderer : GraphEditorBase
- ComponentPalettePanel → plugin (INodePaletteProvider)
- PropertyEditorPanel → plugin (IPropertyPanel)

**Phase 5**: Implement Animation
- AnimationGraphRenderer : GraphEditorBase
- TimelineCanvasEditor : ICanvasEditor
- AnimationVerifier plugin (simple validation)

---

## 5. DÉTAILS D'IMPLÉMENTATION PAR TYPE

### 5.1 VisualScriptEditorPanel (Refactored)

**Avant** (current):
```cpp
class VisualScriptEditorPanel : public IGraphRenderer
{
    // ~800 lines
    // - Toolbar code (150 lines)
    // - Grid setup (50 lines)
    // - Context menu (80 lines)
    // - Selection (100 lines)
    // - Save/SaveAs (100 lines)
    // - Type-specific: Local/Global vars, Presets (200 lines)
    // - Render (120 lines)
};
```

**Après** (refactored):
```cpp
class VisualScriptEditorPanel : public GraphEditorBase
{
    // ~400 lines (reduction of 50%)
    // Inherited from base: toolbar, grid, context menu, selection, save
    // Only implements:
    // - RenderGraphContent() (120 lines - imnodes setup)
    // - RenderTypeSpecificToolbar() (30 lines - Verify, Run buttons)
    // - RenderTypePanels() (150 lines - Local/Global vars, Presets)
    // - Type-specific shortcuts
    // - Load/Save implementation
};

// Plugins:
class VisualScriptVerifier : public IGraphVerifier
{
    // Verify script connections, detect cycles, etc.
};

class VisualScriptVariablesPanel : public IOutputPanel (or custom)
{
    // Existing Local/Global variables logic
};
```

**Code Reduction**:
- Base class provides: ~400 lines of common functionality
- Subclass now only ~300-400 lines (50% reduction)
- ~200-250 lines removed (toolbar, grid, menu, selection)

### 5.2 BehaviorTreeRenderer (Refactored)

**Avant** (current):
```cpp
class BehaviorTreeRenderer : public IGraphRenderer
{
    // ~700 lines
    // - Toolbar code (150 lines)
    // - ImNodes adapter (200 lines)
    // - Verification logic (150 lines)
    // - Output panel (100 lines)
    // - Grid setup (50 lines)
    // - Context menu (50 lines)
};
```

**Après** (refactored):
```cpp
class BehaviorTreeRenderer : public GraphEditorBase
{
    // ~350-400 lines (50% reduction)
    // Inherited: toolbar framework, grid, context menus, selection
    // Only implements:
    // - RenderGraphContent() (100 lines - imnodes + adapter)
    // - RenderTypeSpecificToolbar() (50 lines - Verify, Run buttons)
    // - RenderTypePanels() (100 lines - BT-specific params)
    // - HandleTypeSpecificShortcuts()
};

// Plugins:
class BehaviorTreeVerifier : public IGraphVerifier
{
    // Extracted: existing verification logic
};

class BehaviorTreeExecutionTracer : public IOutputPanel
{
    // Extracted: existing trace/output logic
};

class BehaviorTreeParametersPanel : public IEditorTool
{
    // Extracted: parameter editing UI
};
```

**Code Reduction**:
- Base class provides: ~350 lines
- Subclass now only ~350-400 lines (50% reduction)
- ~250 lines removed to plugins

### 5.3 EntityPrefabRenderer (Minimal Changes - Already Good)

**Avant** (current):
```cpp
class EntityPrefabRenderer : public IGraphRenderer
{
    // ~400-500 lines
    // - Well-structured already
    // - Has tabbed panel system
    // - Uses CustomCanvasEditor effectively
};
```

**Après** (minimal refactoring):
```cpp
class EntityPrefabRenderer : public GraphEditorBase
{
    // ~300-350 lines (reduction via inherited toolbar/grid)
    // Most code remains the same
    // Inherits common toolbar, grid, selection, context menu
    // Focus on: RenderGraphContent (PrefabCanvas)
    // Custom: RenderTypePanels (tabbed: Palette + Properties)
};

// Plugins:
class EntityPrefabComponentPalette : public INodePaletteProvider
{
    // Drag-drop component creation (existing ComponentPalettePanel)
};

class EntityPrefabPropertyEditor : public IEditorTool
{
    // Property editing (existing PropertyEditorPanel)
};

class EntityPrefabVerifier : public IGraphVerifier
{
    // Component compatibility checks
};
```

**Advantage**:
- EntityPrefab already has good structure
- Minimal breaking changes
- Can incrementally adopt base class features
- Custom canvas editor approach works well → keep it

---

## 6. STANDARDIZED LOADING PROCESS

### 6.1 Current Flow (Existing in TabManager)

```
User double-clicks file
    ↓
TabManager::OpenFileInTab(filePath)
    ├─ Check if already open (cache check)
    ├─ DetectGraphType(filePath) → reads JSON, inspects structure
    ├─ Create EditorTab + IGraphDocument (polymorphe)
    ├─ Create IGraphRenderer (polymorphe)
    ├─ document->Load(filePath)
    │  ├─ Parse JSON
    │  ├─ Instantiate internal data structures
    │  └─ Return success/failure
    ├─ renderer->Load(filePath) [OPTIONAL - may be redundant with document]
    └─ Return tabID
```

### 6.2 Proposed Standardized Flow

```
User double-clicks file (.ats, .json, .anim.json)
    ↓
BlueprintEditorGUI::File→Open
    ├─ Show file dialog
    ├─ Call TabManager::OpenFileInTab(filePath)
    └─ Render active tab
    ↓
TabManager::OpenFileInTab(filePath) [STANDARDIZED]
    ├─ [1] FILE RESOLUTION
    │  └─ DataManager::FindResourceRecursive(filename) → absolute path
    │
    ├─ [2] CACHE CHECK
    │  └─ Check if m_loadedFilePaths contains this file
    │  └─ If yes, activate existing tab + return
    │
    ├─ [3] TYPE DETECTION (robust)
    │  └─ DetectGraphType(filePath) → "VisualScript" | "BehaviorTree" | "EntityPrefab" | "Animation"
    │  └─ Inspect JSON structure:
    │     • graphType field (explicit)
    │     • schema_version + structural heuristics
    │     • Fallback: "Unknown"
    │
    ├─ [4] DOCUMENT CREATION (polymorphe IGraphDocument factory)
    │  └─ Create appropriate IGraphDocument subclass:
    │     • VisualScriptGraphDocument
    │     • BehaviorTreeGraphDocument
    │     • EntityPrefabGraphDocument
    │     • AnimationGraphDocument
    │
    ├─ [5] RENDERER CREATION (polymorphe IGraphRenderer factory)
    │  └─ Create appropriate renderer:
    │     • VisualScriptEditorPanel : GraphEditorBase
    │     • BehaviorTreeRenderer : GraphEditorBase
    │     • EntityPrefabRenderer : GraphEditorBase
    │     • AnimationGraphRenderer : GraphEditorBase
    │
    ├─ [6] DATA LOADING (into memory)
    │  ├─ document->Load(filePath)
    │  │  ├─ Open & parse JSON/binary
    │  │  ├─ Deserialize into internal structures
    │  │  ├─ Validate schema version
    │  │  ├─ Instantiate all nodes/connections
    │  │  └─ Mark as clean (isDirty = false)
    │  │
    │  ├─ [OPTIONAL] renderer->Load(filePath) if renderer needs independent loading
    │  │  └─ Usually delegates to document
    │  │
    │  └─ document->RestoreCanvasState() [Phase 35]
    │     └─ Restore previous pan/zoom/selection if saved
    │
    ├─ [7] LIBRARY LINKING (for node types/components)
    │  └─ Resolve node/component types from loaded graph:
    │     • For VisualScript: Load node type library (NodeTypeRegistry)
    │     • For BehaviorTree: Load BT node library (BehaviorNodeRegistry)
    │     • For EntityPrefab: Load component library (ComponentRegistry)
    │     • For Animation: Load animation curve library (AnimationCurveRegistry)
    │
    ├─ [8] CREATE EditorTab
    │  └─ EditorTab tab;
    │     tab.tabID = NextTabID();
    │     tab.displayName = DisplayNameFromPath(filePath);
    │     tab.filePath = filePath;
    │     tab.graphType = DetectedType;
    │     tab.isDirty = false;
    │     tab.document = document;
    │     tab.renderer = renderer;
    │
    ├─ [9] INITIALIZE RENDERER WITH DOCUMENT
    │  └─ renderer->SetDocument(document) [if needed]
    │  └─ renderer->Initialize() [setup UI state]
    │
    ├─ [10] CACHE UPDATE
    │  └─ m_loadedFilePaths[filePath] = filePath;
    │
    ├─ [11] ADD TO TAB COLLECTION
    │  └─ m_tabs.emplace_back(std::move(tab));
    │
    ├─ [12] ACTIVATE TAB
    │  └─ SetActiveTab(tabID);
    │
    └─ [13] RETURN SUCCESS
       └─ return tabID;
```

### 6.3 Verification & Error Handling at Each Step

```cpp
// Pseudo-code for robust loading
std::string TabManager::OpenFileInTab(const std::string& filePath)
{
    // [1] File resolution
    std::string absolutePath = DataManager::FindResourceRecursive(filePath, "GameData");
    if (absolutePath.empty()) {
        SYSTEM_LOG << "[ERROR] File not found: " << filePath << "\n";
        ShowErrorDialog("File not found");
        return "";
    }

    // [2] Cache check
    for (const auto& tab : m_tabs) {
        if (tab.filePath == absolutePath) {
            SetActiveTab(tab.tabID);
            return tab.tabID;
        }
    }

    // [3] Type detection
    std::string graphType = DetectGraphType(absolutePath);
    if (graphType == "Unknown") {
        SYSTEM_LOG << "[ERROR] Cannot determine graph type: " << filePath << "\n";
        ShowErrorDialog("Unknown graph type");
        return "";
    }

    // [4-9] Document + Renderer creation
    IGraphDocument* document = nullptr;
    IGraphRenderer* renderer = nullptr;

    if (graphType == "VisualScript") {
        // Factory pattern
        document = new VisualScriptGraphDocument();
        renderer = new VisualScriptEditorPanel();
    } else if (graphType == "BehaviorTree") {
        // ...
    } else if (graphType == "EntityPrefab") {
        // ...
    } else if (graphType == "Animation") {
        // ...
    }

    if (!document || !renderer) {
        ShowErrorDialog("Failed to create renderer");
        return "";
    }

    // [6] Load data
    if (!document->Load(absolutePath)) {
        SYSTEM_LOG << "[ERROR] Failed to load document: " << filePath << "\n";
        ShowErrorDialog("Failed to load file");
        delete document;
        delete renderer;
        return "";
    }

    // [7] Library linking
    if (!LinkLibrariesForDocument(document, graphType)) {
        SYSTEM_LOG << "[WARNING] Failed to link some libraries for: " << graphType << "\n";
        // Warning, not fatal
    }

    // [8-13] Tab creation and setup
    EditorTab tab;
    tab.tabID = NextTabID();
    tab.displayName = DisplayNameFromPath(absolutePath);
    tab.filePath = absolutePath;
    tab.graphType = graphType;
    tab.isDirty = false;
    tab.isActive = false;
    tab.document = document;
    tab.renderer = renderer;

    m_tabs.emplace_back(std::move(tab));
    m_loadedFilePaths[absolutePath] = absolutePath;
    SetActiveTab(tab.tabID);

    SYSTEM_LOG << "[SUCCESS] Loaded " << graphType << ": " << absolutePath << "\n";
    return tab.tabID;
}
```

---

## 7. VERIFICATION & PLUGIN SYSTEM

### 7.1 Plugin Architecture

```cpp
// === IEditorToolManager.h ===
class IEditorToolManager
{
public:
    virtual ~IEditorToolManager() = default;

    // Register tool
    virtual void RegisterTool(std::shared_ptr<IEditorTool> tool) = 0;

    // Query tools
    virtual std::vector<IGraphVerifier*> GetVerifiers() = 0;
    virtual std::vector<IOutputPanel*> GetOutputPanels() = 0;
    virtual INodePaletteProvider* GetNodePalette(const std::string& graphType) = 0;

    // Run verification
    virtual IGraphVerifier::VerificationResult RunVerification(
        IGraphDocument* document,
        const std::string& verifierType = ""
    ) = 0;

    // Render all active panels
    virtual void RenderToolPanels() = 0;
};

// === GraphEditorBase with tool integration ===
class GraphEditorBase : public IGraphRenderer
{
    // ...
    void RenderTypeSpecificToolbar() override
    {
        // Render type-specific toolbar buttons
        if (ImGui::Button("Verify Graph##btn", ImVec2(100, 0))) {
            RunVerification();
        }

        if (ImGui::Button("Run/Execute##btn", ImVec2(100, 0))) {
            ExecuteGraph();
        }
    }

    void RunVerification()
    {
        // Get verifier from plugin manager
        IEditorToolManager& toolMgr = IEditorToolManager::Get();
        auto result = toolMgr.RunVerification(m_document, m_graphType);

        if (!result.isValid) {
            // Show errors
            HighlightErrors(result.errors);
        }
    }

protected:
    IEditorToolManager* m_toolManager;
};
```

### 7.2 Concrete Tool Implementations

```cpp
// === BehaviorTreeVerifier.h ===
class BehaviorTreeVerifier : public IGraphVerifier
{
public:
    std::string GetToolName() const override { return "BehaviorTreeVerifier"; }

    VerificationResult Verify() override
    {
        // Extract existing BehaviorTreeRenderer::VerifyGraph logic
        // Check for:
        // - Disconnected nodes
        // - Invalid transitions
        // - Missing required fields
        // - Cycles (if not allowed)
        // - Type mismatches
    }

    void HighlightError(const std::string& errorId) override
    {
        // Highlight node in canvas
    }
};

// === VisualScriptVerifier.h (NEW) ===
class VisualScriptVerifier : public IGraphVerifier
{
public:
    std::string GetToolName() const override { return "VisualScriptVerifier"; }

    VerificationResult Verify() override
    {
        // Check for:
        // - Disconnected task chains
        // - Type mismatches in connections
        // - Missing required parameters
        // - Unresolved variable references
    }

    void HighlightError(const std::string& errorId) override { }
};

// === EntityPrefabVerifier.h (NEW) ===
class EntityPrefabVerifier : public IGraphVerifier
{
public:
    std::string GetToolName() const override { return "EntityPrefabVerifier"; }

    VerificationResult Verify() override
    {
        // Check for:
        // - Missing component dependencies
        // - Property type mismatches
        // - Circular component references
    }

    void HighlightError(const std::string& errorId) override { }
};

// === OutputPanel implementations ===
class ExecutionTracePanel : public IOutputPanel
{
    // BT execution trace, logs, breakpoint hits
};

class ErrorHighlightPanel : public IOutputPanel
{
    // Verification error display with clickable navigation
};

class DebuggerPanel : public IOutputPanel
{
    // Variable inspection, step execution
};
```

---

## 8. IMPLEMENTATION ROADMAP

### Phase 1: Foundation (1 week, 1 FTE)

**Objective**: Create base classes and plugin infrastructure without breaking existing code

**Deliverables**:
1. `GraphEditorBase.h/cpp` (400 lines)
   - Common toolbar rendering
   - Grid management
   - Selection management
   - Context menu framework
   - Modal dialog framework
   - Pan/zoom state persistence
   - Keyboard shortcut handling

2. `IEditorTool.h` (interface hierarchy)
   - `IGraphVerifier.h`
   - `IOutputPanel.h`
   - `INodePaletteProvider.h`

3. `IEditorToolManager.h/cpp` (tool registry + lifecycle)

4. Unit tests for GraphEditorBase template methods

**Files to Create**:
- Source/BlueprintEditor/Framework/GraphEditorBase.h (~150 lines)
- Source/BlueprintEditor/Framework/GraphEditorBase.cpp (~250 lines)
- Source/BlueprintEditor/Framework/IEditorTool.h (~200 lines)
- Source/BlueprintEditor/Framework/IEditorToolManager.h (~150 lines)
- Source/BlueprintEditor/Framework/IEditorToolManager.cpp (~100 lines)

**Build Status**: ✅ Should compile (new files, no changes to existing)

### Phase 2: Refactor VisualScript (1 week, 1 FTE)

**Objective**: First migration to new architecture as proof-of-concept

**Changes**:
1. Extract GraphEditorBase functionality from VisualScriptEditorPanel
   - Move toolbar code (150 lines removed)
   - Move grid setup (50 lines removed)
   - Move context menu (80 lines removed)
   - Move selection (100 lines removed)
   - ~380 lines removed, 80 lines added as template method calls

2. VisualScriptEditorPanel becomes:
   ```cpp
   class VisualScriptEditorPanel : public GraphEditorBase
   {
       // ~300 lines (was ~800)
       // Only implements: RenderGraphContent, RenderTypePanels, type-specific shortcuts
   };
   ```

3. Create VisualScriptVerifier plugin
   - Extract verification logic
   - Implement IGraphVerifier interface

4. Create VisualScriptVariablesPanel plugin
   - Extract Local/Global variables panels
   - Implement as toolbar-accessible panel

**Build Status**: ✅ Compiles + existing tests pass

### Phase 3: Refactor BehaviorTree (1 week, 1 FTE)

**Objective**: Migrate BT renderer, extract verification

**Changes**:
1. BehaviorTreeRenderer : GraphEditorBase
   - Move similar toolbar/grid/menu code (300+ lines removed)
   - Simplify by ~50%

2. Extract BehaviorTreeVerifier
   - Move existing verification logic
   - Implement IGraphVerifier interface
   - Register with tool manager

3. Create execution trace panel plugin
   - Existing output logic → IOutputPanel implementation

**Build Status**: ✅ Compiles + existing functionality preserved

### Phase 4: Refactor EntityPrefab (3 days, 1 FTE)

**Objective**: Minimal changes to EntityPrefab, add plugins

**Changes**:
1. EntityPrefabRenderer : GraphEditorBase
   - Leverage inherited toolbar/grid/selection
   - Keep most custom canvas logic
   - Minimal refactoring (~50 lines changed)

2. Extract ComponentPalettePanel → INodePaletteProvider plugin
3. Extract PropertyEditorPanel → IEditorTool plugin
4. Create EntityPrefabVerifier plugin (lightweight)

**Build Status**: ✅ Compiles + EntityPrefab works as before

### Phase 5: Implement Animation (2 weeks, 1 FTE)

**Objective**: Implement new Animation graph type using unified framework

**New Files**:
- AnimationGraphDocument : IGraphDocument (new graph type)
- AnimationGraphRenderer : GraphEditorBase (leverages 80% base functionality)
- TimelineCanvasEditor : ICanvasEditor (custom for timeline)
- AnimationVerifier : IGraphVerifier (lightweight validation)
- AnimationTimelineTracer : IOutputPanel (playback/sync logging)

**Benefits of Unified Framework for Animation**:
- ✅ Toolbar (grid, pan/zoom reset, minimap) already inherited
- ✅ Selection framework already provided
- ✅ Context menu framework ready
- ✅ Modal dialogs (Save/SaveAs) already provided
- ✅ Dirty flag tracking automatic
- ✅ Keyboard shortcuts standard
- → Only need to implement 200-300 lines of timeline-specific code
- → Without framework: would need 600+ lines of duplication

---

## 9. CODE REDUCTION ESTIMATE

### Current State (Fragmented)
```
VisualScriptEditorPanel:  ~800 lines
BehaviorTreeRenderer:     ~700 lines
EntityPrefabRenderer:     ~500 lines
─────────────────────────────────
TOTAL RENDERERS:         2,000 lines (lots of duplication)

Common logic duplicated across:
- Toolbar rendering:  ~150 lines × 3 = 450 lines duplicated
- Grid setup:        ~50 lines × 3 = 150 lines duplicated
- Context menus:     ~80 lines × 3 = 240 lines duplicated
- Selection:         ~100 lines × 3 = 300 lines duplicated
─────────────────────────────────
DUPLICATED:          ~1,140 lines (57% of total)
```

### After Refactoring (Unified Framework)
```
GraphEditorBase:        ~400 lines (ONE implementation)
VisualScriptEditorPanel: ~350 lines (50% reduction)
BehaviorTreeRenderer:    ~350 lines (50% reduction)
EntityPrefabRenderer:    ~350 lines (30% reduction)
─────────────────────────────────
TOTAL RENDERERS+BASE:   1,450 lines

Reduction: 2,000 - 1,450 = 550 lines (27% less code overall)
Plus: Eliminated duplication (1,140 lines was wasted duplicates)

Per new type (e.g., Animation):
- Without framework: ~600-800 lines
- With framework: ~200-300 lines (60-70% reduction!)
```

---

## 10. BACKWARD COMPATIBILITY

**No Breaking Changes**:
1. IGraphDocument interface unchanged ✅
2. IGraphRenderer interface unchanged ✅
3. ICanvasEditor interface unchanged ✅
4. TabManager API unchanged ✅
5. Existing file formats unchanged ✅

**Additive Only**:
- GraphEditorBase is new base class
- IEditorTool system is new, optional
- Existing renderers can migrate incrementally
- Can have mix of old IGraphRenderer + new GraphEditorBase implementations

**Migration Path**:
1. Phase 1: Add new base classes (zero impact on existing)
2. Phase 2-4: Migrate renderers one by one (can run old+new in parallel)
3. Phase 5: Add new graph types using new infrastructure

---

## 11. NEXT STEPS

### Immediate (Review & Approval)
1. Review this audit document with team
2. Confirm proposed architecture
3. Approve implementation roadmap
4. Assign resources (1-1.5 FTE for 4 weeks)

### Week 1: Phase 1 (Foundation)
1. Create GraphEditorBase skeleton
2. Create IEditorTool hierarchy
3. Create IEditorToolManager
4. Add unit tests for base class
5. Verify compilation, no functionality changes

### Weeks 2-3: Phases 2-4 (Migration)
1. Migrate VisualScript (with tests)
2. Migrate BehaviorTree (with tests)
3. Migrate EntityPrefab (with tests)
4. Run full regression tests after each phase

### Week 4: Phase 5 (New Type)
1. Implement Animation Graph using unified framework
2. Verify code reduction (should be 60-70% less than if coded independently)
3. Full integration testing

### Ongoing
1. Update documentation
2. Migrate new graph types to framework automatically
3. Monitor code duplication metrics (target: <20% across renderers)

---

## 12. RISK ASSESSMENT

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Breaking existing renderers | LOW | HIGH | Use additive approach, no existing API changes |
| Performance regression | LOW | MEDIUM | GraphEditorBase uses template methods (no vtable overhead) |
| Plugin system complexity | MEDIUM | LOW | Start with simple registry, can enhance later |
| Migration testing effort | MEDIUM | MEDIUM | Incremental migration with regression tests per phase |
| Adoption resistance | LOW | MEDIUM | Show code reduction metrics, Animation benefits |

---

## 13. SUCCESS METRICS

### Code Quality
- ✅ 40-50% reduction in renderer code duplication
- ✅ 60-70% code reduction for new graph types
- ✅ <20% code duplication across renderers (from 57%)

### Architecture
- ✅ All 4 graph types (VS, BT, Prefab, Animation) share common base
- ✅ Plugin system active with 6+ tools registered
- ✅ Standardized loading process documented + tested

### Developer Experience
- ✅ New graph type implementers inherit 80% functionality for free
- ✅ No need to reimplement toolbar, grid, selection, menus
- ✅ Clear extension points via IEditorTool plugins

### Testing
- ✅ 100% compilation success after each phase
- ✅ All existing tests pass after migration
- ✅ New Animation type fully functional with plugins

---

## APPENDIX A: Feature Comparison Matrix (Detailed)

[See Section 2.1 above for comprehensive feature matrix]

---

## APPENDIX B: File Organization (Proposed)

```
Source/BlueprintEditor/
├── Framework/
│   ├── IGraphDocument.h (existing)
│   ├── IGraphRenderer.h (existing)
│   ├── GraphEditorBase.h ✨ NEW
│   ├── GraphEditorBase.cpp ✨ NEW
│   ├── IEditorTool.h ✨ NEW
│   ├── IGraphVerifier.h ✨ NEW
│   ├── IOutputPanel.h ✨ NEW
│   ├── INodePaletteProvider.h ✨ NEW
│   ├── IEditorToolManager.h ✨ NEW
│   ├── IEditorToolManager.cpp ✨ NEW
│   ├── VisualScriptGraphDocument.h (refactored)
│   ├── BehaviorTreeGraphDocument.h (refactored)
│   ├── VisualScriptVerifier.h ✨ NEW
│   ├── BehaviorTreeVerifier.h (extracted)
│   └── EntityPrefabVerifier.h ✨ NEW
│
├── VisualScriptEditorPanel.h (refactored - 50% smaller)
├── VisualScriptEditorPanel.cpp (refactored)
├── BehaviorTreeRenderer.h (refactored - 50% smaller)
├── BehaviorTreeRenderer.cpp (refactored)
│
├── EntityPrefabEditor/
│   ├── EntityPrefabRenderer.h (minimal changes)
│   ├── EntityPrefabRenderer.cpp (minimal changes)
│   ├── EntityPrefabComponentPalette.h ✨ NEW plugin
│   ├── EntityPrefabComponentPalette.cpp ✨ NEW plugin
│   ├── EntityPrefabPropertyEditor.h ✨ NEW plugin
│   └── EntityPrefabPropertyEditor.cpp ✨ NEW plugin
│
├── AnimationEditor/ ✨ NEW FOLDER
│   ├── AnimationGraphDocument.h ✨ NEW
│   ├── AnimationGraphDocument.cpp ✨ NEW
│   ├── AnimationGraphRenderer.h ✨ NEW
│   ├── AnimationGraphRenderer.cpp ✨ NEW
│   ├── TimelineCanvasEditor.h ✨ NEW
│   ├── TimelineCanvasEditor.cpp ✨ NEW
│   ├── AnimationVerifier.h ✨ NEW
│   └── AnimationVerifier.cpp ✨ NEW
│
├── Utilities/
│   ├── ICanvasEditor.h (existing)
│   ├── ImNodesCanvasEditor.h/cpp (existing)
│   ├── CustomCanvasEditor.h/cpp (existing)
│   ├── CanvasGridRenderer.h/cpp (existing)
│   ├── CanvasMinimapRenderer.h/cpp (existing)
│   ├── ISelectionManager.h ✨ NEW (extract selection logic)
│   ├── ISelectionManager.cpp ✨ NEW
│   ├── IShortcutHandler.h ✨ NEW (standardized shortcuts)
│   └── IShortcutHandler.cpp ✨ NEW
│
├── TabManager.h (unchanged)
├── TabManager.cpp (unchanged, but uses new plugin system)
└── BlueprintEditorGUI.h/cpp (unchanged)
```

---

**Document Status**: ✅ COMPREHENSIVE AUDIT COMPLETE  
**Recommendation**: APPROVE + PROCEED with Phase 1  
**Timeline**: 4 weeks, 1-1.5 FTE  
**Risk Level**: LOW (additive, backward compatible)  
**Code Reduction**: 40-50% for existing types, 60-70% for new types  
**Next Review**: Post-Phase 1 (after GraphEditorBase + plugin system created)

