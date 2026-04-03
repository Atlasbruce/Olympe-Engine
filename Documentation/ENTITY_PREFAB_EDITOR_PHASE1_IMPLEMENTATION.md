# Entity Prefab Editor - Phase 1 Implementation Guide

> **Document** : Detailed step-by-step guide for Phase 1 implementation  
> **Duration** : 2 sprints (8-10 days)  
> **Deliverables** : PrefabViewer with visualization of prefab components  
> **Risk Level** : 🟢 **LOW**

---

## Overview

### Phase 1 Goal
Implement the **Prefab Visualizer** - Load entity prefabs and display them as node graphs using the existing Blueprint Editor infrastructure.

### Key Features
- ✅ Load and display entity prefabs from `Gamedata/EntityPrefab/*.json`
- ✅ Show components as visual nodes on canvas
- ✅ Integrate with Asset Browser (double-click to open)
- ✅ Create new Prefab tab in Tab Manager
- ✅ Read-only mode (editing in later phases)

### Success Criteria
- [ ] Load `beacon.json` without errors
- [ ] Display 5+ component nodes visually
- [ ] Can zoom/pan on canvas
- [ ] Multiple prefabs can be opened in different tabs
- [ ] No performance issues (60 FPS with 20 nodes)

---

## Step-by-Step Implementation

### Step 1: Create PrefabLoader (Day 1)

#### File: `Source/BlueprintEditor/EntityPrefabEditor/PrefabLoader.h`

```cpp
#pragma once

#include <string>
#include <vector>
#include <map>
#include "../../json_helper.h"
#include "../../Core/Math/Vector2.h"

namespace Olympe
{
    // Data structures matching JSON schema

    struct ComponentData {
        std::string type;                    // "Identity_data", "Position_data", etc.
        std::map<std::string, nlohmann::json> properties;
    };

    struct EntityPrefab {
        int schema_version = 4;
        std::string name;                    // "Beacon", "Guard", etc.
        std::string description;
        struct {
            std::string author;
            std::string created;             // ISO 8601
            std::string lastModified;
            std::vector<std::string> tags;
        } metadata;

        struct {
            std::string prefabName;
            std::vector<ComponentData> components;
        } data;
    };

    struct ParameterDefinition {
        std::string name;
        std::string type;                    // "Float", "Int", "Bool", "String", "Vector2"
        nlohmann::json defaultValue;
    };

    struct ComponentSchema {
        std::string componentType;
        std::vector<ParameterDefinition> parameters;
    };

    /**
     * PrefabLoader - Load and parse entity prefabs
     * 
     * Usage:
     *   auto prefab = PrefabLoader::LoadFromFile("Gamedata/EntityPrefab/beacon.json");
     *   PrefabLoader::LoadParameterSchemas("Gamedata/ParameterSchemas.json");
     */
    class PrefabLoader
    {
    public:
        /**
         * Load entity prefab from JSON file
         * @param filePath Path to .json file
         * @return EntityPrefab object
         * @throws std::runtime_error if file not found or invalid JSON
         */
        static EntityPrefab LoadFromFile(const std::string& filePath);

        /**
         * Load all parameter schemas
         * Called once at startup to populate registry
         * @param schemaPath Path to ParameterSchemas.json
         * @throws std::runtime_error if file not found or invalid JSON
         */
        static void LoadParameterSchemas(const std::string& schemaPath);

        /**
         * Get loaded schema for component type
         * @param componentType e.g., "Identity_data"
         * @return ComponentSchema or nullptr if not found
         */
        static const ComponentSchema* GetComponentSchema(const std::string& componentType);

        /**
         * Get all loaded schemas
         * @return Vector of all schemas loaded
         */
        static const std::vector<ComponentSchema>& GetAllSchemas();

        /**
         * Validate prefab against schemas
         * @param prefab EntityPrefab to validate
         * @return true if valid
         */
        static bool ValidateAgainstSchemas(const EntityPrefab& prefab);

        /**
         * Get validation errors for a prefab
         * @param prefab EntityPrefab to validate
         * @return Vector of error messages
         */
        static std::vector<std::string> GetValidationErrors(const EntityPrefab& prefab);

    private:
        // Cache for loaded schemas
        static std::vector<ComponentSchema> s_schemas;
        static bool s_schemasLoaded;

        // Private helper methods
        static nlohmann::json ReadJsonFile(const std::string& path);
        static EntityPrefab ParsePrefabJson(const nlohmann::json& data);
        static ComponentSchema ParseSchemaJson(const nlohmann::json& data);
        static bool MigrateSchemaIfNeeded(EntityPrefab& prefab);
    };
}
```

