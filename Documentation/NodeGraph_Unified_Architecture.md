# 🏛️ ARCHITECTURE UNIFIÉE NODE GRAPH - CONCEPTION DÉTAILLÉE
## Blueprint-Inspired Unified Pipeline for ATS Editors
**Auteur**: Architecte Olympe Engine  
**Date**: 2025-02-19  
**Statut**: Phase 2 - Conception Architecture  
**Version**: v2.0.0

---

## 📐 PRINCIPES DIRECTEURS

### Inspiration Unreal Engine Blueprint Editor
```
1. SÉPARATION MODEL/VIEW stricte
2. COMMAND PATTERN pour toutes les modifications
3. TRANSACTION SYSTEM avec undo/redo illimité
4. GRAPH SCHEMA pour validation des connexions
5. PIN TYPE REGISTRY pour typage fort
6. CONTEXT-AWARE MENUS (clic droit intelligent)
7. COMPILE ON SAVE avec diagnostics
8. DEBUG VISUALIZATION intégrée
```

### Objectifs Architecture
- ✅ **Zéro duplication** de code de rendu
- ✅ **API commune** entre Éditeur et Visualiseur
- ✅ **Pipeline unifié** ImNodes/ImGui
- ✅ **Extensibilité** pour nouveaux types de graphes
- ✅ **Maintenabilité** : < 3000 LOC total

---

## 🏗️ ARCHITECTURE CIBLE - COMPOSANTS DÉTAILLÉS

### 1. NodeGraphRenderer (Nouveau Core Renderer)

#### Responsabilité
Pipeline de rendu unifié pour tous les éditeurs de graphes, compatible ImNodes/ImGui.

