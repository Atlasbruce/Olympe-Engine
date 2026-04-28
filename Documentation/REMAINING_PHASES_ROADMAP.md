# 🚀 ROADMAP - Étapes Restantes à Implémenter

**Document généré:** 2025  
**Status du Projet:** Phases 1-31 ✅ Complétées | Phases 32+ 📋 À faire  
**Build Status:** ✅ Génération réussie (0 erreurs)

---

## 📊 SYNTHÈSE GLOBALE

### ✅ Phases Complétées (1-31)
| Phase | Domaine | Status |
|-------|---------|--------|
| 24 | VisualScript - Condition Presets | ✅ |
| 26 | VisualScript - Tab UI | ✅ |
| 27 | EntityPrefab - Rendering Pipeline | ✅ |
| 28 | EntityPrefab - Interactive Features (Zoom/Pan/Drag) | ✅ |
| 29 | EntityPrefab - Drag-Drop Components | ✅ |
| 29b | EntityPrefab - Dynamic Component Loading from JSON | ✅ |
| 30 | EntityPrefab - Connection UI (Port-based) | ✅ |
| 30.1 | EntityPrefab - ComponentPalette JSON Integration | ✅ |
| 31 | EntityPrefab - Rectangle Selection + Property Panel | ✅ |
| 5 | Canvas Grid Standardization | ✅ |

### 📋 Phases Restantes (32+)

---

## 🎯 PHASE 32: ENTITY PREFAB EDITOR - OPTIMISATION & POLISHING

**Status:** 📋 À faire  
**Priority:** 🔴 HAUTE  
**Estimated Duration:** 2-3 jours

### Sous-phases 32.x

#### **Phase 32.1: Copy/Paste Nodes & Subgraphs**
**Objectif:** Permettre la duplication efficace de nœuds et de sous-graphes

**Features à implémenter:**
- [x] Ctrl+C: Copier nœuds sélectionnés en mémoire
- [x] Ctrl+V: Coller nœuds sélectionnés (offset position)
- [x] Ctrl+D: Dupliquer nœuds sélectionnés (clone local)
- [x] Copie de sous-graphes (nœuds + connections entre eux)
- [x] Réassociation des IDs de nœuds lors du paste
- [x] Preservation des connections internes aux nœuds copiés

**Files à modifier:**
```
Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.cpp
  - AddClipboardManager member
  - OnKeyDown(): Handle Ctrl+C, Ctrl+V, Ctrl+D

Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.cpp
  - CloneNodes(selectedNodeIds) -> std::vector<ComponentNode>
  - GetConnectionsBetweenNodes(nodeIds) -> connections subset

NEW: Source/BlueprintEditor/Utilities/ClipboardManager.h/cpp
  - Serialization/deserialization de nœuds
```

**Example Flow:**
```
User selects: Node 1, Node 2, Node 3 (with connections 1→2, 2→3)
User presses: Ctrl+C
System: Stores nodes + connections in m_clipboard
User drags mouse: 100px right, 50px down
User presses: Ctrl+V
System: Creates new nodes (IDs: 10, 11, 12) at offset position
System: Creates new connections (10→11, 11→12)
Result: Duplicate subgraph appears at new position
```

**Test Case:**
- Copy/paste single node → verify ID differs, position offset correct
- Copy/paste multiple nodes with internal connections → verify all connections preserved
- Copy/paste, then paste again → verify new IDs don't conflict

---

#### **Phase 32.2: Undo/Redo System**
**Objectif:** Implémenter un système d'historique complet des modifications

**Architecture:**
```
Command Pattern:
├── ICommand (abstract)
│   ├── CreateNodeCommand
│   ├── DeleteNodeCommand
│   ├── ConnectNodesCommand
│   ├── DisconnectNodesCommand
│   ├── MoveNodeCommand
│   ├── ModifyPropertyCommand
│   └── (others as needed)
│
├── CommandHistory
│   ├── m_undoStack: std::vector<ICommand*>
│   ├── m_redoStack: std::vector<ICommand*>
│   ├── ExecuteCommand(ICommand*)
│   ├── Undo()
│   └── Redo()
│
└── Integration in EntityPrefabGraphDocument
    ├── m_commandHistory: CommandHistory
    └── All modifications go through command system
```

