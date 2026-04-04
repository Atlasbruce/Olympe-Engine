# Copilot Instructions

## General Guidelines
- First general instruction
- Second general instruction

## Project Directives
- Phase 24 Implementation: Condition Presets migrated from external file storage to graph-embedded serialization. Presets are now stored IN each blueprint's JSON (v4 schema), making graphs self-contained. New field added to TaskGraphTemplate: `std::vector<ConditionPreset> Presets`. Serialization handled by VisualScriptEditorPanel::SerializeAndWrite() and deserialization by TaskGraphLoader::ParseSchemaV4().

- Phase 26 UX Enhancement: Tab-based panel system for right editor section. The 3 panels (Preset Bank, Local Variables, Global Variables) are now grouped into a tabbed interface:
  - Part A (top): Node Properties panel (unchanged)
  - Part B (bottom): Tab-based layout with 3 tabs:
    * Tab 0: Preset Bank (Condition Presets)
    * Tab 1: Local Variables (Local Blackboard)
    * Tab 2: Global Variables (Global Blackboard)
  - Implementation: `m_rightPanelTabSelection` (0=Presets, 1=Local, 2=Global)
  - New functions: `RenderRightPanelTabs()` and `RenderRightPanelTabContent()`
  - Modified: `RenderContent()` in VisualScriptEditorPanel_RenderingCore.cpp
  - Removed: Old splitter system between Preset Bank and Variables; old radio-button selection for Local/Global

## File Management Protocol
- To add new files to the OlympeBlueprintEditor project:
  1. Create new C++ files using the `create_file` tool with complete implementation.
  2. Specify which project the files belong to (typically OlympeBlueprintEditor.vcxproj or Olympe Engine.vcxproj).
  3. Manually add files to the project via Visual Studio: Right-click project → "Add Existing Item" → select file(s) → OK → Compile to verify.
- This approach avoids XML manipulation errors and is faster/more reliable than programmatic project file editing.
- For code modifications to existing files, use `replace_string_in_file` directly without user intervention.