# Blueprint Editor Refactor — March 2026

Journal des phases de refactoring du pipeline Blueprint Editor dans Olympe Engine.

---

## Phase 4 — Pipeline Blueprint Editor dans le debugger F10

Intégration du pipeline Blueprint Editor standalone (`NodeGraphPanel` + `BTGraphDocumentConverter` + `NodeGraphManager`) dans `BehaviorTreeDebugWindow` pour remplacer le rendu custom legacy.

### Pipeline final (Phase 4)
```
F10 → BehaviorTreeDebugWindow
  ├── RenderEntityListPanel()        [ECS — spécifique runtime]
  ├── RenderNodeGraphDebugPanel()    [Blueprint Editor pipeline — 100% partagé]
  │     ├── BTGraphDocumentConverter::FromBehaviorTree()
  │     ├── NodeGraphManager::SetActiveGraph()
  │     ├── NodeGraphPanel::SetActiveDebugNode()  [glow liens actifs]
  │     └── NodeGraphPanel::RenderGraph()
  └── RenderInspectorPanel()         [ECS — spécifique runtime]
```

---

## Phase 5 — Nettoyage legacy & archivage (2026-03-07)

### Objectif
Supprimer définitivement le code legacy du debugger runtime NodeGraph (F10) et archiver les fichiers devenus obsolètes suite à la Phase 4.

### Analyse CRUD/Undo-Redo
Comparaison exhaustive entre `BTEditorCommand` (debugger runtime) et `BPCommandSystem` (Blueprint Editor standalone) :
- CreateNode, DeleteNode, MoveNode, DuplicateNode, EditNode, Connect, Disconnect, Ctrl+Z/Y : **100% couverts par Blueprint Editor**
- Aucune feature à réinjecter dans le Blueprint Editor standalone
- Le mode éditeur embarqué dans le debugger F10 est hors scope par définition (un debugger observe, n'édite pas)

| Feature legacy debugger | Équivalent BP Standalone |
|---|---|
| `CreateNodeCommand` (BTEditorCommand) | `Blueprint::CreateNodeCommand` (BPCommandSystem) |
| `DeleteNodeCommand` | `Blueprint::DeleteNodeCommand` |
| `MoveNodeCommand` | `Blueprint::MoveNodeCommand` |
| `DuplicateNodeCommand` | `Blueprint::DuplicateNodeCommand` |
| `EditNodeCommand` | `Blueprint::EditNodeCommand` |
| Connect/Disconnect | `BlueprintAdapter::ConnectNodes/DisconnectNodes` |
| Ctrl+Z/Y | `NodeGraphPanel::HandleKeyboardShortcuts()` |

### Fichiers archivés dans Source/_deprecated/
- `BTEditorCommand.h/.cpp` — command pattern legacy exclusif au debugger F10
- `BTGraphLayoutEngine.h/.cpp` — layout BFS legacy, remplacé par NodeGraph::FromJson + positions JSON sauvegardées
- `BT_config.json` — config couleurs/layout consommée uniquement par LoadBTConfig() legacy
- `BehaviorTreeDebugWindow.cpp.backup` — backup pré-refactor Phase 4

> Note : les originaux de `BTEditorCommand.h/.cpp` et `BTGraphLayoutEngine.h/.cpp` sont conservés dans `Source/AI/` car des fichiers de `Source/NodeGraphShared/` (CommandAdapter.h, BehaviorTreeAdapter.h, NodeGraphShared.h, Serializer.h, BTGraphDocumentConverter.cpp) en dépendent toujours.

### Code retiré de BehaviorTreeDebugWindow.h/.cpp
- ~2950 LOC supprimés : rendering custom, mode éditeur, CRUD, save/serialize, camera custom, layout legacy
- Structs supprimés : `PinType`, `NodePin`, `BTConfig`, `BTColor`
- Membres morts nettoyés : `m_editorMode`, `m_editingTree`, `m_commandStack`, `m_layoutEngine`, `m_nodeColors`, `m_currentZoom`, `m_showMinimap`, `m_currentLayout`, `m_selectedNodes`, `m_linkMap`, etc.
- Includes retirés : `BTGraphLayoutEngine.h`, `BTEditorCommand.h`, `NodeStyleRegistry.h`, `RenderHelpers.h`
- Méthodes supprimées : `RenderNodeGraphPanel()`, `RenderBehaviorTreeGraph()`, `RenderNode()`, `RenderNodeConnections()`, `RenderBezierConnection()`, `RenderActiveLinkGlow()`, `RenderNodePins()`, `GetNodeColor()`, `GetNodeColorByStatus()`, `GetNodeIcon()`, `RenderNodePalette()`, `RenderEditorToolbar()`, `HandleNodeCreation()`, `HandleNodeDeletion()`, `HandleNodeDuplication()`, `ValidateConnection()`, `SaveEditedTree()`, `UndoLastAction()`, `RedoLastAction()`, `LoadBTConfig()`, `ApplyConfigToLayout()`, `SnapToGrid()`, `FitGraphToView()`, `CenterViewOnGraph()`, `ResetZoom()`, `RenderMinimap()`, `ApplyZoomToStyle()`, `GetGraphBounds()`, `GetSafeZoom()`, `CalculatePanOffset()`, `RenderValidationPanel()`, `GetPinColor()`, `IsConnectionValid()`, `RenderNodeProperties()`, `RenderNodePropertiesSection()`, `ApplyModifiedParameters()`, `GenerateUniqueNodeId()`, `GetMutableNodeById()`, `Save()`, `SaveAs()`, `RenderFileMenu()`, `SerializeTreeToJson()`, `GetCurrentTimestamp()`, `RenderNewBTDialog()`, `CreateFromTemplate()`, `RenderEditMenu()`

### Code conservé
- `RenderEntityListPanel()` + filtrage/tri ECS
- `RenderInspectorPanel()` → RuntimeInfo, Blackboard, ExecutionLog
- `AddExecutionEntry()`
- SDL3 window management
- `BehaviorTreeDebugWindow_NodeGraph.cpp` (pipeline Blueprint Editor — inchangé)

### Pipeline final (Phase 5)
```
F10 → BehaviorTreeDebugWindow
  ├── RenderEntityListPanel()        [ECS — spécifique runtime]
  ├── RenderNodeGraphDebugPanel()    [Blueprint Editor pipeline — 100% partagé]
  │     ├── BTGraphDocumentConverter::FromBehaviorTree()
  │     ├── NodeGraphManager::SetActiveGraph()
  │     ├── NodeGraphPanel::SetActiveDebugNode()  [glow liens actifs]
  │     └── NodeGraphPanel::RenderGraph()
  └── RenderInspectorPanel()         [ECS — spécifique runtime]
```
