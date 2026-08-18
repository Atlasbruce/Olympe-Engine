# New Graph Type Integration Specification

**Scope:** Olympe Blueprint Editor framework  
**Purpose:** Reusable concept + technical template for adding a new graph type and auditing existing graph integrations for framework conformity.

## 1. Goal

This document defines how a graph type must integrate with the current Blueprint Editor stack:

- `TabManager`
- `GraphEditorBase`
- `IGraphRenderer`
- `IGraphDocument`
- serializer / loader / saver
- canvas features
- inspector and output/verification panels
- minimap
- context menus
- multi-selection
- pan/zoom
- modals
- run / verify flows

It also provides a conformity checklist for reviewing an existing integration such as Visual Script, Behavior Tree, Entity Prefab, or a future graph type.

## 2. Core architectural rule

A graph type is framework-conformant only if it is a first-class participant in the editor lifecycle:

1. It opens through `TabManager`.
2. It renders through `IGraphRenderer`.
3. It loads/saves through `IGraphDocument` or an equivalent adapter contract.
4. It participates in common framework UI and document state.
5. It exposes graph-specific behavior only through extension points, not by bypassing the framework.

## 3. Canonical integration model

### 3.1 Required layers

| Layer | Responsibility |
| --- | --- |
| `BlueprintEditorGUI` | Host window, menu entry points, global frame ordering |
| `TabManager` | Open/close/activate tabs, route load/save, manage active renderer/document |
| `GraphEditorBase` | Shared editor behavior: toolbar, selection, shortcuts, context menus, modals, canvas state |
| `IGraphRenderer` | Render/load/save/is-dirty contract for an editor surface |
| `IGraphDocument` | Persistent graph document, metadata, renderer access, dirty tracking |
| Serializer / loader | File format parsing and emission |
| Canvas utilities | Grid, minimap, pan/zoom, selection, modals, common viewport controls |
| Type-specific editor | Node layout, inspector, verification, runtime actions, custom interactions |

### 3.2 Preferred ownership model

- **Document owns graph data.**
- **Renderer owns presentation and input behavior.**
- **Framework owns tab state and document lifecycle routing.**
- **Type-specific panels extend the framework; they do not replace it.**

## 4. Integration contract by module

### 4.1 `TabManager`

`TabManager` is the authoritative tab orchestration layer.

**Required behavior**
- Open a graph file into a tab.
- Detect graph type from file contents or explicit metadata.
- Reuse an already-open tab when reopening the same file.
- Save the active tab and route to the correct document/renderer path.
- Keep active tab, dirty state, and display name synchronized.

**Integration contract**
- New graph types must be discoverable by `DetectGraphType()` or the equivalent type-dispatch path.
- `CreateNewTab()` and `OpenFileInTab()` must create the correct renderer/document pair.
- `SaveActiveTab()` must resolve to the graph’s save implementation without special casing in the GUI layer.
- `OnGraphDocumentSaved()` must update tab metadata after save.

**Conformant signs**
- New graph opens from menu and file path.
- Reopening a file focuses the existing tab.
- Dirty marker updates correctly.
- Save / Save As works from both toolbar and shortcut path.

### 4.2 `GraphEditorBase`

`GraphEditorBase` is the common behavior template for graph editors.

**Required behavior**
- Render pipeline is owned by the base class.
- Subclasses provide type-specific graph content and extension hooks.
- Common features remain centralized:
  - pan/zoom
  - selection
  - rectangle selection
  - context menu
  - keyboard shortcuts
  - save / save-as modals
  - common toolbar
  - canvas state persistence

**Integration contract**
- Override only the designated extension points.
- Do not duplicate common canvas, toolbar, or modal behavior in derived classes.
- Use `RenderGraphContent()` for the graph-specific viewport content.
- Use `InitializeCanvasEditor()` for type-specific canvas setup.

**Conformant signs**
- Derived editor renders through the base pipeline.
- Custom behavior is isolated to overrides.
- Common features work without reimplementation.

### 4.3 `IGraphRenderer`

This interface is the render-side contract for all graph editors.

**Required behavior**
- `Render()`
- `Load(path)`
- `Save(path)`
- `IsDirty()`
- `GetGraphType()`
- `GetCurrentPath()`
- `SaveCanvasState()`
- `RestoreCanvasState()`
- `RenderFrameworkModals()`

