# 🚀 PLAN DE MIGRATION DÉTAILLÉ - BehaviorTreeDebugWindow → Architecture Unifiée

**Date**: 2025-02-19  
**Objectif**: Intégrer l'architecture standalone NodeGraphCore dans le visualisateur debug (F10)  
**Stratégie**: Migration incrémentale avec validation à chaque étape

---

## 📊 COMPLEXITÉ ACTUELLE

### Lignes de Code par Composant
```
BehaviorTreeDebugWindow.cpp:     ~2500 LOC
BehaviorTreeDebugWindow.h:       ~500 LOC
Total:                           ~3000 LOC
```

### Code à Remplacer
- ✅ **RenderBehaviorTreeGraph()** (lignes 1379-1429) → BTDebugAdapter::Render()
- ✅ **RenderNode()** (lignes 1431-1600) → NodeGraphRenderer pipeline
- ✅ **RenderNodeConnections()** (lignes 1602-1670) → NodeGraphRenderer pipeline
- ✅ **RenderNodePins()** → NodeGraphRenderer pin system
- ✅ **Zoom/Pan logic** (lignes 1069-1145) → NodeGraphRenderer built-in
- ✅ **Minimap** (custom) → NodeGraphRenderer::RenderMinimap()

### Code à Préserver
- ✅ **Entity list panel** (lignes 669-782)
- ✅ **Inspector panel** (blackboard, logs)
- ✅ **Menu bar & shortcuts**
- ✅ **Editor mode** (command stack, undo/redo)

---

## 🎯 PLAN D'EXÉCUTION

### Phase 1: Préparation (✅ COMPLETED)
1. ✅ BTGraphDocumentConverter créé (existant)
2. ✅ BTDebugAdapter créé (nouveau)
3. ✅ Analyse de dépendances complétée

### Phase 2: Intégration Minimale
**Objectif**: Faire marcher le rendu de base via NodeGraphRenderer

#### Étape 2.1: Ajouter les membres nécessaires
```cpp
// BehaviorTreeDebugWindow.h
private:
    // Nouveau: Architecture unifiée
    std::unique_ptr<NodeGraph::NodeGraphRenderer> m_unifiedRenderer;
    std::unique_ptr<NodeGraphShared::BTDebugAdapter> m_debugAdapter;
    bool m_useUnifiedRenderer = true; // Flag de migration progressive
```

#### Étape 2.2: Initialiser le renderer dans Initialize()
```cpp
void BehaviorTreeDebugWindow::Initialize()
{
    // ... code existant ...
    
    // Initialize unified renderer
    m_unifiedRenderer = std::make_unique<NodeGraph::NodeGraphRenderer>();
    
    NodeGraph::RenderConfig config;
    config.mode = NodeGraph::RenderMode::Debug;
    config.enableMinimap = m_showMinimap;
    config.enableRuntimeHighlight = true;
    config.pulseFrequency = 2.0f;
    config.enableGridSnapping = m_config.gridSnappingEnabled;
    config.gridSize = m_config.gridSize;
    config.styleRegistry = &NodeStyleRegistry::Get();
    
    m_unifiedRenderer->Initialize(config);
    
    std::cout << "[BTDebugger] ✅ Unified renderer initialized" << std::endl;
}
```

#### Étape 2.3: Créer l'adapter à la sélection d'entité
```cpp
void BehaviorTreeDebugWindow::RenderEntityEntry(const EntityDebugInfo& info)
{
    // ... sélection entity ...
    
    if (ImGui::Selectable(info.entityName.c_str(), isSelected))
    {
        m_selectedEntity = info.entityId;
        
        const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(info.treeId);
        if (tree)
        {
            // Recompute layout
            m_positionedNodes.clear();
            m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, 1.0f);
            
            // Create debug adapter
            m_debugAdapter = std::make_unique<NodeGraphShared::BTDebugAdapter>(tree, &m_layoutEngine);
            m_debugAdapter->Initialize(m_unifiedRenderer.get());
            m_debugAdapter->RecomputeLayout(m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
            
            if (m_autoFitOnLoad)
            {
                m_debugAdapter->FitToView();
            }
            
            std::cout << "[BTDebugger] ✅ Debug adapter created for tree: " << info.treeName << std::endl;
        }
    }
}
```

