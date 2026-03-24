# 🏗️ ANALYSE ARCHITECTURALE - ÉDITEURS NODE GRAPH ATS
## Document de Refonte Pipeline ImNodes/ImGui
**Auteur**: Architecte Olympe Engine  
**Date**: 2025-02-19  
**Statut**: Phase 1 - Analyse Exhaustive  
**Objectif**: Unifier et simplifier l'architecture des éditeurs/visualiseurs de graphes

---

## 📊 RÉSUMÉ EXÉCUTIF

### Problématique Identifiée
- **Code dupliqué** entre Debug Window (F10) et Standalone Editor
- **Pipelines divergents** pour le rendu ImNodes/ImGui
- **APIs incohérentes** entre visualiseurs et éditeurs
- **Maintenance difficile** due à la duplication de logique
- **Confusion** sur les responsabilités de chaque couche

### Solution Proposée
Créer une **architecture unifiée inspirée d'Unreal Blueprint Editor** avec :
1. **NodeGraphCore v2.0** : Core engine partagé (document model, commandes, layout)
2. **NodeGraphRenderer** : Pipeline de rendu unifié ImNodes/ImGui
3. **EditorContext** : Mode éditeur vs visualiseur avec API commune
4. **Adaptateurs spécialisés** : BehaviorTree, HFSM, Blueprint, etc.

---

## 🔍 ANALYSE DÉTAILLÉE DES COMPOSANTS EXISTANTS

### 1. BehaviorTreeDebugWindow (Visualiseur F10)

#### Localisation
- **Fichier principal** : `Source/AI/BehaviorTreeDebugWindow.cpp` (3606 lignes)
- **Header** : `Source/AI/BehaviorTreeDebugWindow.h` (200 lignes)
- **Type** : Visualiseur runtime (read-only par défaut)

#### Responsabilités
```cpp
// PANEL 1: Liste des entités avec BT
void RenderEntityListPanel()
{
    - Filtrage et tri des entités
    - Recherche par nom
    - Filtres: Active Only, Has Target
    - Sélection d'entité pour inspection
}

// PANEL 2: Graphe de nœuds (ImNodes)
void RenderNodeGraphPanel()
{
    - Rendu custom des nœuds BT
    - Connexions (pins custom, pas ImNodes standard)
    - Highlighting du nœud actif (runtime)
    - Zoom/Pan manuel
    - Minimap custom
}

// PANEL 3: Inspecteur
void RenderInspectorPanel()
{
    - Blackboard (variables AI)
    - Execution Log
    - Node properties
}
```

#### Pipeline de Rendu (PROBLÈME)
```cpp
// ❌ PIPELINE CUSTOM - DIVERGE de l'éditeur Standalone
void RenderNodeGraphPanel()
{
    ImNodes::BeginNodeEditor();
    
    // ❌ Rendu CUSTOM sans utiliser NodeGraphShared
    for (const auto& layout : m_currentLayout)
    {
        // BeginNode manuel
        ImNodes::BeginNode(nodeId);
        ImNodes::BeginNodeTitleBar();
        ImGui::Text(...); // Custom header
        ImNodes::EndNodeTitleBar();
        
        // ❌ PINS CUSTOM (pas BeginInputAttribute/BeginOutputAttribute)
        RenderCustomPins();
        
        ImNodes::EndNode();
    }
    
    // ❌ CONNEXIONS CUSTOM (dessinées manuellement)
    RenderCustomConnections();
    
    ImNodes::EndNodeEditor();
}
```

#### Dépendances Spécifiques
- `BTGraphLayoutEngine` : Layout automatique des nœuds
- `BTConfig` : Configuration de rendu (JSON)
- `BTCommandStack` : Undo/Redo basique (mode éditeur expérimental)
- Accès direct aux ECS components (`BehaviorTreeRuntime_data`, `AIBlackboard_data`)

#### Points Forts
- ✅ Layout automatique efficace
- ✅ Zoom/Pan fluide
- ✅ Minimap fonctionnelle
- ✅ Highlighting runtime des nœuds actifs