**Integration contract**
- The renderer must be safe to call from `TabManager`.
- Load/save must be deterministic and side-effect free beyond the expected document update.
- Dirty state must reflect actual unsaved changes.

**Conformant signs**
- Toolbar actions delegate to renderer/document, not to ad hoc editor code.
- Modals are rendered at the correct frame time.

### 4.4 `IGraphDocument`

This is the persistence and metadata contract.

**Required behavior**
- `Load(filePath)`
- `Save(filePath)`
- `IsDirty()`
- `GetName()`
- `GetType()`
- `GetFilePath()`
- `SetFilePath(path)`
- `GetRenderer()`
- `OnDocumentModified()`

**Integration contract**
- The document is the source of truth for serialized data.
- Renderer access must be stable for the document lifetime.
- Document modification must propagate to dirty state.
- File path and type metadata must stay synchronized with the UI.

**Conformant signs**
- Document can be loaded and saved without losing graph identity.
- Renderer and document stay aligned on type and path.

## 5. Graph-type integration template

### 5.1 Minimum implementation checklist

For a new graph type, implement:

- A document class or adapter implementing `IGraphDocument`
- A renderer implementing `IGraphRenderer`
- A `TabManager` open/create path
- File type detection / dispatch logic
- Load/save serialization
- Dirty-state propagation
- Common canvas features
- Inspector or equivalent property editing
- Output / verification panel if the graph supports execution or validation
- Minimap and context menu integration where applicable

### 5.2 Suggested file pattern

```text
Source/BlueprintEditor/
  Framework/
    <NewGraph>GraphDocument.h/.cpp
    <NewGraph>Renderer.h/.cpp
  <NewGraph>Editor/
    <NewGraph>Canvas.h/.cpp
    <NewGraph>InspectorPanel.h/.cpp
    <NewGraph>VerificationPanel.h/.cpp
```

## 6. Common feature requirements

### 6.1 Canvas behavior

The graph editor must support, when applicable:

- pan
- zoom
- canvas state save / restore
- selection
- multi-selection
- rectangle selection
- drag move
- keyboard shortcuts
- node/link hit testing
- context menus
- minimap
- grid rendering

### 6.2 Inspector / property panel

If the graph has editable node or graph properties:

- show selected object details
- allow edits to write back to the document
- mark document dirty on change
- keep selection and inspector in sync

### 6.3 Output / verification panel

If the graph can be run, compiled, simulated, or verified:

- define a single execution/verification entry point
- report status and diagnostics in a dedicated panel
- keep results tied to the active tab/document
- do not hide failures behind success-shaped UI

### 6.4 Modals

Save / Save As / Load / verification prompts must:

- render at the correct frame timing
- be owned by framework state
- not duplicate modal logic in the graph type

## 7. Evaluation rubric for existing integrations

Use this rubric to audit an existing graph type.

| Area | Pass criteria | Fail signal |
| --- | --- | --- |
| Tab integration | Opens, closes, activates through `TabManager` | Bypasses tab system |
| Renderer contract | Implements `IGraphRenderer` cleanly | Missing load/save/dirty behavior |
| Document contract | Implements `IGraphDocument` or equivalent adapter | Data lives only in UI classes |
| Save / load | Uses framework path and updates metadata | Saves in a separate code path |
| Dirty state | Changes propagate to tab and toolbar | Dirty state is stale or local only |
| Canvas state | Pan/zoom/selection persist correctly | State resets unexpectedly |
| Common toolbar | Uses shared save/modal wiring | Duplicated toolbar logic |
| Context menus | Uses common menu structure or approved extension points | No framework menu coherence |
| Minimap | Uses shared minimap contract | Custom one-off minimap behavior |
| Inspector | Updates selected object and marks dirty | Inspector detached from document |
| Verification | Single entry point with clear status | Results scattered across UI |

## 8. Conformity checklist

### 8.1 Structural checklist

- [ ] Graph type is reachable from the main menu or file open flow.
- [ ] `TabManager` can open and activate the graph.
- [ ] `DetectGraphType()` recognizes the graph format.
- [ ] Renderer implements the full `IGraphRenderer` contract.
- [ ] Document implements the full `IGraphDocument` contract or a documented adapter.
- [ ] Save and load are routed through the framework.
- [ ] Dirty state is synchronized with document edits.

### 8.2 Canvas checklist

