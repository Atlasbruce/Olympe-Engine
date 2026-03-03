# 🚀 PHASE 2 - DÉMARRAGE RAPIDE

**Date**: 2025-02-19  
**Statut**: ⚠️ BUILD BLOQUANT - Revert Required First

---

## ⚠️ PRÉREQUIS CRITIQUE

**Vous NE POUVEZ PAS démarrer Phase 2 sans un build qui compile.**

**État actuel**: 242+ erreurs de compilation  
**Action requise**: Revert BehaviorTreeDebugWindow.cpp  
**Temps**: 5 minutes

---

## ÉTAPE 1: RESTAURER LE BUILD (OBLIGATOIRE)

### Option A: Script Automatique (Recommandé)

```cmd
# Exécuter le script de revert
Scripts\Revert_BehaviorTreeDebugWindow.bat

# Suivre les instructions à l'écran
# Le script vous demandera de choisir un commit stable
```

### Option B: Manuel

```bash
# 1. Voir les commits récents
git log --oneline -15 Source/AI/BehaviorTreeDebugWindow.cpp

# 2. Identifier un commit STABLE (avant aujourd'hui)
#    Exemple: a1b2c3d "feat: BT debug working"

# 3. Revert
git checkout a1b2c3d -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout a1b2c3d -- Source/AI/BehaviorTreeDebugWindow.h

# 4. Rebuild
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug

# 5. Vérifier
# ✅ 0 errors
# ✅ Compilation réussie
```

### Validation Post-Revert

```bash
# Lancer le jeu
cd x64/Debug
OlympeEngine.exe

# In-game: Appuyer F10
# ✅ Debugger s'ouvre
# ✅ Peut sélectionner entité
# ✅ Graphe s'affiche
```

**SI ÇA FONCTIONNE**: ✅ Passez à Étape 2  
**SI ÇA NE FONCTIONNE PAS**: ❌ Essayez un commit plus ancien

---

## ÉTAPE 2: IMPLÉMENTER PHASE 2 (2-4h)

### Pré-requis

- [x] Build compile (0 errors)
- [x] F10 debugger fonctionne
- [ ] Lu `SESSION_FINAL_SUMMARY_NodeGraph.md`
- [ ] Lu `NodeGraph_Architecture_Current.md`

### Phase 2.1: Ajouter Membres dans BehaviorTreeDebugWindow.h

**Fichier**: `Source/AI/BehaviorTreeDebugWindow.h`

**Ajouter dans la classe (section private):**

```cpp
private:
    // ========================================================================
    // Unified Architecture (Phase 2) - NEW
    // ========================================================================
    
    /// Unified renderer for NodeGraph visualization
    std::unique_ptr<NodeGraph::NodeGraphRenderer> m_unifiedRenderer;
    
    /// Debug adapter for BehaviorTree → NodeGraphRenderer bridge
    std::unique_ptr<NodeGraphShared::BTDebugAdapter> m_debugAdapter;
    
    /// Migration flag: true = use unified renderer, false = legacy renderer
    /// Set to false for instant rollback if issues occur
    bool m_useUnifiedRenderer = false;  // Start with false, enable after testing
```

**Ajouter les includes en haut du fichier:**

```cpp
// Unified architecture (Phase 2)
#include "../NodeGraphCore/NodeGraphRenderer.h"
#include "../NodeGraphShared/BTDebugAdapter.h"
#include <memory>
```

### Phase 2.2: Initialiser le Renderer

**Fichier**: `Source/AI/BehaviorTreeDebugWindow.cpp`

**Dans la fonction `Initialize()`, après l'init ImNodes:**

```cpp
void BehaviorTreeDebugWindow::Initialize()
{
    // ... code existant (ImNodes init) ...
    
    // ========================================================================
    // Phase 2: Initialize Unified Renderer
    // ========================================================================
    
    m_unifiedRenderer = std::make_unique<NodeGraph::NodeGraphRenderer>();
    
    NodeGraph::RenderConfig config;
    config.mode = NodeGraph::RenderMode::Debug;  // Read-only + runtime highlighting
    config.enableMinimap = m_showMinimap;
    config.enableRuntimeHighlight = true;
    config.pulseFrequency = 2.0f;  // 2Hz pulse for active node
    config.enableGridSnapping = m_config.gridSnappingEnabled;
    config.gridSize = m_config.gridSize;
    config.styleRegistry = &NodeStyleRegistry::Get();
    
    m_unifiedRenderer->Initialize(config);
    
    std::cout << "[BTDebugger] ✅ Unified renderer initialized (Phase 2)" << std::endl;
    std::cout << "[BTDebugger] Flag m_useUnifiedRenderer = " 
              << (m_useUnifiedRenderer ? "true" : "false") << std::endl;
    
    // ========================================================================
}
```