#### Points Faibles
- ❌ **Pipeline de rendu entièrement custom**
- ❌ **N'utilise PAS NodeGraphShared helpers**
- ❌ **Pins dessinés manuellement** (pas ImNodes standard)
- ❌ **Connexions dessinées à la main** (pas ImNodes::Link)
- ❌ **Aucune réutilisation de NodeGraphCore**
- ❌ **Duplication massive de code de rendu**

---

### 2. BlueprintEditorStandalone (Éditeur CRUD)

#### Localisation
- **Entry point** : `Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp`
- **GUI** : `Source/BlueprintEditor/BlueprintEditorGUI.cpp`
- **Core** : `Source/NodeGraphCore/*` (nouveau système 2026)
- **Type** : Éditeur complet (full CRUD)

#### Architecture (Moderne)
```
BlueprintEditorStandaloneMain.cpp
    ├─> BlueprintEditorGUI
    │       ├─> NodeGraphManager (singleton)
    │       │       └─> GraphDocument (model)
    │       │               ├─> NodeGraphData (nodes, pins, links)
    │       │               ├─> CommandSystem (undo/redo)
    │       │               └─> BlackboardSystem
    │       │
    │       └─> NodeGraphPanel (view)
    │               ├─> NodeGraphShared (render helpers)
    │               └─> Renderer/Serializer/CommandAdapter
    │
    └─> ImNodes context (shared)
```

#### Pipeline de Rendu (MODERNE)
```cpp
// ✅ PIPELINE UNIFIÉ - Utilise NodeGraphShared
void NodeGraphPanel::Render()
{
    ImNodes::BeginNodeEditor();
    
    GraphDocument* doc = NodeGraphManager::Get().GetActiveGraph();
    
    // ✅ Utilise NodeGraphShared::RenderNode
    for (const auto& node : doc->GetNodes())
    {
        NodeGraphShared::RenderNodeVisual(
            node,
            isCurrentNode,
            pulseTimer,
            nodeStyle
        );
        // Scope géré par RenderNodeVisual :
        //   - ImNodes::BeginNode
        //   - ImNodes::BeginNodeTitleBar
        //   - NodeGraphShared::RenderNodeHeader
        //   - ImNodes::EndNodeTitleBar
        //   - BeginInputAttribute/BeginOutputAttribute
        //   - ImNodes::EndNode
    }
    
    // ✅ Connexions via ImNodes standard
    for (const auto& link : doc->GetLinks())
    {
        ImNodes::Link(link.id, link.startPinId, link.endPinId);
    }
    
    ImNodes::EndNodeEditor();
    
    // ✅ Event handling unifié
    HandleNodeGraphInteractions();
}
```

#### Points Forts
- ✅ **Architecture propre** avec séparation Model/View
- ✅ **Utilise NodeGraphShared helpers**
- ✅ **Pins ImNodes standard** (BeginInputAttribute/BeginOutputAttribute)
- ✅ **Connexions ImNodes standard** (ImNodes::Link)
- ✅ **CommandSystem** avec undo/redo robuste
- ✅ **Réutilisable** pour Blueprint, HFSM, StateGraph, etc.

#### Points Faibles
- ⚠️ Pas de highlighting runtime (normal pour éditeur)
- ⚠️ Pas de minimap (pourrait être ajouté)

---

### 3. NodeGraphCore (Système Core 2026)

#### Localisation
- `Source/NodeGraphCore/`

#### Composants
```
NodeGraphCore/
├── GraphDocument.h/cpp          # Document model (nodes, pins, links)
├── NodeGraphManager.h/cpp       # Multi-graph manager (tabs)
├── CommandSystem.h/cpp          # Undo/Redo system
├── BlackboardSystem.h/cpp       # Variables system
├── ILayoutEngine.h              # Interface layout
├── NodeAnnotations.h/cpp        # Metadata (comments, breakpoints)
├── GraphMigrator.h/cpp          # Migration tool
└── Commands/                    # Command pattern implementations
    ├── ConnectPinsCommand
    ├── CreateNodeCommand
    ├── DeleteNodeCommand
    ├── MoveNodeCommand
    └── ToggleNodeBreakpointCommand
```

#### Responsabilités
- ✅ **Model pur** (data structures)
- ✅ **Command pattern** (undo/redo)
- ✅ **Blackboard system**
- ✅ **Indépendant d'ImGui/ImNodes**
- ✅ **Serialization JSON**

