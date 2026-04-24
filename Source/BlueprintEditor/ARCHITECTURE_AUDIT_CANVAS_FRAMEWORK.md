# État des Lieux: Framework Canvas/Renderer - Architecture Audit
**Date**: 2026-04-16 (Phase 52+)  
**Objet**: Préparation pour implémentation du type de graphe "Animation"  
**Niveau**: Audit complet du framework existant

---

## 1. ARCHITECTURE ACTUELLE - Vue Globale

### 1.1 Trois Niveaux d'Abstraction

```
┌─────────────────────────────────────────────────────────────────┐
│ TIER 1: Application Layer (BlueprintEditorGUI, TabManager)      │
│         - Gestion des onglets                                    │
│         - Détection de type de fichier                           │
│         - Rendu des menus globaux                               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 2: Document Abstraction Layer (IGraphDocument)             │
│         - VisualScriptGraphDocument (Adapter)                   │
│         - BehaviorTreeGraphDocument (Adapter)                   │
│         - EntityPrefabGraphDocument (Direct)                    │
│         - [FUTURE] AnimationGraphDocument                       │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 3: Renderer Interface (IGraphRenderer)                     │
│         - VisualScriptEditorPanel                               │
│         - BehaviorTreeRenderer                                  │
│         - EntityPrefabRenderer                                  │
│         - [FUTURE] AnimationGraphRenderer                       │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 4: Canvas Abstraction (ICanvasEditor)                      │
│         - ImNodesCanvasEditor (imnodes-based)                   │
│         - CustomCanvasEditor (custom pan/zoom)                  │
│         - [FUTURE] TimelineCanvasEditor (animation timeline)   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│ TIER 5: Rendering Utilities                                     │
│         - CanvasGridRenderer (grid visual)                      │
│         - CanvasMinimapRenderer (minimap)                       │
│         - [FUTURE] AnimationTimelineRenderer                    │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 Systèmes de Graphes Actuels (3 implémentations)

| Type | Status | Document Class | Renderer | Canvas | Stockage | Notes |
|------|--------|-----------------|----------|--------|----------|-------|
| **VisualScript** | ✅ Production | VisualScriptGraphDocument | VisualScriptEditorPanel | ImNodesCanvasEditor | imnodes + JSON | Nodes, Links, Presets |
| **BehaviorTree** | ✅ Production | BehaviorTreeGraphDocument | BehaviorTreeRenderer | ImNodesCanvasEditor | NodeGraph format | Nodes, Links, AI logic |
| **EntityPrefab** | ✅ Production | EntityPrefabGraphDocument | EntityPrefabRenderer | CustomCanvasEditor | JSON (v4 schema) | Components, Connections, Properties |
| **Animation** | ❌ **À implémenter** | AnimationGraphDocument | AnimationGraphRenderer | TimelineCanvasEditor | JSON (v1 schema) | Keyframes, Tracks, Timeline |

---

## 2. INTERFACE IGraphDocument - Contrat Unifié

### 2.1 Méthodes Requises (8 essentielles)

```cpp
class IGraphDocument
{
    // Lifecycle
    virtual bool Load(const std::string& filePath) = 0;
    virtual bool Save(const std::string& filePath) = 0;
    virtual bool IsDirty() const = 0;

    // Metadata
    virtual std::string GetName() const = 0;
    virtual DocumentType GetType() const = 0;
    virtual std::string GetFilePath() const = 0;
    virtual void SetFilePath(const std::string& path) = 0;

    // Renderer Access
    virtual IGraphRenderer* GetRenderer() = 0;
    virtual const IGraphRenderer* GetRenderer() const = 0;

    // Notifications
    virtual void OnDocumentModified() = 0;

    // Canvas State (Phase 35: Multi-Tab Support)
    virtual void SaveCanvasState() {}
    virtual void RestoreCanvasState() {}
};
```

### 2.2 Implémentation Pattern: ADAPTER (VisualScript, BehaviorTree)

**Pour Animation Graph**: Utiliser pattern **DIRECT** (comme EntityPrefab) car:
- Données spécifiques (keyframes, tracks) ne mappent pas à système existant
- Plus simple de créer nouvelle classe que adapter (moins de bridging)
- Meilleur control sur serialization pour format animation

---

## 3. INTERFACE IGraphRenderer - Contrat de Rendu

### 3.1 Méthodes Requises (6 essentielles)

```cpp
class IGraphRenderer
{
    // Rendering
    virtual void Render() = 0;

