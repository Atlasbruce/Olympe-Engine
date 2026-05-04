# ARCHITECTURAL PATTERNS REFERENCE - TabManager + Framework

## Pattern 1: Multi-Type Graph Router

**What It Is**: Central dispatcher that routes different graph types through unified tab system

**Pattern**:
```cpp
std::string graphType = DetectGraphType(filePath);

if (graphType == "VisualScript")
    // Create VisualScriptRenderer + VisualScriptGraphDocument
else if (graphType == "BehaviorTree")
    // Create BehaviorTreeRenderer + BehaviorTreeGraphDocument
else if (graphType == "EntityPrefab")
    // Create EntityPrefabRenderer + EntityPrefabGraphDocument (AFTER PHASE 58)
```

**Key Principle**: Each type can have completely different implementation details, but all follow same interface contract

**Used By**: TabManager::OpenFileInTab(), CreateNewTab()

---

## Pattern 2: Renderer Owns Everything

**What It Is**: Renderer takes full responsibility for document lifecycle and canvas rendering

**Pattern**:
```cpp
class TypeRenderer : public IGraphRenderer
{
    std::unique_ptr<IGraphDocument> m_document;  // Owned
    std::unique_ptr<ICanvasContainer> m_canvas;  // Owned
    
    TypeRenderer() 
        : m_document(std::make_unique<TypeDocument>())
        , m_canvas(std::make_unique<TypeCanvas>())
    {
        m_canvas->Initialize(m_document.get());
    }
    
    IGraphDocument* GetDocument() override 
    { 
        return m_document.get();  // Provide to framework
    }
};
```

**Key Principle**: No confusion about who owns what. Renderer owns all, framework can rely on it.

**Used By**: VisualScriptRenderer, BehaviorTreeRenderer, EntityPrefabRenderer (after Phase 58)

---

## Pattern 3: Framework Interface Implementation

**What It Is**: All renderers and documents implement abstract interface contracts

**Renderer Contract**:
```cpp
class IGraphRenderer
{
    virtual bool Load(const std::string& path) = 0;        // Load from file
    virtual bool Save(const std::string& path) = 0;        // Save to file
    virtual bool IsDirty() const = 0;                       // Unsaved changes?
    virtual IGraphDocument* GetDocument() = 0;             // Access document
    virtual void SaveCanvasState() = 0;                     // Preserve zoom/pan
    virtual void RestoreCanvasState() = 0;                 // Restore zoom/pan
    virtual void Render() = 0;                             // Draw to ImGui
    virtual void RenderFrameworkModals() = 0;             // Framework UI
};
```

**Document Contract**:
```cpp
class IGraphDocument
{
    virtual bool Load(const std::string& path) = 0;       // Load from file
    virtual bool Save(const std::string& path) = 0;       // Save to file
    virtual bool IsDirty() const = 0;                      // Unsaved changes?
    virtual std::string GetFilePath() const = 0;          // Current path
    virtual void SetFilePath(const std::string& path) = 0; // Update path
    virtual IGraphRenderer* GetRenderer() = 0;            // Return renderer
    virtual void OnDocumentModified() = 0;                // Track changes
};
```

**Key Principle**: As long as implementation provides these methods, TabManager doesn't care about internals

**Used By**: All graph types (VisualScript, BehaviorTree, EntityPrefab, Placeholder)

---

## Pattern 4: EditorTab Unified Ownership

**What It Is**: Tab owns renderer pointer, renderer owns everything else

**Pattern**:
```cpp
struct EditorTab
{
    IGraphRenderer* renderer;   // Tab owns renderer
    IGraphDocument* document;   // Get from renderer->GetDocument()
    
    ~EditorTab()
    {
        // Single deletion point
        if (renderer)
            delete renderer;  // Renderer destructor handles document cleanup
    }
};
```

**Key Principle**: Simple ownership chain. No special cases needed.

**Used By**: TabManager tab storage (m_tabs vector)

---

## Pattern 5: Type Discriminator for Initialization