---

### 4. NodeGraphShared (Helpers de Rendu)

#### Localisation
- `Source/NodeGraphShared/NodeGraphShared.h`
- `Source/NodeGraphShared/Renderer.h`
- `Source/NodeGraphShared/Serializer.h`
- `Source/NodeGraphShared/CommandAdapter.h`

#### Responsabilités
```cpp
namespace NodeGraphShared
{
    // Rendu unifié des headers
    void RenderNodeHeader(
        int nodeId,
        const NodeStyle& style,
        const char* icon,
        const std::string& title,
        bool isCurrentNode,
        float pulseTimer
    );
    
    // Calcul position pins
    ImVec2 ComputePinCenterScreen(
        int nodeId,
        const BTNodeLayout* layout,
        bool isOutput,
        float pinOffset,
        float headerPx,
        float currentZoom
    );
    
    // Dessin pins
    void DrawPinCircle(
        ImDrawList* drawList,
        const ImVec2& center,
        float radius,
        ImU32 fillColor,
        ImU32 outlineColor,
        float outlineThickness
    );
}
```

#### Utilisation
- ✅ **Utilisé par** : BlueprintEditorGUI, NodeGraphPanel
- ❌ **NON utilisé par** : BehaviorTreeDebugWindow

---

## 🎯 TABLEAU COMPARATIF

| **Aspect** | **BT Debug Window** | **Standalone Editor** | **Cible Unifiée** |
|------------|---------------------|----------------------|-------------------|
| **Pipeline Rendu** | Custom (manuel) | NodeGraphShared (unifié) | ✅ NodeGraphShared |
| **Pins** | Custom drawings | ImNodes standard | ✅ ImNodes standard |
| **Connexions** | Bezier manual | ImNodes::Link | ✅ ImNodes::Link |
| **Model** | Inline data | GraphDocument | ✅ GraphDocument |
| **Commands** | BTCommandStack (basique) | CommandSystem (robuste) | ✅ CommandSystem |
| **Undo/Redo** | Partiel | Complet | ✅ Complet |
| **Layout** | BTGraphLayoutEngine | ILayoutEngine | ✅ ILayoutEngine |
| **Minimap** | Custom | Absente | ✅ ImNodes minimap |
| **Zoom/Pan** | Custom | ImNodes standard | ✅ ImNodes standard |
| **Runtime Highlighting** | ✅ Présent | ❌ Absent | ✅ Présent (mode debug) |

---

## 🏛️ ARCHITECTURE CIBLE (Inspirée Unreal Blueprint)

### Principes Unreal Blueprint Editor
1. **Séparation Model/View** stricte
2. **Command Pattern** pour toutes les modifications
3. **Transaction System** avec undo/redo illimité
4. **Asset Browser** + Pin Type Registry
5. **Graph Schema** : validation des connexions
6. **Context-aware menus** (clic droit)
7. **Compile on save** avec diagnostics
8. **Debug visualization** : breakpoints, execution flow, watch variables

### Architecture Proposée

