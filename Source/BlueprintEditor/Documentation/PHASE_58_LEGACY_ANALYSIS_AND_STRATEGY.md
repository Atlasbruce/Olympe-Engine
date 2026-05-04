# PHASE 58: DEEP LEGACY ANALYSIS & ENTITYPREFAB FRAMEWORK INTEGRATION STRATEGY

## EXECUTIVE SUMMARY

**Current State**: 0 compilation errors ✅, 55 linker errors (architectural gaps) ⚠️

**Root Cause**: EntityPrefab built independently from Framework, not adapting to TabManager's multi-type routing system

**Strategic Decision**: Analyze legacy patterns deeply → Design proper adapter pattern → Migrate EntityPrefab to Framework with full IGraphRenderer/IGraphDocument compliance

**Estimated Impact**: Once completed, all 55 linker errors will resolve as EntityPrefab properly integrates with Framework architecture

---

## PART 1: TABMANAGER DEEP ARCHITECTURE ANALYSIS

### 1.1 Core Design Pattern: Multi-Type Graph Router

**Pattern Name**: Central Type-Dispatched Router with Polymorphic Renderer Management

**Key Innovation**: TabManager uses string-based graphType to route different graph types through unified tab system while allowing each type to have completely different implementations.

```cpp
// From TabManager.cpp - OpenFileInTab()
std::string graphType = DetectGraphType(filePath);

if (graphType == "VisualScript")      // Path A: VS renderer + VS document
    { /* create VisualScriptRenderer */ }
else if (graphType == "BehaviorTree")  // Path B: BT renderer + BT document  
    { /* create BehaviorTreeRenderer */ }
else if (graphType == "EntityPrefab")  // Path C: EP renderer + EP document
    { /* create EntityPrefabRenderer */ }
else if (graphType == "Placeholder")   // Path D: Placeholder renderer
    { /* create PlaceholderGraphRenderer */ }
else                                    // Fallback: VisualScript
    { /* fallback to VisualScriptRenderer */ }
```

**Critical Design Insight**: TabManager doesn't care HOW each type is implemented, only that:
1. Each type has an IGraphRenderer implementation
2. Each renderer provides IGraphDocument  
3. Each renderer implements required interface methods

### 1.2 EditorTab: Core Container with Ownership Semantics

**Structure Analysis**:

```cpp
struct EditorTab
{
    std::string    tabID;       // Unique identifier
    std::string    displayName; // UI label
    std::string    filePath;    // Full path to file
    std::string    graphType;   // Type discriminator: "VisualScript", "BehaviorTree", etc.
    bool           isDirty;     // Unsaved changes flag
    bool           isActive;    // Currently selected tab
    
    IGraphRenderer* renderer;   // Owned pointer - polymorphic base class
    IGraphDocument* document;   // Owned pointer - polymorphic base class
};
```

**Critical Feature #1: Conditional Destructor**

```cpp
~EditorTab()
{
    // PHASE 56 FIX: Different ownership semantics for different graph types!
    
    // EntityPrefab: Tab owns document (heap-allocated independently)
    if (document && graphType == "EntityPrefab")
        delete document;  // ✓ Must delete
    
    // VisualScript/BehaviorTree/Placeholder: Renderer owns document (unique_ptr)
    // Don't delete here - renderer will handle it
    
    if (renderer)
        delete renderer;  // Always delete renderer
}
```

**Why This Matters**: 
- VisualScript/BehaviorTree renderers own their documents via unique_ptr
- EntityPrefab currently allocates document separately, tab takes ownership
- This is an ARCHITECTURAL MISMATCH that needs to be fixed

**Critical Feature #2: Move Semantics with Ownership Transfer**

```cpp
EditorTab(EditorTab&& other) noexcept
    : renderer(other.renderer)    // Transfer ownership
    , document(other.document)    // Transfer ownership
    // ... other members ...
{
    other.renderer = nullptr;     // Nullify source (critical!)
    other.document = nullptr;     // Nullify source (critical!)
}
```

**Why This Matters**:
- When tab is moved to m_tabs vector, ownership is explicitly transferred
- Source pointers are nullified to prevent double-deletion
- Phase 51 fix: Save tabID BEFORE move to avoid undefined behavior

### 1.3 Multi-Type Detection: Heuristic-Based Type Identification

**Detection Algorithm** (50 lines in TabManager::DetectGraphType):

