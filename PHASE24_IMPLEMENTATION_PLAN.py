#!/usr/bin/env python3
# PHASE24_IMPLEMENTATION_PLAN.py
# Detailed actionable plan with exact code locations and diffs

"""
PHASE 24 - FINAL INTEGRATION PLAN
This document is a CHECKLIST with exact code locations for the 3 final integrations.

TIME ESTIMATE: 2-3 hours total
DIFFICULTY: Easy (no complex logic, just wiring)
RISK: Low (changes are isolated, tests verify everything)

============================================================================
INTEGRATION 1: ADD DISPATCHER FOR BRANCH NODES
============================================================================

LOCATION: Source\BlueprintEditor\VisualScriptEditorPanel.cpp
SECTION: RenderCanvas() method, lines ~1765

WHAT TO DO:
  Add a check before the generic VisualScriptNodeRenderer::RenderNode() call.
  If node type is Branch, use the specialized NodeBranchRenderer instead.

BEFORE (current code, lines 1765-1788):
  VisualScriptNodeRenderer::RenderNode(
      eNode.nodeID,
      eNode.nodeID,
      0 /* graphID placeholder */,
      eNode.def,
      hasBreakpoint,
      isActive,
      execIn, execOut,
      dataIn, dataOut,
      [](int nid, void* ud) { ... },
      this,
      [](int nid, int dynIdx, void* ud) { ... },
      this);

AFTER:
  // Check if this is a Branch node — if so, use specialized renderer
  if (eNode.def.Type == TaskNodeType::Branch) {
      // Create NodeBranchData from TaskNodeDefinition
      NodeBranchData branchData;
      branchData.nodeID = eNode.nodeID;
      branchData.nodeName = eNode.def.NodeName;
      branchData.conditionRefs = eNode.def.conditionRefs;
      branchData.dynamicPins = eNode.def.dynamicPins;
      branchData.breakpoint = hasBreakpoint;
      
      // Render via NodeBranchRenderer (4-section layout)
      m_branchRenderer.RenderNode(branchData);
  } else {
      // Use generic renderer for all other node types
      VisualScriptNodeRenderer::RenderNode(
          eNode.nodeID,
          eNode.nodeID,
          0 /* graphID placeholder */,
          eNode.def,
          hasBreakpoint,
          isActive,
          execIn, execOut,
          dataIn, dataOut,
          [](int nid, void* ud) { ... },
          this,
          [](int nid, int dynIdx, void* ud) { ... },
          this);
  }

TESTING:
  [ ] Build: Run `build.cmd` or VS Build
  [ ] Create a new Branch node in canvas
  [ ] Verify node shows 4-section layout (title/exec/conditions/pins)
  [ ] Verify node does NOT crash on hover/click
  [ ] Verify conditions preview renders correctly
  [ ] Verify dynamic pins show correctly

TIME: 15 minutes


============================================================================
INTEGRATION 2: ADD CONDITION PRESET LIBRARY PANEL TO UI
============================================================================

LOCATION: Source\BlueprintEditor\VisualScriptEditorPanel.h / .cpp

STEP 1: Add member variable (VisualScriptEditorPanel.h, after m_conditionsPanel)

BEFORE (in private section):
  std::unique_ptr<NodeConditionsPanel> m_conditionsPanel;
  // ... other members ...

AFTER:
  std::unique_ptr<NodeConditionsPanel> m_conditionsPanel;
  std::unique_ptr<ConditionPresetLibraryPanel> m_libraryPanel;  // ← ADD THIS


STEP 2: Initialize in VisualScriptEditorPanel::Initialize() 

LOCATION: Source\BlueprintEditor\VisualScriptEditorPanel.cpp::Initialize()
AFTER line where m_conditionsPanel is created (~85):

  m_conditionsPanel = std::unique_ptr<NodeConditionsPanel>(
                          new NodeConditionsPanel(m_presetRegistry));

ADD BELOW IT:

  m_libraryPanel = std::unique_ptr<ConditionPresetLibraryPanel>(
                          new ConditionPresetLibraryPanel(m_presetRegistry));


STEP 3: Add button to toolbar (RenderToolbar method, lines ~1450)

BEFORE (after "Save" button):
  if (ImGui::Button("Save"))
      Save();

ADD AFTER IT:

  ImGui::SameLine();
  if (ImGui::Button("Condition Presets")) {
      m_libraryPanel->Open();
  }


STEP 4: Call render in main loop (Render method)

LOCATION: Source\BlueprintEditor\VisualScriptEditorPanel.cpp::Render()
AFTER line 1387 (ImGui::End()):

  ImGui::Begin("VS Graph Editor", &m_visible);
  RenderContent();
  ImGui::End();
  
  // ← ADD HERE:
  m_libraryPanel->Render();


TESTING:
  [ ] Build project
  [ ] Look for "Condition Presets" button in toolbar
  [ ] Click button → panel should appear
  [ ] Create new preset in panel
  [ ] Close panel
  [ ] Verify preset appears in NodeConditionsPanel dropdown
  [ ] Test duplicate/delete operations
  [ ] Verify save/load preserves presets

TIME: 20 minutes


============================================================================
INTEGRATION 3: INTEGRATE PRESET DROPDOWN HELPER (OPTIONAL)
============================================================================

LOCATION: Source\Editor\Panels\NodeConditionsPanel.cpp
SECTION: RenderConditionList() method

WHY: Reduce code duplication, consistent UI across panels

WHAT TO DO:
  Replace inline dropdown code with PresetDropdownHelper calls

BEFORE (lines ~528-565 in RenderConditionList):
  if (ImGui::Button("[+ Add Condition]", ImVec2(-1.f, 0.f)))
      ImGui::OpenPopup("##AddCondPopup");

  if (ImGui::BeginPopup("##AddCondPopup")) {
      // ... inline dropdown logic ...
  }

AFTER:
  // Use PresetDropdownHelper for consistent dropdown behavior
  static PresetDropdownHelper addConditionDropdown(m_registry);
  
  if (ImGui::Button("[+ Add Condition]", ImVec2(-1.f, 0.f))) {
      // Open dropdown (implementation detail in helper)
  }
  
  std::string selectedID;
  if (addConditionDropdown.Render(selectedID)) {
      AddCondition(selectedID);
  }

NOTE: This is optional — current inline implementation works fine.
      Only do this if you want to centralize dropdown logic.

TIME: 30 minutes (optional)


============================================================================
STEP-BY-STEP EXECUTION GUIDE
============================================================================

1. SETUP
   [ ] Open Visual Studio
   [ ] Open solution: Olympe-Engine.sln
   [ ] Ensure no uncommitted changes or stash them

2. INTEGRATION 1 (15 min)
   [ ] Open VisualScriptEditorPanel.cpp
   [ ] Find line 1765 (VisualScriptNodeRenderer::RenderNode call)
   [ ] Add if-check for TaskNodeType::Branch (see code above)
   [ ] Save file
   [ ] Build (F7 or Build → Build Solution)
   [ ] Verify no errors

3. INTEGRATION 2 (20 min)
   [ ] Open VisualScriptEditorPanel.h
   [ ] Add m_libraryPanel member (see code above)
   [ ] Open VisualScriptEditorPanel.cpp
   [ ] Add initialization in Initialize() method (line ~85)
   [ ] Add button in RenderToolbar() method (line ~1450)
   [ ] Add render call in Render() method (after ImGui::End)
   [ ] Save files
   [ ] Build again (F7)
   [ ] Verify no errors

4. TESTING (1 hour)
   [ ] Run editor (F5)
   [ ] Create new Branch node
   [ ] Check toolbar for "Condition Presets" button
   [ ] Click button → verify panel opens
   [ ] Create new preset: "[mHealth] <= [2]"
   [ ] Close panel, select node in canvas
   [ ] Click "Edit Conditions" button
   [ ] Modal should open
   [ ] Add condition from dropdown
   [ ] Modal close → should show new condition
   [ ] Verify pins generated if needed
   [ ] Save graph (Ctrl+S)
   [ ] Close and reopen graph
   [ ] Verify conditions preserved

5. OPTIONAL: INTEGRATION 3 (30 min)
   [ ] Review PresetDropdownHelper.h/cpp (already created)
   [ ] Replace inline dropdowns in NodeConditionsPanel
   [ ] Build and test
   [ ] Verify UI behavior unchanged


============================================================================
VALIDATION CHECKLIST
============================================================================

After each integration, verify:

INTEGRATION 1 (Dispatcher):
  [ ] Build succeeds (no C++ errors)
  [ ] Branch nodes render in canvas
  [ ] 4-section layout visible (title/exec/conditions/pins)
  [ ] Node responds to hover/click
  [ ] No crash on selection

INTEGRATION 2 (LibraryPanel UI):
  [ ] Build succeeds
  [ ] Toolbar has "Condition Presets" button
  [ ] Button opens library panel
  [ ] Can create preset in panel
  [ ] Preset appears in dropdown
  [ ] Save/load preserves presets

INTEGRATION 3 (Optional):
  [ ] Build succeeds
  [ ] Dropdown behavior unchanged
  [ ] No visual differences
  [ ] Code compiles with no warnings

FINAL (All three):
  [ ] Run all tests: `ctest --output-on-failure`
  [ ] All Phase 24 tests pass
  [ ] No regressions in other tests
  [ ] Build has no warnings
  [ ] No valgrind/ASAN errors (if applicable)


============================================================================
ROLLBACK PLAN (If things go wrong)
============================================================================

Each integration is independent:

- INTEGRATION 1 broke: Comment out if-check, revert to generic renderer
- INTEGRATION 2 broke: Comment out panel init/render, remove button
- INTEGRATION 3 broke: Keep inline dropdowns as-is

No cascading failures — you can undo any integration independently.


============================================================================
ESTIMATED TIME BREAKDOWN
============================================================================

INTEGRATION 1: 15 min
  - Read code: 5 min
  - Implement: 5 min
  - Build + verify: 5 min

INTEGRATION 2: 20 min
  - Add member: 2 min
  - Initialize: 3 min
  - Add button: 3 min
  - Add render call: 3 min
  - Build + verify: 9 min (usually longer on first build)

INTEGRATION 3 (Optional): 30 min
  - Review helper: 5 min
  - Integrate in 1-2 panels: 15 min
  - Build + verify: 10 min

END-TO-END TESTING: 1 hour
  - Create node/preset: 15 min
  - Add condition: 15 min
  - Save/load verify: 15 min
  - Debug any issues: 15 min

TOTAL: 2-3 hours


============================================================================
SUPPORT
============================================================================

If you get stuck:

1. Check build errors — copy-paste exact error message
2. Search codebase for similar patterns
3. Review the audit report (PHASE24_FINAL_AUDIT_REPORT_FR.md)
4. Review the checklist (PHASE24_FinalImplementationChecklist.h)
5. Run existing tests to verify no regressions
6. All existing code compiles — your changes are additive only

No major rewrites needed — this is clean integration work.

"""

# End of file