#### Interface Publique
```cpp
// Source/NodeGraphCore/NodeGraphRenderer.h

namespace Olympe {
namespace NodeGraph {

/**
 * @enum RenderMode
 * @brief Mode de rendu du graphe
 */
enum class RenderMode
{
    Editor,      // Mode édition (interactions complètes)
    Visualizer,  // Mode visualisation (read-only)
    Debug        // Mode debug (runtime highlighting)
};

/**
 * @struct RenderConfig
 * @brief Configuration du pipeline de rendu
 */
struct RenderConfig
{
    RenderMode mode = RenderMode::Editor;
    
    // Minimap
    bool enableMinimap = false;
    float minimapSizeFraction = 0.15f;
    ImNodesMiniMapLocation minimapLocation = ImNodesMiniMapLocation_TopRight;
    
    // Runtime highlighting (mode Debug)
    bool enableRuntimeHighlight = false;
    float pulseFre

quency = 2.0f;
    
    // Pin rendering
    float pinRadius = 6.0f;
    float pinOutlineThickness = 2.0f;
    float pinHeaderHeight = 20.0f;
    float pinOutputReserve = 40.0f;
    
    // Zoom/Pan
    float defaultZoom = 1.0f;
    float minZoom = 0.1f;
    float maxZoom = 3.0f;
    bool enableGridSnapping = false;
    float gridSize = 16.0f;
    
    // Style
    const NodeStyleRegistry* styleRegistry = nullptr;
};

/**
 * @class NodeGraphRenderer
 * @brief Moteur de rendu unifié pour graphes de nœuds
 */
class NodeGraphRenderer
{
public:
    NodeGraphRenderer();
    ~NodeGraphRenderer();
    
    // ========================================================================
    // Lifecycle
    // ========================================================================
    
    /**
     * @brief Initialiser le renderer avec une configuration
     * @param config Configuration de rendu
     */
    void Initialize(const RenderConfig& config);
    
    /**
     * @brief Mettre à jour la configuration
     * @param config Nouvelle configuration
     */
    void UpdateConfig(const RenderConfig& config);
    
    /**
     * @brief Nettoyer les ressources
     */
    void Shutdown();
    
    // ========================================================================
    // Rendering Pipeline
    // ========================================================================
    
    /**
     * @brief Commencer le rendu d'un graphe
     * @param document GraphDocument à rendre
     * @param deltaTime Temps écoulé depuis dernier frame (pour animations)
     */
    void BeginRender(GraphDocument* document, float deltaTime);
    
    /**
     * @brief Rendre tous les nœuds
     */
    void RenderNodes();
    
    /**
     * @brief Rendre tous les liens
     */
    void RenderLinks();
    
    /**
     * @brief Rendre la minimap (si activée)
     */
    void RenderMinimap();
    
    /**
     * @brief Terminer le rendu et gérer les interactions
     * @return true si le graphe a été modifié
     */
    bool EndRender();
    
    // ========================================================================
    // Runtime Highlighting (mode Debug)
    // ========================================================================
    
    /**
     * @brief Définir le nœud actif (runtime)
     * @param nodeId ID du nœud actif (0 = aucun)
     */
    void SetCurrentNode(uint32_t nodeId);
    
    /**
     * @brief Définir le token d'exécution (pour animations)
     * @param nodeId ID du nœud avec token
     */
    void SetExecutionToken(uint32_t nodeId);
    
    /**
     * @brief Effacer le highlighting runtime
     */
    void ClearRuntimeHighlight();
    
    // ========================================================================
    // Zoom & Pan
    // ========================================================================
    
    /**
     * @brief Ajuster le zoom pour voir tout le graphe
     */
    void FitGraphToView();
    
    /**
     * @brief Centrer la vue sur le graphe
     */
    void CenterView();
    
    /**
     * @brief Réinitialiser le zoom à 100%
     */
    void ResetZoom();
    
    /**
     * @brief Obtenir le zoom actuel
     */
    float GetCurrentZoom() const { return m_currentZoom; }
    
    /**
     * @brief Définir le zoom
     * @param zoom Facteur de zoom (clamped entre min/max)
     */
    void SetZoom(float zoom);
    
    // ========================================================================
    // Interaction Handling
    // ========================================================================
    
    /**
     * @brief Obtenir le nœud survolé
     * @return ID du nœud ou 0
     */
    uint32_t GetHoveredNode() const;
    
    /**
     * @brief Obtenir le pin survolé
     * @return ID du pin ou 0
     */
    uint32_t GetHoveredPin() const;
    
    /**
     * @brief Obtenir le lien survolé
     * @return ID du lien ou 0
     */
    uint32_t GetHoveredLink() const;
    
    /**
     * @brief Vérifier si un lien a été créé
     * @param outStartPin ID du pin de départ
     * @param outEndPin ID du pin d'arrivée
     * @return true si un lien vient d'être créé
     */
    bool IsLinkCreated(uint32_t* outStartPin, uint32_t* outEndPin) const;
    
    /**
     * @brief Vérifier si un lien a été supprimé
     * @param outLinkId ID du lien supprimé
     * @return true si un lien vient d'être supprimé
     */
    bool IsLinkDestroyed(uint32_t* outLinkId) const;
    
private:
    // Rendering helpers
    void RenderNode(const NodeData& node);
    void RenderLink(const LinkData& link);
    void ApplyRuntimeHighlight(uint32_t nodeId, NodeStyle& style);
    void UpdatePulseAnimation(float deltaTime);
    
    // Zoom/Pan helpers
    void ApplyZoomToStyle();
    void GetGraphBounds(Vector& outMin, Vector& outMax) const;
    Vector CalculatePanOffset(const Vector& graphCenter, const Vector& viewportSize) const;
    
    // State
    GraphDocument* m_document = nullptr;
    RenderConfig m_config;
    float m_currentZoom = 1.0f;
    uint32_t m_currentNodeId = 0;
    uint32_t m_executionTokenId = 0;
    float m_pulseTimer = 0.0f;
    
    // Interaction state
    uint32_t m_hoveredNodeId = 0;
    uint32_t m_hoveredPinId = 0;
    uint32_t m_hoveredLinkId = 0;
    bool m_linkCreated = false;
    uint32_t m_createdLinkStartPin = 0;
    uint32_t m_createdLinkEndPin = 0;
    bool m_linkDestroyed = false;
    uint32_t m_destroyedLinkId = 0;
};

} // namespace NodeGraph
} // namespace Olympe
```

---

### 2. EditorContext (Nouveau Mode Manager)

#### Responsabilité
Gère le contexte d'édition : mode éditeur vs visualiseur vs debug.