```
┌─────────────────────────────────────────────────────┐
│          APPLICATION LAYER                           │
│  ┌─────────────────┐     ┌──────────────────────┐  │
│  │ Standalone App  │     │ Debug Visualizer F10 │  │
│  └────────┬────────┘     └─────────┬────────────┘  │
│           │                        │                │
│           └───────────┬────────────┘                │
│                       │                             │
└───────────────────────┼─────────────────────────────┘
                        │
┌───────────────────────┼─────────────────────────────┐
│       UI LAYER        │                             │
│  ┌────────────────────▼───────────────┐            │
│  │   NodeGraphEditorGUI                │            │
│  │  ┌──────────────┐ ┌──────────────┐ │            │
│  │  │  TabSystem   │ │ PanelManager │ │            │
│  │  └──────────────┘ └──────────────┘ │            │
│  │  ┌──────────────┐ ┌──────────────┐ │            │
│  │  │  AssetBrowser│ │ Inspector    │ │            │
│  │  └──────────────┘ └──────────────┘ │            │
│  └────────────────────┬───────────────┘            │
│                       │                             │
│  ┌────────────────────▼───────────────┐            │
│  │   NodeGraphPanel (ImNodes Renderer)│            │
│  │  - Render loop                     │            │
│  │  - Event handling                  │            │
│  │  - Interaction (zoom, pan, select) │            │
│  └────────────────────┬───────────────┘            │
└───────────────────────┼─────────────────────────────┘
                        │
┌───────────────────────┼─────────────────────────────┐
│     CORE LAYER        │                             │
│  ┌────────────────────▼───────────────┐            │
│  │   NodeGraphManager (Singleton)      │            │
│  │  - Multi-graph management           │            │
│  │  - Active graph selection           │            │
│  └────────────────────┬───────────────┘            │
│                       │                             │
│  ┌────────────────────▼───────────────┐            │
│  │   GraphDocument                     │            │
│  │  ┌──────────────┐ ┌──────────────┐ │            │
│  │  │ NodeData     │ │ PinData      │ │            │
│  │  └──────────────┘ └──────────────┘ │            │
│  │  ┌──────────────┐ ┌──────────────┐ │            │
│  │  │ LinkData     │ │ Metadata     │ │            │
│  │  └──────────────┘ └──────────────┘ │            │
│  └────────────────────┬───────────────┘            │
│                       │                             │
│  ┌────────────────────▼───────────────┐            │
│  │   CommandSystem                     │            │
│  │  - Undo/Redo stack                 │            │
│  │  - Transaction management          │            │
│  └────────────────────────────────────┘            │
│                                                     │
│  ┌─────────────────────────────────────┐          │
│  │   BlackboardSystem                   │          │
│  │  - Variable management               │          │
│  │  - Type registry                     │          │
│  └─────────────────────────────────────┘          │
│                                                     │
│  ┌─────────────────────────────────────┐          │
│  │   ILayoutEngine (interface)          │          │
│  │  ├─> HierarchicalLayoutEngine        │          │
│  │  ├─> ForceDirectedLayoutEngine       │          │
│  │  └─> BTGraphLayoutEngine             │          │
│  └─────────────────────────────────────┘          │
└─────────────────────────────────────────────────────┘
                        │
┌───────────────────────┼─────────────────────────────┐
│   RENDER LAYER        │                             │
│  ┌────────────────────▼───────────────┐            │
│  │   NodeGraphRenderer                 │            │
│  │  - RenderNode()                     │            │
│  │  - RenderLink()                     │            │
│  │  - RenderMinimap()                  │            │
│  │  - ApplyZoom()                      │            │
│  └────────────────────┬───────────────┘            │
│                       │                             │
│  ┌────────────────────▼───────────────┐            │
│  │   NodeGraphShared Helpers           │            │
│  │  - RenderNodeHeader()               │            │
│  │  - DrawPinCircle()                  │            │
│  │  - ComputePinPosition()             │            │
│  └────────────────────┬───────────────┘            │
└───────────────────────┼─────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────┐
│   THIRD-PARTY LAYER                                 │
│  ┌────────────┐ ┌────────────┐ ┌────────────────┐ │
│  │   ImGui    │ │  ImNodes   │ │  nlohmann/json │ │
│  └────────────┘ └────────────┘ └────────────────┘ │
└─────────────────────────────────────────────────────┘
```

---

## 🔀 STRATÉGIE DE MIGRATION

### Étape 1 : Créer NodeGraphRenderer (Nouveau)
**Fichier** : `Source/NodeGraphCore/NodeGraphRenderer.h/cpp`

```cpp
namespace Olympe {
namespace NodeGraph {

struct RenderConfig
{
    bool enableMinimap = false;
    bool enableRuntimeHighlight = false;
    float pinRadius = 6.0f;
    float pinOutlineThickness = 2.0f;
    // ...
};

class NodeGraphRenderer
{
public:
    void BeginRender(GraphDocument* doc, const RenderConfig& config);
    void RenderNodes();
    void RenderLinks();
    void RenderMinimap();
    void EndRender();
    
    // Runtime highlighting (pour Debug mode)
    void SetCurrentNode(uint32_t nodeId);
    void SetExecutionToken(uint32_t nodeId);
    
private:
    GraphDocument* m_document = nullptr;
    RenderConfig m_config;
    uint32_t m_currentNodeId = 0;
    float m_pulseTimer = 0.0f;
};

} // namespace NodeGraph
} // namespace Olympe
```

