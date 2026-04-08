# 🎯 PHASE 35 - Réintégration BehaviorTree dans Blueprint Editor

**Date**: 08-04-2026
**Status**: PLANIFICATION EN COURS
**Effort Estimé**: 12-19 jours (phases 1-3 du rapport complet)

---

## 📊 Vue d'ensemble

L'objectif est de **réintégrer complètement** le système BehaviorTree dans le Blueprint Editor existant avec :
- ✅ Canvas de rendu avec tabs (via TabManager)
- ✅ Palette de nœuds drag-drop
- ✅ Panel d'édition des propriétés
- ✅ Sérialisation JSON v2
- ✅ Undo/Redo commands
- ✅ Validation et compilation

### État Actuel ⚠️
- ✅ **Runtime BT System**: 100% fonctionnel (execution, conditions, actions, debugger)
- ⚠️ **Editor Canvas**: BehaviorTreeRenderer adapter existe mais PAS instancié
- ⚠️ **UI Panels**: BTNodePalette existe mais pas intégré au editor principal
- ❌ **Property Panel**: Manquant (PropertyPanel_BT.h/cpp)
- ⚠️ **Serialization**: Parsing JSON v2 existe mais SaveToFile() incomplet

---

## 🔧 Architecture Technique Actuelle

### 1. Détection des types de graphe ✅
**Fichier**: `Source\BlueprintEditor\TabManager.cpp:84-141`

```cpp
std::string TabManager::DetectGraphType(const std::string& filePath)
{
    // Détecte "BehaviorTree" à partir de:
    // - Champ "blueprintType": "BehaviorTree"
    // - Champ "graphType": "BehaviorTree"
    // - Structure { "rootNodeId": ..., "nodes": ... }
}
```

✅ Détection OK | ⚠️ Mais instantiation manquante

### 2. Instantiation du Renderer ❌
**Fichier**: `Source\BlueprintEditor\TabManager.cpp:147-189`

```cpp
std::string TabManager::CreateNewTab(const std::string& graphType)
{
    if (graphType == "VisualScript")
    {
        VisualScriptRenderer* r = new VisualScriptRenderer();
        tab.renderer = r;
    }
    else if (graphType == "EntityPrefab")
    {
        // ... EntityPrefab creation
    }
    // ❌ MISSING: BehaviorTree case!
}
```

**GAP IDENTIFIÉ**: BehaviorTree détecté mais jamais instancié

### 3. Composants existants ✅

| Composant | Fichier | Status |
|-----------|---------|--------|
| BehaviorTreeRenderer | `Source\BlueprintEditor\BehaviorTreeRenderer.h/cpp` | ✅ Existe (adapter IGraphRenderer) |
| BehaviorTreeEditorPlugin | `Source\BlueprintEditor\BehaviorTreeEditorPlugin.h/cpp` | ✅ Existe |
| BTNodeGraphManager | `Source\BlueprintEditor\BTNodeGraphManager.h/cpp` | ✅ Existe |
| BTNodePalette | `Source\AI\AIGraphPlugin_BT\BTNodePalette.h/cpp` | ✅ Existe |
| BTNodeRegistry | `Source\AI\AIGraphPlugin_BT\BTNodeRegistry.h/cpp` | ✅ Existe |
| BTGraphValidator | `Source\AI\AIGraphPlugin_BT\BTGraphValidator.h/cpp` | ✅ Existe |
| BTGraphCompiler | `Source\AI\AIGraphPlugin_BT\BTGraphCompiler.h/cpp` | ✅ Existe |
| BehaviorTree (core) | `Source\AI\BehaviorTree.h/cpp` | ✅ Existe |
| PropertyPanel_BT | **MANQUANT** | ❌ À créer |

---

## 🚀 Roadmap d'Implémentation

### PHASE 1: Canvas & TabManager Integration (3-5 jours)

**Objectif**: Activer création/ouverture de graphes BehaviorTree dans l'éditeur

#### Step 1.1: TabManager Integration
**Fichier à modifier**: `Source\BlueprintEditor\TabManager.cpp`

Ajouter case BehaviorTree dans `CreateNewTab()`:
```cpp
else if (graphType == "BehaviorTree")
{
    static NodeGraphPanel s_btPanel;
    static bool s_btPanelInit = false;
    if (!s_btPanelInit)
    {
        s_btPanel.Initialize();
        s_btPanelInit = true;
    }
    
    BehaviorTreeRenderer* r = new BehaviorTreeRenderer(s_btPanel);
    tab.renderer = r;
}
```