#### Interface Publique
```cpp
// Source/NodeGraphCore/EditorContext.h

namespace Olympe {
namespace NodeGraph {

/**
 * @enum EditorMode
 * @brief Mode d'édition du graphe
 */
enum class EditorMode
{
    Editor,      // Édition complète (CRUD)
    Visualizer,  // Visualisation seule (read-only)
    Debug        // Debug runtime (read-only + highlighting)
};

/**
 * @struct EditorCapabilities
 * @brief Capacités disponibles selon le mode
 */
struct EditorCapabilities
{
    bool canCreateNodes = false;
    bool canDeleteNodes = false;
    bool canMoveNodes = false;
    bool canConnectPins = false;
    bool canEditProperties = false;
    bool canUndo = false;
    bool canRedo = false;
    bool canSave = false;
    bool showRuntimeState = false;
};

/**
 * @class EditorContext
 * @brief Contexte d'édition pour un graphe
 */
class EditorContext
{
public:
    /**
     * @brief Créer un contexte en mode édition
     */
    static EditorContext CreateEditor();
    
    /**
     * @brief Créer un contexte en mode visualisation
     */
    static EditorContext CreateVisualizer();
    
    /**
     * @brief Créer un contexte en mode debug
     */
    static EditorContext CreateDebugger();
    
    /**
     * @brief Obtenir le mode actuel
     */
    EditorMode GetMode() const { return m_mode; }
    
    /**
     * @brief Obtenir les capacités
     */
    const EditorCapabilities& GetCapabilities() const { return m_capabilities; }
    
    /**
     * @brief Vérifier si une action est autorisée
     */
    bool CanCreateNodes() const { return m_capabilities.canCreateNodes; }
    bool CanDeleteNodes() const { return m_capabilities.canDeleteNodes; }
    bool CanMoveNodes() const { return m_capabilities.canMoveNodes; }
    bool CanConnectPins() const { return m_capabilities.canConnectPins; }
    bool CanEditProperties() const { return m_capabilities.canEditProperties; }
    bool CanUndo() const { return m_capabilities.canUndo; }
    bool CanRedo() const { return m_capabilities.canRedo; }
    bool CanSave() const { return m_capabilities.canSave; }
    bool ShowRuntimeState() const { return m_capabilities.showRuntimeState; }
    
private:
    EditorContext(EditorMode mode);
    
    EditorMode m_mode = EditorMode::Editor;
    EditorCapabilities m_capabilities;
};

} // namespace NodeGraph
} // namespace Olympe
```

---

### 3. BehaviorTreeAdapter (Bridge Pattern)

#### Responsabilité
Convertir BehaviorTreeAsset ↔ GraphDocument pour compatibilité.

#### Interface Publique
```cpp
// Source/NodeGraphShared/BehaviorTreeAdapter.h

namespace Olympe {

/**
 * @class BehaviorTreeAdapter
 * @brief Adaptateur entre BehaviorTreeAsset et GraphDocument
 */
class BehaviorTreeAdapter
{
public:
    /**
     * @brief Convertir un BehaviorTreeAsset en GraphDocument
     * @param tree Arbre de comportement source
     * @param layout Layout précalculé (optionnel)
     * @return GraphDocument nouvellement créé
     */
    static NodeGraph::GraphDocument* ToGraphDocument(
        const BehaviorTreeAsset* tree,
        const std::vector<BTNodeLayout>* layout = nullptr
    );
    
    /**
     * @brief Mettre à jour un GraphDocument depuis un BehaviorTreeAsset
     * @param document GraphDocument à mettre à jour
     * @param tree Arbre de comportement source
     * @param layout Layout précalculé (optionnel)
     */
    static void UpdateFromBehaviorTree(
        NodeGraph::GraphDocument* document,
        const BehaviorTreeAsset* tree,
        const std::vector<BTNodeLayout>* layout = nullptr
    );
    
    /**
     * @brief Exporter un GraphDocument vers BehaviorTreeAsset
     * @param document GraphDocument source
     * @return BehaviorTreeAsset nouvellement créé
     */
    static BehaviorTreeAsset* ToBehaviorTree(
        const NodeGraph::GraphDocument* document
    );
    
    /**
     * @brief Mettre à jour un BehaviorTreeAsset depuis un GraphDocument
     * @param tree Arbre de comportement à mettre à jour
     * @param document GraphDocument source
     */
    static void UpdateFromGraphDocument(
        BehaviorTreeAsset* tree,
        const NodeGraph::GraphDocument* document
    );
    
private:
    static NodeGraph::NodeData ConvertNode(const BTNode& btNode);
    static NodeGraph::PinData ConvertPin(const BTNode& btNode, bool isOutput);
    static NodeGraph::LinkData ConvertLink(const BTNode& parent, uint32_t childIdx);
    
    static BTNode ConvertToB TNode(const NodeGraph::NodeData& node);
};

} // namespace Olympe
```

---

### 4. NodeGraphPanel (Refactoré)

#### Responsabilité
Panel ImGui réutilisable pour afficher/éditer un graphe.