    // File I/O
    virtual bool Load(const std::string& path) = 0;
    virtual bool Save(const std::string& path) = 0;

    // State
    virtual bool IsDirty() const = 0;
    virtual std::string GetGraphType() const = 0;
    virtual std::string GetCurrentPath() const = 0;

    // Canvas State Management (Phase 35)
    virtual void SaveCanvasState() {}
    virtual void RestoreCanvasState() {}

    // Framework Modals (Phase 43-45)
    virtual void RenderFrameworkModals() {}
};
```

### 3.2 Implémentations Actuelles

| Renderer | Canvas | Pattern | Key Files |
|----------|--------|---------|-----------|
| VisualScriptEditorPanel | ImNodesCanvasEditor | imnodes native | VisualScriptEditorPanel.h/cpp |
| BehaviorTreeRenderer | ImNodesCanvasEditor | imnodes + adapter | BehaviorTreeRenderer.h/cpp |
| EntityPrefabRenderer | CustomCanvasEditor | Custom ImGui | EntityPrefabRenderer.h/cpp |
| **AnimationGraphRenderer** | **TimelineCanvasEditor** | **NEW: Timeline-based** | (à créer) |

---

## 4. SYSTÈME CANVAS: ICanvasEditor

### 4.1 Deux Implémentations Existantes

#### 4.1.1 ImNodesCanvasEditor (VisualScript, BehaviorTree)

**Caractéristiques:**
- Wraps `ImNodes::BeginNodeEditor()` / `EndNodeEditor()`
- Pan/zoom: Gérés nativement par imnodes via `EditorContext`
- Zoom: Fixe à 1.0x (limitation imnodes)
- Pan: Limité à offset simple du contexte
- Grille: Standard `CanvasGridRenderer` (couleur #26262FFF)

**Limitations pour Animation:**
- Pas adapté aux timelines (architecture node-based)
- Pas flexible pour interactions timeline spécifiques
- Zoom fixed 1.0x inadapté aux keyframes très rapprochées/éloignées

#### 4.1.2 CustomCanvasEditor (EntityPrefab)

**Caractéristiques:**
- Rendu ImGui natif (Dummy items, DrawList)
- Pan: `m_canvasOffset` (ImVec2, persistent)
- Zoom: `m_canvasZoom` (float, 0.1x - 3.0x)
- Grille: Utilise `CanvasGridRenderer` scalée avec zoom
- Minimap: `CanvasMinimapRenderer` intégré

**Avantages pour Animation:**
- Zoom illimité: adapté pour keyframes zooming
- Pan customisable: support scroll horizontal/vertical
- Architecture extensible: ajout d'éléments UI facile

**Pattern recommandé pour Animation:**
→ **Utiliser CustomCanvasEditor comme base, créer TimelineCanvasEditor spécialisé**

### 4.2 Contrat ICanvasEditor

```cpp
class ICanvasEditor
{
    // Lifecycle (chaque frame)
    virtual void BeginRender() = 0;
    virtual void EndRender() = 0;

    // Pan/Zoom
    virtual ImVec2 GetCanvasOffset() const = 0;
    virtual void SetCanvasOffset(const ImVec2& offset) = 0;
    virtual float GetCanvasZoom() const = 0;
    virtual void SetCanvasZoom(float zoom) = 0;

    // Coordinate Systems
    virtual ImVec2 ScreenToCanvas(const ImVec2& screenPos) const = 0;
    virtual ImVec2 CanvasToScreen(const ImVec2& canvasPos) const = 0;

    // Grid
    virtual void SetGridStyle(const CanvasGridRenderer::Style& style) = 0;
    virtual CanvasGridRenderer::Style GetGridStyle() const = 0;

    // Selection / Interaction
    virtual int GetSelectedNode() const = 0;
    virtual std::vector<int> GetSelectedNodes() const = 0;
    virtual bool IsNodeHovered() const = 0;

    // Context
    virtual ImGuiContext* GetImGuiContext() const = 0;
};
```

---

## 5. PATTERNS DE COORDONNÉES - 4-Space System

Système cohérent à travers tous les canvas types:

```
Screen Space
    │
    │ (absolute window pixels)
    │ Transformation: subtract ImGui::GetCursorScreenPos()
    ▼