### Étape 2 : Migrer BehaviorTreeDebugWindow vers NodeGraphRenderer

**Avant** (custom pipeline) :
```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    ImNodes::BeginNodeEditor();
    
    // ❌ Custom rendering
    for (const auto& layout : m_currentLayout)
    {
        RenderCustomNode(layout);
    }
    
    RenderCustomConnections();
    
    ImNodes::EndNodeEditor();
}
```

**Après** (unified pipeline) :
```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    // ✅ Utilise GraphDocument comme source de vérité
    NodeGraph::GraphDocument* doc = GetOrCreateGraphDocument();
    
    // ✅ Configuration pour mode debug
    NodeGraph::RenderConfig config;
    config.enableMinimap = true;
    config.enableRuntimeHighlight = true;
    
    // ✅ Pipeline unifié
    m_renderer.BeginRender(doc, config);
    m_renderer.SetCurrentNode(GetCurrentRuntimeNode());
    m_renderer.RenderNodes();
    m_renderer.RenderLinks();
    m_renderer.RenderMinimap();
    m_renderer.EndRender();
}
```

### Étape 3 : Adapter les données BT vers GraphDocument

**Nouveau helper** :
```cpp
namespace Olympe {

class BehaviorTreeAdapter
{
public:
    static NodeGraph::GraphDocument* ConvertToGraphDocument(
        const BehaviorTreeAsset* tree,
        const std::vector<BTNodeLayout>& layout
    );
    
    static void UpdateLayoutFromGraphDocument(
        NodeGraph::GraphDocument* doc,
        std::vector<BTNodeLayout>& outLayout
    );
};

} // namespace Olympe
```

---

## 📝 PLAN D'IMPLÉMENTATION DÉTAILLÉ

### Phase 1 : Foundation (NodeGraphRenderer)
1. ✅ Créer `NodeGraphRenderer.h/cpp`
2. ✅ Implémenter `BeginRender/EndRender`
3. ✅ Implémenter `RenderNodes` (délègue à NodeGraphShared)
4. ✅ Implémenter `RenderLinks` (ImNodes::Link standard)
5. ✅ Implémenter `RenderMinimap` (ImNodes minimap)
6. ✅ Ajouter support runtime highlighting

### Phase 2 : Adapter BehaviorTreeDebugWindow
1. ✅ Créer `BehaviorTreeAdapter`
2. ✅ Convertir `BehaviorTreeAsset` → `GraphDocument`
3. ✅ Remplacer `RenderNodeGraphPanel` custom par `NodeGraphRenderer`
4. ✅ Migrer zoom/pan vers ImNodes standard
5. ✅ Migrer minimap vers ImNodes minimap
6. ❌ Supprimer custom pin rendering
7. ❌ Supprimer custom connection rendering

### Phase 3 : Unification Standalone
1. ✅ Valider que Standalone utilise déjà NodeGraphShared
2. ✅ Migrer Standalone vers NodeGraphRenderer
3. ✅ Tester mode éditeur vs mode debug

### Phase 4 : Cleanup & Documentation
1. ❌ Déprécier `BehaviorTreeDebugWindow` ancien code
2. ✅ Archiver dans `Source/Deprecated/`
3. ✅ Documenter nouveau pipeline
4. ✅ Créer guide migration pour autres éditeurs (HFSM, etc.)

---

## 📦 FICHIERS À CRÉER

```
Source/NodeGraphCore/
├── NodeGraphRenderer.h          # ✅ NOUVEAU
├── NodeGraphRenderer.cpp        # ✅ NOUVEAU
└── NodeGraphEditorContext.h     # ✅ NOUVEAU (mode editor vs debug)

Source/NodeGraphShared/
├── BehaviorTreeAdapter.h        # ✅ NOUVEAU
└── BehaviorTreeAdapter.cpp      # ✅ NOUVEAU

Docs/Developer/
└── NodeGraph_Unified_Pipeline.md  # ✅ NOUVEAU
```