**Effort**: 0.5 jour

#### Step 1.2: File Loading Integration
**Fichier à modifier**: `Source\BlueprintEditor\TabManager.cpp:OpenFileInTab()`

Vérifier que BehaviorTree files (.json) sont correctement chargés et parsés.

**Effort**: 0.5 jour

#### Step 1.3: Menu Integration
**Fichier à modifier**: `Source\BlueprintEditor\BlueprintEditorGUI.cpp`

Ajouter "New -> Behavior Tree" au menu File:
```cpp
// RenderMenuBar()
if (ImGui::MenuItem("Behavior Tree", "Ctrl+Alt+B"))
{
    TabManager::Get().CreateNewTab("BehaviorTree");
}
```

**Effort**: 0.5 jour

#### Step 1.4: Keyboard Shortcut
**Fichier à modifier**: `Source\BlueprintEditor\BlueprintEditorGUI.cpp`

Implémenter `Ctrl+Alt+B` pour créer nouveau BT.

**Effort**: 0.5 jour

---

### PHASE 2: Node Palette & Creation (2-3 jours)

**Objectif**: Permettre drag-drop de nœuds depuis la palette

#### Step 2.1: Integrate BTNodePalette UI
**Fichier à modifier**: `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`

Ajouter BTNodePalette panel à côté du canvas:
```cpp
// In Render():
ImGui::BeginChild("btNodePalette", ImVec2(150, 0), true);
m_btNodePalette.Render();
ImGui::EndChild();

ImGui::SameLine();

ImGui::BeginChild("btCanvas");
m_panel.Render();
ImGui::EndChild();
```

**Effort**: 1 jour

#### Step 2.2: Drag-Drop Event Handling
**Fichier à modifier**: `Source\BlueprintEditor\BTNodeGraphManager.cpp`

Implémenter `OnNodeDropped()` callback:
```cpp
void BTNodeGraphManager::OnNodeDropped(const std::string& nodeType, 
                                        const ImVec2& position)
{
    // Create node at position
    // Add to active graph
    // Mark dirty
}
```

**Effort**: 1 jour

#### Step 2.3: Node Validation
**Fichier à modifier**: `Source\AI\AIGraphPlugin_BT\BTNodeRegistry.cpp`

Vérifier constraints lors de création:
- Max/min children
- Node type compatibility
- Parent-child relationships

**Effort**: 0.5 jour

---

### PHASE 3: Property Panel & Serialization (3-4 jours)

**Objectif**: Éditer propriétés de nœuds et sauvegarder en JSON

#### Step 3.1: Create PropertyPanel_BT
**Fichier à créer**: `Source\BlueprintEditor\PropertyPanel_BT.h/cpp`

Nouvelle classe pour afficher/éditer propriétés:
```cpp
class PropertyPanel_BT
{
public:
    void Render(const BTNode* selectedNode);
    
private:
    void RenderNodeBasics(const BTNode* node);
    void RenderNodeParameters(const BTNode* node);
    void RenderConditionProperties(const BTNode* node);
    void RenderActionProperties(const BTNode* node);
};
```

**Effort**: 1.5 jour

#### Step 3.2: Property Binding
**Fichiers à modifier**: 
- `Source\BlueprintEditor\BTNodeGraphManager.cpp`
- `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`

Connecter node selection → PropertyPanel affichage:
```cpp
// In OnNodeSelected(nodeId):
if (m_propertyPanel)
{
    const BTNode* node = GetNodeById(nodeId);
    m_propertyPanel->SetSelectedNode(node);
}
```

**Effort**: 1 jour

#### Step 3.3: Serialization Save/Load
**Fichiers à modifier**: `Source\AI\BehaviorTree.cpp`

Implémenter:
```cpp
bool BehaviorTreeAsset::SaveToFile(const std::string& path)
{
    nlohmann::json root;
    root["schema_version"] = 2;
    root["blueprintType"] = "BehaviorTree";
    root["name"] = m_name;
    
    // Serialize nodes
    nlohmann::json nodesArray;
    for (const auto& node : m_nodes)
    {
        nodesArray.push_back(SerializeNode(node));
    }
    root["data"]["nodes"] = nodesArray;
    
    // Write to file
    std::ofstream ofs(path);
    ofs << root.dump(2);
    return true;
}
```