Canvas Space
    │
    │ (relative to canvas top-left)
    │ Transformation: apply pan offset
    ▼
Editor Space
    │
    │ (pan-adjusted, zoom not applied)
    │ Transformation: divide by zoom
    ▼
Grid Space
    │
    │ (logical grid units, pan/zoom independent)
    │ Used for: save/load coordinates
    │
```

**Formules de transformation (vérifiées Phase 29):**
```cpp
ImVec2 ScreenToCanvas(const ImVec2& screen) {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    return (screen - canvasPos - m_canvasOffset) / m_canvasZoom;
}

ImVec2 CanvasToScreen(const ImVec2& canvas) {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    return (canvas * m_canvasZoom) + m_canvasOffset + canvasPos;
}
```

**ERREURS À ÉVITER (basées sur Phases 28-30):**
- ❌ `offset * zoom` au lieu de `offset / zoom` (Phase 29)
- ❌ Oublier `canvasPos` lors de transformation (Phase 29)
- ❌ Ne pas appliquer zoom à ALL rendering (minimap, grid)

---

## 6. SYSTÈME MINIMAP - CanvasMinimapRenderer (Phase 37)

### 6.1 Architecture Centralisée

```cpp
class CanvasMinimapRenderer
{
    // Normalisation des coordonnées [0..1]
    void UpdateNodes(const std::vector<MinimapNodeData>& nodes);
    void UpdateViewport(const MinimapViewportData& viewport);

    // Deux chemins de rendu
    void RenderImNodes();      // Native ImNodes::MiniMap()
    void RenderCustom();       // ImGui DrawList overlay

    // Configuration
    void SetPosition(MinimapPosition pos);
    void SetSize(float ratio);  // 0.05 - 0.5 du canvas
    void SetColors(...);
};
```

### 6.2 Intégration pour Animation

**Recommandation:**
- Créer `AnimationTimelineRenderer` qui utilise CanvasMinimapRenderer pour "timeline minimap"
- Afficher portion de timeline visible vs total timeline scope
- Utile pour édition de longues animations (montage cuts, etc.)

---

## 7. SYSTÈME DE FICHIERS - Détection de Type

### 7.1 Fonction de Détection (TabManager::DetectGraphType)

```cpp
GraphType DetectGraphType(const std::string& filePath)
{
    // .ats ou .json -> Charge et inspecte contenu
    // VisualScript: champ "taskType" dans root ou nodes
    // BehaviorTree: champ "graphType": "BehaviorTree"
    // EntityPrefab: champ "nodeType": "Component"
    
    // Animation: nouveau critère
    // Animation: champ "timelineData" ou "tracks"
}
```

### 7.2 Mappings Fichiers Existants

| Type | Extensions | Dossier | Exemple |
|------|-----------|---------|---------|
| VisualScript | .ats, .json | `Gamedata/Script/` | `myScript.ats` |
| BehaviorTree | .json | `Gamedata/BehaviorTree/` | `patrol_bt.json` |
| EntityPrefab | .json | `Gamedata/EntityPrefab/` | `guard.json` |
| **Animation** | **.anim.json** | **`Gamedata/Animation/`** | **`run_cycle.anim.json`** |

**Recommandation:** Format `.anim.json` pour éviter conflits avec VisualScript `.json`

---

## 8. DOCUMENT ABSTRACTION LAYER - IGraphDocument Flows

### 8.1 Lifecycle Complet

```
User double-click file
    │
    ▼
TabManager::OpenFileInTab()
    │
    ├─ DetectGraphType(filepath)  // Détermine type
    │
    ├─ Créer IGraphDocument approprié
    │  ├─ VisualScriptGraphDocument -> VisualScriptEditorPanel
    │  ├─ BehaviorTreeGraphDocument -> BehaviorTreeRenderer
    │  ├─ EntityPrefabGraphDocument (direct)
    │  └─ [NEW] AnimationGraphDocument -> AnimationGraphRenderer
    │
    ├─ document->Load(filepath)
    │  ├─ Charge données fichier
    │  ├─ Crée renderer associé
    │  └─ Retourne true/false
    │
    ├─ Créer EditorTab
    │  ├─ Stocke document pointer
    │  ├─ Stocke renderer pointer
    │  └─ Fait render() chaque frame
    │
    └─ Retourner tabID -> affiche tab