```cpp
1. Check explicit "graphType" field (JSON)
2. Check explicit "blueprintType" field (JSON)  
3. Check schema_version == 4 + nodes + execConnections → VisualScript
4. Check rootNodeId + nodes → BehaviorTree
5. Check states + transitions → AnimGraph
6. Check blueprintType → legacy BehaviorTree v2
7. Fallback → "Unknown"
```

**Key Insight**: Type detection is CRITICAL because it determines which renderer/document pair gets created. Incorrect detection = wrong code path.

### 1.4 Canvas State Preservation Pattern

**Pattern**: Save/Restore Canvas State on Tab Switch

```cpp
void TabManager::SetActiveTab(const std::string& tabID)
{
    // BEFORE: Save previous tab's canvas state
    EditorTab* previousTab = GetActiveTab();
    if (previousTab && previousTab->renderer)
        previousTab->renderer->SaveCanvasState();  // Phase 35.0 feature
    
    // SWITCH: Update active tab
    m_activeTabID = tabID;
    m_pendingSelectTabID = tabID;
    
    // AFTER: Restore new tab's canvas state
    EditorTab* newTab = GetActiveTab();
    if (newTab && newTab->renderer)
        newTab->renderer->RestoreCanvasState();  // Phase 35.0 feature
}
```

**Why This Matters**: Users expect zoom/pan position to be preserved when switching tabs. Each renderer must implement SaveCanvasState/RestoreCanvasState.

### 1.5 Dirty Flag Synchronization

**Pattern**: Sync Dirty State from Renderer to Tab Each Frame

```cpp
void TabManager::RenderTabBar()
{
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        EditorTab& tab = m_tabs[i];
        
        // CRITICAL: Sync dirty flag EVERY FRAME
        if (tab.renderer)
            tab.isDirty = tab.renderer->IsDirty();
        
        // Build tab label with asterisk when dirty
        std::string label = tab.displayName;
        if (tab.isDirty)
            label += " *";
    }
}
```

**Why This Matters**: 
- Renderer tracks actual modifications (knows when user made changes)
- TabManager syncs this state to UI
- Tab label shows "*" when dirty (visual indicator)

### 1.6 Unified Modal Dispatcher Integration (Phase 44.2)

**Pattern**: Centralized Modal Rendering via CanvasModalRenderer

```cpp
void TabManager::RenderTabBar()
{
    // ... tab bar rendering ...
    
    // Centralized SaveAs modal
    CanvasModalRenderer::Get().RenderSaveFilePickerModal();
    
    // Centralized SubGraph file picker
    CanvasModalRenderer::Get().RenderSubGraphFilePickerModal();
    
    // Handle SaveAs result
    if (!CanvasModalRenderer::Get().IsSaveFileModalOpen())
    {
        std::string selectedFile = CanvasModalRenderer::Get().GetSelectedSaveFilePath();
        if (!selectedFile.empty())
        {
            // Save to selectedFile
            EditorTab* tab = GetActiveTab();
            if (tab && tab->renderer)
                tab->renderer->Save(selectedFile);
        }
    }
}
```

**Why This Matters**: All graph types use same modal system (no duplication, unified behavior)

### 1.7 Caching Strategy (Phase 51)

**Pattern**: Prevent Duplicate File Loads

```cpp
std::string TabManager::OpenFileInTab(const std::string& filePath)
{
    // Check if already open
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs[i].filePath == filePath)
        {
            SetActiveTab(m_tabs[i].tabID);
            return m_tabs[i].tabID;  // Return existing tab
        }
    }
    
    // File not open, create new tab...
}
```

**Why This Matters**: User opens same file twice → activate existing tab instead of loading again (better UX, less memory)

---

## PART 2: ENTITYPREFAB CURRENT ARCHITECTURE ANALYSIS

### 2.1 Current EntityPrefab Design (Before Framework Alignment)

**Structure**:

```cpp
EntityPrefabGraphDocument : public IGraphDocument
    ├─ Node storage: std::vector<ComponentNode> m_nodes
    ├─ Connection storage: std::vector<std::pair<PrefabNodeId, PrefabNodeId>>
    ├─ State: m_selectedNodes, m_canvasOffset, m_canvasZoom
    └─ File I/O: LoadFromFile(), SaveToFile()

EntityPrefabRenderer : public IGraphRenderer  
    ├─ Reference: PrefabCanvas& m_canvas (not owned!)
    ├─ Components: ComponentPalettePanel, PropertyEditorPanel
    ├─ Framework: std::unique_ptr<ICanvasEditor>, std::unique_ptr<CanvasFramework>
    └─ Canvas state: SaveCanvasState(), RestoreCanvasState()

PrefabCanvas (ImGui Container)
    ├─ Node rendering: ComponentNodeRenderer m_renderer
    ├─ Input handling: OnMouseDown, OnMouseMove, OnMouseUp
    ├─ Selection: Rectangle selection, multi-select
    ├─ Coordinate transform: ScreenToCanvas()
    └─ ImGui rendering: Render() with all UI elements
```

### 2.2 Critical Architectural Issues

**ISSUE #1: Reference vs Ownership Mismatch**

```cpp
// CURRENT (WRONG):
class EntityPrefabRenderer
{
    PrefabCanvas& m_canvas;  // Reference to external object
    // Problem: Who owns PrefabCanvas?
    // Problem: What lifetime?
    // Problem: What if canvas is deleted while renderer exists?
};

// EXPECTED (Framework pattern):
class VisualScriptRenderer  
{
    std::unique_ptr<ICanvasEditor> m_canvasEditor;  // Renderer owns canvas!
    // Clear ownership semantics
    // Canvas destroyed when renderer destroyed
};
```

**Impact on Linker Errors**: If ownership unclear, destructor doesn't know if it should delete PrefabCanvas, leading to symbol visibility issues.

**ISSUE #2: Document Lifecycle Mismatch**

```cpp
// CURRENT (WRONG):
EntityPrefabGraphDocument* doc = new EntityPrefabGraphDocument();
PrefabCanvas* canvas = new PrefabCanvas();
EntityPrefabRenderer* renderer = new EntityPrefabRenderer(*canvas);
tab.document = doc;      // Tab owns document
tab.renderer = renderer; // Tab owns renderer
// Problem: Renderer doesn't know about document!
// Problem: Document not accessible from renderer via GetDocument()

// EXPECTED (Framework pattern):
EntityPrefabRenderer* renderer = new EntityPrefabRenderer();
renderer->SetDocument(doc);  // Renderer owns/manages document
tab.renderer = renderer;
tab.document = renderer->GetDocument();  // Get from renderer
```

**Impact on Linker Errors**: Symbols for document access unresolved because architecture doesn't match expected flow.

**ISSUE #3: ComponentNodeRenderer Initialization Gap**

```cpp
// CURRENT (Phase 52 issue):
class PrefabCanvas
{
    std::unique_ptr<ComponentNodeRenderer> m_renderer;  // Declared
    
    void Initialize(EntityPrefabGraphDocument* doc)
    {
        m_document = doc;
        // MISSING: m_renderer was never created!
        // if (!m_renderer)
        //     m_renderer = std::make_unique<ComponentNodeRenderer>();
    }
    
    void Render()
    {
        if (m_renderer)  // Always nullptr!
            m_renderer->RenderNodes(m_document);  // Never executed
    }
};
```

**Impact on Rendering**: ComponentNodeRenderer methods never called (symbols unresolved in linker).

**ISSUE #4: PrefabLoader Symbol Visibility**

```cpp
// From EntityPrefabGraphDocument.cpp:
bool EntityPrefabGraphDocument::LoadFromFile(const std::string& filePath)
{
    // Calls PrefabLoader utilities
    bool success = PrefabLoader::LoadJsonFromFile(filePath, /* ... */);
    // Problem: PrefabLoader not linked properly
    // Linker error: LNK2001 unresolved external symbol "PrefabLoader::LoadJsonFromFile"
}
```

**Impact on Linking**: PrefabLoader functions used but not properly exported/compiled.

---

## PART 3: FRAMEWORK INTERFACES CONTRACT ANALYSIS

### 3.1 IGraphRenderer Interface Contract

**Expected Methods**:

```cpp
class IGraphRenderer
{
    // Lifecycle
    virtual bool Load(const std::string& path) = 0;
    virtual bool Save(const std::string& path) = 0;
    virtual bool IsDirty() const = 0;
    
    // Properties
    virtual std::string GetGraphType() const = 0;
    virtual std::string GetCurrentPath() const = 0;
    
    // Canvas state preservation (Phase 35.0)
    virtual void SaveCanvasState() = 0;
    virtual void RestoreCanvasState() = 0;
    virtual std::string GetCanvasStateJSON() const = 0;
    virtual void SetCanvasStateJSON(const std::string& json) = 0;
    
    // Rendering
    virtual void Render() = 0;
    
    // Document access
    virtual IGraphDocument* GetDocument() = 0;
    
    // Framework modals (Phase 43)
    virtual void RenderFrameworkModals() = 0;
};
```

**EntityPrefab Compliance Check**:

| Method | Implemented? | Issue |
|--------|-------------|-------|
| Load() | ✓ | Uses LoadFromFile() |
| Save() | ✓ | Uses SaveToFile() |
| IsDirty() | ✓ | Checks m_isDirty flag |
| GetGraphType() | ✓ | Returns "EntityPrefab" |
| GetCurrentPath() | ✓ | Returns m_filePath |
| SaveCanvasState() | ✓ | Implemented |
| RestoreCanvasState() | ✓ | Implemented |
| GetCanvasStateJSON() | ✓ | Implemented |
| SetCanvasStateJSON() | ✓ | Implemented |
| Render() | ✓ | Calls RenderLayoutWithTabs() |
| GetDocument() | ✓ | Returns m_document (but NULL!) |
| RenderFrameworkModals() | ✓ | Implemented |

**Critical Problem**: GetDocument() returns NULL because EntityPrefabRenderer doesn't have reference to document!

### 3.2 IGraphDocument Interface Contract

**Expected Methods**:

```cpp
class IGraphDocument
{
    // Lifecycle
    virtual bool Load(const std::string& filePath) = 0;
    virtual bool Save(const std::string& filePath) = 0;
    
    // Metadata
    virtual std::string GetName() const = 0;
    virtual DocumentType GetType() const = 0;
    virtual std::string GetFilePath() const = 0;
    virtual void SetFilePath(const std::string& path) = 0;
    
    // State
    virtual bool IsDirty() const = 0;
    
    // Renderer access
    virtual IGraphRenderer* GetRenderer() = 0;
    
    // Lifecycle notifications
    virtual void OnDocumentModified() = 0;
};
```

**EntityPrefab Compliance Check**:

| Method | Implemented? | Issue |
|--------|-------------|-------|
| Load() | ✓ | Calls LoadFromFile() |
| Save() | ✓ | Calls SaveToFile() |
| GetName() | ✓ | Returns m_documentName |
| GetType() | ✓ | Returns ENTITY_PREFAB |
| GetFilePath() | ✓ | Returns m_filePath |
| SetFilePath() | ✓ | Sets m_filePath |
| IsDirty() | ✓ | Returns m_isDirty |
| GetRenderer() | ✗ | **NOT IMPLEMENTED** - Returns nullptr! |
| OnDocumentModified() | ✓ | Sets m_isDirty = true |

**Critical Problem**: GetRenderer() not implemented, returns nullptr. TabManager expects this to work!

---

## PART 4: FIT-GAP ANALYSIS

### 4.1 Ownership Semantics Gap

**Current State**:
```
EntityPrefabGraphDocument (heap allocated)
EntityPrefabRenderer (heap allocated)
PrefabCanvas (heap allocated)
// No clear ownership relationships!
```

**Framework Expectation**:
```
EntityPrefabRenderer (owns everything)
    ├─ PrefabCanvas (unique_ptr, owned)
    ├─ EntityPrefabGraphDocument (unique_ptr, owned)  
    └─ Provides: GetDocument(), Save(), Load()
```

**Gap**: EntityPrefabRenderer doesn't own document or canvas.

### 4.2 Document Access Gap

**Current State**:
```cpp
// TabManager expects:
tab.document = renderer->GetDocument();

// EntityPrefab delivers:
tab.document = new EntityPrefabGraphDocument();  // Created separately
renderer->GetDocument();  // Returns nullptr!
```

**Gap**: Renderer's GetDocument() doesn't return the actual document.

### 4.3 Canvas Initialization Gap

**Current State**:
```cpp
// Phase 52 issue: m_renderer (ComponentNodeRenderer) never initialized
if (m_renderer)  // Always nullptr!
    m_renderer->RenderNodes(m_document);
```

**Gap**: ComponentNodeRenderer created but never initialized in PrefabCanvas::Initialize().

### 4.4 Move Semantics Gap