### Phase 2.3: Créer l'Adapter à la Sélection d'Entité

**Fichier**: `Source/AI/BehaviorTreeDebugWindow.cpp`

**Dans `RenderEntityEntry()`, après la sélection d'entité:**

```cpp
if (ImGui::Selectable(info.entityName.c_str(), isSelected))
{
    m_selectedEntity = info.entityId;
    
    const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(info.treeId);
    if (tree)
    {
        // Recompute layout
        m_positionedNodes.clear();
        m_currentLayout = m_layoutEngine.ComputeLayout(tree, m_nodeSpacingX, m_nodeSpacingY, 1.0f);
        
        // ====================================================================
        // Phase 2: Create Debug Adapter
        // ====================================================================
        if (m_useUnifiedRenderer && m_unifiedRenderer)
        {
            m_debugAdapter = std::make_unique<NodeGraphShared::BTDebugAdapter>(
                tree, 
                &m_layoutEngine
            );
            m_debugAdapter->Initialize(m_unifiedRenderer.get());
            m_debugAdapter->RecomputeLayout(m_nodeSpacingX, m_nodeSpacingY, m_currentZoom);
            
            if (m_autoFitOnLoad)
            {
                m_debugAdapter->FitToView();
            }
            
            std::cout << "[BTDebugger] ✅ Debug adapter created for: " 
                      << info.treeName << std::endl;
        }
        // ====================================================================
        
        // Legacy auto-fit (si unified renderer désactivé)
        if (!m_useUnifiedRenderer && m_autoFitOnLoad)
        {
            // Existing auto-fit logic
        }
    }
}
```

### Phase 2.4: Remplacer le Rendu

**Fichier**: `Source/AI/BehaviorTreeDebugWindow.cpp`

**Dans `RenderNodeGraphPanel()`, remplacer le bloc de rendu:**

```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    // ... préparation (entity check, tree fetch) ...
    
    const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
    const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
    
    if (!tree)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Behavior Tree asset not found!");
        return;
    }
    
    uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;
    bool isActive = btRuntime.isActive && !m_editorMode;
    
    // Begin ImNodes editor
    ImNodes::BeginNodeEditor();
    
    // ========================================================================
    // Phase 2: Unified Rendering Pipeline
    // ========================================================================
    if (m_useUnifiedRenderer && m_debugAdapter && m_unifiedRenderer)
    {
        // Update active node for highlighting
        if (isActive)
        {
            m_debugAdapter->SetActiveNode(currentNodeId);
        }
        else
        {
            m_debugAdapter->ClearRuntimeState();
        }
        
        // Render using unified pipeline
        float deltaTime = GameEngine::fDt;
        m_debugAdapter->Render(deltaTime);
        
        // Minimap (if enabled)
        if (m_showMinimap && m_unifiedRenderer)
        {
            m_unifiedRenderer->RenderMinimap();
        }
    }
    else
    {
        // ====================================================================
        // LEGACY RENDERING (Fallback)
        // ====================================================================
        
        // Existing rendering code preserved for rollback
        for (const auto& node : tree->nodes)
        {
            const BTNodeLayout* layout = m_layoutEngine.GetNodeLayout(node.id);
            if (layout)
            {
                bool isCurrentNode = (node.id == currentNodeId) && isActive;
                NodeGraphShared::RenderBTNode(&node, layout, isCurrentNode, 
                                             m_currentZoom, m_pulseTimer, 
                                             m_config, m_positionedNodes);
            }
        }
        
        NodeGraphShared::RenderBTNodeConnections(tree, currentNodeId);
        
        // Legacy minimap
        if (m_showMinimap)
        {
            RenderMinimap();  // Legacy implementation
        }
    }
    // ========================================================================
    
    ImNodes::EndNodeEditor();
    
    // ... post-processing (autosave, etc.) ...
}
```

### Phase 2.5: Tester

**Test 1: Compilation**

```bash
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug

# ✅ 0 errors
# ✅ Compilation réussie
```

**Test 2: Lancement**

```bash
cd x64/Debug
OlympeEngine.exe

# ✅ Jeu démarre sans crash
```

**Test 3: F10 Debugger (Legacy Mode)**

```
1. In-game, appuyer F10
2. ✅ Debugger s'ouvre
3. Sélectionner entité avec BT
4. ✅ Graphe s'affiche (mode legacy)
5. ✅ Highlighting fonctionne
6. ✅ Pas de régression
```

**Test 4: Activer Unified Renderer**

Dans `BehaviorTreeDebugWindow.h`:

```cpp
bool m_useUnifiedRenderer = true;  // Enable Phase 2
```