#### File: `Source/BlueprintEditor/EntityPrefabEditor/PrefabLoader.cpp`

```cpp
#include "PrefabLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "../../Core/Logger.h"

namespace Olympe
{
    // Static member initialization
    std::vector<ComponentSchema> PrefabLoader::s_schemas;
    bool PrefabLoader::s_schemasLoaded = false;

    // ========================================================================
    // Public Methods
    // ========================================================================

    EntityPrefab PrefabLoader::LoadFromFile(const std::string& filePath)
    {
        try {
            auto jsonData = ReadJsonFile(filePath);
            auto prefab = ParsePrefabJson(jsonData);

            if (!ValidateAgainstSchemas(prefab)) {
                auto errors = GetValidationErrors(prefab);
                std::string errorMsg = "Prefab validation failed:\n";
                for (const auto& err : errors) {
                    errorMsg += "  - " + err + "\n";
                }
                LogWarning(errorMsg);
            }

            return prefab;
        }
        catch (const std::exception& e) {
            LogError("Failed to load prefab from " + filePath + ": " + e.what());
            throw;
        }
    }

    void PrefabLoader::LoadParameterSchemas(const std::string& schemaPath)
    {
        try {
            auto jsonData = ReadJsonFile(schemaPath);

            // Assuming ParameterSchemas.json is an array
            if (!jsonData.is_array()) {
                throw std::runtime_error("ParameterSchemas.json must be an array");
            }

            s_schemas.clear();
            for (const auto& schemaJson : jsonData) {
                auto schema = ParseSchemaJson(schemaJson);
                s_schemas.push_back(schema);
            }

            s_schemasLoaded = true;
            LogInfo("Loaded " + std::to_string(s_schemas.size()) + " component schemas");
        }
        catch (const std::exception& e) {
            LogError("Failed to load parameter schemas: " + std::string(e.what()));
            throw;
        }
    }

    const ComponentSchema* PrefabLoader::GetComponentSchema(const std::string& componentType)
    {
        if (!s_schemasLoaded) {
            LogWarning("Schemas not loaded yet. Call LoadParameterSchemas() first.");
            return nullptr;
        }

        for (const auto& schema : s_schemas) {
            if (schema.componentType == componentType) {
                return &schema;
            }
        }

        return nullptr;
    }

    const std::vector<ComponentSchema>& PrefabLoader::GetAllSchemas()
    {
        return s_schemas;
    }

    bool PrefabLoader::ValidateAgainstSchemas(const EntityPrefab& prefab)
    {
        auto errors = GetValidationErrors(prefab);
        return errors.empty();
    }

    std::vector<std::string> PrefabLoader::GetValidationErrors(const EntityPrefab& prefab)
    {
        std::vector<std::string> errors;

        // Check required fields
        if (prefab.name.empty()) {
            errors.push_back("Prefab name is empty");
        }

        if (prefab.data.components.empty()) {
            errors.push_back("Prefab has no components");
        }

        // Check each component
        for (const auto& component : prefab.data.components) {
            if (component.type.empty()) {
                errors.push_back("Component has no type");
                continue;
            }

            const auto* schema = GetComponentSchema(component.type);
            if (!schema) {
                errors.push_back("Unknown component type: " + component.type);
                continue;
            }

            // TODO: Validate properties match schema
        }

        return errors;
    }

    // ========================================================================
    // Private Methods
    // ========================================================================

    nlohmann::json PrefabLoader::ReadJsonFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }

        try {
            nlohmann::json data;
            file >> data;
            return data;
        }
        catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Invalid JSON in file " + path + ": " + e.what());
        }
    }

    EntityPrefab PrefabLoader::ParsePrefabJson(const nlohmann::json& data)
    {
        EntityPrefab prefab;

        try {
            prefab.schema_version = data.value("schema_version", 4);
            prefab.name = data.value("name", "");
            prefab.description = data.value("description", "");

            // Parse metadata
            if (data.contains("metadata")) {
                const auto& meta = data["metadata"];
                prefab.metadata.author = meta.value("author", "Unknown");
                prefab.metadata.created = meta.value("created", "");
                prefab.metadata.lastModified = meta.value("lastModified", "");

                if (meta.contains("tags") && meta["tags"].is_array()) {
                    for (const auto& tag : meta["tags"]) {
                        prefab.metadata.tags.push_back(tag.get<std::string>());
                    }
                }
            }

            // Parse components
            if (data.contains("data") && data["data"].contains("components")) {
                const auto& components = data["data"]["components"];
                if (!components.is_array()) {
                    throw std::runtime_error("Components must be an array");
                }

                for (const auto& compJson : components) {
                    ComponentData component;
                    component.type = compJson.value("type", "");

                    if (compJson.contains("properties") && compJson["properties"].is_object()) {
                        for (auto& [key, value] : compJson["properties"].items()) {
                            component.properties[key] = value;
                        }
                    }

                    prefab.data.components.push_back(component);
                }
            }

            return prefab;
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Error parsing prefab JSON: " + std::string(e.what()));
        }
    }

    ComponentSchema PrefabLoader::ParseSchemaJson(const nlohmann::json& data)
    {
        ComponentSchema schema;

        try {
            schema.componentType = data.value("componentType", "");

            if (data.contains("parameters") && data["parameters"].is_array()) {
                for (const auto& paramJson : data["parameters"]) {
                    ParameterDefinition param;
                    param.name = paramJson.value("name", "");
                    param.type = paramJson.value("type", "");
                    param.defaultValue = paramJson.value("defaultValue", nullptr);

                    schema.parameters.push_back(param);
                }
            }

            return schema;
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Error parsing schema JSON: " + std::string(e.what()));
        }
    }

    bool PrefabLoader::MigrateSchemaIfNeeded(EntityPrefab& prefab)
    {
        // TODO: Implement schema migration if needed
        // For now, assume schema v4
        if (prefab.schema_version < 4) {
            LogWarning("Prefab schema version " + std::to_string(prefab.schema_version) + 
                      " is outdated. Migration not yet implemented.");
            return false;
        }
        return true;
    }
}
```