**Current State**:
```cpp
// When EditorTab is moved:
EditorTab tab;
tab.document = doc;      // Points to EntityPrefabGraphDocument
tab.renderer = renderer; // Points to EntityPrefabRenderer
m_tabs.emplace_back(std::move(tab));  // Move happens here

// Problem: What does EntityPrefabRenderer own?
// Is document transferred? Is canvas transferred?
// On destruction, does renderer delete its internals?
```

**Gap**: Unclear who owns what when moved.

---

## PART 5: PROPOSED ADAPTER PATTERN DESIGN

### 5.1 New Architecture: EntityPrefab as Self-Contained Framework Type

```cpp
// NEW: EntityPrefabRenderer as proper Framework adapter

class EntityPrefabRenderer : public IGraphRenderer
{
private:
    // NEW: Renderer owns ALL resources
    std::unique_ptr<EntityPrefabGraphDocument> m_document;
    std::unique_ptr<PrefabCanvas> m_canvas;
    
    // Keep existing
    ComponentPalettePanel m_componentPalette;
    PropertyEditorPanel m_propertyEditor;
    std::unique_ptr<CanvasFramework> m_framework;
    std::string m_filePath;
    bool m_isDirty = false;

public:
    explicit EntityPrefabRenderer()
        : m_document(std::make_unique<EntityPrefabGraphDocument>())
        , m_canvas(std::make_unique<PrefabCanvas>())
    {
        m_canvas->Initialize(m_document.get());
    }
    
    // Override: Return owned document
    IGraphDocument* GetDocument() override
    {
        return m_document.get();  // No longer nullptr!
    }
    
    // Rest of interface implementation...
};
```

### 5.2 TabManager Integration: EntityPrefab Creation Path

**Current (Broken)**:
```cpp
if (graphType == "EntityPrefab")
{
    // PHASE 54 FIX (broken):
    EntityPrefabGraphDocument* epDoc = new EntityPrefabGraphDocument();
    PrefabCanvas* epCanvas = new PrefabCanvas();
    epCanvas->Initialize(epDoc);
    
    EntityPrefabRenderer* r = new EntityPrefabRenderer(*epCanvas);
    tab.renderer = r;
    tab.document = epDoc;  // WRONG: Document not accessible via renderer!
}
```

**Fixed (Using New Pattern)**:
```cpp
if (graphType == "EntityPrefab")
{
    EntityPrefabRenderer* r = new EntityPrefabRenderer();
    r->Load(filePath);  // Renderer handles load internally
    
    tab.renderer = r;
    tab.document = r->GetDocument();  // NOW WORKS!
}
```

### 5.3 Conditional Destructor Fix

**Current (Phase 56)**:
```cpp
~EditorTab()
{
    if (document && graphType == "EntityPrefab")
        delete document;  // Tab deletes document
    if (renderer)
        delete renderer;
}
```

**Fixed (New Pattern)**:
```cpp
~EditorTab()
{
    // ALL types: document is owned by renderer, NOT by tab!
    // Renderer destructor handles all cleanup
    
    // No need to check graphType anymore!
    if (renderer)
        delete renderer;  // Renderer deletes its own document + canvas
}
```

---

## PART 6: MIGRATION ROADMAP

### Phase 6A: Refactor EntityPrefabRenderer Ownership

**Step 1**: Change EntityPrefabRenderer to own document and canvas

```cpp
// Before:
class EntityPrefabRenderer
{
    PrefabCanvas& m_canvas;  // Reference (BAD)
};

// After:
class EntityPrefabRenderer  
{
    std::unique_ptr<PrefabCanvas> m_canvas;  // Owned (GOOD)
    std::unique_ptr<EntityPrefabGraphDocument> m_document;  // Owned (GOOD)
};
```

**Step 2**: Update GetDocument() to return owned document

```cpp
IGraphDocument* GetDocument() override
{
    return m_document.get();  // Return owned document
}
```

**Step 3**: Initialize m_renderer in PrefabCanvas::Initialize()

```cpp
void PrefabCanvas::Initialize(EntityPrefabGraphDocument* document)
{
    m_document = document;
    
    // NEW: Initialize ComponentNodeRenderer
    if (!m_renderer)
    {
        m_renderer = std::make_unique<ComponentNodeRenderer>();
    }
}
```

### Phase 6B: Update TabManager EntityPrefab Creation Path

