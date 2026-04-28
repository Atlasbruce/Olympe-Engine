# PHASE 44.4 - FRAMEWORK INITIALIZATION TRACE

## Current Architecture

All three graph editors use the same framework pattern:

### BehaviorTreeRenderer
```cpp
// Line 28: Constructor
m_document = std::make_unique<BehaviorTreeGraphDocument>(this);
m_framework = std::make_unique<CanvasFramework>(m_document.get());

// Line 125-129: Render() method
if (m_framework && m_framework->GetToolbar())
{
    m_framework->GetToolbar()->Render();
}
```

### VisualScriptEditorPanel
- **Question**: Does VisualScriptEditorPanel create and pass document to framework?
- **Status**: Need to check - might be TabManager's responsibility

### EntityPrefabRenderer
- **Question**: Does EntityPrefabRenderer create and pass document to framework?
- **Status**: Need to check

## The Key Question: WHO Creates the Framework for VisualScript and EntityPrefab?

### Theory 1: Each renderer creates its own
- BehaviorTreeRenderer does (we see it line 47-48)
- VisualScriptEditorPanel should do same
- EntityPrefabRenderer should do same
- **Issue**: VisualScriptEditorPanel might not be creating framework

### Theory 2: TabManager creates shared framework
- TabManager could create one CanvasFramework per document
- Pass it to all renderers
- **Issue**: Would need to be created at right lifecycle point

### Theory 3: Renderer only created for BehaviorTree
- Other editors still use legacy UI
- Framework only partially integrated
- **Issue**: Would explain why buttons don't work for all editors

## Investigation Output

We need to search for:

1. **In VisualScriptEditorPanel.cpp**:
   - Is `m_framework` member created? Search: "m_framework"
   - Is `m_document` member created? Search: "m_document"
   - Does Render() call framework? Search: "RenderModals" OR "toolbar"

2. **In EntityPrefabRenderer.cpp**:
   - Same three questions

3. **In TabManager.cpp**:
   - Does it create framework for loaded editors? Search: "CanvasFramework"

## Critical Path for Save Button

```
User clicks [Save]
  ↓
BehaviorTreeRenderer::RenderLayoutWithTabs() shows toolbar
  ↓
m_framework->GetToolbar()->Render() calls CanvasToolbarRenderer::RenderButtons()
  ↓
[Save] button visible and enabled (if dirty)
  ↓
User clicks button
  ↓
ImGui::Button("Save") returns true
  ↓
OnSaveClicked() called
  ↓
m_document->Save() called (= BehaviorTreeGraphDocument::Save())
  ↓
NodeGraphManager::SaveGraph() called
  ↓
File written!
```

**For VisualScript**, same path but:
- m_framework might be missing (never created)
- OR VisualScriptGraphDocument::Save() has different backend
- Check: Does VisualScriptEditorPanel::SaveAs() actually write file?

**For EntityPrefab**, same path but:
- m_framework might be missing
- Check: Does EntityPrefabGraphDocument exist?

## What the user should test FIRST

Before we dig deeper, user should test:

1. **Does BehaviorTree Save button work?**
   - Create new BT
   - Add node
   - Click [Save]
   - Does output show SUCCESS?
   - Does file exist?
   - **If YES**: Framework works for BehaviorTree ✓
   - **If NO**: Problem in BT save path

2. **Do buttons even appear for other editors?**
   - Create new VisualScript
   - Look at toolbar - do [Save] [SaveAs] [Browse] buttons appear?
   - **If NO**: Framework not initialized for VisualScript
   - **If YES but grayed**: Dirty flag not being set
   - **If YES and enabled but doesn't work**: Backend issue

3. **Does clicking ANY button anywhere show ANY logs?**
   - Open Output window
   - Click ANYTHING - [Save], [SaveAs], [Browse]
   - Filter for "CanvasToolbar" or "GraphDocument"
   - **If nothing appears**: Buttons not wired up
   - **If logs appear but say ERROR**: Something is null/broken
   - **If logs show SUCCESS but file missing**: Backend issue

## Files That Need Verification

1. **VisualScriptEditorPanel.h / .cpp**
   - ✅ Has SaveAs() method (we saw it in _FileOperations)
   - ❓ Has m_framework member?
   - ❓ Creates VisualScriptGraphDocument?
   - ❓ Renders framework toolbar?

2. **EntityPrefabRenderer.h / .cpp**
   - ❓ Has m_framework member?
   - ✅ EntityPrefabGraphDocument exists (we found it)
   - ❓ Renders framework toolbar?
   - ❓ Has Save/SaveAs methods?

3. **TabManager.h / .cpp**
   - ❓ Does it create frameworks when opening editors?
   - ❓ Does it pass framework to renderers?

## Hypothesis - Most Likely Issue

**Hypothesis**: Framework works for BehaviorTree but not the other editors.

**Evidence**:
- BehaviorTreeRenderer clearly creates m_framework (we see code)
- VisualScriptEditorPanel might not create m_framework
- EntityPrefabRenderer might not create m_framework
- User might have tested VisualScript/EntityPrefab, found buttons don't work
- Reported "save buttons broken for all graphs"
- But actually only broken for non-BehaviorTree graphs

**If this is correct**, fix is:
1. Add m_framework to VisualScriptEditorPanel
2. Add m_framework to EntityPrefabRenderer
3. Both create document adapters same way BehaviorTree does
4. Both render framework toolbar like BehaviorTree does

**Result**: Buttons work for ALL three editors

## Next Steps

User should:
1. Test BehaviorTree save specifically - see if THAT works
2. Report exact logs from Output window
3. Tell us which editor type the buttons DON'T work in

We should:
1. Check VisualScriptEditorPanel for m_framework
2. Check EntityPrefabRenderer for m_framework
3. If missing, add them (5-minute fix per editor)
4. Verify all three editors now have working save

## Estimated Time to Full Resolution

- **Testing**: 5 minutes (user runs test, copies logs)
- **Investigation**: 5 minutes (we check VR Script & EntityPrefab)
- **Implementation** (if needed): 15-30 minutes (add framework to other editors)
- **Final testing**: 10 minutes
- **Total**: 35-50 minutes

**Critical**: Framework itself works fine. Just need to verify it's being used by all three editors.