```

### 8.2 Dirty Flag Tracking

Tous les IGraphDocument doivent:
- Initialiser `m_isDirty = false` après Load()
- Setter `m_isDirty = true` sur modification données
- Setter `m_isDirty = false` après Save()
- Implémenter `IsDirty()` const

---

## 9. PATTERNS & ANTI-PATTERNS IDENTIFIÉS

### 9.1 ✅ BONNES PRATIQUES (à respecter pour Animation)

1. **Initialization Completeness** (Phase 52)
   - Tous les objets utilisés dans Render() doivent être initialisés dans Initialize()
   - ❌ Don't: Utiliser `if (ptr)` guards comme workaround
   - ✅ Do: Créer objet explicitement

2. **Move Semantics Safety** (Phase 51)
   - Ne JAMAIS accéder à objet après `std::move()`
   - ❌ Don't: `m_tabs.emplace_back(std::move(tab)); return tab.tabID;`
   - ✅ Do: Sauvegarder avant move, retourner sauvegarde

3. **Logging Discipline** (Phases 44.4-47)
   - ❌ JAMAIS loger dans render loops (60 FPS spam)
   - ✅ ONLY: Logs sur state changes et user actions
   - Exemple: SaveClicked() → Log ONCE, RenderButtons() → NO LOGS

4. **Frame Cycle Timing** (Phase 45)
   - Modals MUST render APRÈS content (pas avant)
   - ❌ Don't: `RenderContent(); RenderModals();` dans même block
   - ✅ Do: Content in RenderTabBar(), Modals after RenderActiveCanvas()

5. **Coordinate Transformation** (Phases 28-30)
   - Toujours utiliser formules prouvées: `(screen - pos - offset) / zoom`
   - ❌ Don't: Inventer variations (pan_offset * zoom)
   - ✅ Do: Documenter transformation chain dans classe

### 9.2 ❌ ANTI-PATTERNS (à éviter pour Animation)

1. Null pointer guards au lieu d'initialization (Phase 52 bug)
2. Accessing moved objects (Phase 51 bug)
3. Frame-by-frame logging (Phase 44.4 spam)
4. Incorrect ImGui frame timing (Phase 45 modal issue)
5. Over-complicated coordinate math (Phase 29 transformation bug)
6. Incomplete adapter implementations (need all IGraphDocument methods)

---

## 10. ÉTAT DES CLÉS CLASSES/FICHIERS

### 10.1 Core Interfaces (Existantes)

| Fichier | Classe | Rôle | Status |
|---------|--------|------|--------|
| `IGraphDocument.h` | IGraphDocument | Abstraction document | ✅ Complet |
| `IGraphRenderer.h` | IGraphRenderer | Abstraction renderer | ✅ Complet |
| `ICanvasEditor.h` | ICanvasEditor | Abstraction canvas | ✅ Complet |

### 10.2 Implementations Existantes

| Fichier | Classe | Type | Status |
|---------|--------|------|--------|
| `VisualScriptGraphDocument.h/cpp` | VisualScriptGraphDocument | Adapter | ✅ Production |
| `BehaviorTreeGraphDocument.h/cpp` | BehaviorTreeGraphDocument | Adapter | ✅ Production |
| `EntityPrefabGraphDocument.h/cpp` | EntityPrefabGraphDocument | Direct | ✅ Production |
| `VisualScriptEditorPanel.h/cpp` | VisualScriptEditorPanel | Renderer | ✅ Production |
| `BehaviorTreeRenderer.h/cpp` | BehaviorTreeRenderer | Renderer | ✅ Production |
| `EntityPrefabRenderer.h/cpp` | EntityPrefabRenderer | Renderer | ✅ Production |
| `ImNodesCanvasEditor.h/cpp` | ImNodesCanvasEditor | Canvas | ✅ Production |
| `CustomCanvasEditor.h/cpp` | CustomCanvasEditor | Canvas | ✅ Production |
| `CanvasGridRenderer.h/cpp` | CanvasGridRenderer | Utility | ✅ Complete |
| `CanvasMinimapRenderer.h/cpp` | CanvasMinimapRenderer | Utility | ✅ Complete |

### 10.3 Implementations À Créer (Animation)

| Fichier | Classe | Type | Basé Sur |
|---------|--------|------|----------|
| `AnimationGraphDocument.h/cpp` | AnimationGraphDocument | Direct | EntityPrefabGraphDocument |
| `AnimationGraphRenderer.h/cpp` | AnimationGraphRenderer | Renderer | EntityPrefabRenderer |
| `TimelineCanvasEditor.h/cpp` | TimelineCanvasEditor | Canvas | CustomCanvasEditor |
| `KeyframeTrackRenderer.h/cpp` | KeyframeTrackRenderer | Utility | ComponentNodeRenderer |

---

## 11. GAPS IDENTIFIÉS À COMBLER POUR ANIMATION

### 11.1 Canvas Interactions Manquantes

| Feature | VisualScript | BehaviorTree | EntityPrefab | Animation | Priorité |
|---------|--------------|--------------|--------------|-----------|----------|
| Pan | ✅ ImNodes | ✅ ImNodes | ✅ Custom | ❌ NEEDED | HIGH |
| Zoom | ❌ Fixed 1.0x | ❌ Fixed 1.0x | ✅ 0.1-3.0x | ⚠️ PARTIAL | HIGH |
| Grid | ✅ Visual | ✅ Visual | ✅ Visual | ⚠️ Time-based | MEDIUM |
| Minimap | ✅ ImNodes | ✅ ImNodes | ✅ Custom | ❌ NEEDED | MEDIUM |
| Playback | ❌ N/A | ⚠️ Partial | ❌ N/A | ⚠️ CORE | HIGH |
| Keyframe Snap | ❌ N/A | ❌ N/A | ❌ N/A | ⚠️ CORE | HIGH |

### 11.2 Document Features Manquantes

| Feature | VisualScript | BehaviorTree | EntityPrefab | Animation | Priorité |
|---------|--------------|--------------|--------------|-----------|----------|
| Undo/Redo | ❌ | ❌ | ❌ | ⚠️ NEEDED | MEDIUM |
| Copy/Paste | ❌ | ❌ | ❌ | ⚠️ NEEDED | MEDIUM |
| Search | ❌ | ❌ | ⚠️ Partial | ⚠️ CORE | MEDIUM |
| Preview | ❌ | ⚠️ Native BT | ❌ | ✅ CORE | HIGH |
| Bookmarks | ❌ | ❌ | ❌ | ⚠️ CORE | LOW |

---

## 12. PLAN D'IMPLÉMENTATION POUR ANIMATION GRAPH

### 12.1 Phase 1: Fondations Document

**Fichiers à créer:**
```
Source/BlueprintEditor/AnimationEditor/
├── AnimationGraphDocument.h/cpp         (Direct IGraphDocument impl)
├── AnimationTimelineData.h/cpp          (Data model: tracks, keyframes)
├── AnimationLoader.h/cpp                (JSON deserialization)
└── AnimationSerializer.h/cpp            (JSON serialization)
```

**Schema JSON proposé:**
```json
{
  "version": 1,
  "animationType": "Animation",
  "name": "run_cycle",
  "duration": 1.0,
  "frameRate": 30,
  "tracks": [
    {
      "trackId": 1,
      "name": "Position.X",
      "type": "Float",
      "keyframes": [
        { "time": 0.0, "value": 0.0 },
        { "time": 0.5, "value": 50.0 },
        { "time": 1.0, "value": 0.0 }
      ]
    }
  ]
}
```

### 12.2 Phase 2: Renderer & Canvas

**Fichiers à créer:**
```
Source/BlueprintEditor/AnimationEditor/
├── AnimationGraphRenderer.h/cpp         (IGraphRenderer impl)
├── TimelineCanvasEditor.h/cpp           (ICanvasEditor impl)
├── KeyframeTrackRenderer.h/cpp          (Track/keyframe rendering)
├── TimelinePlaybackController.h/cpp     (Playback state)
└── TimelineGUIComponents.h/cpp          (Timeline UI widgets)
```

**Architecture renderer:**
```cpp
class AnimationGraphRenderer : public IGraphRenderer {
    std::unique_ptr<AnimationGraphDocument> m_document;
    std::unique_ptr<TimelineCanvasEditor> m_canvas;
    std::unique_ptr<KeyframeTrackRenderer> m_trackRenderer;
    std::unique_ptr<TimelinePlaybackController> m_playback;
};
```

### 12.3 Phase 3: Integration & UI

**Modifications existantes:**
```
Source/BlueprintEditor/
├── TabManager.cpp               (+AnimationGraphDocument case)
├── BlueprintEditorGUI.cpp       (+Animation menu entry)
├── Utilities/ICanvasEditor.h    (+TimelineCanvasEditor case)
└── Framework/IGraphDocument.h   (+ANIMATION enum value)
```

### 12.4 Phase 4: Features Avancées

- Timeline scrubber and playback
- Keyframe selection and manipulation
- Track management UI
- Curve interpolation editor
- Animation preview panel

---

## 13. CONSIDÉRATIONS D'IMPLÉMENTATION

### 13.1 Choix Architecturaux Clés

| Décision | Options | Recommandation | Raison |
|----------|---------|-----------------|--------|
| Document impl | Adapter vs Direct | **Direct** | Données timeline ≠ node graph |
| Canvas impl | ImNodes vs Custom | **Custom-based** | Zoom illimité requis |
| Storage format | JSON vs Binary | **JSON** | Consistency avec autres types |
| Playback | Standalone vs Framework | **Standalone** | Indépendant du TAB render |

### 13.2 Intégration avec Existing Framework

**Points de modification:**
1. `TabManager::DetectGraphType()` - Ajouter cas Animation
2. `TabManager::OpenFileInTab()` - Créer AnimationGraphDocument
3. `BlueprintEditorGUI::RenderFileMenu()` - Ajouter "New Animation"
4. `IGraphDocument` - Ajouter case `DocumentType::ANIMATION`

**Points de création:**
1. 7 nouveaux fichiers dans `AnimationEditor/`
2. Dérivé de CustomCanvasEditor via `TimelineCanvasEditor`
3. 1000-1500 LOC total (estimation)

### 13.3 Testing Strategy

```cpp
// Unit tests
- AnimationTimelineData serialization/deserialization
- TimelineCanvasEditor coordinate transforms
- KeyframeTrackRenderer positioning calculations