- [ ] Pan works.
- [ ] Zoom works and is clamped.
- [ ] Selection is visible and correct.
- [ ] Multi-selection works.
- [ ] Rectangle selection works if applicable.
- [ ] Context menu appears in the correct priority order.
- [ ] Minimap renders and matches the shared style.
- [ ] Canvas state persists across reload or tab switch when required.

### 8.3 UI checklist

- [ ] Toolbar buttons reflect graph state.
- [ ] Modals render after content, not before it.
- [ ] Inspector updates with the active selection.
- [ ] Output / verification panel is tied to the active document.
- [ ] Status text and tab title reflect file path and dirty state.

### 8.4 Persistence checklist

- [ ] Serialize all graph data needed to restore the editor.
- [ ] Deserialize without losing selection-independent graph structure.
- [ ] Save writes the current schema version.
- [ ] Load tolerates older schema versions where required.
- [ ] File path metadata is updated after save-as.

## 9. Integration patterns by graph family

### 9.1 Direct document ownership

Use when the graph type owns its own data model directly.

Example fit:
- Entity Prefab style graphs

Pattern:
- document owns nodes, links, metadata
- renderer reads and writes document state
- framework interacts through document interface only

### 9.2 Adapter wrapper

Use when the graph type already has an internal editor object or legacy panel.

Example fit:
- Visual Script style graphs
- Behavior Tree style graphs

Pattern:
- existing editor state remains internal
- adapter implements `IGraphDocument`
- renderer bridges legacy editor to framework

### 9.3 Hybrid type

Use when the graph needs both framework features and a custom runtime/verification surface.

Pattern:
- document handles persistence
- renderer handles canvas
- additional panel handles run/verify/output
- shared framework controls routing and state

## 10. Adaptation guidelines for non-conformant existing integrations

When evaluating an existing graph type, prefer these fixes in order:

1. **Route through the framework first.**
   - Connect open/save/load to `TabManager` and the document contract.

2. **Extract document state from UI state.**
   - Move persistence out of rendering widgets and into a document model.

3. **Centralize common canvas behavior.**
   - Remove duplicated pan/zoom, selection, and modal handling.

4. **Normalize toolbar and modal flows.**
   - Ensure shared save/save-as/load behavior uses the same timing and callbacks.

5. **Unify verification and output routing.**
   - Give the graph one clear execution entry point.

6. **Only then specialize.**
   - Keep graph-specific controls in overrides, panels, or dedicated hooks.

## 11. Review questions for a framework conformity audit

- Does the graph have a single source of truth for document state?
- Can the graph be opened, saved, and re-opened through `TabManager`?
- Are common features inherited or duplicated?
- Is the inspector tied to selected graph entities?
- Is the output / verification path deterministic and visible?
- Are modals rendered in the correct frame stage?
- Does the graph type use shared minimap, grid, and selection behavior?
- Can the graph be extended without breaking the common framework?

## 12. Recommended acceptance criteria for a new graph type

A new graph type is ready when:

- it opens and saves from the main editor flow,
- dirty state is accurate,
- pan / zoom / selection / context menus work,
- the inspector edits the real document,
- any verification or execution flow is visible and deterministic,
- the tab title and file path are correct,
- the implementation conforms to the framework instead of bypassing it.

## 13. Template section for future graph types

### Graph type name

`<GraphType>`

### File format

`<extension / schema / version>`

### Document implementation

`<document class, adapter strategy, ownership model>`

### Renderer implementation

`<renderer class, canvas strategy, framework hooks>`

### Common feature support

`<pan / zoom / selection / minimap / context menus / modals>`

### Inspector

`<property panel or equivalent>`

### Output / verification

`<run / verify / compile / simulate flow>`

### Conformity risks

`<known deviations from framework conventions>`

### Migration steps

`<ordered list of integration changes>`

## 14. Source references

Relevant existing framework files:

- `Source/BlueprintEditor/Framework/GraphEditorBase.h`
- `Source/BlueprintEditor/Framework/IGraphRenderer.h`
- `Source/BlueprintEditor/Framework/IGraphDocument.h`
- `Source/BlueprintEditor/TabManager.h/.cpp`
- `Source/BlueprintEditor/BlueprintEditorGUI.cpp`
- `Source/BlueprintEditor/Framework/CanvasFramework.h/.cpp`

Relevant editor examples:

- `Source/BlueprintEditor/EntityPrefabEditor/`
- `Source/BlueprintEditor/VisualScriptEditorPanel*.cpp`
- `Source/BlueprintEditor/BehaviorTreeRenderer*.cpp`

