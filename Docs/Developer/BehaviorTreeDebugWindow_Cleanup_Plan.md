# 🔧 CLEANUP PLAN - BehaviorTreeDebugWindow.cpp
## Méthodes à Supprimer vs Conserver
**Date**: 2025-02-19  
**Statut**: File Corrupted - Needs Manual Cleanup

---

## ❌ MÉTHODES À SUPPRIMER (Custom Rendering - Obsolete)

Ces méthodes sont remplacées par NodeGraphRenderer :

```cpp
// Rendering methods (replaced by NodeGraphPanel)
void RenderNode(const BTNode* node, const BTNodeLayout* layout, bool isCurrentNode);
void RenderNodeConnections(const BTNode* node, const BTNodeLayout* layout, const BehaviorTreeAsset* tree, uint32_t activeNodeId);
void RenderBezierConnection(const Vector& start, const Vector& end, uint32_t color, float thickness, float tangent);
void RenderActiveLinkGlow(const Vector& start, const Vector& end, float tangent);
void RenderNodePins(const BTNode* node, const BTNodeLayout* layout);

// Camera/Zoom (now in NodeGraphRenderer)
void ApplyZoomToStyle();          // Remplacé par NodeGraphRenderer::ApplyZoomToStyle()
void FitGraphToView();            // Remplacé par m_graphPanel.GetRenderer()->FitGraphToView()
void CenterViewOnGraph();         // Remplacé par m_graphPanel.GetRenderer()->CenterView()
void ResetZoom();                 // Remplacé par m_graphPanel.GetRenderer()->ResetZoom()
void RenderMinimap();             // Remplacé par NodeGraphRenderer::RenderMinimap()
void GetGraphBounds(...);         // Remplacé par NodeGraphRenderer (private)
Vector CalculatePanOffset(...);   // Remplacé par NodeGraphRenderer (private)
float GetSafeZoom() const;        // Remplacé par NodeGraphRenderer (private)
```

**Total** : ~13 méthodes custom rendering à supprimer

---

## ✅ MÉTHODES À CONSERVER (Non-Rendering)

Ces méthodes restent nécessaires :

```cpp
// Lifecycle
void Initialize();
void Shutdown();
void ToggleVisibility();
void ProcessEvent(SDL_Event* event);

// Panels
void RenderEntityListPanel();       // Liste entités (gauche)
void RenderNodeGraphPanel();        // ✅ NOUVELLE VERSION UNIFIÉE
void RenderInspectorPanel();        // Inspecteur (droite)

// Entity management
void RefreshEntityList();
void UpdateEntityFiltering();
void UpdateEntitySorting();
void RenderEntityEntry(const EntityDebugInfo& info);

// Inspector helpers
void RenderRuntimeInfo();
void RenderBlackboardSection();
void RenderExecutionLog();

// Editor mode (à conserver même si moins utilisé)
void RenderEditorToolbar();
void HandleNodeCreation(BTNodeType nodeType);
void HandleNodeDeletion();
void HandleNodeDuplication();
void SaveEditedTree();

// Helpers
uint32_t GetNodeColor(BTNodeType type) const;
const char* GetNodeIcon(BTNodeType type) const;
void LoadBTConfig();
Vector SnapToGrid(const Vector& pos) const;

// Execution log
void AddExecutionEntry(EntityID entity, uint32_t nodeId, const std::string& nodeName, BTStatus status);

// Window management
void CreateSeparateWindow();
void DestroySeparateWindow();
void RenderInSeparateWindow();
```

**Total** : ~30 méthodes à conserver

---

## 🔍 ÉTAT ACTUEL DU FICHIER

### Problèmes Identifiés
1. **GetNodeColor** dupliqué (ligne ~898 ET ligne ~1698)
2. **RenderNodeConnections** existe encore (devrait être supprimé)
3. **~700+ LOC de code orphelin** entre nouvelle RenderNodeGraphPanel et vraies méthodes
4. **RenderInspectorPanel** a du code orphelin au début

### Structure Attendue (Ligne)
```
1-200    : Includes + Constructor/Destructor
200-300  : Initialize/Shutdown
300-500  : Window management (CreateSeparateWindow, etc.)
500-700  : RenderInSeparateWindow + main Render
700-900  : RenderEntityListPanel
900-1000 : RenderNodeGraphPanel (✅ NOUVELLE VERSION ~60 LOC)
1000-1200: RenderInspectorPanel
1200-1400: Inspector helpers (RenderRuntimeInfo, RenderBlackboardSection, etc.)
1400-1500: Entity helpers (RefreshEntityList, etc.)
1500-1600: Editor mode methods (si conservés)
1600-1700: Helpers (GetNodeColor, GetNodeIcon, LoadBTConfig, etc.)
```

**LOC Attendu** : ~1700 LOC (vs ~3606 LOC actuels après cleanup)

---

## 🛠️ STRATÉGIE DE NETTOYAGE

### Option 1 : Remplacement Manuel Ligne par Ligne (Trop Long)
- Supprimer chaque méthode obsolète une par une
- Risque d'erreurs et d'oublis
- ⏱️ Temps estimé : 2h

### Option 2 : Reconstruction Fichier (Recommandé)
1. Extraire toutes les méthodes à **conserver** dans un fichier temporaire
2. Créer nouveau fichier propre avec structure correcte
3. Réinjecter méthodes conservées
4. Compiler et valider
- ⏱️ Temps estimé : 30 min

### Option 3 : Script PowerShell de Nettoyage
1. Lire fichier ligne par ligne
2. Identifier début/fin de chaque méthode
3. Supprimer méthodes obsolètes
4. Régénérer fichier propre
- ⏱️ Temps estimé : 20 min (script) + 10 min (validation)

---

## ✅ VALIDATION POST-NETTOYAGE

Après nettoyage, le fichier doit :
- [ ] Compiler sans erreurs (0 errors)
- [ ] Compiler sans warnings (0 warnings)
- [ ] Avoir ~1700 LOC (vs 3606 avant)
- [ ] GetNodeColor n'apparaît qu'UNE fois
- [ ] RenderNodeConnections n'existe plus
- [ ] RenderNode n'existe plus
- [ ] ApplyZoomToStyle n'existe plus
- [ ] FitGraphToView n'existe plus (ou redirige vers m_graphPanel)
- [ ] RenderMinimap n'existe plus

---

## 🚀 ACTIONS IMMÉDIATES

1. **Backup complet** : ✅ FAIT (BehaviorTreeDebugWindow.cpp.backup)
2. **Option choisie** : Option 2 (Reconstruction)
3. **Extraction** : Lire fichier backup, extraire méthodes à conserver
4. **Reconstruction** : Créer fichier propre
5. **Validation** : Compiler et tester

---

**STATUT** : En Cours  
**PROCHAINE ÉTAPE** : Reconstruction Fichier