#### Étape 2.4: Remplacer le rendu dans RenderNodeGraphPanel()
```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    // ... préparation tree ...
    
    ImNodes::BeginNodeEditor();
    
    if (m_useUnifiedRenderer && m_debugAdapter)
    {
        // NOUVEAU: Unified rendering pipeline
        uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;
        m_debugAdapter->SetActiveNode(currentNodeId);
        
        float deltaTime = GameEngine::fDt;
        m_debugAdapter->Render(deltaTime);
    }
    else
    {
        // ANCIEN: Legacy rendering (fallback)
        RenderBehaviorTreeGraph();
    }
    
    if (m_showMinimap)
    {
        if (m_useUnifiedRenderer && m_unifiedRenderer)
        {
            m_unifiedRenderer->RenderMinimap();
        }
        else
        {
            RenderMinimap(); // Legacy
        }
    }
    
    ImNodes::EndNodeEditor();
    
    // ... post-processing (autosave, etc.) ...
}
```

### Phase 3: Migration des Interactions
**Objectif**: Zoom, pan, sélection via NodeGraphRenderer

#### Étape 3.1: Déléguer zoom au renderer
```cpp
// Remplacer lignes 1069-1145 par:
if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
{
    if (io.MouseWheel != 0.0f)
    {
        float zoomDelta = io.MouseWheel * 0.1f;
        float newZoom = std::max(MIN_ZOOM, std::min(MAX_ZOOM, m_currentZoom + zoomDelta));
        
        if (m_useUnifiedRenderer && m_unifiedRenderer)
        {
            m_unifiedRenderer->SetZoom(newZoom);
            m_currentZoom = m_unifiedRenderer->GetCurrentZoom();
        }
        else
        {
            // Legacy zoom logic
            m_currentZoom = newZoom;
            ApplyZoomToStyle();
        }
    }
}
```

#### Étape 3.2: Déléguer fit/center au renderer
```cpp
void BehaviorTreeDebugWindow::FitGraphToView()
{
    if (m_useUnifiedRenderer && m_debugAdapter)
    {
        m_debugAdapter->FitToView();
    }
    else
    {
        // Legacy fit logic
    }
}

void BehaviorTreeDebugWindow::CenterViewOnGraph()
{
    if (m_useUnifiedRenderer && m_unifiedRenderer)
    {
        m_unifiedRenderer->CenterView();
    }
    else
    {
        // Legacy center logic
    }
}
```

### Phase 4: Mode Éditeur
**Objectif**: Préserver le mode édition avec CommandStack

#### Étape 4.1: Configurer l'adapter en mode éditeur
```cpp
void BehaviorTreeDebugWindow::ToggleEditorMode()
{
    m_editorMode = !m_editorMode;
    
    if (m_debugAdapter)
    {
        m_debugAdapter->SetEditorMode(m_editorMode);
    }
    
    std::cout << "[BTDebugger] Editor mode: " << (m_editorMode ? "ON" : "OFF") << std::endl;
}
```

#### Étape 4.2: Gérer les interactions utilisateur
```cpp
void BehaviorTreeDebugWindow::HandleEditorInteractions()
{
    if (!m_editorMode || !m_unifiedRenderer)
        return;
    
    // Get hovered elements from renderer
    uint32_t hoveredNode = m_unifiedRenderer->GetHoveredNode();
    uint32_t hoveredLink = m_unifiedRenderer->GetHoveredLink();
    
    // Check if link was created
    uint32_t startPin, endPin;
    if (m_unifiedRenderer->IsLinkCreated(&startPin, &endPin))
    {
        // Extract node IDs from pin IDs
        uint32_t parentId = startPin >> 16;
        uint32_t childId = endPin >> 16;
        
        // Execute command via adapter
        NodeGraphShared::CommandAdapter cmdAdapter(&m_commandStack, &m_editingTree);
        cmdAdapter.ConnectNodes(parentId, childId);
        
        m_isDirty = true;
        m_treeModified = true;
    }
    
    // Check if link was destroyed
    uint32_t linkId;
    if (m_unifiedRenderer->IsLinkDestroyed(&linkId))
    {
        // Find link in tree and disconnect
        // ... similar to existing logic ...
    }
}
```

