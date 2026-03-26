# Copilot Instructions

## Directives de projet
- Phase 24 Implementation: Condition Presets migrated from external file storage to graph-embedded serialization. Presets are now stored IN each blueprint's JSON (v4 schema), making graphs self-contained. New field added to TaskGraphTemplate: `std::vector<ConditionPreset> Presets`. Serialization handled by VisualScriptEditorPanel::SerializeAndWrite() and deserialization by TaskGraphLoader::ParseSchemaV4().