#pragma once

#include "../BlueprintEditorPlugin.h"
#include "PrefabLoader.h"
#include "EntityPrefabGraphDocument.h"
#include "PrefabCanvas.h"
#include "PropertyInspectorPrefab.h"
#include "ParameterSchemaRegistry.h"
#include <memory>
#include <string>

namespace OlympeEngine {

// ============================================================================
// EntityPrefabEditorPlugin - Main plugin class for Entity Prefab Editor
// ============================================================================

class EntityPrefabEditorPlugin : public BlueprintEditorPlugin {
public:
    EntityPrefabEditorPlugin();
    ~EntityPrefabEditorPlugin();

    // ========================================================================
    // BlueprintEditorPlugin Interface Implementation
    // ========================================================================

    // Lifecycle
    bool Initialize() override;
    void Shutdown() override;

    // Identification
    std::string GetPluginName() const override { return "Entity Prefab Editor"; }
    std::string GetPluginVersion() const override { return "0.1.0"; }

    // File Handling
    bool CanHandle(const std::string& filePath) const override;
    bool CanHandle(GraphDocument::GraphType graphType) const override;

    // Document Management
    std::shared_ptr<GraphDocument> CreateNewDocument(const std::string& name) override;
    std::shared_ptr<GraphDocument> LoadDocument(const std::string& filePath) override;
    bool SaveDocument(const std::shared_ptr<GraphDocument>& document, const std::string& filePath) override;

    // UI Rendering
    void RenderEditor(
        std::shared_ptr<GraphDocument> document,
        float width,
        float height
    ) override;

    void RenderPropertyPanel() override;
    void RenderComponentLibrary() override;

    // Validation
    bool ValidateDocument(const std::shared_ptr<GraphDocument>& document) override;
    std::vector<std::string> GetValidationErrors() const override;

    // ========================================================================
    // Entity Prefab Editor Specific Methods
    // ========================================================================

    // File Operations
    EntityPrefab LoadPrefabFile(const std::string& filePath);
    bool SavePrefabFile(const std::string& filePath);

    // Canvas Operations
    void FitCanvasToView();
    void ResetCanvasView();
    void SetCanvasZoom(float zoomLevel);

    // Selection Management
    void SelectComponentNode(NodeGraph::NodeId nodeId);
    void DeselectComponentNode(NodeGraph::NodeId nodeId);
    void DeselectAllNodes();

    // Component Operations
    void CreateComponentNode(const std::string& componentType, const glm::vec2& position);
    void RemoveSelectedComponentNodes();

    // Property Inspector
    void UpdatePropertyInspectorForNode(NodeGraph::NodeId nodeId);

    // Layout
    void AutoLayoutComponents();

private:
    // Document management
    std::shared_ptr<EntityPrefabGraphDocument> m_currentDocument;

    // Canvas & Visualization
    std::shared_ptr<PrefabCanvas> m_canvas;

    // UI Panels
    std::shared_ptr<PropertyInspectorPrefab> m_propertyInspector;

    // Schema management
    bool m_schemasLoaded = false;

    // UI State
    bool m_showComponentLibrary = true;
    bool m_showPropertyPanel = true;

    // Helper methods
    void InitializeSchemas();
    void SetupUI();
    void RenderMainLayout(float width, float height);
    void RenderNodePropertiesPanel();
    void RenderComponentLibraryPanel();

    // Callbacks
    void OnNodeSelected(NodeGraph::NodeId nodeId);
    void OnNodeDeselected(NodeGraph::NodeId nodeId);
    void OnPropertyChanged(
        const std::string& propertyName,
        const std::string& oldValue,
        const std::string& newValue
    );
};

}  // namespace OlympeEngine