### Phase 5: Cleanup
**Objectif**: Supprimer le code legacy

#### Étapes de cleanup
1. Supprimer `RenderNode()` (lignes 1431-1600)
2. Supprimer `RenderNodeConnections()` (lignes 1602-1670)
3. Supprimer `RenderBehaviorTreeGraph()` (lignes 1379-1429)
4. Supprimer custom zoom/pan logic
5. Supprimer custom minimap rendering
6. Supprimer m_positionedNodes logic (géré par renderer)
7. Retirer flag `m_useUnifiedRenderer` (toujours true)

**LOC Reduction Estimée:**
```
Avant:  ~3000 LOC
Après:  ~1200 LOC
Gain:   ~1800 LOC (60% reduction)
```

---

## ✅ VALIDATION À CHAQUE ÉTAPE

### Tests Manuels
1. **Rendu de base**
   - [ ] Les nodes s'affichent correctement
   - [ ] Les liens sont visibles
   - [ ] Le layout est préservé

2. **Runtime highlighting**
   - [ ] Le node actif pulse en jaune
   - [ ] Les liens actifs brillent
   - [ ] La couleur change selon le statut

3. **Zoom & Pan**
   - [ ] Molette de souris zoom/dezoom
   - [ ] Drag middle-button pour pan
   - [ ] Shortcuts clavier (F, C, 0, +, -)

4. **Minimap**
   - [ ] Minimap visible en bas à droite
   - [ ] Reflète la position actuelle
   - [ ] Click sur minimap téléporte la vue

5. **Mode Éditeur**
   - [ ] Création de nodes via context menu
   - [ ] Drag nodes pour déplacer
   - [ ] Création de liens via drag pins
   - [ ] Suppression avec Delete
   - [ ] Undo/Redo (Ctrl+Z/Ctrl+Y)
   - [ ] Autosave fonctionne

### Tests de Régression
- [ ] Compilation sans warnings
- [ ] Pas de crash au démarrage
- [ ] Pas de crash au changement d'entité
- [ ] Pas de memory leak (vérifier destructeurs)
- [ ] Performance similaire (~60 FPS)

---

## 🚨 POINTS D'ATTENTION

### Risques Identifiés
1. **Pin ID scheme différent**
   - Legacy: `nodeId * 10000 + pinIndex`
   - GraphDocument: `GeneratePinId(nodeId, isOutput, index)`
   - **Solution**: Adapter dans BTGraphDocumentConverter

2. **ImNodes context différent**
   - Legacy: ImNodes direct
   - Unified: Via NodeGraphRenderer
   - **Solution**: S'assurer que BeginNodeEditor/EndNodeEditor ne sont appelés qu'une fois

3. **Layout synchronization**
   - BTGraphLayoutEngine doit rester synchronisé avec GraphDocument
   - **Solution**: BTDebugAdapter::SyncLayoutToDocument()

4. **Autosave avec document**
   - L'autosave actuel serialize BehaviorTreeAsset
   - Avec GraphDocument, besoin de convertir back
   - **Solution**: ToBehaviorTree() dans lambda d'autosave

### Dépendances Externes
- ✅ NodeGraphCore compilé et linké
- ✅ BTGraphDocumentConverter testé
- ⏳ NodeGraphRenderer API complète
- ⏳ EditorContext Debug mode implémenté

---

## 📝 NEXT ACTIONS

1. ⏳ Implémenter Phase 2.1-2.4 (Intégration minimale)
2. ⏳ Tester rendu de base avec flag `m_useUnifiedRenderer`
3. ⏳ Valider highlighting runtime
4. ⏳ Implémenter Phase 3 (Interactions)
5. ⏳ Implémenter Phase 4 (Mode éditeur)
6. ⏳ Phase 5 Cleanup + tests de régression

---

## 🔗 RÉFÉRENCES

- `BehaviorTreeDebugWindow_UnifiedMigration_Analysis.md` - Analyse initiale
- `NodeGraph_Unified_Architecture.md` - Architecture cible
- `BTDebugAdapter.h` - Adapter créé
- `BTGraphDocumentConverter.h` - Converter existant