// Integration tests
- Load .anim.json file through TabManager
- Save modifications and verify file
- Switch between tabs and restore canvas state
- Playback scrubbing

// E2E tests
- Create new animation from File menu
- Add tracks and keyframes
- Save/load round-trip
- Verify dirty flag tracking
```

---

## 14. CHECKLIST PRÉ-IMPLÉMENTATION

### 14.1 Documentation Requise

- [ ] Finaliser Animation JSON schema v1
- [ ] Définir data model complet (AnimationTimelineData)
- [ ] Documenter coordinate system pour timeline
- [ ] Spécifier playback behavior
- [ ] Lister tous les shortcuts clavier

### 14.2 Framework Preparation

- [ ] Vérifier tous patterns (Phases 44.4-52) applicables
- [ ] Créer base classes avec tests
- [ ] Valider coordinate transforms
- [ ] Tester TabManager integration points

### 14.3 Code Review Milestones

- [ ] Phase 1: Document + Loader (peer review)
- [ ] Phase 2: Renderer + Canvas (UI review)
- [ ] Phase 3: Integration (framework integration review)
- [ ] Phase 4: Features (feature completeness review)

---

## 15. MAPPINGS PATTERNS PAR TYPE

### 15.1 VisualScript (Pattern: ADAPTER)

```
File (.ats) 
  ↓ Load
