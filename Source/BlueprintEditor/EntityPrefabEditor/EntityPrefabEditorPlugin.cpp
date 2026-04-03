#include "EntityPrefabEditorPlugin.h"
#include <imgui.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace OlympeEngine {

// ============================================================================
// Constructor & Destructor
// ============================================================================

EntityPrefabEditorPlugin::EntityPrefabEditorPlugin()
    : m_schemasLoaded(false),
      m_showComponentLibrary(true),
      m_showPropertyPanel(true) {
}

EntityPrefabEditorPlugin::~EntityPrefabEditorPlugin() {
    Shutdown();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool EntityPrefabEditorPlugin::Initialize() {
    // Initialize schema registry
    InitializeSchemas();

    // Create components
    m_propertyInspector = std::make_shared<PropertyInspectorPrefab>();
    m_propertyInspector->SetReadOnly(true);  // Phase 1: Read-only

    return true;
}

void EntityPrefabEditorPlugin::Shutdown() {
    m_canvas.reset();
    m_currentDocument.reset();
    m_propertyInspector.reset();
}

// ============================================================================
// File Handling
// ============================================================================

bool EntityPrefabEditorPlugin::CanHandle(const std::string& filePath) const {
    // Check if file is a valid prefab file
    return PrefabLoader::IsValidPrefabFile(filePath);
}

bool EntityPrefabEditorPlugin::CanHandle(GraphDocument::GraphType graphType) const {
    // Check if graph type is PREFAB_ENTITY (would need to be defined in GraphDocument)
    return false;  // Placeholder
}

// ============================================================================
// Document Management
// ============================================================================

std::shared_ptr<GraphDocument> EntityPrefabEditorPlugin::CreateNewDocument(const std::string& name) {
    auto doc = std::make_shared<EntityPrefabGraphDocument>();

    // Create a default entity prefab
    EntityPrefab prefab;
    prefab.name = name;
    prefab.author = "OlympeEngine";
    prefab.created = "2026-04-02";
    prefab.modified = prefab.created;

    doc->LoadFromPrefab(prefab);
    m_currentDocument = doc;

    // Create canvas
    m_canvas = std::make_shared<PrefabCanvas>(m_currentDocument);

    return doc;
}

std::shared_ptr<GraphDocument> EntityPrefabEditorPlugin::LoadDocument(const std::string& filePath) {
    auto doc = std::make_shared<EntityPrefabGraphDocument>();

    if (!doc->LoadFromFile(filePath)) {
        return nullptr;
    }

    m_currentDocument = doc;

    // Create canvas
    m_canvas = std::make_shared<PrefabCanvas>(m_currentDocument);
    m_canvas->FitToView(50.0f);

    return doc;
}

bool EntityPrefabEditorPlugin::SaveDocument(
    const std::shared_ptr<GraphDocument>& document,
    const std::string& filePath
) {
    // In Phase 1, this is read-only, so always return false
    // Phase 2+ will implement actual save
    return false;
}

// ============================================================================
// UI Rendering
// ============================================================================

void EntityPrefabEditorPlugin::RenderEditor(
    std::shared_ptr<GraphDocument> document,
    float width,
    float height
) {
    if (!document) return;

    m_currentDocument = std::dynamic_pointer_cast<EntityPrefabGraphDocument>(document);
    if (!m_currentDocument) return;

    if (!m_canvas) {
        m_canvas = std::make_shared<PrefabCanvas>(m_currentDocument);
    }

    RenderMainLayout(width, height);
}

void EntityPrefabEditorPlugin::RenderPropertyPanel() {
    if (!m_propertyInspector) return;

    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Prefab Properties")) {
        m_propertyInspector->Render(
            ImGui::GetContentRegionAvail().x,
            ImGui::GetContentRegionAvail().y
        );
    }
    ImGui::End();
}

void EntityPrefabEditorPlugin::RenderComponentLibrary() {
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Component Library")) {
        auto components = ComponentLibrary::Get().GetAvailableComponents();

        if (ImGui::BeginListBox("##components", ImVec2(-1, -1))) {
            for (const auto& componentType : components) {
                if (ImGui::Selectable(componentType.c_str())) {
                    ImGui::Spacing();
                    // In Phase 2+, add to library or drag-drop
                }
            }
            ImGui::EndListBox();
        }
    }
    ImGui::End();
}

// ============================================================================
// Validation
// ============================================================================

bool EntityPrefabEditorPlugin::ValidateDocument(
    const std::shared_ptr<GraphDocument>& document
) {
    auto prefabDoc = std::dynamic_pointer_cast<EntityPrefabGraphDocument>(document);
    if (!prefabDoc) return false;

    return prefabDoc->ValidateDocument();
}

std::vector<std::string> EntityPrefabEditorPlugin::GetValidationErrors() const {
    if (!m_currentDocument) return {};

    return m_currentDocument->GetValidationErrors();
}

// ============================================================================
// Entity Prefab Editor Specific Methods
// ============================================================================

EntityPrefab EntityPrefabEditorPlugin::LoadPrefabFile(const std::string& filePath) {
    return PrefabLoader::LoadFromFile(filePath);
}