#### Testing

Create simple unit tests:

```cpp
// Test loading valid prefab
auto prefab = PrefabLoader::LoadFromFile("test_data/beacon.json");
assert(prefab.name == "Beacon");
assert(prefab.data.components.size() > 0);

// Test loading schemas
PrefabLoader::LoadParameterSchemas("test_data/ParameterSchemas.json");
const auto* schema = PrefabLoader::GetComponentSchema("Identity_data");
assert(schema != nullptr);

// Test validation
auto errors = PrefabLoader::GetValidationErrors(prefab);
assert(errors.empty());  // Should be valid
```

---

### Step 2: Create EntityPrefabGraphDocument (Day 1-2)

#### File: `Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.h`

```cpp
#pragma once

#include "../../NodeGraphCore/GraphDocument.h"
#include "../../Core/Math/Vector2.h"
#include "PrefabLoader.h"
#include <memory>

namespace Olympe
{
    // Forward declare component node
    struct ComponentNode;

    /**
     * EntityPrefabGraphDocument - Specialized GraphDocument for entity prefabs
     * 
     * Extends GraphDocument to handle:
     * - Entity center node (special visual)
     * - Component nodes (arranged in circle)
     * - Auto-layout algorithm
     * - Conversion to/from EntityPrefab JSON
     */
    class EntityPrefabGraphDocument : public NodeGraph::GraphDocument
    {
    public:
        EntityPrefabGraphDocument();
        virtual ~EntityPrefabGraphDocument();

        // ====================================================================
        // Entity Management
        // ====================================================================

        /**
         * Load prefab and create graph nodes
         * @param prefab EntityPrefab to visualize
         * @return true if successful
         */
        bool LoadFromPrefab(const EntityPrefab& prefab);

        /**
         * Serialize graph back to EntityPrefab format
         * @return EntityPrefab JSON
         */
        EntityPrefab SerializeToPrefab() const;

        /**
         * Get entity name
         * @return Entity prefab name
         */
        std::string GetEntityName() const { return m_entityName; }

        /**
         * Get entity center node ID
         * @return NodeId of central entity node
         */
        NodeGraph::NodeId GetEntityCenterNodeId() const { return m_entityCenterNodeId; }

        // ====================================================================
        // Component Management
        // ====================================================================

        /**
         * Create a new component node
         * @param componentType e.g., "Movement_data"
         * @param position Canvas position (optional, auto-laid out)
         * @return NodeId of created node, or INVALID_NODE_ID if failed
         */
        NodeGraph::NodeId CreateComponentNode(
            const std::string& componentType,
            Vector2 position = Vector2{0, 0}
        );

        /**
         * Delete a component node
         * @param nodeId Node to delete
         * @return true if deleted
         */
        bool DeleteComponentNode(NodeGraph::NodeId nodeId);

        /**
         * Get component node
         * @param nodeId Node ID
         * @return ComponentNode* or nullptr
         */
        ComponentNode* GetComponentNode(NodeGraph::NodeId nodeId);
        const ComponentNode* GetComponentNode(NodeGraph::NodeId nodeId) const;

        /**
         * Get all component nodes
         * @return Vector of all component nodes
         */
        std::vector<NodeGraph::NodeId> GetComponentNodeIds() const;

        /**
         * Update component property
         * @param nodeId Node to update
         * @param propertyName Property name
         * @param value New value
         * @return true if updated
         */
        bool UpdateComponentProperty(
            NodeGraph::NodeId nodeId,
            const std::string& propertyName,
            const nlohmann::json& value
        );

        /**
         * Get component property
         * @param nodeId Node ID
         * @param propertyName Property name
         * @return Property value or nullptr
         */
        const nlohmann::json* GetComponentProperty(
            NodeGraph::NodeId nodeId,
            const std::string& propertyName
        ) const;

        // ====================================================================
        // Layout
        // ====================================================================

        /**
         * Auto-layout components in circle around entity center
         * @param radius Radius of circle (default: 300)
         */
        void AutoLayoutComponents(float radius = 300.0f);

        /**
         * Fit all nodes in view
         */
        void FitAllNodes();

        // ====================================================================
        // Validation
        // ====================================================================

        /**
         * Validate the prefab
         * @return Vector of validation errors (empty if valid)
         */
        std::vector<ValidationError> ValidatePrefab() const;

    private:
        // Helper methods
        ComponentNode* CreateComponentNodeInternal(
            const std::string& componentType,
            const ComponentSchema* schema
        );

        Vector2 CalculateCirclePosition(int index, int total, float radius) const;

        // State
        std::string m_entityName;
        NodeGraph::NodeId m_entityCenterNodeId = NodeGraph::INVALID_NODE_ID;
        std::map<NodeGraph::NodeId, std::unique_ptr<ComponentNode>> m_componentNodes;

        // Cached data
        std::map<NodeGraph::NodeId, ComponentData> m_componentData;
    };
}
```