**Features à implémenter:**
- [x] Undo stack (LIFO)
- [x] Redo stack (LIFO)
- [x] Ctrl+Z: Undo
- [x] Ctrl+Y: Redo
- [x] Clear redo stack on new command
- [x] Command logging for debugging
- [x] Max history size (e.g., 100 commands)
- [x] Memory management for discarded commands

**Files à créer:**
```
NEW: Source/BlueprintEditor/Utilities/ICommand.h
NEW: Source/BlueprintEditor/Utilities/CommandHistory.h/cpp
NEW: Source/BlueprintEditor/EntityPrefabEditor/Commands/
  ├── CreateNodeCommand.h/cpp
  ├── DeleteNodeCommand.h/cpp
  ├── ConnectNodesCommand.h/cpp
  ├── DisconnectNodesCommand.h/cpp
  ├── MoveNodeCommand.h/cpp
  └── ModifyPropertyCommand.h/cpp
```

**Files à modifier:**
```
Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.cpp
  - All operations wrapped in commands

Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.cpp
  - Add command history member
  - Modify all public methods to use commands
```

**Example Flow:**
```
User drags node: Position (100,50) → (200,100)
System creates: MoveNodeCommand(nodeId, oldPos, newPos)
System executes: command.Execute() → updates node position
System pushes: command to undoStack
User presses: Ctrl+Z
System pops: from undoStack, calls Undo()
Result: Node returns to (100,50)
```

---

#### **Phase 32.3: Node Organization Features**
**Objectif:** Aider l'utilisateur à organiser les nœuds sur le canvas

**Features à implémenter:**
- [x] Comment nodes (text-only, no component)
  - [x] Right-click → "Add Comment"
  - [x] Edit comment text (double-click)
  - [x] Delete comment (Delete key)
  - [x] Change comment color (context menu)

- [x] Node grouping/alignment
  - [x] Ctrl+G: Group selected nodes (visual grouping only, no component)
  - [x] Align Left/Center/Right: Align selected nodes horizontally
  - [x] Align Top/Middle/Bottom: Align selected nodes vertically
  - [x] Distribute Horizontal/Vertical: Even spacing

- [x] Minimap viewport
  - [x] Small preview of entire graph in corner
  - [x] Shows current camera viewport as rectangle
  - [x] Click on minimap to pan to that location
  - [x] Can toggle on/off with button

**Files à créer:**
```
NEW: Source/BlueprintEditor/EntityPrefabEditor/CommentNode.h/cpp
NEW: Source/BlueprintEditor/EntityPrefabEditor/Minimap.h/cpp
```

**Files à modifier:**
```
Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.cpp
  - Add vector<CommentNode> m_comments

Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.cpp
  - RenderComments()
  - HandleCommentInteraction()
  - RenderMinimap()
  - OnKeyDown(): Handle alignment/grouping commands
```

---

#### **Phase 32.4: Prefab Instance Validation**
**Objectif:** Valider qu'un prefab est correctement structuré avant export

**Features à implémenter:**
- [x] Auto-validation on save
  - [x] Check: All nodes have valid component types
  - [x] Check: No duplicate node IDs
  - [x] Check: All connections point to valid nodes
  - [x] Check: No circular dependencies (if applicable)
  - [x] Check: Required components present (e.g., Identity, Position)
  - [x] Check: Parameter types match schema

- [x] Validation UI
  - [x] Show validation results in panel
  - [x] Red error indicators on problematic nodes
  - [x] Warning level (yellow) for optional issues
  - [x] Info level (blue) for suggestions