**Effort**: 1 jour

#### Step 3.4: JSON Schema Validation
**Fichier à modifier**: `Source\AI\AIGraphPlugin_BT\BTGraphValidator.cpp`

Valider avant save:
- No cycles
- All connections valid
- Required fields present

**Effort**: 0.5 jour

---

### PHASE 4: Undo/Redo & Validation (2-3 jours)

**Objectif**: Support undo/redo et validation en temps réel

#### Step 4.1: Command Integration
**Fichier à modifier**: `Source\BlueprintEditor\BTNodeGraphManager.cpp`

Connecter commandes au stack undo/redo:
```cpp
// When creating node:
CommandStack::Get().Execute(
    std::make_unique<AddBTNodeCommand>(graphId, nodeType, position)
);

// When deleting:
CommandStack::Get().Execute(
    std::make_unique<DeleteBTNodeCommand>(graphId, nodeId)
);
```

**Effort**: 1 jour

#### Step 4.2: Toolbar Buttons
**Fichier à modifier**: `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`

Ajouter buttons toolbar:
```cpp
if (ImGui::Button("Validate"))
{
    ValidateCurrentGraph();
}
ImGui::SameLine();
if (ImGui::Button("Compile"))
{
    CompileCurrentGraph();
}
```

**Effort**: 0.5 jour

#### Step 4.3: Error Display
**Fichier à modifier**: `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`

Afficher validation errors/warnings:
```cpp
// Render status bar
if (!m_validationErrors.empty())
{
    ImGui::TextColored(ImVec4(1,0,0,1), "Errors: %zu", m_validationErrors.size());
}
```

**Effort**: 0.5 jour

---

### PHASE 5: Testing & Polish (2-3 jours)

**Objectif**: Tests complets et optimisations

#### Step 5.1: Create Test BehaviorTree
Créer test file `Gamedata\Blueprints\AI\test_editor_bt.json`

#### Step 5.2: Integration Testing
- Load existing BT files
- Create new BT
- Add nodes via palette
- Edit properties
- Save/load round-trip
- Test undo/redo

#### Step 5.3: Performance Optimization
- Profile rendering
- Optimize node layout
- Cache performance

---

## 📋 Checklist Complète des Modifications

### Fichiers à MODIFIER

- [ ] `Source\BlueprintEditor\TabManager.cpp` (CreateNewTab + OpenFileInTab)
- [ ] `Source\BlueprintEditor\BlueprintEditorGUI.cpp` (Menu + Shortcuts)
- [ ] `Source\BlueprintEditor\BehaviorTreeRenderer.cpp` (Palette + Toolbar)
- [ ] `Source\BlueprintEditor\BTNodeGraphManager.cpp` (Node creation + Events)
- [ ] `Source\AI\BehaviorTree.cpp` (SaveToFile + LoadFromFile)
- [ ] `Source\AI\AIGraphPlugin_BT\BTGraphValidator.cpp` (UI Integration)
- [ ] `Source\AI\AIGraphPlugin_BT\BTNodeRegistry.cpp` (Validation)

### Fichiers à CRÉER

- [ ] `Source\BlueprintEditor\PropertyPanel_BT.h` (New)
- [ ] `Source\BlueprintEditor\PropertyPanel_BT.cpp` (New)

### Test Files à CRÉER

- [ ] `Gamedata\Blueprints\AI\test_editor_bt.json` (Test case)

### Documentation à METTRE À JOUR

- [ ] `.github/copilot-instructions.md` (Add Phase 35 status)
- [ ] `Documentation/PHASE_35_BT_EDITOR_REINTEGRATION_PLAN.md` (This file)

---

## 🔗 Ressources de Référence

### Rapport Complet d'Analyse
`Project Management\Features\RAPPORT COMPLET D'ANALYSE - BEHAVIOUR TREE SYSTEM (Olympe Engine) - 08-04-2026.md`

### Architecture Existante
- **TabManager**: Central tab system (`Source\BlueprintEditor\TabManager.h/cpp`)
- **IGraphRenderer**: Abstract renderer interface
- **NodeGraphPanel**: Shared canvas rendering
- **EntityPrefab Editor**: Reference implementation (full integration example)