---

### Step 3: Create ComponentNode (Day 2)

#### File: `Source/BlueprintEditor/EntityPrefabEditor/ComponentNode.h`

```cpp
#pragma once

#include <string>
#include "../../json_helper.h"
#include "../../NodeGraphCore/GraphDocument.h"
#include "../../Core/Math/Vector2.h"

namespace Olympe
{
    /**
     * ComponentNode - Data model for a component in entity prefab
     * 
     * Wraps GraphDocument node with component-specific data
     */
    struct ComponentNode
    {
        // === Identification ===
        NodeGraph::NodeId nodeId = NodeGraph::INVALID_NODE_ID;
        std::string componentType;                              // "Identity_data", etc.

        // === Visual ===
        Vector2 position;                                       // Canvas position
        Vector2 size;                                           // Node size
        bool selected = false;
        bool hovered = false;

        // === Data ===
        std::map<std::string, nlohmann::json> properties;      // {"x": 0, "y": 100}
        const ComponentSchema* schema = nullptr;               // Reference to schema

        // === Utility ===
        bool operator==(const ComponentNode& other) const;
        bool operator!=(const ComponentNode& other) const;

        std::string ToDisplayString() const;

        // Serialization
        nlohmann::json ToJson() const;
        static ComponentNode FromJson(const nlohmann::json& data, NodeGraph::NodeId nodeId);
    };
}
```