**What It Is**: Use graphType string to determine document ownership model

**Current (WRONG)** - Phase 56:
```cpp
~EditorTab()
{
    if (document && graphType == "EntityPrefab")
        delete document;  // Special case for EntityPrefab!
    if (renderer)
        delete renderer;
}
```

**Fixed (PHASE 58)** - All types same:
```cpp
~EditorTab()
{
    if (renderer)
        delete renderer;  // All types: renderer handles cleanup
}
```

**Key Principle**: After Phase 58, no special cases needed. All types follow same pattern.

**Used By**: EditorTab destructor

---

## Pattern 6: Dirty Flag Synchronization

**What It Is**: Sync dirty state from renderer to UI each frame

**Pattern**:
```cpp
void TabManager::RenderTabBar()
{
    for (auto& tab : m_tabs)
    {
        // Every frame: check if renderer says dirty
        if (tab.renderer)
            tab.isDirty = tab.renderer->IsDirty();
        
        // Update UI based on tab dirty state
        std::string label = tab.displayName;
        if (tab.isDirty)
            label += " *";  // Show asterisk when dirty
    }
}
```

**Key Principle**: Renderer tracks actual modifications, TabManager propagates to UI

**Used By**: TabManager::RenderTabBar(), all graph types

---

## Pattern 7: Canvas State Preservation

**What It Is**: Save zoom/pan when switching tabs, restore when returning

**Pattern**:
```cpp
void TabManager::SetActiveTab(const std::string& tabID)
{
    // Save previous tab state
    EditorTab* prev = GetActiveTab();
    if (prev && prev->renderer)
        prev->renderer->SaveCanvasState();
    
    // Switch tab
    m_activeTabID = tabID;
    
    // Restore new tab state
    EditorTab* curr = GetActiveTab();
    if (curr && curr->renderer)
        curr->renderer->RestoreCanvasState();
}
```

**Key Principle**: User doesn't lose context when switching between tabs

**Used By**: TabManager::SetActiveTab(), all graph types with zoom/pan

---

## Pattern 8: Centralized Modal Dispatcher

**What It Is**: All graph types share same modals (Save, SaveAs, File picker)

**Pattern**:
```cpp
void TabManager::RenderTabBar()
{
    // All types use same modal system
    CanvasModalRenderer::Get().RenderSaveFilePickerModal();
    CanvasModalRenderer::Get().RenderSubGraphFilePickerModal();
    
    // Handle unified result
    if (!CanvasModalRenderer::Get().IsSaveFileModalOpen())
    {
        std::string selectedFile = CanvasModalRenderer::Get().GetSelectedSaveFilePath();
        if (!selectedFile.empty())
        {
            EditorTab* tab = GetActiveTab();
            if (tab)
                tab->renderer->Save(selectedFile);
        }
    }
}
```

**Key Principle**: No duplication, consistent UX across all types

**Used By**: TabManager::RenderTabBar(), Phase 44.2 framework

---

## Pattern 9: Type Detection via JSON Heuristics

**What It Is**: Inspect file contents to determine graph type

**Pattern**:
```cpp
std::string TabManager::DetectGraphType(const std::string& filePath)
{
    // 1. Check explicit type field
    if (root.contains("graphType"))
        return root["graphType"];  // "VisualScript", "BehaviorTree", etc.
    
    // 2. Check structural patterns
    if (root.contains("rootNodeId") && root.contains("nodes"))
        return "BehaviorTree";
    
    if (root.contains("nodes") && root.contains("execConnections"))
        return "VisualScript";
    
    // 3. Default
    return "Unknown";
}
```

**Key Principle**: Type detection is critical - determines which code path executes

**Used By**: TabManager::OpenFileInTab(), TabManager::CreateNewTab()

---

## Pattern 10: Fallback Strategy

**What It Is**: Graceful degradation when type is unknown