**Files à créer:**
```
NEW: Source/BlueprintEditor/EntityPrefabEditor/PrefabValidator.h/cpp
NEW: Source/BlueprintEditor/EntityPrefabEditor/ValidationReport.h
```

**Files à modifier:**
```
Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.cpp
  - Validate() -> ValidationReport

Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp
  - m_validationReport member
  - RenderValidationPanel()
```

---

#### **Phase 32.5: Export & Runtime Integration**
**Objectif:** Exporter les prefabs au format runtime et les utiliser dans le moteur

**Features à implémenter:**
- [x] Export to binary format (optimized for runtime)
  - [x] Node data → binary
  - [x] Connection data → binary
  - [x] Metadata → binary
  - [x] Create .prefab file format

- [x] Runtime loader
  - [x] Load .prefab files in game
  - [x] Instantiate entities from loaded prefabs
  - [x] Property override system for instances

- [x] Integration with PrefabFactory
  - [x] PrefabFactory::CreateFromFile(filepath)
  - [x] Support both JSON and binary formats

**Files à créer:**
```
NEW: Source/PrefabFormat.h (binary format specification)
NEW: Source/PrefabExporter.h/cpp
NEW: Source/PrefabRuntimeLoader.h/cpp
```

**Files à modifier:**
```
Source/PrefabFactory.cpp
  - AddCreateFromFile() method
  - AddLoadFromBinary() method
```

**Binary Format Example:**
```
[Header: 4 bytes "PFAB"]
[Version: 2 bytes, e.g., 0x0100]
[NodeCount: 4 bytes]
[ConnectionCount: 4 bytes]
[For each node:]
  [NodeID: 4 bytes]
  [ComponentType length: 2 bytes]
  [ComponentType: variable]
  [ParametersCount: 2 bytes]
  [For each parameter:]
    [Key length: 2 bytes]
    [Key: variable]
    [Value serialized]
```

---

## 🎯 PHASE 33: BLUEPRINT EDITOR - ADVANCED FEATURES

**Status:** 📋 À faire  
**Priority:** 🟡 MOYENNE  
**Estimated Duration:** 1-2 jours

### Sous-phases 33.x

#### **Phase 33.1: Blueprint Breakpoints & Debugging**
**Objectif:** Aider le debug des blueprints visuels

**Features à implémenter:**
- [x] Set breakpoint on nodes
  - [x] Right-click node → "Set Breakpoint"
  - [x] Visual indicator (red dot)
  - [x] Pause execution when hit

- [x] Debugger view
  - [x] Show current execution path (highlighted nodes)
  - [x] Display variable values on hover
  - [x] Step through execution (Step In, Step Over, Step Out)
  - [x] Continue/Pause buttons

**Files à créer:**
```
NEW: Source/BlueprintEditor/VisualScriptDebugger.h/cpp
NEW: Source/BlueprintEditor/BreakpointManager.h/cpp
```

---

#### **Phase 33.2: Node Search & Find-Replace**
**Objectif:** Naviguer et modifier rapidement dans les grands blueprints

**Features à implémenter:**
- [x] Ctrl+F: Find node by name/type
  - [x] Show all matches in list
  - [x] Highlight matches on canvas
  - [x] Arrow keys to navigate matches
  - [x] Esc to close search

- [x] Ctrl+H: Find-Replace
  - [x] Replace node properties
  - [x] Replace node type (if compatible)
  - [x] Preview before replace

---

#### **Phase 33.3: Blueprint Variables (Blackboard Global)**
**Objectif:** Variables persistantes accessibles par tous les nodes

**Features à implémenter:**
- [x] Create global variables in blueprint
  - [x] UI: New variable button
  - [x] Name, Type, Default Value
  - [x] Visibility: Public/Private

- [x] Use variables in nodes
  - [x] Variable reference node type
  - [x] Get/Set variable operations
  - [x] Automatic type validation

---