---

### Step 4: Create PrefabCanvas (Day 2-3)

#### File: `Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.h`

```cpp
#pragma once

#include <memory>
#include "EntityPrefabGraphDocument.h"
#include "../../Core/Math/Vector2.h"
#include "../../Core/Math/Rect.h"

namespace Olympe
{
    // Forward declarations
    class ComponentNodeRenderer;
    struct ImDrawList;

    /**
     * PrefabCanvas - Canvas for rendering entity prefab graphs
     * 
     * Wraps ImNodes functionality for entity prefab visualization.
     * Handles:
     * - Node rendering (via ComponentNodeRenderer)
     * - User input (click, drag, zoom)
     * - Layout management
     */
    class PrefabCanvas
    {
    public:
        PrefabCanvas();
        ~PrefabCanvas();

        // ====================================================================
        // Lifecycle
        // ====================================================================

        /**
         * Initialize canvas (setup ImNodes context)
         */
        void Initialize();

        /**
         * Shutdown canvas (cleanup ImNodes context)
         */
        void Shutdown();

        /**
         * Check if initialized
         */
        bool IsInitialized() const { return m_initialized; }

        // ====================================================================
        // Rendering
        // ====================================================================

        /**
         * Render canvas and all nodes
         * @param doc EntityPrefabGraphDocument to render
         * @param availableWidth Width of rendering area
         * @param availableHeight Height of rendering area
         */
        void Render(
            EntityPrefabGraphDocument& doc,
            float availableWidth = 800.0f,
            float availableHeight = 600.0f
        );

        /**
         * Update canvas state (input, animations)
         * @param deltaTime Time since last frame
         */
        void Update(float deltaTime);

        // ====================================================================
        // Input Handling
        // ====================================================================

        /**
         * Handle mouse click
         * @param position Mouse position in canvas space
         * @param button 0=left, 1=right, 2=middle
         */
        void OnMouseClick(Vector2 position, int button);

        /**
         * Handle mouse drag
         * @param position Current mouse position
         * @param delta Movement since last frame
         */
        void OnMouseDrag(Vector2 position, Vector2 delta);

        /**
         * Handle mouse drop
         * @param position Drop position
         * @param dragData Drag payload (optional)
         */
        void OnMouseDrop(Vector2 position, const void* dragData);

        /**
         * Handle keyboard input
         * @param keyCode Key code
         */
        void OnKeyPress(int keyCode);

        // ====================================================================
        // Selection
        // ====================================================================

        /**
         * Select a node
         * @param nodeId Node to select
         */
        void SelectNode(NodeGraph::NodeId nodeId);

        /**
         * Deselect a node
         * @param nodeId Node to deselect
         */
        void DeselectNode(NodeGraph::NodeId nodeId);

        /**
         * Clear all selections
         */
        void ClearSelection();

        /**
         * Get selected node
         * @return NodeId or INVALID_NODE_ID
         */
        NodeGraph::NodeId GetSelectedNode() const { return m_selectedNode; }

        // ====================================================================
        // Layout & View
        // ====================================================================

        /**
         * Auto-layout all nodes
         * @param doc Document to layout
         */
        void AutoLayout(EntityPrefabGraphDocument& doc);

        /**
         * Fit all nodes in view
         * @param padding Space around nodes
         */
        void FitAll(float padding = 50.0f);

        /**
         * Zoom canvas
         * @param factor Zoom factor (> 1 = zoom in, < 1 = zoom out)
         */
        void Zoom(float factor);

        /**
         * Pan canvas
         * @param delta Pan amount
         */
        void Pan(Vector2 delta);

        /**
         * Reset view
         */
        void ResetView();

        // ====================================================================
        // State Queries
        // ====================================================================

        bool IsDragging() const { return m_isDragging; }
        bool IsZooming() const { return m_isZooming; }
        float GetZoomLevel() const { return m_zoomLevel; }
        Vector2 GetCanvasCenter() const { return m_canvasCenter; }
        Vector2 GetCameraOffset() const { return m_cameraOffset; }

    private:
        // Helpers
        void RenderEntityCenterNode(const EntityPrefabGraphDocument& doc);
        void RenderComponentNodes(const EntityPrefabGraphDocument& doc);
        void RenderGridBackground();

        Vector2 ScreenToCanvasSpace(Vector2 screenPos) const;
        Vector2 CanvasToScreenSpace(Vector2 canvasPos) const;

        // State
        bool m_initialized = false;
        bool m_isDragging = false;
        bool m_isZooming = false;

        NodeGraph::NodeId m_selectedNode = NodeGraph::INVALID_NODE_ID;
        Vector2 m_dragStartPos;
        Vector2 m_canvasCenter;
        Vector2 m_cameraOffset;
        float m_zoomLevel = 1.0f;

        // ImNodes context
        void* m_imNodesContext = nullptr;

        // Renderer
        std::unique_ptr<ComponentNodeRenderer> m_nodeRenderer;
    };
}
```