bool EntityPrefabEditorPlugin::SavePrefabFile(const std::string& filePath) {
    if (!m_currentDocument) return false;

    EntityPrefab prefab = m_currentDocument->SerializeToPrefab();
    return PrefabLoader::SaveToFile(prefab, filePath);
}

// ============================================================================
// Canvas Operations
// ============================================================================

void EntityPrefabEditorPlugin::FitCanvasToView() {
    if (m_canvas) {
        m_canvas->FitToView(50.0f);
    }
}

void EntityPrefabEditorPlugin::ResetCanvasView() {
    if (m_canvas) {
        m_canvas->ResetView();
    }
}

void EntityPrefabEditorPlugin::SetCanvasZoom(float zoomLevel) {
    if (m_canvas) {
        auto& state = m_canvas->GetState();
        state.zoomLevel = zoomLevel;
    }
}

// ============================================================================
// Selection Management
// ============================================================================

void EntityPrefabEditorPlugin::SelectComponentNode(NodeGraph::NodeId nodeId) {
    if (m_currentDocument) {
        m_currentDocument->SelectNode(nodeId);
        OnNodeSelected(nodeId);
    }
}

void EntityPrefabEditorPlugin::DeselectComponentNode(NodeGraph::NodeId nodeId) {
    if (m_currentDocument) {
        m_currentDocument->DeselectNode(nodeId);
        OnNodeDeselected(nodeId);
    }
}

void EntityPrefabEditorPlugin::DeselectAllNodes() {
    if (m_currentDocument) {
        m_currentDocument->DeselectAll();
    }
}

// ============================================================================
// Component Operations
// ============================================================================

void EntityPrefabEditorPlugin::CreateComponentNode(
    const std::string& componentType,
    const glm::vec2& position
) {
    if (!m_currentDocument) return;

    m_currentDocument->CreateComponentNode(componentType, componentType + "_node", position);
}

void EntityPrefabEditorPlugin::RemoveSelectedComponentNodes() {
    if (!m_currentDocument) return;

    auto selected = m_currentDocument->GetSelectedNodes();
    for (NodeGraph::NodeId nodeId : selected) {
        m_currentDocument->RemoveComponentNode(nodeId);
    }
}

// ============================================================================
// Property Inspector
// ============================================================================

void EntityPrefabEditorPlugin::UpdatePropertyInspectorForNode(NodeGraph::NodeId nodeId) {
    if (!m_propertyInspector || !m_currentDocument) return;

    auto node = m_currentDocument->GetComponentNode(nodeId);
    if (node) {
        ComponentNodeData nodeData(node->componentType);
        nodeData.SetComponentName(node->componentName);
        m_propertyInspector->SetSelectedNode(nodeData);
    }
}

// ============================================================================
// Layout
// ============================================================================

void EntityPrefabEditorPlugin::AutoLayoutComponents() {
    if (m_currentDocument) {
        m_currentDocument->AutoLayoutComponents(300.0f);
    }
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void EntityPrefabEditorPlugin::InitializeSchemas() {
    if (m_schemasLoaded) return;

    // Load schemas from default location
    // This path would be configurable in the actual implementation
    std::string schemaPath = "Assets/Schemas/ComponentSchemas.json";
    if (fs::exists(schemaPath)) {
        PrefabLoader::LoadParameterSchemas(schemaPath);
    }

    // Also load via ComponentLibrary
    ComponentLibrary::Get().Initialize("Assets/Schemas/");

    m_schemasLoaded = true;
}

void EntityPrefabEditorPlugin::SetupUI() {
    // Placeholder for UI setup
}

void EntityPrefabEditorPlugin::RenderMainLayout(float width, float height) {
    // Create main tab bar or splitter for different panels
    // Phase 1: Simple layout with canvas and property panel side-by-side

    float canvasWidth = width * 0.7f;
    float panelWidth = width * 0.3f;

    // Render canvas
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(canvasWidth, height), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Prefab Canvas")) {
        if (m_canvas) {
            m_canvas->Render(
                ImGui::GetContentRegionAvail().x,
                ImGui::GetContentRegionAvail().y
            );
        }
    }
    ImGui::End();

    // Render property panel
    ImGui::SetNextWindowPos(ImVec2(canvasWidth, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, height), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Prefab Properties")) {
        if (m_propertyInspector) {
            m_propertyInspector->Render(panelWidth, height);
        }
    }
    ImGui::End();
}

void EntityPrefabEditorPlugin::RenderNodePropertiesPanel() {
    RenderPropertyPanel();
}

void EntityPrefabEditorPlugin::RenderComponentLibraryPanel() {
    RenderComponentLibrary();
}

// ============================================================================
// Callbacks
// ============================================================================

void EntityPrefabEditorPlugin::OnNodeSelected(NodeGraph::NodeId nodeId) {
    UpdatePropertyInspectorForNode(nodeId);
}

void EntityPrefabEditorPlugin::OnNodeDeselected(NodeGraph::NodeId nodeId) {
    // Clear property inspector when node is deselected
    if (m_propertyInspector) {
        m_propertyInspector->ClearSelection();
    }
}

void EntityPrefabEditorPlugin::OnPropertyChanged(
    const std::string& propertyName,
    const std::string& oldValue,
    const std::string& newValue
) {
    // In Phase 1, properties are read-only
    // Phase 2+: Create CommandStack entry for undo/redo
}

}  // namespace OlympeEngine
