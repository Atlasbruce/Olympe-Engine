/**
 * @file FRAMEWORK_INTEGRATION_GUIDE.md
 * @brief Phase 41 Framework Integration Implementation Guide
 * @author Olympe Engine
 * @date 2026-03-25
 *
 * Complete guide for integrating CanvasFramework into existing graph editors
 */

# Framework Integration Guide - Phase 41

## Architecture Overview

The unified framework consists of four main components:

1. **IGraphDocument** - Abstract interface for all graph types
   - Location: `Source/BlueprintEditor/Framework/IGraphDocument.h`
   - Enables polymorphic document handling
   - Implemented by: EntityPrefabGraphDocument, VisualScriptGraphDocument, BehaviorTreeGraphDocument

2. **CanvasToolbarRenderer** - Unified toolbar for Save/SaveAs/Browse
   - Location: `Source/BlueprintEditor/Framework/CanvasToolbarRenderer.h/cpp`
   - Works with any IGraphDocument
   - Manages modal state and routing

3. **CanvasFramework** - Central orchestrator
   - Location: `Source/BlueprintEditor/Framework/CanvasFramework.h/cpp`
   - Coordinates toolbar, modals, and canvas rendering
   - Provides document lifecycle management

4. **Graph Document Adapters** - Implementations of IGraphDocument
   - EntityPrefabGraphDocument: Direct inheritance
   - VisualScriptGraphDocument: Adapter wrapper
   - BehaviorTreeGraphDocument: Adapter wrapper

## Integration Checklist

### Phase 1: EntityPrefabRenderer Integration (Priority: HIGH)

**File to Modify:** `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.h`

```cpp
// Add to includes
#include "../Framework/CanvasFramework.h"
#include "../Framework/IGraphDocument.h"
```

**Add member variables:**
```cpp
private:
    CanvasFramework* m_framework;
    GraphDocumentPtr m_document;  // IGraphDocument instance
```

**Update constructor:**
```cpp
// In constructor, create document and framework
m_document = EntityPrefabGraphDocument::CreateNew();  // Or FromJson()
m_framework = new CanvasFramework(m_document.get());
```

**Update Render() method:**
```cpp
void EntityPrefabRenderer::Render() override
{
    if (!m_framework) {
        ImGui::Text("Framework not initialized");
        return;
    }
    
    // Let framework handle all rendering (toolbar + canvas + modals)
    m_framework->Render();
}
```

**Update destruction:**
```cpp
// In destructor
if (m_framework) {
    delete m_framework;
    m_framework = nullptr;
}
```

### Phase 2: VisualScriptEditorPanel Integration (Priority: MEDIUM)

**File to Modify:** `Source/BlueprintEditor/VisualScriptEditorPanel.h`

```cpp
// Add to includes
#include "Framework/CanvasFramework.h"
#include "Framework/VisualScriptGraphDocument.h"
```

**Add member:**
```cpp
private:
    CanvasFramework* m_framework;
    GraphDocumentPtr m_document;  // IGraphDocument wrapper
```

**Key Challenge:** VisualScriptEditorPanel uses imnodes which has custom rendering.
**Solution:** CanvasFramework.RenderCanvas() should check renderer type and delegate appropriately.

### Phase 3: BehaviorTreeRenderer Integration (Priority: MEDIUM)

**File to Modify:** `Source/BlueprintEditor/BehaviorTreeRenderer.h`

Similar pattern to VisualScriptEditorPanel (imnodes-based rendering).

### Phase 4: TabManager Refactoring (Priority: LOW)

**File to Modify:** `Source/BlueprintEditor/TabManager.h/cpp`

Currently TabManager creates/manages individual panels. With framework:

```cpp
// Instead of:
VisualScriptEditorPanel* panel = new VisualScriptEditorPanel();
TabManager::AddTab("MyScript", panel);

// New approach:
GraphDocumentPtr doc = VisualScriptGraphDocument::Create(panel);
CanvasFramework framework(doc.get());
TabManager::AddTab("MyScript", framework);
```

## Implementation Pattern

### For Direct Inheritance (EntityPrefab)

```cpp
// EntityPrefabGraphDocument already inherits IGraphDocument
// Simply create it and pass to framework:

GraphDocumentPtr doc = EntityPrefabGraphDocument::FromJson(jsonData);
CanvasFramework framework(doc.get());

// In render loop:
framework.Render();  // Handles everything
```

### For Adapter Wrappers (VisualScript, BehaviorTree)

```cpp
// Create document adapter:
VisualScriptGraphDocument* adapter = new VisualScriptGraphDocument(editorPanel);

// Create framework:
CanvasFramework framework(adapter);

// In render loop:
framework.Render();  // Handles everything
```

## Modal Integration

### Save Operation Flow

