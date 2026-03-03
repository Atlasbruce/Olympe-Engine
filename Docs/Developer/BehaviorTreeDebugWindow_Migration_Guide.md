# 🔄 MIGRATION GUIDE - BehaviorTreeDebugWindow vers NodeGraphPanel
## Refonte Debug Visualizer (F10) - Utilisation Pipeline Unifié
**Date**: 2025-02-19  
**Statut**: Prêt pour implémentation  
**Objectif**: Remplacer custom rendering par NodeGraphRenderer

---

## 📊 RÉSUMÉ MIGRATION

### Avant (Custom Pipeline)
- **Rendering custom** : ~2000 LOC de code dupliqué
- **Pins custom** : dessinés manuellement
- **Connexions custom** : Bezier curves manuelles
- **Minimap custom** : implémentation complète custom
- **Zoom/Pan** : gestion manuelle

### Après (Unified Pipeline)
- **Rendering unifié** : NodeGraphPanel (~50 LOC d'appel)
- **Pins standard** : ImNodes BeginInputAttribute/BeginOutputAttribute
- **Connexions standard** : ImNodes::Link
- **Minimap standard** : ImNodes::MiniMap
- **Zoom/Pan** : ImNodes standard

### Gain
- **-90% de code** dans BehaviorTreeDebugWindow
- **API commune** avec Standalone Editor
- **Maintenabilité** : un seul pipeline à maintenir

---

## 🔧 ÉTAPES DE MIGRATION

### Étape 1 : Ajouter Dépendances
**Fichier** : `Source/AI/BehaviorTreeDebugWindow.h`

```cpp
// ✅ AJOUTER ces includes
#include "../NodeGraphCore/NodeGraphPanel.h"
#include "../NodeGraphCore/EditorContext.h"
#include "../NodeGraphShared/BTGraphDocumentConverter.h"
```

### Étape 2 : Ajouter Membres Privés
**Fichier** : `Source/AI/BehaviorTreeDebugWindow.h`

```cpp
private:
    // ✅ AJOUTER ces membres
    NodeGraph::NodeGraphPanel m_graphPanel;
    NodeGraph::GraphDocument* m_cachedGraphDoc = nullptr;
    uint32_t m_cachedTreeId = 0;
```

### Étape 3 : Initialiser le Panel
**Fichier** : `Source/AI/BehaviorTreeDebugWindow.cpp`

```cpp
void BehaviorTreeDebugWindow::Initialize()
{
    // ... code existant ...
    
    // ✅ AJOUTER : Initialiser NodeGraphPanel
    NodeGraph::EditorContext debugCtx = NodeGraph::EditorContext::CreateDebugger();
    
    NodeGraph::RenderConfig config;
    config.mode = NodeGraph::RenderMode::Debug;
    config.enableMinimap = true;
    config.minimapSizeFraction = 0.15f;
    config.minimapLocation = ImNodesMiniMapLocation_BottomRight;
    config.enableRuntimeHighlight = true;
    config.pulseFrequency = 2.0f;
    config.styleRegistry = &NodeStyleRegistry::Get();
    
    m_graphPanel.Initialize(debugCtx, config);
}
```

### Étape 4 : Remplacer RenderNodeGraphPanel
**Fichier** : `Source/AI/BehaviorTreeDebugWindow.cpp`

**AVANT** (~2000 LOC) :
```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    ImNodes::BeginNodeEditor();
    
    // ❌ CUSTOM RENDERING (à supprimer)
    for (const auto& layout : m_currentLayout)
    {
        RenderCustomNode(layout);
    }
    
    RenderCustomConnections();
    RenderCustomPins();
    
    ImNodes::EndNodeEditor();
    
    RenderMinimap(); // Custom minimap
}
```

**APRÈS** (~50 LOC) :
```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    auto& world = World::Get();
    
    if (m_selectedEntity == 0 || !world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No entity selected");
        return;
    }
    
    // ✅ Récupérer BehaviorTreeAsset
    const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
    const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
    
    if (!tree)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Tree not found");
        return;
    }
    
    // ✅ Convertir en GraphDocument (cache pour performance)
    if (m_cachedTreeId != tree->id)
    {
        if (m_cachedGraphDoc)
        {
            delete m_cachedGraphDoc;
        }
        m_cachedGraphDoc = BTGraphDocumentConverter::ToGraphDocument(tree, &m_currentLayout);
        m_cachedTreeId = tree->id;
    }
    
    // ✅ Définir nœud actif (runtime highlighting)
    uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;
    m_graphPanel.SetCurrentRuntimeNode(currentNodeId);
    
    // ✅ Rendre via NodeGraphPanel
    m_graphPanel.SetDocument(m_cachedGraphDoc);
    float deltaTime = ImGui::GetIO().DeltaTime;
    m_graphPanel.Render(deltaTime);
}
```

### Étape 5 : Nettoyer Code Obsolète
**Fichiers à modifier** : `Source/AI/BehaviorTreeDebugWindow.cpp`

**SUPPRIMER** ces méthodes (devenues obsolètes) :
```cpp
// ❌ SUPPRIMER (remplacé par NodeGraphRenderer)
void RenderCustomNode(const BTNodeLayout& layout);
void RenderCustomConnections();
void RenderCustomPins();
void DrawBezierConnection(...);
void DrawPin(...);
void RenderMinimap();  // Remplacé par ImNodes::MiniMap

// ❌ SUPPRIMER (gestion zoom/pan maintenant dans NodeGraphRenderer)
void ApplyZoomToStyle();
void FitGraphToView();    // Remplacé par m_graphPanel.GetRenderer()->FitGraphToView()
void CenterViewOnGraph(); // Remplacé par m_graphPanel.GetRenderer()->CenterView()
void ResetZoom();         // Remplacé par m_graphPanel.GetRenderer()->ResetZoom()
```

### Étape 6 : Migrer Actions Menu
**Fichier** : `Source/AI/BehaviorTreeDebugWindow.cpp`

**AVANT** :
```cpp
if (ImGui::MenuItem("Fit Graph to View", "F"))
{
    FitGraphToView();  // Custom implementation
}
```

**APRÈS** :
```cpp
if (ImGui::MenuItem("Fit Graph to View", "F"))
{
    m_graphPanel.GetRenderer()->FitGraphToView();  // ✅ Utilise NodeGraphRenderer
}

if (ImGui::MenuItem("Center View", "C"))
{
    m_graphPanel.GetRenderer()->CenterView();
}

if (ImGui::MenuItem("Reset Zoom", "0"))
{
    m_graphPanel.GetRenderer()->ResetZoom();
}
```

### Étape 7 : Cleanup Destructeur
**Fichier** : `Source/AI/BehaviorTreeDebugWindow.cpp`

```cpp
BehaviorTreeDebugWindow::~BehaviorTreeDebugWindow()
{
    // ✅ AJOUTER : Libérer GraphDocument cached
    if (m_cachedGraphDoc)
    {
        delete m_cachedGraphDoc;
        m_cachedGraphDoc = nullptr;
    }
    
    // ... reste du code existant ...
}
```

---

## 📦 FICHIERS À MODIFIER

### Fichiers Principaux
```
Source/AI/
├── BehaviorTreeDebugWindow.h    # ⚠️ Ajouter membres m_graphPanel, m_cachedGraphDoc
└── BehaviorTreeDebugWindow.cpp  # ⚠️ Refactor RenderNodeGraphPanel (~2000 LOC -> ~50 LOC)
```

### Fichiers Créés (Nouveaux)
```
Source/NodeGraphCore/
├── NodeGraphRenderer.h/cpp       # ✅ CRÉÉ
├── EditorContext.h/cpp           # ✅ CRÉÉ
├── NodeGraphPanel.h/cpp          # ✅ CRÉÉ
└── Commands/
    └── DeleteLinkCommand.h/cpp   # ✅ CRÉÉ

Source/NodeGraphShared/
└── BTGraphDocumentConverter.h/cpp # ✅ CRÉÉ
```

---

## 🎯 VALIDATION POST-MIGRATION

### Tests Fonctionnels
- [ ] Debug window (F10) s'ouvre sans erreur
- [ ] Liste des entités affichée correctement
- [ ] Sélection d'entité affiche le graphe BT
- [ ] Nœud actif (runtime) est highlighted (pulse jaune/orange)
- [ ] Minimap visible (coin bas-droit)
- [ ] Zoom/Pan fonctionne (molette + drag)
- [ ] Fit to View (F) fonctionne
- [ ] Center View (C) fonctionne
- [ ] Reset Zoom (0) fonctionne
- [ ] Blackboard panel affiche les données
- [ ] Execution log affiche les entrées

### Tests Techniques
- [ ] 0 erreurs compilation
- [ ] 0 warnings compilation
- [ ] Pas de fuites mémoire (m_cachedGraphDoc libéré)
- [ ] Performance : 60 FPS constant avec 50+ nœuds
- [ ] SYSTEM_LOG utilisé (pas std::cout)

---

## 📊 MÉTRIQUES AVANT/APRÈS

| Métrique | Avant | Après | Gain |
|----------|-------|-------|------|
| LOC RenderNodeGraphPanel | ~2000 | ~50 | -97% |
| LOC Total BehaviorTreeDebugWindow | 3606 | 1200 | -67% |
| Méthodes custom rendering | 15 | 0 | -100% |
| Duplication vs Standalone | 60% | 0% | -100% |

---

## 🚨 POINTS D'ATTENTION

### Cache GraphDocument
```cpp
// ⚠️ IMPORTANT : Invalider cache si l'arbre change
if (m_cachedTreeId != tree->id)
{
    // Recréer GraphDocument
}
```

### Runtime Highlighting
```cpp
// ✅ Mettre à jour le nœud actif chaque frame
uint32_t currentNodeId = btRuntime.AICurrentNodeIndex;
m_graphPanel.SetCurrentRuntimeNode(currentNodeId);
```

### Performance
```cpp
// ✅ Convertir BT -> GraphDocument UNE FOIS (pas chaque frame)
// Cache avec m_cachedGraphDoc
```

---

## 🔄 ROLLBACK PLAN (si problème)

Si la migration pose problème :
1. **Git checkout** : Restaurer BehaviorTreeDebugWindow.cpp original
2. **Commentaire** : Désactiver appel m_graphPanel.Render()
3. **Uncomment** : Réactiver ancien RenderNodeGraphPanel
4. **Compile** : Vérifier compilation OK
5. **Debug** : Identifier la cause du problème

---

**FIN DU GUIDE MIGRATION - PRÊT POUR IMPLÉMENTATION**