## 🎯 PHASE 34: BEHAVIOR TREE EDITOR

**Status:** 📋 À faire  
**Priority:** 🟡 MOYENNE  
**Estimated Duration:** 3-4 jours

### Objectif Global
Créer un éditeur visuel complet pour les Behavior Trees (AI décisionnel)

### Architecture

```
BehaviorTreeEditor (IGraphRenderer adapter)
  ├── BehaviorTreeCanvas (ImGui container)
  │   ├── BT Node rendering (Task, Selector, Sequence, Decorator)
  │   ├── Connection rendering
  │   └── Input handling
  │
├── BehaviorTreeNodeRenderer
  │   ├── RenderTaskNode()
  │   ├── RenderSelectorNode()
  │   ├── RenderSequenceNode()
  │   └── RenderDecoratorNode()
  │
├── BehaviorTreeDocument (data model)
  │   ├── m_nodes: std::vector<BTNode>
  │   ├── m_connections: std::vector<Connection>
  │   └── Serialization (JSON + runtime)
  │
└── BehaviorTreeNodePalette
      ├── Task types (Move, Attack, Idle, etc.)
      ├── Composite types (Selector, Sequence, Parallel)
      └── Decorator types (Inverter, Repeater, etc.)
```

### Sous-phases 34.x

#### **Phase 34.1: BT Editor Basic Rendering**
- BehaviorTree node types (Task, Selector, Sequence, etc.)
- Node rendering with different shapes
- Connection rendering (tree structure)
- Load/Save BT JSON

#### **Phase 34.2: BT Editor Interactive Features**
- Create/Delete nodes
- Drag/Pan/Zoom
- Connection creation
- Property editing

#### **Phase 34.3: BT Node Library**
- Standard task library (Move, Attack, Wait, etc.)
- Composite nodes (and/or logic)
- Decorator nodes (inverter, repeater, timeout)
- Custom node creation

#### **Phase 34.4: BT Runtime Integration**
- Execute BT from editor for testing
- Visual feedback of execution flow
- Debug mode with breakpoints
- Export to runtime format

---

## 🎯 PHASE 35: LEVEL EDITOR ENHANCEMENTS

**Status:** 📋 À faire  
**Priority:** 🟡 MOYENNE  
**Estimated Duration:** 2-3 jours

### Objectif
Améliorer et compléter l'éditeur de niveaux

### Features

#### **Phase 35.1: Prefab Instance System**
- [x] Place prefab instances in level
- [x] Modify instance properties
- [x] Link instances (parent-child relationships)
- [x] Group instances (layer management)

#### **Phase 35.2: Level Validation & Export**
- [x] Validate level before export (all entities valid)
- [x] Export to binary format (optimized for runtime)
- [x] Runtime level loader

#### **Phase 35.3: Level Layer System**
- [x] Multiple render layers per level
- [x] Layer visibility toggle (editor UI)
- [x] Layer property editor (parallax, tint, etc.)
- [x] Layer export/import

---

## 🎯 PHASE 36: PARTICLE SYSTEM EDITOR

**Status:** 📋 À faire  
**Priority:** 🟢 BASSE  
**Estimated Duration:** 2-3 jours

### Objectif
Éditeur visuel pour les systèmes de particules

### Features
- Particle emitter configuration UI
- Real-time preview of particle effects
- Save/Load particle presets
- Integration with EntityPrefab (drag-drop FX nodes)

---

## 🎯 PHASE 37: ANIMATION EDITOR

**Status:** 📋 À faire  
**Priority:** 🟢 BASSE  
**Estimated Duration:** 3-4 jours

### Objectif
Éditeur pour les animations (sprite sheets + animation graphs)

### Features
- Timeline-based animation editor
- Keyframe management
- Animation preview
- Sprite sheet slicing tool
- Animation graph (state machine for complex animations)

---

## 🎯 PHASE 38: SHADER EDITOR