```
User clicks [Save] button
    ↓
CanvasToolbarRenderer::OnSaveClicked()
    ↓
If no path: OpenSaveAsDialog()
Else: ExecuteSave(currentPath)
    ↓
IGraphDocument::Save(filePath)
    ↓
Document-specific save logic
    ↓
CanvasToolbarRenderer callback triggered
    ↓
Framework status bar updated
```

### SaveAs Operation Flow

```
User clicks [SaveAs] button
    ↓
CanvasToolbarRenderer::OnSaveAsClicked()
    ↓
Open SaveFilePickerModal with folder panel
    ↓
User selects location and filename
    ↓
CanvasToolbarRenderer::OnSaveAsComplete()
    ↓
IGraphDocument::Save(filePath)
    ↓
CanvasFramework callback triggered
    ↓
Framework status bar: "Saved: path"
```

### Browse Operation Flow

```
User clicks [Browse] button
    ↓
CanvasToolbarRenderer::OnBrowseClicked()
    ↓
Open FilePickerModal
    ↓
User selects file
    ↓
CanvasToolbarRenderer::OnBrowseComplete()
    ↓
CanvasFramework::OnToolbarBrowseComplete()
    ↓
IGraphDocument::Load(filePath)
    ↓
Document loads and displays
    ↓
Framework status bar: "Loaded: path"
```

## Testing Checklist

- [ ] EntityPrefab editor shows unified toolbar
- [ ] VisualScript editor shows unified toolbar
- [ ] BehaviorTree editor shows unified toolbar
- [ ] Save button disabled when not dirty
- [ ] SaveAs opens modal with folder panel (LEFT) + files (RIGHT)
- [ ] Browse opens file picker
- [ ] Save/SaveAs complete successfully
- [ ] Browse loads document correctly
- [ ] Document path displayed in toolbar
- [ ] Dirty flag tracked correctly across types
- [ ] No regressions in existing functionality
- [ ] All builds pass (0 errors)

## File Structure After Integration

```
Source/BlueprintEditor/
├── Framework/                          [NEW]
│   ├── IGraphDocument.h               [NEW]
│   ├── CanvasFramework.h/cpp          [NEW]
│   ├── CanvasToolbarRenderer.h/cpp    [NEW]
│   ├── VisualScriptGraphDocument.h/cpp [NEW]
│   └── BehaviorTreeGraphDocument.h/cpp [NEW]
├── VisualScriptEditorPanel.h/cpp      [MODIFIED - integrate framework]
├── BehaviorTreeRenderer.h/cpp         [MODIFIED - integrate framework]
├── EntityPrefabEditor/
│   ├── EntityPrefabRenderer.h/cpp     [MODIFIED - integrate framework]
│   └── EntityPrefabGraphDocument.h/cpp [MODIFIED - inherit IGraphDocument]
└── TabManager.h/cpp                   [MODIFIED - use polymorphic interface]
```

## Expected Behavior After Integration

### Save Operation
1. User edits graph
2. Graph marked dirty
3. [Save] button becomes enabled
4. User clicks [Save]
5. If path empty → SaveAs dialog opens
6. If path set → Saves immediately
7. Toolbar shows: "File: path/file.json" (green if saved)

### SaveAs Operation
1. User clicks [SaveAs]
2. SaveFilePickerModal opens
3. LEFT panel shows folders
4. RIGHT panel shows files
5. User navigates and selects location
6. User enters filename
7. User clicks OK
8. File saved
9. Toolbar updated with new path

### Browse Operation
1. User clicks [Browse]
2. FilePickerModal opens
3. User selects file
4. User clicks Select
5. Document loaded
6. Toolbar shows new file path
7. All unsaved changes discarded

## Future Enhancements

- [ ] Undo/Redo system
- [ ] Recent files menu
- [ ] Copy/Paste across documents
- [ ] Search within graph
- [ ] Minimap in all editors
- [ ] Custom theme support
- [ ] Multi-document tabs using framework

## Common Issues & Solutions

**Issue:** "C2338: Modal not opening"
**Solution:** Verify DataManager::OpenSaveFilePickerModal() is called in render loop

**Issue:** "Graph not loading after Browse"
**Solution:** Check IGraphDocument::Load() implementation for document type

**Issue:** "Toolbar buttons not visible"
**Solution:** Ensure CanvasFramework::Render() is called in render loop

**Issue:** "Save/SaveAs buttons grayed out"
**Solution:** Verify IGraphDocument::IsDirty() returns correct value

## Integration Timeline Estimate

- EntityPrefab: 30 minutes
- VisualScript: 45 minutes
- BehaviorTree: 45 minutes
- TabManager refactoring: 60 minutes
- Testing & fixes: 60 minutes
- **Total: ~4 hours**

---

**Framework Status:** ✅ Complete and ready for integration
**Build Status:** ✅ Clean (0 errors)
**Next Action:** Begin EntityPrefabRenderer integration (STEP 11)