#### Interface Publique
```cpp
// Source/NodeGraphCore/NodeGraphPanel.h

namespace Olympe {
namespace NodeGraph {

/**
 * @class NodeGraphPanel
 * @brief Panel ImGui réutilisable pour graphes de nœuds
 */
class NodeGraphPanel
{
public:
    NodeGraphPanel();
    ~NodeGraphPanel();
    
    /**
     * @brief Initialiser le panel
     * @param context Contexte d'édition
     * @param config Configuration de rendu
     */
    void Initialize(const EditorContext& context, const RenderConfig& config);
    
    /**
     * @brief Définir le GraphDocument à afficher
     * @param document GraphDocument (peut être nullptr)
     */
    void SetDocument(GraphDocument* document);
    
    /**
     * @brief Obtenir le GraphDocument actuel
     */
    GraphDocument* GetDocument() const { return m_document; }
    
    /**
     * @brief Rendre le panel
     * @param deltaTime Temps écoulé depuis dernier frame
     */
    void Render(float deltaTime);
    
    /**
     * @brief Définir le nœud actif (runtime highlighting)
     * @param nodeId ID du nœud actif
     */
    void SetCurrentRuntimeNode(uint32_t nodeId);
    
    /**
     * @brief Obtenir le renderer (pour accès avancé)
     */
    NodeGraphRenderer* GetRenderer() { return &m_renderer; }
    
private:
    void HandleInteractions();
    void HandleContextMenu();
    void HandleKeyboardShortcuts();
    
    GraphDocument* m_document = nullptr;
    EditorContext m_context;
    NodeGraphRenderer m_renderer;
    
    // Interaction state
    uint32_t m_selectedNodeId = 0;
    std::vector<uint32_t> m_selectedNodes;
    uint32_t m_contextMenuNodeId = 0;
};

} // namespace NodeGraph
} // namespace Olympe
```

---

## 🔄 FLUX DE DONNÉES (Data Flow)

### Mode Éditeur (Standalone)
```
┌──────────────────────────────────────────────────────┐
│ BlueprintEditorStandalone                             │
│                                                        │
│  ┌─────────────────────────────────────────────────┐ │
│  │ BlueprintEditorGUI                               │ │
│  │                                                   │ │
│  │  ┌──────────────────────────────────────────┐   │ │
│  │  │ NodeGraphPanel                            │   │ │
│  │  │                                            │   │ │
│  │  │  GraphDocument* doc =                     │   │ │
│  │  │      NodeGraphManager::Get().GetActive(); │   │ │
│  │  │                                            │   │ │
│  │  │  m_renderer.BeginRender(doc, deltaTime);  │   │ │
│  │  │  m_renderer.RenderNodes();                │   │ │
│  │  │  m_renderer.RenderLinks();                │   │ │
│  │  │  m_renderer.RenderMinimap();              │   │ │
│  │  │  bool modified = m_renderer.EndRender();  │   │ │
│  │  │                                            │   │ │
│  │  │  if (modified) {                          │   │ │
│  │  │      CommandSystem::ExecuteCommand(...);  │   │ │
│  │  │  }                                         │   │ │
│  │  └──────────────────────────────────────────┘   │ │
│  └─────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

### Mode Debug (F10)
```
┌──────────────────────────────────────────────────────┐
│ BehaviorTreeDebugWindow                               │
│                                                        │
│  1) Récupérer BehaviorTreeAsset depuis BT Manager    │
│     const BehaviorTreeAsset* tree =                   │
│         BehaviorTreeManager::Get().GetTree(...);      │
│                                                        │
│  2) Convertir en GraphDocument (adapter)              │
│     GraphDocument* doc =                              │
│         BehaviorTreeAdapter::ToGraphDocument(tree);   │
│                                                        │
│  3) Obtenir nœud actif depuis ECS runtime             │
│     uint32_t currentNodeId =                          │
│         btRuntime.AICurrentNodeIndex;                 │
│                                                        │
│  4) Rendre avec NodeGraphPanel                        │
│     m_graphPanel.SetDocument(doc);                    │
│     m_graphPanel.SetCurrentRuntimeNode(currentNodeId);│
│     m_graphPanel.Render(deltaTime);                   │
└──────────────────────────────────────────────────────┘
```

---

## 🎨 STYLES & THEMING

### NodeStyleRegistry (Centralisé)
```cpp
// Source/BlueprintEditor/NodeStyleRegistry.h