**Status:** 📋 À faire  
**Priority:** 🟢 BASSE  
**Estimated Duration:** 2-3 jours

### Objectif
Éditeur visuel pour les shaders

### Features
- Node-based shader graph (similar to Unity Shader Graph)
- Real-time shader preview
- Export to GLSL/HLSL
- Integration with material system

---

## 🎯 PHASE 39: LOCALIZATION SYSTEM

**Status:** 📋 À faire  
**Priority:** 🟡 MOYENNE  
**Estimated Duration:** 1-2 jours

### Objectif
Support multilingue complet

### Features
- UI for managing translations
- String key management
- Integration with in-game UI rendering
- Export/Import from external tools (CSV, etc.)

---

## 🎯 PHASE 40: PERFORMANCE PROFILING & OPTIMIZATION

**Status:** 📋 À faire  
**Priority:** 🟡 MOYENNE  
**Estimated Duration:** 2-3 jours

### Objectif
Outils pour analyser et optimiser les performances

### Features
- Frame profiler (CPU/GPU timing)
- Memory profiler (allocation tracking)
- Entity performance analysis
- System profiling (update time per system)
- Optimization recommendations

---

## 📋 ROADMAP VISUEL - TIMELINE

```
Q1 2025:
├── Phase 32.1-32.5 (EntityPrefab Optimization)
└── Phase 33.1-33.3 (VisualScript Advanced)

Q2 2025:
├── Phase 34 (BehaviorTree Editor)
└── Phase 35 (Level Editor Enhancements)

Q3 2025:
├── Phase 36 (Particle System Editor)
├── Phase 37 (Animation Editor)
└── Phase 38 (Shader Editor)

Q4 2025:
├── Phase 39 (Localization)
└── Phase 40 (Performance Tools)
```

---

## 🎯 PRIORITES RECOMMANDEES (Court Terme)

### 🔴 URGENTES (À faire immédiatement)
1. **Phase 32.1**: Copy/Paste (fondamental pour UX)
2. **Phase 32.2**: Undo/Redo (fondamental pour UX)
3. **Phase 32.5**: Export & Runtime (connecte editor ↔ game)

### 🟡 IMPORTANTES (Semaines 2-3)
1. **Phase 32.4**: Validation (assure qualité des prefabs)
2. **Phase 32.3**: Organization (améliore usabilité)
3. **Phase 33.1**: Debugging (aide au développement)

### 🟢 SECUNDAIRES (Mois 2+)
1. Phase 34: BehaviorTree Editor
2. Phase 35: Level Editor
3. Phase 36-40: Editors spécialisés

---

## 📊 MÉTRIQUES DE COMPLEXITÉ

| Phase | Complexité | Fichiers | Estimation |
|-------|-----------|----------|------------|
| 32.1 | 🟡 Moyen | 3-4 | 1-2 jours |
| 32.2 | 🔴 Élevé | 10+ | 2-3 jours |
| 32.3 | 🟡 Moyen | 4-5 | 1-2 jours |
| 32.4 | 🟡 Moyen | 3-4 | 1 jour |
| 32.5 | 🔴 Élevé | 5-6 | 2 jours |
| 33.x | 🟡 Moyen | 4-5 par phase | 1 jour chacun |
| 34.x | 🔴 Élevé | 8-10 | 3-4 jours |
| 35.x | 🟡 Moyen | 5-6 par phase | 1-2 jours |
| 36-40 | 🟡-🔴 Variable | 5-10 | 2-4 jours chacun |

---

## ✅ CHECKLIST AVANT DE COMMENCER

- [ ] Tous les tests de Phase 1-31 passent
- [ ] Build sans erreurs/warnings
- [ ] Documentation à jour
- [ ] Git commits réguliers
- [ ] Backup de la dernière version stable
- [ ] Revue de code des grandes phases

---

**Last Updated:** 2025  
**Next Review:** Après Phase 32 completion

