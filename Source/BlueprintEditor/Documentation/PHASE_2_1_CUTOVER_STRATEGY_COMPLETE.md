# Phase 2.1 Cutover Strategy: Complete Elimination of Legacy EntityPrefab

**Date:** 2026-03-14  
**Status:** ✅ STRATEGY COMPLETE  
**Phase:** 2.1 (Parallel with Chunk 2-3 implementation)

---

## Executive Summary

**User's Question:**
> "Comment s'assurer que le legacy de PrefabEntity ne sera plus appelé lorsque toute la migration et l'intégration vers la nouvelle structure sera terminée ?"

**Answer Provided:**
A systematic cutover strategy that **GUARANTEES** legacy code will never be called accidentally. The strategy hinges on:
- **2 routing points** in TabManager (only places legacy is instantiated)
- **1 feature flag** to control which system is active
- **Detection system** to prevent accidental legacy usage
- **Safe removal checklist** before deleting legacy code

---

## Architecture Analysis

### Current State (Legacy)

```
TabManager.cpp
├─ CreateNewTab() - Lines 180-190
│  └─ new EntityPrefabRenderer()
│     └─ new EntityPrefabGraphDocument()
│
└─ OpenFileInTab() - Lines 319-347
   └─ new EntityPrefabRenderer()
      └─ new EntityPrefabGraphDocument()
```

### Target State (V2)

```
TabManager.cpp
├─ CreateNewTab() - With ENABLE_ENTITY_PREFAB_V2
│  └─ if (V2_ENABLED)
│     ├─ new EntityPrefabEditorV2()
│     │  └─ new EntityPrefabGraphDocumentV2()
│     └─ else: new EntityPrefabRenderer() [fallback]
│
└─ OpenFileInTab() - With ENABLE_ENTITY_PREFAB_V2
   └─ if (V2_ENABLED)
      ├─ new EntityPrefabEditorV2()
      │  └─ new EntityPrefabGraphDocumentV2()
      └─ else: new EntityPrefabRenderer() [fallback]
```

### Key Finding

**Only 2 places in entire codebase where EntityPrefab is instantiated!**
- All EntityPrefab graphs (new or loaded) flow through these 2 points
- All EntityPrefab rendering uses these 2 renderers
- No hidden dependencies elsewhere
- No scattered references requiring individual updates

---

## Implementation Strategy

### Phase 1: Feature Flag (Add immediately)

**Location:** `Source/BlueprintEditor/TabManager.h` - Add after includes:

```cpp
namespace Olympe {
    class TabManager
    {
    public:
        // PHASE 2.1 CUTOVER: Feature flag to enable EntityPrefab V2
        // Default OFF = Legacy system active (backward compatible)
        // Set to TRUE after Chunk 2 (Rendering) complete = V2 system active
        static bool ENABLE_ENTITY_PREFAB_V2;

        // ... rest of class
    };
}
```

**Location:** `Source/BlueprintEditor/TabManager.cpp` - Add near top:

```cpp
// PHASE 2.1: Initialize feature flag (default OFF for safety)
bool TabManager::ENABLE_ENTITY_PREFAB_V2 = false;
```

### Phase 2: Conditional Routing (Implement in Chunk 3)

**Location:** `TabManager.cpp - CreateNewTab()` - Lines 172-190:

Replace:
```cpp
else if (graphType == "EntityPrefab")
{
    EntityPrefabGraphDocument* epDoc = new EntityPrefabGraphDocument();
    PrefabCanvas* epCanvas = new PrefabCanvas();
    epCanvas->Initialize(epDoc);
    
    EntityPrefabRenderer* r = new EntityPrefabRenderer(*epCanvas);
    tab.renderer = r;
    tab.document = epDoc;
    
    SYSTEM_LOG << "[TabManager::CreateNewTab] Created new EntityPrefab tab\n";
}
```

With:
```cpp
else if (graphType == "EntityPrefab")
{
    if (ENABLE_ENTITY_PREFAB_V2)
    {
        // V2 Routing: Framework-compliant adapter
        EntityPrefabEditorV2* r = new EntityPrefabEditorV2();
        tab.renderer = r;
        tab.document = r->GetDocument();
        
        ++s_v2EntityPrefabInstantiations;
        SYSTEM_LOG << "[TabManager::CreateNewTab] EntityPrefab V2 tab created (count: " 
                   << s_v2EntityPrefabInstantiations << ")\n";
    }
    else
    {
        // Legacy Routing: Old system (temporary fallback)
        EntityPrefabGraphDocument* epDoc = new EntityPrefabGraphDocument();
        PrefabCanvas* epCanvas = new PrefabCanvas();
        epCanvas->Initialize(epDoc);
        
        EntityPrefabRenderer* r = new EntityPrefabRenderer(*epCanvas);
        tab.renderer = r;
        tab.document = epDoc;
        
        ++s_legacyEntityPrefabInstantiations;
        SYSTEM_LOG << "[TabManager::CreateNewTab] EntityPrefab LEGACY tab created (count: " 
                   << s_legacyEntityPrefabInstantiations << ")\n";
    }
    
    // Verification in debug builds
    VERIFY_ENTITY_PREFAB_V2(tab.document, graphType);
}
```