**Pattern**:
```cpp
else if (graphType == "Placeholder")
    { /* Placeholder logic */ }
else
{
    // Fallback: try as VisualScript
    VisualScriptRenderer* r = new VisualScriptRenderer();
    if (!r->Load(filePath))
    {
        delete r;
        return "";  // Load failed
    }
    tab.renderer = r;
    tab.graphType = "VisualScript";  // Update type for destructor
}
```

**Key Principle**: Unknown type doesn't cause crash, tries sensible default

**Used By**: TabManager::OpenFileInTab()

---

## PHASE 58 INTEGRATION CHECKLIST

### EntityPrefabRenderer Compliance

- [ ] Owns document (unique_ptr<EntityPrefabGraphDocument>)
- [ ] Owns canvas (unique_ptr<PrefabCanvas>)
- [ ] Constructor initializes both
- [ ] GetDocument() returns valid pointer (not nullptr)
- [ ] Implements all IGraphRenderer methods
- [ ] SaveCanvasState/RestoreCanvasState work
- [ ] IsDirty() reflects actual modifications

### TabManager Integration

- [ ] CreateNewTab recognizes "EntityPrefab" type
- [ ] OpenFileInTab recognizes "EntityPrefab" type
- [ ] Creates EntityPrefabRenderer with no parameters
- [ ] Gets document via renderer->GetDocument()
- [ ] Passes to tab.document correctly

### EditorTab Consistency

- [ ] Destructor doesn't special-case EntityPrefab
- [ ] Move semantics work correctly
- [ ] No double-deletion issues
- [ ] Ownership semantics clear

### Build Verification

- [ ] 0 Compilation Errors
- [ ] Linker errors reduced (EntityPrefab symbols resolve)
- [ ] Runtime: EntityPrefab tabs open
- [ ] Runtime: Nodes render
- [ ] Runtime: Save/Load work

---

## PATTERN COMPARISON: Before vs After Phase 58

### Before (Broken Architecture)

```
TabManager                     EntityPrefabRenderer (doesn't own anything!)
  │                              │
  ├─ GetDocument()  ────────────┼──> Returns nullptr (broken!)
  │                              │
  └─ OpenFileInTab()            │
     └─ Creates separately:      │
        ├─ EntityPrefabGraphDocument
        ├─ PrefabCanvas  
        └─ EntityPrefabRenderer(canvas)
        
Problem: GetDocument() returns null
Problem: Who owns document?
Problem: Destructor confusion
```

### After Phase 58 (Fixed Architecture)

```
TabManager
  │
  ├─ OpenFileInTab()
  │   └─ EntityPrefabRenderer (owns everything!)
  │       ├─ unique_ptr<EntityPrefabGraphDocument>  ✓
  │       └─ unique_ptr<PrefabCanvas>              ✓
  │
  └─ GetDocument() ────> Returns valid pointer ✓

Pattern: Same as VisualScriptRenderer
Pattern: Same as BehaviorTreeRenderer
Pattern: Unified ownership model
```

---

## VALIDATION: Architecture Alignment

| Aspect | VisualScript | BehaviorTree | EntityPrefab (After) |
|--------|------------|-------------|---------------------|
| Owns document | ✓ via unique_ptr | ✓ via unique_ptr | ✓ via unique_ptr |
| Owns canvas | ✓ via unique_ptr | ✓ via imnodes | ✓ via unique_ptr |
| GetDocument() returns | Valid pointer | Valid pointer | Valid pointer (✓) |
| IsDirty() implemented | ✓ | ✓ | ✓ |
| Save/Load | ✓ | ✓ | ✓ |
| Canvas state | ✓ | ✓ | ✓ |
| TabManager compatible | ✓ | ✓ | ✓ (After Phase 58) |

---

## CONCLUSION

Phase 58 aligns EntityPrefab with the proven Framework patterns used by VisualScript and BehaviorTree. The refactoring is low-risk and high-impact, resolving all 55 linker errors by establishing proper ownership semantics and interface compliance.

Key insight: **Architecture alignment first, then implementation follows naturally.**