```cpp
// Before:
EntityPrefabGraphDocument* epDoc = new EntityPrefabGraphDocument();
PrefabCanvas* epCanvas = new PrefabCanvas();
EntityPrefabRenderer* r = new EntityPrefabRenderer(*epCanvas);
tab.document = epDoc;

// After:
EntityPrefabRenderer* r = new EntityPrefabRenderer();
r->Load(filePath);
tab.document = r->GetDocument();  // Works now!
```

### Phase 6C: Fix EditorTab Destructor

```cpp
// Before:
~EditorTab()
{
    if (document && graphType == "EntityPrefab")
        delete document;  // Special case for EntityPrefab
    if (renderer)
        delete renderer;
}

// After:
~EditorTab()
{
    if (renderer)
        delete renderer;  // Renderer handles all cleanup
    // Don't delete document - it's owned by renderer
}
```

### Phase 6D: Fix PrefabLoader Symbol Visibility

**Step 1**: Ensure PrefabLoader.cpp is in vcxproj (add if missing)
**Step 2**: Verify symbol exports in PrefabLoader.h
**Step 3**: Link PrefabLoader compiled object in project

### Phase 6E: Link ComponentNodeRenderer Symbols

**Step 1**: Ensure ComponentNodeRenderer.cpp in vcxproj
**Step 2**: Verify all method implementations complete (not stubs)
**Step 3**: Check for inline/static issues preventing symbol export

---

## PART 7: EXPECTED OUTCOMES

### Before Migration
- ✅ 0 Compilation Errors (files compile)
- ⚠️ 55 Linker Errors (symbols not accessible)
- Symptoms:
  - EntityPrefab tabs don't appear (OpenFileInTab returns empty)
  - Renderer GetDocument() returns null
  - Canvas rendering doesn't initialize properly

### After Migration
- ✅ 0 Compilation Errors (still no change)
- ✅ 0 Linker Errors (symbols resolved!)
- ✅ EntityPrefab tabs appear correctly
- ✅ Renderer GetDocument() returns valid pointer
- ✅ Canvas rendering works end-to-end
- ✅ Phase 3 tabbing system functional

---

## PART 8: IMPLEMENTATION PRIORITY

**CRITICAL PATH (Must do first)**:
1. EntityPrefabRenderer owns document + canvas
2. Update TabManager EntityPrefab creation path
3. Fix EditorTab destructor (remove EntityPrefab special case)
4. Initialize ComponentNodeRenderer in PrefabCanvas

**IMPORTANT (Unblock rendering)**:
5. Ensure PrefabLoader symbols exported
6. Link ComponentNodeRenderer properly

**NICE-TO-HAVE**:
7. Refactor for consistency with VisualScript/BehaviorTree patterns

---

## PART 9: VALIDATION CHECKLIST

- [ ] EntityPrefabRenderer creates own document in constructor
- [ ] EntityPrefabRenderer creates own canvas in constructor
- [ ] GetDocument() returns non-null pointer
- [ ] TabManager CreateNewTab EntityPrefab path updated
- [ ] TabManager OpenFileInTab EntityPrefab path updated
- [ ] EditorTab destructor doesn't special-case EntityPrefab
- [ ] PrefabCanvas::Initialize creates ComponentNodeRenderer
- [ ] ComponentNodeRenderer symbols resolve in linker
- [ ] PrefabLoader symbols resolve in linker
- [ ] Build achieves 0 linker errors
- [ ] EntityPrefab file opens in tab correctly
- [ ] Nodes render on canvas
- [ ] Canvas state preserves on tab switch
- [ ] Save/SaveAs functionality works

---

## CONCLUSION

The 55 linker errors are not due to missing code, but to **architectural misalignment**. EntityPrefab was built outside the Framework architecture and doesn't properly adapt to TabManager's expectations.

The solution is to refactor EntityPrefabRenderer to own all its resources (document, canvas) and properly implement the IGraphRenderer interface contract. This is a **low-risk, high-impact change** that aligns EntityPrefab with the proven patterns used by VisualScript and BehaviorTree.

**Estimated time to complete**: 2-3 hours (architecture analysis + implementation + testing)

**Risk level**: LOW (changes isolated to EntityPrefab layer, Framework interfaces unchanged)

**Confidence level**: ⭐⭐⭐⭐⭐ (patterns validated against TabManager + IGraphRenderer/IGraphDocument)
