# 🎬 PHASE 2.1 CHUNK 2 - RENDERING & CANVAS INTEGRATION

## 📋 Overview

**Objective:** Create the rendering layer (IGraphRenderer adapter) and integrate with canvas system

**Current Status:**
- ✅ Data Model (EntityPrefabGraphDocumentV2): Complete
- ✅ Routing System (DocumentVersionManager): Complete
- ⏳ Rendering Layer (EntityPrefabEditorV2): Next

**Build Dependency:** 
- EntityPrefabGraphDocumentV2.h/cpp ✅
- DocumentVersionManager.h/cpp ✅

---

## 🎯 Chunk 2 Goals

1. **Create EntityPrefabEditorV2**
   - Implements IGraphRenderer interface
   - Adapts EntityPrefabGraphDocumentV2 to framework
   - Controls PrefabCanvas for rendering

2. **Integrate Rendering Pipeline**
   - Document → Renderer → Canvas → Screen
   - Verify nodes render correctly
   - Verify interactions work

3. **Register in DocumentVersionManager**
   - Add renderer factory to Framework strategy
   - Test complete create → render flow

4. **Test End-to-End**
   - Create EntityPrefab document
   - Auto-render in canvas
   - Verify interactive features

---

## 🏗️ Architecture

### Current State (Chunks 1 + 1b)
```
EntityPrefabGraphDocumentV2 ──── ✅ (Data Model)
         │
         └─ IGraphDocument interface
              │
              ├── Load/Save ✅
              ├── Node management ✅
              └── Property system ✅

DocumentVersionManager ──── ✅ (Routing)
         │
         ├─ Strategy Registry
         ├─ Version Routing
         └─ Error Handling ✅
```

### After Chunk 2 (Target)
```
EntityPrefabGraphDocumentV2  ✅
         │
         ▼
EntityPrefabEditorV2 ⏳ (NEW - Chunk 2)
         │
         ├─ Implements IGraphRenderer
         ├─ Contains PrefabCanvas
         ├─ Handles UI rendering
         └─ Manages interactions
              │
              ▼
         Canvas Rendering
              │
              ├─ Grid display
              ├─ Node rendering
              ├─ Connection lines
              └─ UI controls
```

### Registration in DocumentVersionManager
```cpp
// Framework Strategy (to be registered)
DocumentCreationStrategy frameworkStrategy = {
    []() { 
        return new EntityPrefabGraphDocumentV2(); 
    },
    [](const std::string& path) { 
        auto doc = new EntityPrefabGraphDocumentV2();
        return doc->Load(path) ? doc : nullptr;
    },
    [](IGraphDocument* doc) {  // ← THIS IS CHUNK 2
        auto v2Doc = static_cast<EntityPrefabGraphDocumentV2*>(doc);
        return new EntityPrefabEditorV2(v2Doc);  // NEW
    },
    "Framework EntityPrefab V2",
    GraphTypeVersion::Framework
};
```

---

## 📝 Implementation Plan

### Step 1: Create EntityPrefabEditorV2 Class
```cpp
class EntityPrefabEditorV2 : public IGraphRenderer
{
public:
    EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document);
    ~EntityPrefabEditorV2();

    // IGraphRenderer implementation
    void Render() override;
    bool IsDirty() const override;
    void SaveToFile(const std::string& filePath) override;
    
    // Canvas management
    PrefabCanvas* GetCanvas() const;
    
private:
    EntityPrefabGraphDocumentV2* m_document;
    PrefabCanvas* m_canvas;
};
```

### Step 2: Implement Canvas Integration
```cpp
EntityPrefabEditorV2::EntityPrefabEditorV2(EntityPrefabGraphDocumentV2* document)
    : m_document(document)
{
    // Create canvas with V2 document
    m_canvas = new PrefabCanvas();
    m_canvas->Initialize(document);
}

void EntityPrefabEditorV2::Render()
{
    if (m_canvas)
    {
        m_canvas->Render();  // Delegates to existing PrefabCanvas
    }
}
```

### Step 3: Register in DocumentVersionManager
```cpp
// In initialization code:
DocumentCreationStrategy frameworkStrategy = {
    []() { return new EntityPrefabGraphDocumentV2(); },
    [](const std::string& path) { 
        auto doc = new EntityPrefabGraphDocumentV2();
        return doc->Load(path) ? doc : nullptr;
    },
    [](IGraphDocument* doc) { 
        return new EntityPrefabEditorV2(
            static_cast<EntityPrefabGraphDocumentV2*>(doc)
        ); 
    },
    "Framework EntityPrefab V2",
    GraphTypeVersion::Framework
};

DocumentVersionManager::Get().RegisterStrategy(
    "EntityPrefab", 
    GraphTypeVersion::Framework, 
    frameworkStrategy
);
```

---

## 🔄 Expected Flow After Chunk 2