TaskGraphTemplate (existing)
  ↓ VisualScriptGraphDocument (adapter)
IGraphDocument interface
  ↓ GetRenderer
VisualScriptEditorPanel (existing)
  ↓ Contains
ImNodesCanvasEditor
  ↓ Renders
imnodes graph view
```

### 15.2 BehaviorTree (Pattern: ADAPTER)

```
File (.json)
  ↓ Load (via NodeGraphManager)
GraphDocument (existing, NodeGraph namespace)
  ↓ BehaviorTreeGraphDocument (adapter)
IGraphDocument interface
  ↓ GetRenderer
BehaviorTreeRenderer (existing)
  ↓ Contains
ImNodesCanvasEditor
  ↓ Renders
imnodes graph view
```

### 15.3 EntityPrefab (Pattern: DIRECT)

```
File (.json)
  ↓ Load
EntityPrefabGraphDocument (direct impl)
  ├─ Contains EntityPrefabGraphDocument data
  ├─ Implements IGraphDocument
  ↓ GetRenderer
EntityPrefabRenderer (new)
  ↓ Contains
CustomCanvasEditor
  ↓ Renders
custom ImGui canvas
```

### 15.4 Animation (Pattern: DIRECT - RECOMMENDED)

```
File (.anim.json)
  ↓ Load
