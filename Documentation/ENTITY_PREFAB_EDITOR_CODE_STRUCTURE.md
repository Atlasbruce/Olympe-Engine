/*
 * Entity Prefab Editor Plugin - Skeleton Structure
 * 
 * This document outlines the complete file structure and code skeleton
 * for the Entity Prefab Editor integration into Blueprint Editor v4.
 * 
 * Version: 1.0
 * Status: Code generation ready
 */

#ifndef ENTITY_PREFAB_EDITOR_STRUCTURE_H
#define ENTITY_PREFAB_EDITOR_STRUCTURE_H

/*
════════════════════════════════════════════════════════════════════════════════
                        FILE STRUCTURE OVERVIEW
════════════════════════════════════════════════════════════════════════════════

Source/BlueprintEditor/
├── EntityPrefabEditor/
│   ├── PrefabLoader.h
│   ├── PrefabLoader.cpp
│   ├── PrefabSerializer.h
│   ├── PrefabSerializer.cpp
│   ├── PrefabValidator.h
│   ├── PrefabValidator.cpp
│   ├── EntityPrefabGraphDocument.h
│   ├── EntityPrefabGraphDocument.cpp
│   ├── ComponentNode.h
│   ├── ComponentNode.cpp
│   ├── PrefabCanvas.h
│   ├── PrefabCanvas.cpp
│   ├── ComponentNodeRenderer.h
│   ├── ComponentNodeRenderer.cpp
│   ├── ParameterSchemaRegistry.h
│   ├── ParameterSchemaRegistry.cpp
│   ├── ComponentLibrary.h
│   ├── ComponentLibrary.cpp
│   ├── PropertyInspectorPrefab.h
│   ├── PropertyInspectorPrefab.cpp
│   └── README.md
│
├── EntityPrefabEditorPlugin.h (MODIFIED - was stub)
├── EntityPrefabEditorPlugin.cpp (MODIFIED - implement)
│
└── (No changes to other files for Phase 1)

Documentation/
├── ENTITY_PREFAB_EDITOR_INTEGRATION_PLAN.md (✅ Created)
└── Entity_Prefab_Editor_Implementation_Guide.md (To create in Phase 6)

════════════════════════════════════════════════════════════════════════════════
                          CLASS HIERARCHY
════════════════════════════════════════════════════════════════════════════════

Base Classes (Existing):
├── BlueprintEditorPlugin
│   └─ EntityPrefabEditorPlugin ◄─ OUR IMPLEMENTATION
│
├── GraphDocument (NodeGraphCore)
│   └─ EntityPrefabGraphDocument ◄─ OUR EXTENSION
│
├── Command (NodeGraphCore)
│   ├─ AddComponentCommand ◄─ Phase 5
│   ├─ RemoveComponentCommand ◄─ Phase 6
│   ├─ UpdateComponentPropertyCommand ◄─ Phase 6
│   └─ ReorderComponentCommand ◄─ Phase 6

New Classes:
├── PrefabLoader - File I/O and deserialization
├── PrefabSerializer - JSON serialization
├── PrefabValidator - Schema validation
├── ComponentNode - Component data model
├── ComponentNodeRenderer - ImGui rendering
├── PrefabCanvas - Canvas abstraction (ImNodes wrapper)
├── ParameterSchemaRegistry - Schema cache and queries
├── ComponentLibrary - Component filtering and search
└── PropertyInspectorPrefab - Property editing UI

════════════════════════════════════════════════════════════════════════════════
                        PHASE 1 DELIVERABLES
════════════════════════════════════════════════════════════════════════════════

Core Files (9 files):
✅ PrefabLoader.h/cpp           - Load prefab.json, ParameterSchemas.json
✅ EntityPrefabGraphDocument.h/cpp - Convert to/from GraphDocument
✅ ComponentNode.h/cpp           - Component data model
✅ PrefabCanvas.h/cpp            - ImNodes wrapper for rendering
✅ ComponentNodeRenderer.h/cpp    - Specialized node rendering
✅ ParameterSchemaRegistry.h/cpp  - Schema cache
✅ ComponentLibrary.h/cpp         - Component queries
✅ PropertyInspectorPrefab.h/cpp  - Property editing (basic)
✅ EntityPrefabEditorPlugin.cpp   - Integration point

Modified Files (2 files):
✅ EntityPrefabEditorPlugin.h     - Extend stub
✅ BlueprintEditorPlugin.h        - (No changes needed)

Integration Points:
✅ AssetBrowser.cpp              - Scan Gamedata/EntityPrefab/
✅ TabManager.cpp                - Handle prefab tab creation
✅ BlueprintEditorGUI.cpp        - Route to plugin renderer

════════════════════════════════════════════════════════════════════════════════
*/