### Flow 1: Create New Document
```
User click "New Entity Prefab"
    ↓
TabManager::CreateNewTab("EntityPrefab")
    ↓
DocumentVersionManager::Get().CreateNewDocument("EntityPrefab")
    ↓
Strategy: createNewDocument() → EntityPrefabGraphDocumentV2() ✓
    ↓
DocumentVersionManager::Get().CreateRenderer("EntityPrefab", doc)
    ↓
Strategy: createRenderer() → EntityPrefabEditorV2(doc) ✓ NEW
    ↓
tab->document = doc;
tab->renderer = new_editor;
    ↓
Render() called → EntityPrefabEditorV2::Render()
    ↓
m_canvas->Render() → Shows empty canvas (new document)
```

### Flow 2: Load Existing Document
```
User double-click "entity.json"
    ↓
TabManager::OpenFileInTab("EntityPrefab", "entity.json")
    ↓
DocumentVersionManager::Get().LoadDocument("EntityPrefab", "entity.json")
    ↓
Strategy: loadDocumentFromFile() → EntityPrefabGraphDocumentV2::Load() ✓
    ↓
DocumentVersionManager::Get().CreateRenderer("EntityPrefab", doc)
    ↓
Strategy: createRenderer() → EntityPrefabEditorV2(doc) ✓ NEW
    ↓
tab->document = doc;
tab->renderer = new_editor;
    ↓
Render() called → EntityPrefabEditorV2::Render()
    ↓
m_canvas->Render() → Shows loaded nodes + connections + grid
```

---

## ✅ Acceptance Criteria

### Must Have
- [ ] EntityPrefabEditorV2 created
- [ ] Implements IGraphRenderer interface fully
- [ ] PrefabCanvas integrated correctly
- [ ] New document creates empty canvas
- [ ] Existing document loads and displays nodes
- [ ] Create + Render flow works end-to-end
- [ ] Framework strategy registered successfully
- [ ] No build errors or warnings

### Should Have
- [ ] Nodes render with correct positions
- [ ] Connections render as Bezier curves
- [ ] Grid displays properly
- [ ] Canvas panning works
- [ ] Canvas zooming works
- [ ] Node selection works

### Nice to Have
- [ ] Component palette displays
- [ ] Property panel shows correctly
- [ ] Save button works
- [ ] Undo/Redo responsive

---

## 🧪 Testing Plan

### Unit Tests
```cpp
TEST(EntityPrefabEditorV2, Constructor)
{
    auto doc = std::make_unique<EntityPrefabGraphDocumentV2>();
    auto editor = std::make_unique<EntityPrefabEditorV2>(doc.get());
    EXPECT_NOT_NULL(editor);
    EXPECT_NOT_NULL(editor->GetCanvas());
}

TEST(EntityPrefabEditorV2, RenderDoesNotCrash)
{
    auto doc = std::make_unique<EntityPrefabGraphDocumentV2>();
    auto editor = std::make_unique<EntityPrefabEditorV2>(doc.get());
    
    // Should not crash
    editor->Render();
}

TEST(EntityPrefabEditorV2, ImplementsIGraphRenderer)
{
    auto doc = std::make_unique<EntityPrefabGraphDocumentV2>();
    auto editor = std::make_unique<EntityPrefabEditorV2>(doc.get());
    
    IGraphRenderer* renderer = editor.get();
    EXPECT_NOT_NULL(renderer);
}
```

### Integration Tests
```cpp
TEST(EntityPrefabEditorV2Integration, CreateNewDocument)
{
    DocumentVersionManager& manager = DocumentVersionManager::Get();
    
    IGraphDocument* doc = manager.CreateNewDocument("EntityPrefab");
    EXPECT_NOT_NULL(doc);
    
    IGraphRenderer* renderer = manager.CreateRenderer("EntityPrefab", doc);
    EXPECT_NOT_NULL(renderer);
    
    // Should not crash
    renderer->Render();
    
    delete renderer;
    delete doc;
}

TEST(EntityPrefabEditorV2Integration, LoadDocument)
{
    DocumentVersionManager& manager = DocumentVersionManager::Get();
    
    IGraphDocument* doc = manager.LoadDocument(
        "EntityPrefab", 
        "./Gamedata/EntityPrefab/Sample.json"
    );
    EXPECT_NOT_NULL(doc);
    
    if (doc)
    {
        IGraphRenderer* renderer = manager.CreateRenderer("EntityPrefab", doc);
        EXPECT_NOT_NULL(renderer);
        
        renderer->Render();
        
        delete renderer;
        delete doc;
    }
}
```

### Manual Tests
- [ ] Create new EntityPrefab → Renders empty canvas
- [ ] Load guard.json → Renders 7 nodes + 5 connections
- [ ] Create new node → Appears on canvas
- [ ] Click node → Selects and highlights
- [ ] Drag node → Moves on canvas
- [ ] Pan canvas → Offsets rendered correctly
- [ ] Zoom canvas → Scales rendered correctly
- [ ] Delete node → Disappears from canvas
- [ ] Save → Persists changes
- [ ] Close & Reopen → Loads saved state

---

## 📊 Estimated Effort