AnimationGraphDocument (direct impl)
  ├─ Contains AnimationTimelineData
  ├─ Implements IGraphDocument
  ↓ GetRenderer
AnimationGraphRenderer (new)
  ↓ Contains
TimelineCanvasEditor (derived from CustomCanvasEditor)
  ↓ Renders
timeline view with tracks/keyframes
```

---

## 16. RISQUES & MITIGATIONS

| Risque | Impact | Mitigation |
|--------|--------|-----------|
| Coordinate transform bugs | HIGH | Test transforms exhaustively Phase 1 |
| ImGui context issues | MEDIUM | Follow Phase 45 modal timing |
| Move semantics crashes | MEDIUM | Code review for Phase 51 patterns |
| Frame-by-frame logging spam | MEDIUM | Implement logging discipline |
| Canvas lag (large animations) | MEDIUM | Implement culling early |
| Incompatible JSON format | HIGH | Validate schema v1 upfront |

---

## 17. SUMMARY TABLE - Framework Coverage

```
┌──────────────────┬──────────────┬──────────────┬──────────────┬──────────────┐
│ System           │ VisualScript │ BehaviorTree │ EntityPrefab │ Animation*   │
├──────────────────┼──────────────┼──────────────┼──────────────┼──────────────┤
│ IGraphDocument   │ ✅ Complete  │ ✅ Complete  │ ✅ Complete  │ ⏳ Planned   │
│ IGraphRenderer   │ ✅ Complete  │ ✅ Complete  │ ✅ Complete  │ ⏳ Planned   │
│ ICanvasEditor    │ ✅ ImNodes   │ ✅ ImNodes   │ ✅ Custom    │ ⏳ Timeline  │
│ Grid Rendering   │ ✅ Standard  │ ✅ Standard  │ ✅ Standard  │ ⏳ Time-grid │
│ Minimap          │ ✅ ImNodes   │ ✅ ImNodes   │ ✅ Custom    │ ⏳ Timeline  │
│ Dirty Tracking   │ ✅ Yes       │ ✅ Yes       │ ✅ Yes       │ ⏳ Yes       │
│ Canvas State     │ ✅ Yes       │ ✅ Yes       │ ✅ Yes       │ ⏳ Yes       │
│ Framework Modals │ ✅ Yes       │ ✅ Yes       │ ✅ Yes       │ ⏳ Yes       │
│ Tab Support      │ ✅ Yes       │ ✅ Yes       │ ✅ Yes       │ ⏳ Yes       │
│ Undo/Redo        │ ❌ No        │ ❌ No        │ ❌ No        │ ⏳ Planned   │
│ Copy/Paste       │ ❌ No        │ ❌ No        │ ❌ No        │ ⏳ Planned   │
└──────────────────┴──────────────┴──────────────┴──────────────┴──────────────┘
* = Animation: Phase de planification actuellement
```

---

## 18. NEXT STEPS

### Immédiat (Avant implémentation Animation)

1. **Review ce document** avec équipe
2. **Valider JSON schema** v1 pour Animation
3. **Confirmer patterns** à utiliser (recommandation: Direct impl + TimelineCanvasEditor)
4. **Préparer test harness** pour new graph type

### Pré-implémentation Phase 1

1. Créer `AnimationGraphDocument` skeleton avec todos
2. Créer `AnimationTimelineData` data model
3. Implémenter JSON schema parsing
4. Écrire unit tests de serialization

### Phase 1 Completion Criteria

- [ ] AnimationGraphDocument implémente 100% IGraphDocument
- [ ] Load/Save round-trip JSON fonctionne
- [ ] TabManager::OpenFileInTab() route .anim.json correctement
- [ ] 0 compilation errors, 0 logical bugs

---

**Document Status**: ✅ COMPLETE - Ready for Architecture Review  
**Next Review**: Post-Team-Feedback Phase  
**Estimated LOC Animation**: 1200-1500 (7 files)  
**Estimated Timeline**: 3-4 phases (1-2 weeks)