namespace Olympe {

/**
 * @struct NodeStyle
 * @brief Style visuel d'un nœud
 */
struct NodeStyle
{
    ImU32 headerColor = IM_COL32(60, 60, 60, 255);
    ImU32 headerHoveredColor = IM_COL32(80, 80, 80, 255);
    ImU32 headerSelectedColor = IM_COL32(100, 150, 200, 255);
    ImU32 bodyColor = IM_COL32(50, 50, 50, 255);
    ImU32 outlineColor = IM_COL32(100, 100, 100, 255);
    
    float cornerRounding = 4.0f;
    float borderThickness = 1.0f;
    
    const char* icon = nullptr;
};

/**
 * @class NodeStyleRegistry
 * @brief Registre de styles pour les nœuds
 */
class NodeStyleRegistry
{
public:
    static NodeStyleRegistry& Get();
    
    /**
     * @brief Enregistrer un style pour un type de nœud
     * @param nodeType Type de nœud (ex: "Selector", "Sequence")
     * @param style Style à enregistrer
     */
    void RegisterStyle(const std::string& nodeType, const NodeStyle& style);
    
    /**
     * @brief Obtenir le style d'un type de nœud
     * @param nodeType Type de nœud
     * @return Style (ou style par défaut si non trouvé)
     */
    NodeStyle GetStyle(const std::string& nodeType) const;
    
    /**
     * @brief Charger les styles depuis JSON
     * @param filepath Chemin du fichier JSON
     */
    void LoadFromFile(const std::string& filepath);
    
private:
    NodeStyleRegistry();
    std::map<std::string, NodeStyle> m_styles;
    NodeStyle m_defaultStyle;
};

} // namespace Olympe
```

---

## 📦 STRUCTURE FICHIERS FINALE

```
Source/
├── NodeGraphCore/                    # Core partagé (Model)
│   ├── GraphDocument.h/cpp           # Existant (inchangé)
│   ├── NodeGraphManager.h/cpp        # Existant (inchangé)
│   ├── CommandSystem.h/cpp           # Existant (inchangé)
│   ├── BlackboardSystem.h/cpp        # Existant (inchangé)
│   ├── ILayoutEngine.h               # Existant (inchangé)
│   │
│   ├── NodeGraphRenderer.h/cpp       # ✅ NOUVEAU - Pipeline rendu unifié
│   ├── EditorContext.h/cpp           # ✅ NOUVEAU - Mode manager
│   └── NodeGraphPanel.h/cpp          # ✅ NOUVEAU - Panel réutilisable
│
├── NodeGraphShared/                  # Helpers partagés (View)
│   ├── NodeGraphShared.h/cpp         # Existant (inchangé)
│   ├── Renderer.h                    # Existant (inchangé)
│   ├── Serializer.h                  # Existant (inchangé)
│   ├── CommandAdapter.h              # Existant (inchangé)
│   │
│   ├── BehaviorTreeAdapter.h/cpp     # ✅ NOUVEAU - Adapter BT <-> Graph
│   ├── HFSMAdapter.h/cpp             # ✅ NOUVEAU - Adapter HFSM <-> Graph
│   └── BlueprintAdapter.h/cpp        # ✅ NOUVEAU - Adapter BP <-> Graph
│
├── BlueprintEditor/                  # Éditeur Standalone
│   ├── BlueprintEditorGUI.cpp        # ⚠️ MODIFIÉ - Utilise NodeGraphPanel
│   ├── NodeGraphPanel.cpp            # ❌ SUPPRIMÉ (remplacé par NodeGraphCore/NodeGraphPanel)
│   └── NodeStyleRegistry.h/cpp       # Existant (inchangé)
│
└── AI/                               # Debug Window
    ├── BehaviorTreeDebugWindow.h/cpp # ⚠️ REFACTOR MAJEUR - Utilise NodeGraphPanel
    └── BTGraphLayoutEngine.h/cpp     # Existant (inchangé)
```

---

## 🚀 EXEMPLE D'UTILISATION

### Éditeur Standalone (Mode Editor)
```cpp
// BlueprintEditorGUI.cpp