### Système BT
- **BehaviorTree.h/cpp**: Core data structures
- **BehaviorTreeSystem**: ECS runtime execution
- **BehaviorTreeManager**: Asset loading/caching
- **BehaviorTreeDebugWindow**: Runtime visualization

---

## 🎯 Success Criteria

Phase 35 est COMPLETE quand:
- ✅ "New -> Behavior Tree" menu item works
- ✅ Can open existing BT files from filesystem
- ✅ Drag-drop node creation from palette works
- ✅ Property panel shows/edits node properties
- ✅ Save/load round-trip preserves all data
- ✅ Undo/redo works for all operations
- ✅ Validation shows errors with error navigation
- ✅ No compilation errors or warnings
- ✅ 3-5 existing BT files can be loaded and edited
- ✅ New BTs can be created from scratch

---

## 📝 Notes d'Implémentation

### Point Clé 1: Shared NodeGraphPanel
EntityPrefab uses separate PrefabCanvas. BehaviorTree COULD use shared NodeGraphPanel or create separate BTCanvas. Recommandation: **Partager NodeGraphPanel** (comme VisualScript) pour maintenir cohérence.

### Point Clé 2: Serialization Format
BehaviorTree v2 JSON schema:
```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "name": "patrol_ai",
  "data": {
    "rootNodeId": 1,
    "nodes": [
      { "id": 1, "type": "Selector", "children": [2, 3] },
      { "id": 2, "type": "Condition", "conditionType": "TargetVisible" },
      { "id": 3, "type": "Action", "actionType": "PatrolPickNextPoint" }
    ]
  }
}
```

### Point Clé 3: Node Parameter Storage
BTNode stores parameters as:
```cpp
struct BTNode {
    std::map<std::string, std::string> stringParams;
    std::map<std::string, int> intParams;
    std::map<std::string, float> floatParams;
};
```
PropertyPanel_BT doit supporter ces 3 types.

---

## 🔄 Integration Flow

```
User Action                Step                                Output
─────────────────────────────────────────────────────────────────
File -> New BT      → BlueprintEditorGUI::NewBehaviorTree()   → TabManager::CreateNewTab("BehaviorTree")
                    → TabManager creates BehaviorTreeRenderer  → BehaviorTreeRenderer owns NodeGraphPanel
                    → SetActiveTab triggers Render()

Drag node            → NodeGraphPanel detects drag-drop        → BTNodeGraphManager::OnNodeDropped()
from palette         → Palette payload contains node type       → Creates BTNode, adds to graph

Click node           → NodeGraphPanel::OnNodeSelected()        → Notifies BTNodeGraphManager
                    → BTNodeGraphManager updates selection     → PropertyPanel_BT::SetSelectedNode()

Edit property        → PropertyPanel_BT updates BTNode         → Marks graph dirty
                    → OnPropertyChanged triggers mark dirty

Save (Ctrl+S)        → TabManager::SaveActiveTab()             → BehaviorTreeRenderer::Save()
                    → BehaviorTreeRenderer delegates to Asset  → BehaviorTreeAsset::SaveToFile()
                    → Writes JSON v2 to disk

Load file            → TabManager::OpenFileInTab()             → DetectGraphType() → "BehaviorTree"
                    → CreateNewTab("BehaviorTree")             → BehaviorTreeRenderer created
                    → Renderer::Load() → BehaviorTreeAsset::LoadFromFile()
                    → Parses JSON, populates NodeGraphPanel
```

---

## 📊 Effort Breakdown

| Phase | Task | Days | Cumulative |
|-------|------|------|-----------|
| 1 | Canvas & TabManager | 3-5 | 3-5 |
| 2 | Node Palette | 2-3 | 5-8 |
| 3 | Property Panel & Serialization | 3-4 | 8-12 |
| 4 | Undo/Redo & Validation | 2-3 | 10-15 |
| 5 | Testing & Polish | 2-3 | 12-18 |
| **TOTAL** | **Full Phase 35** | **12-19 days** | **12-19** |

---

## 🚀 Next Steps

1. **Immediate**: Modify TabManager.cpp (Step 1.1)
2. **Quick**: Add Menu items (Step 1.3)
3. **Core**: Create PropertyPanel_BT (Step 3.1)
4. **Integration**: Wire all components together
5. **Testing**: Comprehensive test suite

---

*Last Updated: 08-04-2026*
*Status: PLANIFICATION COMPLÈTE - PRÊT À IMPLÉMENTER*