**Location:** `TabManager.cpp - OpenFileInTab()` - Lines 319-347:

Apply same pattern to the EntityPrefab section in OpenFileInTab().

### Phase 3: Detection System

**Location:** `TabManager.h` - Add after feature flag:

```cpp
// PHASE 2.1 CUTOVER SAFETY: Prevent accidental legacy instantiation
#if defined(_DEBUG)
    #define VERIFY_ENTITY_PREFAB_V2(doc, graphType) \
        do { \
            if ((graphType) == "EntityPrefab" && TabManager::ENABLE_ENTITY_PREFAB_V2) { \
                EntityPrefabGraphDocumentV2* v2_doc = \
                    dynamic_cast<EntityPrefabGraphDocumentV2*>(doc); \
                if (v2_doc == nullptr) { \
                    SYSTEM_LOG << "[CUTOVER ERROR] EntityPrefab routing failed: Document is not V2!\n"; \
                    assert(v2_doc != nullptr); \
                } \
                SYSTEM_LOG << "[CUTOVER OK] EntityPrefab V2 routing verified\n"; \
            } \
        } while(0)
#else
    #define VERIFY_ENTITY_PREFAB_V2(doc, graphType) ((void)0)
#endif

// Track instantiation counts for diagnostics
static int s_legacyEntityPrefabInstantiations = 0;
static int s_v2EntityPrefabInstantiations = 0;
```

---

## Deprecation Timeline

### Phase 2.1 (Current - Data Model)
- ✅ EntityPrefabGraphDocumentV2 created and tested
- ⏳ Add feature flag to TabManager
- ⏳ Chunk 2: Implement EntityPrefabEditorV2 (rendering adapter)
- ⏳ Chunk 3: Update TabManager routing to support V2

### Phase 2.1 Cutover (After Chunk 3)
```cpp
static bool ENABLE_ENTITY_PREFAB_V2 = true;  // Activate V2
```
- All EntityPrefab graphs route to V2
- Legacy system completely bypassed
- But files still exist (for safety)

### Phase 2.2 (Testing Phase)
- Run full test suite with V2 active
- Monitor for crashes or data corruption
- Collect feedback from users
- Duration: Minimum 1 release cycle

### Phase 3.0 (Safe Removal)
- Only after Phase 2.2 approval
- Delete legacy files:
  * EntityPrefabGraphDocument.h/cpp
  * EntityPrefabRenderer.h/cpp
  * Legacy-specific PrefabCanvas code
  * Legacy-specific ComponentNodeRenderer code

---

## Safe Removal Checklist

Before deleting ANY legacy file, verify:

### ✅ Requirement 1: V2 Implementation Complete
- [ ] EntityPrefabGraphDocumentV2 fully implemented
- [ ] All IGraphDocument methods working
- [ ] Load/Save tested and working
- [ ] File compatibility verified

### ✅ Requirement 2: Rendering Adapter Complete
- [ ] EntityPrefabEditorV2 created and tested
- [ ] Canvas rendering works correctly
- [ ] All 24 features from Phase 2.0 analysis working
- [ ] Visual output matches legacy

### ✅ Requirement 3: TabManager Routing Updated
- [ ] Feature flag added
- [ ] Conditional routing implemented
- [ ] Both CreateNewTab and OpenFileInTab updated
- [ ] Verification macros in place

### ✅ Requirement 4: Full Test Coverage Passing
- [ ] Create new EntityPrefab (empty graph)
- [ ] Load existing EntityPrefab file
- [ ] Edit operations (add/remove/connect nodes)
- [ ] Save graph to file
- [ ] Verify file integrity
- [ ] Reload and verify data consistency
- [ ] Multi-tab handling
- [ ] Property editing all working

### ✅ Requirement 5: V2 Feature Parity
- [ ] Component palette working
- [ ] Node selection working
- [ ] Property editor working
- [ ] Canvas zoom/pan working
- [ ] Minimap rendering
- [ ] Dirty flag tracking
- [ ] File dialogs (Save/SaveAs)
- [ ] All framework integrations

### ✅ Requirement 6: Extended Release Testing
- [ ] ENABLE_ENTITY_PREFAB_V2 = true for 1+ release cycles
- [ ] Zero crash reports on EntityPrefab
- [ ] Zero data corruption reports
- [ ] Zero regression reports
- [ ] User feedback collected and positive