## 📦 FICHIERS À MODIFIER

```
Source/AI/
├── BehaviorTreeDebugWindow.h    # ⚠️ REFACTOR MAJEUR
└── BehaviorTreeDebugWindow.cpp  # ⚠️ REFACTOR MAJEUR

Source/BlueprintEditor/
├── BlueprintEditorGUI.cpp       # ⚠️ Migration vers NodeGraphRenderer
└── NodeGraphPanel.cpp           # ⚠️ Migration vers NodeGraphRenderer
```

## 📦 FICHIERS À DÉPRÉCIER

```
Source/Deprecated/
└── BehaviorTreeDebugWindow_Legacy/
    ├── BehaviorTreeDebugWindow_OLD.h
    ├── BehaviorTreeDebugWindow_OLD.cpp
    └── README_DEPRECATED.md
```

---

## 🎯 CRITÈRES DE SUCCÈS

### Fonctionnels
- [ ] Debug Window (F10) utilise NodeGraphRenderer
- [ ] Standalone Editor utilise NodeGraphRenderer
- [ ] Pins sont ImNodes standard (BeginInputAttribute/BeginOutputAttribute)
- [ ] Connexions sont ImNodes standard (ImNodes::Link)
- [ ] Minimap fonctionne (ImNodes minimap)
- [ ] Zoom/Pan fonctionne (ImNodes standard)
- [ ] Runtime highlighting fonctionne (mode debug)
- [ ] Undo/Redo fonctionne (CommandSystem)

### Techniques
- [ ] 0 erreurs compilation
- [ ] 0 warnings compilation
- [ ] Respect C++14 strict
- [ ] Tout dans namespace Olympe
- [ ] SYSTEM_LOG utilisé (pas std::cout)
- [ ] JSON helpers utilisés
- [ ] Build log fourni

### Maintenance
- [ ] Code dupliqué éliminé (< 5% duplication)
- [ ] API commune entre Debug et Standalone
- [ ] Documentation complète du nouveau pipeline
- [ ] Guide migration pour futurs éditeurs (HFSM, StateGraph)

---

## 📚 RÉFÉRENCES

### Unreal Engine Blueprint Editor
- [UE4 Blueprint Technical Guide](https://docs.unrealengine.com/en-US/ProgrammingAndScripting/Blueprints/TechnicalGuide/)
- Graph Editor Architecture : `EdGraphNode`, `EdGraph`, `UEdGraphSchema`
- Transaction System : `FScopedTransaction`
- Pin Type Registry : `FEdGraphPinType`

### ImNodes Documentation
- [ImNodes GitHub](https://github.com/Nelarius/imnodes)
- Minimap API : `ImNodes::MiniMap()`
- Standard Pins : `BeginInputAttribute/BeginOutputAttribute`
- Standard Links : `ImNodes::Link(id, startPin, endPin)`

---

## 📊 MÉTRIQUES ACTUELLES (Baseline)

| **Composant** | **Lignes de Code** | **Duplication** | **Complexité Cyclomatique** |
|---------------|--------------------|-----------------|-----------------------------|
| BehaviorTreeDebugWindow | 3606 LOC | ~60% (custom render) | 45 |
| NodeGraphPanel | 800 LOC | ~10% | 12 |
| NodeGraphShared | 200 LOC | 0% | 5 |
| **TOTAL** | **4606 LOC** | **~40% global** | **62** |

### Métriques Cibles (Post-Refonte)

| **Composant** | **Lignes de Code** | **Duplication** | **Complexité Cyclomatique** |
|---------------|--------------------|-----------------|-----------------------------|
| BehaviorTreeDebugWindow | 1200 LOC | <5% | 15 |
| NodeGraphPanel | 600 LOC | <5% | 10 |
| NodeGraphRenderer | 800 LOC | 0% | 18 |
| NodeGraphShared | 300 LOC | 0% | 8 |
| **TOTAL** | **2900 LOC** | **<5% global** | **51** |

**Réduction attendue** : -37% lignes de code, -87% duplication

---

**FIN DE L'ANALYSE - PRÊT POUR PHASE 2 (CONCEPTION)**
