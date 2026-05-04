# PHASE 58: ENTITYPREFAB FRAMEWORK INTEGRATION - IMPLEMENTATION GUIDE

## QUICK REFERENCE: Root Cause & Solution

**Root Cause**: 55 linker errors because EntityPrefabRenderer architecture doesn't match Framework expectations
- EntityPrefabRenderer doesn't own document or canvas  
- GetDocument() returns null pointer
- ComponentNodeRenderer never initialized
- TabManager can't access document via renderer

**Solution**: Refactor EntityPrefabRenderer to own all resources and follow proven patterns

---

## IMPLEMENTATION STEPS (In Priority Order)

### STEP 1: Refactor EntityPrefabRenderer Constructor & Ownership

**File**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.h`

**Current Code**:
```cpp
class EntityPrefabRenderer : public IGraphRenderer
{
private:
    PrefabCanvas& m_canvas;  // Reference - WRONG!
    // No document member - WRONG!
```

**Required Changes**:
1. Remove PrefabCanvas reference parameter from constructor
2. Add unique_ptr members for document and canvas
3. Initialize both in constructor

**Steps**:
- Line 24: Change constructor signature from `EntityPrefabRenderer(PrefabCanvas& canvas)` to `EntityPrefabRenderer()`
- Line 44: Remove `PrefabCanvas& m_canvas;` reference
- Add new line after 44: `std::unique_ptr<PrefabCanvas> m_canvas;`
- Add new line: `std::unique_ptr<EntityPrefabGraphDocument> m_document;`

**New Code Structure**:
```cpp
class EntityPrefabRenderer : public IGraphRenderer
{
public:
    explicit EntityPrefabRenderer();  // No parameters
    ~EntityPrefabRenderer();

private:
    std::unique_ptr<PrefabCanvas> m_canvas;              // OWN IT
    std::unique_ptr<EntityPrefabGraphDocument> m_document;  // OWN IT
    // ... rest of members unchanged ...
};
```

---

### STEP 2: Update EntityPrefabRenderer Constructor Implementation

**File**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp`

**Current Code** (Find in file):
```cpp
EntityPrefabRenderer::EntityPrefabRenderer(PrefabCanvas& canvas)
    : m_canvas(canvas)
    , m_componentPalette()
    , m_propertyEditor()
    , m_framework(std::make_unique<CanvasFramework>())
    , m_filePath("")
    , m_isDirty(false)
{
    // Phase 37 — Minimap visibility setup
    m_minimapVisible = true;
    // ... rest of init
}
```

**Required Changes**:
1. Change constructor signature to no parameters
2. Initialize m_document and m_canvas with make_unique
3. Call m_canvas->Initialize(m_document.get())

**New Code**:
```cpp
EntityPrefabRenderer::EntityPrefabRenderer()
    : m_canvas(std::make_unique<PrefabCanvas>())
    , m_document(std::make_unique<EntityPrefabGraphDocument>())
    , m_componentPalette()
    , m_propertyEditor()
    , m_framework(std::make_unique<CanvasFramework>())
    , m_filePath("")
    , m_isDirty(false)
{
    // Initialize canvas with document
    m_canvas->Initialize(m_document.get());
    
    // Phase 37 — Minimap visibility setup
    m_minimapVisible = true;
    // ... rest unchanged ...
}
```

---

### STEP 3: Fix GetDocument() Implementation

**File**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp`

**Current Code** (Find):
```cpp
// Currently this method is likely:
// - Not implemented, OR
// - Returns nullptr, OR
// - Returns some other reference
```

**New Implementation**:
```cpp
IGraphDocument* EntityPrefabRenderer::GetDocument()
{
    return m_document.get();  // Return owned document (no longer nullptr!)
}
```

**Why**: TabManager expects renderer->GetDocument() to return valid document. Currently returns null because EntityPrefabRenderer didn't own document.

---

### STEP 4: Update Render() to Use Owned Canvas

**File**: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp`

**Find**: Current Render() implementation

**Change**: Any reference from `m_canvas` (reference) to `m_canvas.get()` (pointer from unique_ptr)

**Example**:
```cpp
// Before:
m_canvas.Render();  // Reference syntax

// After:
m_canvas->Render();  // Pointer syntax (get() is implicit in ->)
```

Likely locations:
- `RenderLayoutWithTabs()` method
- Any call to `m_canvas` member

---

### STEP 5: Fix PrefabCanvas::Initialize Initialization Gap

**File**: `Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.h`

**Current Code** (Lines 38-46):
```cpp
void Initialize(EntityPrefabGraphDocument* document)
{
    m_document = document;
    // PHASE 52 FIX is incomplete - m_renderer never created!
    if (!m_renderer)
    {
        m_renderer = std::make_unique<ComponentNodeRenderer>();
    }
}
```

**Validation**: Check if m_renderer initialization is already present. If NOT:

```cpp
void Initialize(EntityPrefabGraphDocument* document)
{
    m_document = document;
    
    // PHASE 52 FIX: Initialize ComponentNodeRenderer for rendering nodes
    if (!m_renderer)
    {
        m_renderer = std::make_unique<ComponentNodeRenderer>();
    }
}
```

**If already present**: No change needed for this step.

---

### STEP 6: Update TabManager EntityPrefab Creation Path

**File**: `Source/BlueprintEditor/TabManager.cpp`

**Find**: CreateNewTab() method EntityPrefab path (around line ~180)

**Current Code**:
```cpp
if (graphType == "EntityPrefab")
{
    // PHASE 54 FIX: Allocate document and canvas on heap per tab
    EntityPrefabGraphDocument* epDoc = new EntityPrefabGraphDocument();
    PrefabCanvas* epCanvas = new PrefabCanvas();
    epCanvas->Initialize(epDoc);

    EntityPrefabRenderer* r = new EntityPrefabRenderer(*epCanvas);
    tab.renderer = r;
    tab.document = epDoc;

    SYSTEM_LOG << "[TabManager::CreateNewTab] Created new EntityPrefab tab with heap-allocated document\n";
}
```

**New Code**:
```cpp
else if (graphType == "EntityPrefab")
{
    // PHASE 58 FIX: Renderer owns all resources (document + canvas)
    // Matches pattern: VisualScriptRenderer, BehaviorTreeRenderer ownership model
    EntityPrefabRenderer* r = new EntityPrefabRenderer();
    r->CreateNew(nameSS.str());  // If CreateNew method exists
    tab.renderer = r;
    tab.document = r->GetDocument();  // Get owned document from renderer

    SYSTEM_LOG << "[TabManager::CreateNewTab] Created new EntityPrefab tab via renderer\n";
}
```

---

### STEP 7: Update TabManager OpenFileInTab EntityPrefab Path

**File**: `Source/BlueprintEditor/TabManager.cpp`

**Find**: OpenFileInTab() method EntityPrefab path (around line ~310-350)

**Current Code**:
```cpp
else if (graphType == "EntityPrefab")
{
    // PHASE 2.1 CHUNK 3: Use DocumentVersionManager + EntityPrefabEditorV2 Framework
    DocumentVersionManager& docManager = DocumentVersionManager::Get();

    SYSTEM_LOG << "[TabManager::OpenFileInTab] Loading EntityPrefab via DocumentVersionManager\n";

    IGraphDocument* doc = docManager.LoadDocument("EntityPrefab", filePath);
    // ... etc
    
    tab.renderer = renderer;
    tab.document = doc;
}
```

**New Code** (simpler, matches VisualScript pattern):
```cpp
else if (graphType == "EntityPrefab")
{
    // PHASE 58 FIX: Simple pattern - renderer owns document
    // Matches VisualScript/BehaviorTree pattern (no DocumentVersionManager complexity)
    EntityPrefabRenderer* r = new EntityPrefabRenderer();
    SYSTEM_LOG << "[TabManager::OpenFileInTab] EntityPrefabRenderer created for tab: " << tab.tabID << "\n";

    if (!r->Load(filePath))
    {
        SYSTEM_LOG << "[TabManager::OpenFileInTab] ERROR: EntityPrefabRenderer::Load() failed for " << filePath << "\n";
        delete r;
        return "";
    }
    SYSTEM_LOG << "[TabManager::OpenFileInTab] EntityPrefabRenderer::Load() SUCCESS\n";

    tab.renderer = r;
    tab.document = r->GetDocument();  // Renderer owns and provides document
}
```

**Why Simpler**: EntityPrefabRenderer now follows same pattern as VisualScriptRenderer/BehaviorTreeRenderer. No DocumentVersionManager complexity needed.

---

### STEP 8: Update EditorTab Destructor

**File**: `Source/BlueprintEditor/TabManager.h`

**Current Code** (Lines 64-83):
```cpp
~EditorTab()
{
    // PHASE 56 FIX: Conditional document deletion prevents double-delete
    // For EntityPrefab: tab owns document
    if (document && graphType == "EntityPrefab")
    {
        delete document;
        document = nullptr;
    }

    if (renderer)
    {
        delete renderer;
        renderer = nullptr;
    }
}
```

**New Code** (simplified - EntityPrefab now same as others):
```cpp
~EditorTab()
{
    // PHASE 58 FIX: ALL types now follow same pattern
    // Document is owned by renderer (via unique_ptr)
    // Renderer destructor handles document cleanup
    // No need to special-case EntityPrefab!
    
    if (renderer)
    {
        delete renderer;  // Renderer deletes its own document + canvas
        renderer = nullptr;
    }
    
    // Don't delete document - it's owned by renderer's unique_ptr
    document = nullptr;
}
```

**Move Assignment Operator** (Lines 104-131): Also update to remove special case:

```cpp
EditorTab& operator=(EditorTab&& other) noexcept
{
    if (this != &other)
    {
        // Clean up existing resources
        if (renderer) { delete renderer; renderer = nullptr; }
        // PHASE 58 FIX: No special case for EntityPrefab anymore
        // Document handled by renderer destructor

        // Transfer ownership
        tabID = std::move(other.tabID);
        displayName = std::move(other.displayName);
        filePath = std::move(other.filePath);
        graphType = std::move(other.graphType);
        isDirty = other.isDirty;
        isActive = other.isActive;
        renderer = other.renderer;
        document = other.document;

        // Nullify source
        other.renderer = nullptr;
        other.document = nullptr;
    }
    return *this;
}
```

---

### STEP 9: Verify PrefabLoader Symbol Visibility

**File**: `Source/BlueprintEditor/EntityPrefabEditor/PrefabLoader.h`

**Check**: Are PrefabLoader methods properly declared?

```cpp
// Should have public static methods:
public:
    static bool LoadJsonFromFile(const std::string& filepath, json& output);
    static bool SaveJsonToFile(const std::string& filepath, const json& data);
    // ... etc
```

**If declarations look good**: Next verify PrefabLoader.cpp is in vcxproj (it should be after earlier fix).

---

### STEP 10: Verify ComponentNodeRenderer Complete Implementation

**File**: `Source/BlueprintEditor/EntityPrefabEditor/ComponentNodeRenderer.h` and `.cpp`

**Check**: Ensure all methods have implementations (not stubs):

```cpp
// Should be fully implemented:
void RenderNodes(EntityPrefabGraphDocument* document);
void RenderConnections(EntityPrefabGraphDocument* document);
void RenderNodePorts(const ComponentNode& node);
float GetDistanceToConnection(const Vector& point, const ComponentNode& source, const ComponentNode& target);
// ... etc
```

**If any method is a stub** (like `{ }`): Implement it properly.

---

## STEP 11: Build and Verify

**Command**: Build solution
```powershell
msbuild OlympeBlueprintEditor.sln /p:Configuration=Debug /p:Platform=x64
```

**Expected Outcome**:
- ✅ 0 Compilation Errors
- ✅ Reduced linker errors (should eliminate EntityPrefab-related ones)

**If linker errors remain**: 
- Check which symbols still unresolved (likely PrefabLoader or ComponentNodeRenderer)
- Verify those files are in vcxproj
- Verify implementations are complete (not stubs)

---

## STEP 12: Runtime Verification

**Test**: Load EntityPrefab file
1. Open editor
2. Double-click an EntityPrefab .json file
3. Verify: Tab appears with filename
4. Verify: Nodes render on canvas
5. Verify: Pan/zoom works
6. Verify: Save functionality works

**Expected**: All features work end-to-end

---

## FILE MODIFICATION CHECKLIST

- [ ] EntityPrefabRenderer.h - Updated constructor signature + added unique_ptr members
- [ ] EntityPrefabRenderer.cpp - Updated constructor implementation + added GetDocument()
- [ ] EntityPrefabRenderer.cpp - Updated Render() to use pointer syntax
- [ ] PrefabCanvas.h - Verified m_renderer initialized in Initialize()
- [ ] TabManager.cpp - Updated CreateNewTab EntityPrefab path
- [ ] TabManager.cpp - Updated OpenFileInTab EntityPrefab path
- [ ] TabManager.h - Updated EditorTab destructor (removed special case)
- [ ] TabManager.h - Updated EditorTab move assignment (removed special case)
- [ ] PrefabLoader.h/cpp - Verified symbol visibility
- [ ] ComponentNodeRenderer.h/cpp - Verified implementations complete
- [ ] Solution builds: 0 errors (or only unrelated errors)
- [ ] EntityPrefab files open correctly
- [ ] Canvas renders nodes
- [ ] Nodes can be manipulated

---

## WHY THIS WORKS

**Current Problem**:
```
TabManager calls: renderer->GetDocument()
EntityPrefabRenderer returns: nullptr
Result: Tab system breaks, linker can't resolve symbols
```

**After Fix**:
```
TabManager calls: renderer->GetDocument()
EntityPrefabRenderer returns: m_document.get() (valid pointer!)
Result: Tab system works, all symbols resolve, no linker errors
```

**Key Pattern Alignment**:
- EntityPrefabRenderer now matches VisualScriptRenderer pattern
- Both own their document and canvas
- Both return valid document via GetDocument()
- TabManager treats EntityPrefab same as all other types
- No special cases needed

---

## RISK ASSESSMENT

**Risk Level**: ⭐ LOW
- Changes isolated to EntityPrefab layer
- No Framework interface changes
- Follows proven patterns from VisualScript/BehaviorTree
- All changes are internal to renderer

**Test Coverage**: 
- Unit: EntityPrefabRenderer ownership tests
- Integration: Tab system EntityPrefab paths
- End-to-end: Load/save/canvas rendering

**Rollback**: Simple reversal of these changes (revert to referenced ownership model)

---

## TIMELINE

**Estimated Implementation**: 2-3 hours
- Code changes: 30-45 min
- Build/debugging: 30-45 min
- Testing: 15-30 min
- Documentation: 15-20 min

**Critical Path**:
1. EntityPrefabRenderer ownership refactor (30 min)
2. TabManager path updates (15 min)
3. EditorTab destructor fix (10 min)
4. Build and resolve remaining errors (30-60 min)
5. Runtime testing (15-30 min)