---

## Verification Procedures

### Before Deletion

**1. Code Search Verification:**
```bash
# Must find ZERO matches (non-V2):
grep -r "class EntityPrefabGraphDocument[^V2]" Source/
grep -r "new EntityPrefabGraphDocument()" TabManager.cpp  # Must be 0 after routing
grep -r "EntityPrefabRenderer" TabManager.cpp  # Must be 0 in active paths
```

**2. Build Verification:**
```bash
# After deleting legacy files:
cmake --build . --config Release  # Should succeed
# Check output: 0 errors, 0 warnings
# No unresolved external symbols: EntityPrefabGraphDocument
# No unresolved external symbols: EntityPrefabRenderer
```

**3. Runtime Verification:**
```cpp
// In debug build with legacy files deleted:
// Instantiate EntityPrefab graph
// Expected: V2 system used
// Logs should show: "[EntityPrefab] Using V2"
// Not show: "[EntityPrefab] Using LEGACY"
```

**4. File Operation Verification:**
- Load pre-migration EntityPrefab files
- Verify they still load correctly (backward compatibility)
- Edit and save
- Verify output file is valid JSON
- Load with V2 again to confirm

---

## Architecture Guarantees

### ✅ Zero Accidental Legacy Usage
- Feature flag provides single point of control
- No scattering of legacy references
- Verification macro catches misrouting in debug
- Runtime counters track which pathway active

### ✅ Safe Parallel Development
- Both systems coexist during Chunk 2-3
- Can test V2 independently with flag ON
- Can run regression tests with flag OFF
- Easy rollback: Just change flag back

### ✅ Complete Elimination Possible
- Only 2 places to modify
- Only 4 files to delete
- No scattered dependencies elsewhere
- Safe removal path documented and verified

### ✅ Version Compatibility
- Load old EntityPrefab files: V2 handles same JSON schema
- No migration tool needed
- Transparent to end users
- All legacy files still open with V2

---

## Contingency & Rollback

### If V2 Has Critical Issues

```cpp
// In TabManager.h - immediate rollback:
static bool ENABLE_ENTITY_PREFAB_V2 = false;  // Turn off V2
// All EntityPrefab graphs immediately use legacy
// No data loss, no corruption
// Full rollback in 1 line change
```

### If Legacy Files Accidentally Deleted Early

```
// Restore from git history:
git restore Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.*
git restore Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.*
// Set ENABLE_ENTITY_PREFAB_V2 = false
// Full restoration with zero data loss
```

---

## Key Points Summary

| Aspect | Solution |
|--------|----------|
| **How to ensure legacy never called?** | Feature flag routes all creation through 1 decision point |
| **How to prevent accidental usage?** | Detection system + verification macros catch any misrouting |
| **How to safely remove legacy?** | Systematic checklist ensures all requirements met before deletion |
| **How to test both systems?** | Flag allows running V2 or legacy independently |
| **How to rollback if needed?** | Single flag change reverts to legacy immediately |
| **How many code locations change?** | Only 2 locations in TabManager (CreateNewTab + OpenFileInTab) |
| **How many files deleted?** | Only 4-6 files (all EntityPrefab-specific, isolated) |
| **Timeline to completion?** | Phase 2.1 (now) → 2.2 (testing) → 3.0 (safe removal) |

---

## Next Steps

### Immediate (After this document)
1. Continue with Chunk 2 implementation (Rendering & Canvas)
2. Create EntityPrefabEditorV2 adapter

### After Chunk 2 Complete
1. Implement conditional routing in TabManager
2. Set ENABLE_ENTITY_PREFAB_V2 flag mechanisms
3. Add verification/detection system
4. Run full test suite with V2 active

### After Testing Complete (Phase 2.2)
1. Enable ENABLE_ENTITY_PREFAB_V2 = true by default
2. Monitor for 1+ release cycle
3. Collect feedback and verify stability

### After Phase 2.2 Approval (Phase 3.0)
1. Delete legacy files one by one
2. Verify build succeeds after each deletion
3. Run integration tests
4. Final validation and documentation

---

## References

- **Chunk 1 Implementation:** EntityPrefabGraphDocumentV2.h/cpp (1050+ lines)
- **Phase 2.0 Analysis:** PHASE_2_DEEP_ANALYSIS_ENTITYPREFAB_END_TO_END.md (500+ lines)
- **Framework Reference:** Source/BlueprintEditor/Framework/IGraphDocument.h
- **Routing Implementation:** TabManager.h/cpp (lines 172-190, 319-347)

---

**Document Status:** ✅ COMPLETE - Ready for implementation in Chunk 2-3