void BlueprintEditorGUI::RenderGraphEditor()
{
    // 1. Créer contexte éditeur
    static NodeGraph::EditorContext editorCtx = NodeGraph::EditorContext::CreateEditor();
    
    // 2. Configurer renderer
    NodeGraph::RenderConfig config;
    config.mode = NodeGraph::RenderMode::Editor;
    config.enableMinimap = true;
    config.minimapSizeFraction = 0.15f;
    config.enableRuntimeHighlight = false;
    config.styleRegistry = &NodeStyleRegistry::Get();
    
    // 3. Créer panel (une fois)
    static NodeGraph::NodeGraphPanel panel;
    static bool initialized = false;
    if (!initialized)
    {
        panel.Initialize(editorCtx, config);
        initialized = true;
    }
    
    // 4. Récupérer document actif
    NodeGraph::GraphDocument* doc = NodeGraph::NodeGraphManager::Get().GetActiveGraph();
    panel.SetDocument(doc);
    
    // 5. Rendre
    float deltaTime = ImGui::GetIO().DeltaTime;
    panel.Render(deltaTime);
}
```

### Visualiseur Debug (Mode Debug)
```cpp
// BehaviorTreeDebugWindow.cpp

void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    // 1. Créer contexte debug
    static NodeGraph::EditorContext debugCtx = NodeGraph::EditorContext::CreateDebugger();
    
    // 2. Configurer renderer
    NodeGraph::RenderConfig config;
    config.mode = NodeGraph::RenderMode::Debug;
    config.enableMinimap = true;
    config.minimapSizeFraction = 0.15f;
    config.enableRuntimeHighlight = true;  // ✅ Highlighting activé
    config.pulseFre

quency = 2.0f;
    config.styleRegistry = &NodeStyleRegistry::Get();
    
    // 3. Créer panel (une fois)
    static NodeGraph::NodeGraphPanel panel;
    static bool initialized = false;
    if (!initialized)
    {
        panel.Initialize(debugCtx, config);
        initialized = true;
    }
    
    // 4. Récupérer BehaviorTreeAsset
    auto& world = World::Get();
    const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
    const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
    
    if (!tree) return;
    
    // 5. Convertir en GraphDocument (cache)
    if (m_cachedTreeId != tree->id)
    {
        if (m_cachedGraphDoc)
        {
            delete m_cachedGraphDoc;
        }
        m_cachedGraphDoc = BehaviorTreeAdapter::ToGraphDocument(tree, &m_currentLayout);
        m_cachedTreeId = tree->id;
    }
    
    // 6. Définir nœud actif (runtime)
    uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;
    panel.SetCurrentRuntimeNode(currentNodeId);
    
    // 7. Rendre
    panel.SetDocument(m_cachedGraphDoc);
    float deltaTime = ImGui::GetIO().DeltaTime;
    panel.Render(deltaTime);
}
```

---

## ✅ VALIDATION ARCHITECTURE

### Tests de Conformité

#### 1. Zéro Duplication
```
✅ RenderNodes() → UNE seule implémentation (NodeGraphRenderer)
✅ RenderLinks() → UNE seule implémentation (NodeGraphRenderer)
✅ RenderMinimap() → UNE seule implémentation (NodeGraphRenderer)
✅ Zoom/Pan → UNE seule implémentation (NodeGraphRenderer)
```

#### 2. API Commune
```
✅ BehaviorTreeDebugWindow utilise NodeGraphPanel
✅ BlueprintEditorGUI utilise NodeGraphPanel
✅ HFSMEditor utilise NodeGraphPanel (futur)
✅ StateGraphEditor utilise NodeGraphPanel (futur)
```

#### 3. Extensibilité
```
✅ Nouveau type de graphe = créer un Adapter
✅ Nouveau style de nœud = enregistrer dans NodeStyleRegistry
✅ Nouveau mode = étendre EditorContext
```

#### 4. Maintenabilité
```
✅ NodeGraphRenderer.cpp < 800 LOC
✅ NodeGraphPanel.cpp < 600 LOC
✅ BehaviorTreeAdapter.cpp < 400 LOC
✅ TOTAL < 3000 LOC (vs 4606 LOC actuels)
```

---

## 🎯 PROCHAINES ÉTAPES (Phase 3)

1. ✅ Implémenter `NodeGraphRenderer.h/cpp`
2. ✅ Implémenter `EditorContext.h/cpp`
3. ✅ Implémenter `NodeGraphPanel.h/cpp`
4. ✅ Implémenter `BehaviorTreeAdapter.h/cpp`
5. ✅ Migrer `BehaviorTreeDebugWindow` vers nouveau pipeline
6. ✅ Migrer `BlueprintEditorGUI` vers nouveau pipeline
7. ✅ Compiler et valider (0 errors, 0 warnings)
8. ✅ Archiver ancien code dans `Source/Deprecated/`

---

**FIN DE LA CONCEPTION - PRÊT POUR PHASE 3 (IMPLÉMENTATION)**