| Task | Time | Difficulty |
|------|------|-----------|
| Create EntityPrefabEditorV2 class | 1h | Low |
| Implement IGraphRenderer interface | 0.5h | Low |
| Integrate PrefabCanvas | 0.5h | Low |
| Register in DocumentVersionManager | 0.5h | Low |
| Testing & verification | 1h | Medium |
| Bug fixes & refinement | 1h | Medium |
| **Total** | **4.5h** | **Low-Medium** |

---

## 📚 Reference Files

### Existing Implementation to Study
- `PrefabCanvas.h/cpp` - Canvas rendering (already works)
- `EntityPrefabRenderer.h/cpp` - Legacy renderer (for reference)
- `ComponentNodeRenderer.h/cpp` - Node rendering (already works)
- `IGraphRenderer.h` - Interface to implement

### New Files to Create
- `EntityPrefabEditorV2.h` - Header
- `EntityPrefabEditorV2.cpp` - Implementation

### Files to Modify
- `DocumentVersionManager.cpp` - Add Framework strategy registration (optional, can do in Chunk 3)

---

## 🎓 Key Design Principles

### 1. Adapter Pattern
```
EntityPrefabEditorV2 adapts:
  EntityPrefabGraphDocumentV2 (data model)
    ↓ (to)
  IGraphRenderer interface (framework contract)
```

### 2. Delegation Pattern
```
EntityPrefabEditorV2::Render()
  ↓
  PrefabCanvas::Render()  (delegates to existing canvas)
  ↓
  ComponentNodeRenderer::RenderNodes()  (already working)
```

### 3. Separation of Concerns
```
Data:       EntityPrefabGraphDocumentV2
Rendering:  EntityPrefabEditorV2 + PrefabCanvas
Framework:  IGraphRenderer interface
```

---

## ⚠️ Potential Issues & Solutions

### Issue 1: PrefabCanvas expects specific construction
**Solution:** Pass EntityPrefabGraphDocumentV2* to PrefabCanvas, already designed for this

### Issue 2: Memory management (who owns PrefabCanvas?)
**Solution:** EntityPrefabEditorV2 owns and deletes PrefabCanvas in destructor

### Issue 3: Document modifications from canvas
**Solution:** Canvas modifies document directly (PrefabCanvas already does this)

### Issue 4: IGraphRenderer interface mismatch
**Solution:** Check IGraphRenderer.h for required methods, implement all

---

## 🚀 Readiness Checklist

Before starting Chunk 2:

- [ ] Read IGraphRenderer.h interface
- [ ] Review PrefabCanvas.h/cpp
- [ ] Review EntityPrefabGraphDocumentV2.h
- [ ] Review ComponentNodeRenderer.h/cpp
- [ ] Understand existing rendering pipeline
- [ ] Build succeeds with current code
- [ ] Have test data (guard.json or similar)

---

## 📞 Questions to Answer Before Implementation

1. Should EntityPrefabEditorV2 be in same directory as other renderers?
   - Suggested: `Source/BlueprintEditor/EntityPrefabEditor/`

2. Should PrefabCanvas be moved to a shared location?
   - Suggested: Keep in EntityPrefabEditor, Chunk 2 can refactor later

3. What happens if Document is deleted while renderer exists?
   - Suggested: Renderer should be deleted first (TabManager handles this)

4. Should EntityPrefabEditorV2 inherit from EntityPrefabRenderer?
   - Suggested: NO - EntityPrefabRenderer is legacy, EntityPrefabEditorV2 is framework

---

## ✅ Success Criteria

Chunk 2 is complete when:

1. ✅ EntityPrefabEditorV2 created and compiles
2. ✅ Implements all IGraphRenderer methods
3. ✅ New document creates empty canvas
4. ✅ Existing document loads and renders nodes
5. ✅ Canvas operations work (pan, zoom, select, drag)
6. ✅ No build errors or warnings
7. ✅ Manual testing passes all scenarios
8. ✅ DocumentVersionManager can create Framework documents + renderers

---

## 📝 Commit Messages

### When complete, commit with:
```
feat: Phase 2.1 Chunk 2 - EntityPrefabEditorV2 Rendering Layer

- Create EntityPrefabEditorV2 (implements IGraphRenderer)
- Integrate with PrefabCanvas for rendering
- Support end-to-end create → render → interact flow
- Enable Framework strategy to produce renderers
- All nodes, connections, and interactions working
- Build: 0 errors, 0 warnings
- Ready for Chunk 3 (TabManager integration)

RELATED: Phase 2.1, Chunk 1b ✅, Chunk 2 ✅
```

---

```
╔════════════════════════════════════════════════════════╗
║                                                        ║
║  PHASE 2.1 CHUNK 2 - READY TO START                   ║
║                                                        ║
║  Previous Work: ✅ Data Model + Routing System        ║
║  Current Task:  ⏳ Rendering Layer Integration        ║
║  After:         ⏳ TabManager Integration (Chunk 3)   ║
║                                                        ║
║  Effort: ~4.5 hours                                   ║
║  Complexity: Low-Medium                               ║
║  Risk: Low (reuses existing PrefabCanvas)             ║
║                                                        ║
╚════════════════════════════════════════════════════════╝
```

**READY TO PROCEED** 🚀