Rebuild & retest:

```
1. Rebuild solution
2. Lancer jeu
3. F10 → Debugger
4. Sélectionner entité
5. ✅ Graphe s'affiche (unified renderer)
6. ✅ Highlighting pulse jaune
7. ✅ Minimap visible
8. ✅ Pas de crash
```

**Si Test 4 échoue**:

```cpp
bool m_useUnifiedRenderer = false;  // Instant rollback
```

---

## ÉTAPE 3: VALIDATION COMPLÈTE

### Checklist Tests Manuels

- [ ] **Compilation**: 0 errors
- [ ] **Launch**: Jeu démarre
- [ ] **F10**: Debugger s'ouvre
- [ ] **Entity selection**: Affiche graphe
- [ ] **Runtime highlighting**: Node actif pulse jaune
- [ ] **Minimap**: Visible et fonctionnelle
- [ ] **Zoom**: Molette souris fonctionne
- [ ] **Pan**: Drag middle-button fonctionne
- [ ] **No regression**: Comportement identique au legacy

### Checklist Code

- [ ] `m_unifiedRenderer` initialisé dans `Initialize()`
- [ ] `m_debugAdapter` créé à la sélection d'entité
- [ ] Rendu via `m_debugAdapter->Render()` si flag activé
- [ ] Legacy rendering préservé comme fallback
- [ ] Includes ajoutés correctement
- [ ] Pas de warnings de compilation

---

## ÉTAPE 4: COMMIT & PR

**Si tous les tests passent:**

```bash
# Stage changes
git add Source/AI/BehaviorTreeDebugWindow.*
git add Source/NodeGraphShared/BTDebugAdapter.*
git add Docs/Developer/*.md
git add PR_DESCRIPTION.md

# Commit
git commit -m "feat(nodegraph): Phase 2 - Integrate BTDebugAdapter in BehaviorTreeDebugWindow

Implemented:
- Added m_unifiedRenderer and m_debugAdapter members
- Initialize unified renderer in Initialize()
- Create BTDebugAdapter on entity selection
- Replace rendering with unified pipeline
- Preserve legacy rendering as fallback (m_useUnifiedRenderer flag)

Testing:
- ✅ Compilation successful (0 errors)
- ✅ F10 debugger works (unified + legacy)
- ✅ Runtime highlighting functional
- ✅ Minimap visible
- ✅ No regression from current behavior

Migration:
- Flag m_useUnifiedRenderer enables/disables unified renderer
- Instant rollback available by setting flag to false
- Legacy code preserved for safety

Ref: SESSION_FINAL_SUMMARY_NodeGraph.md, BehaviorTreeDebugWindow_UnifiedMigration_Plan.md
Phase: 2/5 Complete"

# Push
git push origin feature/nodegraph-shared-migration
```

---

## 🚨 SI PROBLÈMES

### Build ne compile pas

→ Vérifier includes:
```cpp
#include "../NodeGraphCore/NodeGraphRenderer.h"
#include "../NodeGraphShared/BTDebugAdapter.h"
```

### Crash au lancement

→ Vérifier initialisation:
```cpp
if (m_unifiedRenderer) { 
    m_unifiedRenderer->Initialize(config); 
}
```

### Graphe ne s'affiche pas

→ Vérifier flag:
```cpp
if (m_useUnifiedRenderer && m_debugAdapter) {
    // Unified rendering
}
```

### Rollback immédiat

```cpp
// Dans BehaviorTreeDebugWindow.h
bool m_useUnifiedRenderer = false;  // Disable Phase 2
```

---

## 📚 DOCUMENTATION RÉFÉRENCE

- **Instructions détaillées**: `SESSION_FINAL_SUMMARY_NodeGraph.md`
- **Plan complet**: `BehaviorTreeDebugWindow_UnifiedMigration_Plan.md`
- **Architecture**: `NodeGraph_Architecture_Current.md`
- **Build fix**: `NodeGraph_Build_Fix_Final.md`

---

## 🎯 RÉSUMÉ

**Temps total Phase 2**: 2-4 heures  
**Complexité**: Moyenne  
**Risque**: Faible (rollback flag)  

**État actuel**:
- ⚠️ Étape 1 REQUISE: Revert build
- ⏳ Étape 2 PRÊTE: Instructions complètes
- ✅ Documentation: Exhaustive

**Action immédiate**:
1. Exécuter `Scripts\Revert_BehaviorTreeDebugWindow.bat`
2. Valider build compile
3. Suivre Phase 2.1 → 2.5

---

**🚀 START: Revert le build, THEN procéder Phase 2**

*Dernière mise à jour: 2025-02-19*  
*Phase: 2/5 - Integration Ready*