/*
════════════════════════════════════════════════════════════════════════════════
                    CODE SKELETON - PHASE 1 CLASSES
════════════════════════════════════════════════════════════════════════════════

## 1. PrefabLoader.h

class PrefabLoader {
public:
    // Load complete prefab from JSON file
    // Returns EntityPrefab object, or throws on error
    static EntityPrefab LoadFromFile(const std::string& filePath);

    // Load parameter schemas (called once at startup)
    static void LoadParameterSchemas(const std::string& schemaPath);

    // Validate prefab against schema
    static bool ValidateAgainstSchema(const EntityPrefab& prefab);

private:
    static json ReadJsonFile(const std::string& path);
    static EntityPrefab ParsePrefabJson(const json& data);
    static void MigrateIfNeeded(EntityPrefab& prefab);
};

## 2. EntityPrefabGraphDocument.h

class EntityPrefabGraphDocument : public GraphDocument {
public:
    // Create entity center node
    NodeId CreateEntityCenterNode(const std::string& entityName);

    // Create component node
    NodeId CreateComponentNode(
        const std::string& componentType,
        const json& properties
    );

    // Delete component node
    bool DeleteComponentNode(NodeId nodeId);

    // Update component property
    bool UpdateComponentProperty(
        NodeId nodeId,
        const std::string& propName,
        const json& value
    );

    // Serialize back to EntityPrefab
    EntityPrefab SerializeToEntityPrefab() const;

    // Deserialize from EntityPrefab
    bool DeserializeFromEntityPrefab(const EntityPrefab& prefab);

    // Auto-layout components in circle
    void AutoLayoutComponents();

    // Get component node info
    ComponentNode* GetComponentNode(NodeId nodeId);
    const ComponentNode* GetComponentNode(NodeId nodeId) const;

private:
    NodeId m_entityCenterNodeId;
    std::map<NodeId, ComponentNode> m_componentNodes;
};

## 3. ComponentNode.h

struct ComponentNode {
    std::string componentType;      // "Identity_data", "Position_data", etc.
    json properties;                 // {"x": 0, "y": 0, ...}
    Vector2 position;                // Canvas position
    NodeId nodeId;                   // Reference to GraphDocument node

    // Utility methods
    bool operator==(const ComponentNode& other) const;
    bool operator!=(const ComponentNode& other) const;

    // Serialize/deserialize
    json ToJson() const;
    static ComponentNode FromJson(const json& data);
};

## 4. PrefabCanvas.h

class PrefabCanvas {
public:
    // Initialize ImNodes context
    void Initialize();
    void Shutdown();

    // Render canvas with current graph
    void Render(EntityPrefabGraphDocument& doc);

    // Update (handle input, animations, etc.)
    void Update(float deltaTime);

    // Input handling
    void OnMouseClick(Vector2 pos);
    void OnMouseDrag(Vector2 pos, Vector2 delta);
    void OnMouseDrop(Vector2 pos, const void* dragData);
    void OnKeyPress(int key);

    // Layout
    void AutoLayout(EntityPrefabGraphDocument& doc);
    void FitAll();
    void Zoom(float factor);
    void Pan(Vector2 delta);

    // Selection
    void SelectNode(NodeId nodeId);
    void DeselectNode(NodeId nodeId);
    void ClearSelection();
    NodeId GetSelectedNode() const;

    // State
    bool IsDragging() const { return m_isDragging; }
    Vector2 GetCanvasCenter() const { return m_canvasCenter; }
    float GetZoomLevel() const { return m_zoomLevel; }

private:
    bool m_isDragging = false;
    NodeId m_selectedNode = INVALID_NODE_ID;
    Vector2 m_canvasCenter;
    float m_zoomLevel = 1.0f;

    // ImNodes context
    imnodes::EditorContext* m_imNodesContext = nullptr;
};

## 5. ComponentNodeRenderer.h

class ComponentNodeRenderer {
public:
    // Render a single component node
    void Render(const ComponentNode& node, const ComponentSchema& schema, bool selected);

    // Check if point is inside node
    bool IsPointInside(const ComponentNode& node, Vector2 point) const;

    // Get visual bounds
    ImRect GetBounds(const ComponentNode& node) const;
    ImVec2 GetSize(const ComponentSchema& schema) const;

private:
    ImVec4 GetColorForType(const std::string& componentType) const;
    std::string GetIconForType(const std::string& componentType) const;
};

## 6. ParameterSchemaRegistry.h

class ParameterSchemaRegistry {
public:
    // Initialize registry (load from ParameterSchemas.json)
    static void Initialize(const std::string& schemaPath);

    // Get schema for component type
    static const ComponentSchema* GetSchema(const std::string& componentType);

    // Get all schemas
    static const std::vector<ComponentSchema>& GetAllSchemas();

    // Get default values for component type
    static json GetDefaultProperties(const std::string& componentType);

    // Check if component type exists
    static bool HasComponent(const std::string& componentType);

private:
    static std::map<std::string, ComponentSchema> s_schemas;
    static bool s_initialized;
};

## 7. ComponentLibrary.h

class ComponentLibrary {
public:
    // Get all available components (not in entity)
    std::vector<ComponentLibraryEntry> GetAvailableComponents(
        const EntityPrefab& prefab
    );

    // Search components by name
    std::vector<ComponentLibraryEntry> SearchComponents(const std::string& query);

    // Get component info
    ComponentLibraryEntry GetComponentInfo(const std::string& componentType);

    // Check if already in entity
    bool IsComponentInPrefab(
        const EntityPrefab& prefab,
        const std::string& componentType
    ) const;

private:
    struct ComponentLibraryEntry {
        std::string type;
        std::string displayName;
        std::string description;
        std::string icon;
        ComponentSchema schema;
        bool isAlreadyInEntity = false;
    };
};

## 8. PropertyInspectorPrefab.h

class PropertyInspectorPrefab {
public:
    // Display properties of selected component
    void DisplayComponent(const ComponentNode& node, const ComponentSchema& schema);

    // Called when property value changes
    void OnPropertyChanged(
        const std::string& propertyName,
        const json& newValue
    );

    // Reset property to default
    void ResetProperty(const std::string& propertyName);

    // Validate property value
    ValidationResult ValidateProperty(
        const std::string& propertyName,
        const json& value,
        const ParameterDefinition& paramDef
    );

private:
    void RenderPropertyEditor(
        const std::string& propName,
        const ParameterDefinition& paramDef,
        json& value
    );

    void RenderFloatEditor(const std::string& label, float& value);
    void RenderIntEditor(const std::string& label, int& value);
    void RenderBoolEditor(const std::string& label, bool& value);
    void RenderStringEditor(const std::string& label, std::string& value);
    void RenderVector2Editor(const std::string& label, json& value);
    void RenderColorEditor(const std::string& label, json& value);
};

## 9. EntityPrefabEditorPlugin.cpp

class EntityPrefabEditorPlugin : public BlueprintEditorPlugin {
public:
    // Plugin identification
    std::string GetBlueprintType() const override { return "EntityPrefab"; }
    std::string GetDisplayName() const override { return "Entity Prefab"; }
    std::string GetDescription() const override { 
        return "Edit entity prefabs with components"; 
    }

    // Create new blank prefab
    nlohmann::json CreateNew(const std::string& name) override;

    // Check if this plugin can handle the blueprint
    bool CanHandle(const nlohmann::json& blueprint) const override;

    // Validate prefab
    std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;

    // Main rendering entry point
    void RenderEditor(nlohmann::json& blueprintData, EditorContext_st& ctx) override;
    void RenderProperties(const nlohmann::json& blueprintData) override;
    void RenderToolbar(nlohmann::json& blueprintData) override;

private:
    void InitializeIfNeeded();

    // Sub-renderers
    void RenderCanvas(EntityPrefabGraphDocument& doc);
    void RenderComponentLibrary();
    void RenderPropertyInspector();

    // State
    std::unique_ptr<EntityPrefabGraphDocument> m_currentDocument;
    std::unique_ptr<PrefabCanvas> m_canvas;
    std::unique_ptr<PropertyInspectorPrefab> m_propertyInspector;
    ComponentLibrary m_componentLibrary;

    bool m_initialized = false;
};

════════════════════════════════════════════════════════════════════════════════
                          DATA STRUCTURES
════════════════════════════════════════════════════════════════════════════════

EntityPrefab (from JSON):
{
  "schema_version": 4,
  "type": "Beacon",
  "blueprintType": "EntityPrefab",
  "name": "Beacon",
  "metadata": {
    "author": "Team",
    "created": "2026-01-30T...",
    "tags": ["Beacon", "location"]
  },
  "data": {
    "prefabName": "Beacon",
    "components": [
      {
        "type": "Identity_data",
        "properties": {"name": "...", "tag": "Beacon"}
      },
      ...
    ]
  }
}

ComponentSchema (from ParameterSchemas.json):
{
  "componentType": "Movement_data",
  "parameters": [
    {"name": "speed", "type": "Float", "defaultValue": 100.0},
    {"name": "acceleration", "type": "Float", "defaultValue": 500.0}
  ]
}

════════════════════════════════════════════════════════════════════════════════
                         INTEGRATION POINTS
════════════════════════════════════════════════════════════════════════════════

1. AssetBrowser Integration:
   - Add ScanPrefabDirectory() in BlueprintEditor::RefreshAssets()
   - Adds prefab entries to asset tree with prefab icon

2. TabManager Integration:
   - TabManager creates new tab when prefab double-clicked
   - Tab contains prefab file path in pluginData
   - Tab type = EditorTabType::PREFAB_EDITOR

3. BlueprintEditorGUI Integration:
   - RenderNodeEditor() routes to plugin->RenderEditor()
   - Plugin renders into provided area

4. CommandStack Integration:
   - Phase 5+: AddComponentCommand, RemoveComponentCommand, etc.
   - Inherited from existing Command class

5. ValidationPanel Integration:
   - Validator calls PrefabValidator::Validate()
   - Shows errors in validation panel

════════════════════════════════════════════════════════════════════════════════
                            KEY PATTERNS
════════════════════════════════════════════════════════════════════════════════

1. Singleton Pattern (Existing):
   - BlueprintEditor::Get() for backend access
   - ParameterSchemaRegistry static methods

2. Plugin Pattern (Existing):
   - EntityPrefabEditorPlugin extends BlueprintEditorPlugin
   - Registered with BlueprintEditor::RegisterPlugin()

3. Command Pattern (Existing, Phase 5+):
   - Add/Remove/Update = Commands on CommandStack
   - Full undo/redo support

4. Observer Pattern (Existing):
   - ValidationPanel listens to editor changes
   - Auto-validates on graph modification

5. Factory Pattern (Phase 3+):
   - ComponentLibrary creates ComponentNodes with defaults
   - ParameterSchemaRegistry provides schema data

════════════════════════════════════════════════════════════════════════════════
                         BUILD INTEGRATION
════════════════════════════════════════════════════════════════════════════════

CMakeLists.txt changes:
- Add SOURCE files for 9 new .cpp files
- Add HEADER includes for 9 new .h files
- Dependencies: BlueprintEditor, NodeGraphCore, nlohmann/json

Include paths:
- Source/BlueprintEditor/EntityPrefabEditor/

Link against:
- Olympe_BlueprintEditor
- Olympe_NodeGraphCore
- nlohmann_json (header-only)

════════════════════════════════════════════════════════════════════════════════
                            NEXT STEPS
════════════════════════════════════════════════════════════════════════════════

1. Create all 9 .h files (skip .cpp for now, just function stubs)
2. Modify EntityPrefabEditorPlugin.h (from stub to extended)
3. PR for architectural review
4. Proceed with .cpp implementations one by one

Estimated time per file: 1-2 hours (including review)

════════════════════════════════════════════════════════════════════════════════
*/

#endif // ENTITY_PREFAB_EDITOR_STRUCTURE_H