---

### Step 5: Create Remaining Classes (Day 3)

**ComponentNodeRenderer.h** - Renders component nodes
**ParameterSchemaRegistry.h** - Schema cache
**ComponentLibrary.h** - Component queries  
**PropertyInspectorPrefab.h** - Property editing

*Each file follows the pattern shown above with clear separation of concerns.*

---

### Step 6: Extend EntityPrefabEditorPlugin (Day 3)

#### Modify `Source/BlueprintEditor/EntityPrefabEditorPlugin.h`

```cpp
class EntityPrefabEditorPlugin : public BlueprintEditorPlugin
{
public:
    EntityPrefabEditorPlugin();
    virtual ~EntityPrefabEditorPlugin();

    // Plugin identification
    std::string GetBlueprintType() const override { return "EntityPrefab"; }
    std::string GetDisplayName() const override { return "Entity Prefab"; }
    std::string GetDescription() const override { return "Edit entity prefabs"; }

    // Lifecycle
    nlohmann::json CreateNew(const std::string& name) override;
    bool CanHandle(const nlohmann::json& blueprint) const override;
    std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;

    // Rendering (main entry point)
    void RenderEditor(nlohmann::json& blueprintData, EditorContext_st& ctx) override;
    void RenderProperties(const nlohmann::json& blueprintData) override;
    void RenderToolbar(nlohmann::json& blueprintData) override;

private:
    void Initialize(const nlohmann::json& blueprintData);

    // Sub-renderers
    void RenderCanvas();
    void RenderPropertyInspector();
    void RenderToolbar();

    // State
    std::unique_ptr<EntityPrefabGraphDocument> m_currentDocument;
    std::unique_ptr<PrefabCanvas> m_canvas;
    std::unique_ptr<PropertyInspectorPrefab> m_propertyInspector;

    bool m_initialized = false;
};
```

---

### Step 7: Integration Points (Day 3-4)

#### Modify `Source/BlueprintEditor/AssetBrowser.cpp`

Add prefab scanning:

```cpp
void BlueprintEditor::RefreshAssets()
{
    // Existing: Scan Blueprints/
    ScanBlueprintDirectory("Blueprints/");

    // NEW: Scan Entity Prefabs
    ScanPrefabDirectory("Gamedata/EntityPrefab/");
}

void BlueprintEditor::ScanPrefabDirectory(const std::string& path)
{
    // Add prefab .json files to asset tree
    // Set icon to prefab icon
    // On double-click: create Prefab tab
}
```

#### Modify `Source/BlueprintEditor/TabManager.cpp`

Handle prefab tab creation:

```cpp
void TabManager::OnPrefabDoubleClicked(const std::string& filePath)
{
    auto tab = std::make_unique<EditorTab>();
    tab->type = EditorTabType::PREFAB_EDITOR;
    tab->name = GetFileName(filePath);  // "beacon"
    tab->pluginData = filePath;  // Full path

    AddTab(std::move(tab));
    SetActiveTab(m_tabs.back()->id);
}
```

---

### Step 8: Testing & Polish (Day 4-5)

#### Unit Tests

```cpp
TEST(PrefabLoader, LoadsValidPrefab) {
    auto prefab = PrefabLoader::LoadFromFile("test_data/beacon.json");
    EXPECT_EQ(prefab.name, "Beacon");
    EXPECT_GT(prefab.data.components.size(), 0);
}

TEST(EntityPrefabGraphDocument, LoadsAndConvertsToGraph) {
    EntityPrefab prefab = /* ... */;
    EntityPrefabGraphDocument doc;
    EXPECT_TRUE(doc.LoadFromPrefab(prefab));
    EXPECT_NE(doc.GetEntityCenterNodeId(), INVALID_NODE_ID);
}

TEST(PrefabCanvas, InitializeAndRender) {
    PrefabCanvas canvas;
    canvas.Initialize();
    EXPECT_TRUE(canvas.IsInitialized());
    // Render should not crash
    canvas.Render(doc, 800, 600);
    canvas.Shutdown();
}
```

#### Manual Testing Checklist

- [ ] Load `beacon.json` - displays without errors
- [ ] Load `player.json` - displays 13 components
- [ ] Zoom in/out - smooth and responsive
- [ ] Pan - can move around canvas
- [ ] Click on node - highlights
- [ ] Double-click prefab - creates new tab
- [ ] Open 3 prefabs simultaneously - no state collision
- [ ] Close tab - cleans up resources
- [ ] 60 FPS with 20 nodes

---

## Deliverables Checklist

### Code Files (9 new files)

- [ ] `PrefabLoader.h/cpp` - File I/O and parsing
- [ ] `EntityPrefabGraphDocument.h/cpp` - Graph conversion
- [ ] `ComponentNode.h` - Component data model
- [ ] `ComponentNodeRenderer.h/cpp` - ImGui rendering
- [ ] `PrefabCanvas.h/cpp` - Canvas wrapper
- [ ] `ParameterSchemaRegistry.h/cpp` - Schema cache
- [ ] `ComponentLibrary.h/cpp` - Component queries
- [ ] `PropertyInspectorPrefab.h/cpp` - Property UI
- [ ] `EntityPrefabEditorPlugin.cpp` - Integration

### Integration Changes (3 files)

- [ ] `EntityPrefabEditorPlugin.h` - Extend stub
- [ ] `AssetBrowser.cpp` - Add prefab scanning
- [ ] `TabManager.cpp` - Handle prefab tabs

### Documentation

- [ ] `Source/BlueprintEditor/EntityPrefabEditor/README.md` - Overview
- [ ] Code comments in all public APIs
- [ ] Architecture diagram (ASCII art)

### Tests

- [ ] Unit tests for PrefabLoader
- [ ] Unit tests for EntityPrefabGraphDocument
- [ ] Integration test for full load workflow
- [ ] Manual test checklist (all pass)

---

## Success Metrics

Upon completion of Phase 1:

| Metric | Target | Status |
|--------|--------|--------|
| Load time | < 100ms | ⏳ To measure |
| Nodes render | All visible | ⏳ To verify |
| FPS | 60+ | ⏳ To profile |
| Memory | < 10 MB per tab | ⏳ To profile |
| Code coverage | > 70% | ⏳ To measure |
| Documentation | 100% | ⏳ To complete |

---

## Estimated Effort

- **Architecture & Setup** : 0.5 day
- **PrefabLoader** : 1 day
- **EntityPrefabGraphDocument** : 1 day
- **Canvas & Rendering** : 2 days
- **Schema Registry & Library** : 1 day
- **Plugin Integration** : 0.5 day
- **Testing & Polish** : 1 day
- **Total** : **7-8 days** (fits in 1 sprint)

---

## Next Phase Trigger

**Phase 1 COMPLETE when:**
- ✅ All tests pass
- ✅ Code reviewed and approved
- ✅ Merged to main branch
- ✅ Documentation updated

**Phase 2 (Component Node Visualization)** begins immediately.

---

**Document Version** : 1.0  
**Last Updated** : 2026-04-02  
**Author** : Implementation Guide
