#include "PropertyInspectorPrefab.h"
#include "../../Source/third_party/imgui/imgui.h"

namespace Olympe
{
    PropertyInspectorPrefab::PropertyInspectorPrefab() { }
    PropertyInspectorPrefab::~PropertyInspectorPrefab() { }

    void PropertyInspectorPrefab::Initialize(EntityPrefabGraphDocument* document) { m_document = document; }
    void PropertyInspectorPrefab::Render() { if (!m_document) { return; } ImGui::Begin("Property Inspector"); ImGui::End(); }
    void PropertyInspectorPrefab::Update(float deltaTime) { (void)deltaTime; }

    void PropertyInspectorPrefab::SelectNode(NodeId nodeId) { m_selectedNodeId = nodeId; }
    void PropertyInspectorPrefab::DeselectNode() { m_selectedNodeId = InvalidNodeId; }
    NodeId PropertyInspectorPrefab::GetSelectedNode() const { return m_selectedNodeId; }

    void PropertyInspectorPrefab::SetPanelWidth(float width) { m_panelWidth = width; }
    float PropertyInspectorPrefab::GetPanelWidth() const { return m_panelWidth; }
    void PropertyInspectorPrefab::SetPanelHeight(float height) { m_panelHeight = height; }
    float PropertyInspectorPrefab::GetPanelHeight() const { return m_panelHeight; }

    void PropertyInspectorPrefab::SetPanelPosition(float x, float y) { m_panelX = x; m_panelY = y; }
    void PropertyInspectorPrefab::GetPanelPosition(float& outX, float& outY) const { outX = m_panelX; outY = m_panelY; }

    void PropertyInspectorPrefab::SetShowComponentInfo(bool show) { m_showComponentInfo = show; }
    bool PropertyInspectorPrefab::GetShowComponentInfo() const { return m_showComponentInfo; }
    void PropertyInspectorPrefab::SetShowPropertyGrid(bool show) { m_showPropertyGrid = show; }
    bool PropertyInspectorPrefab::GetShowPropertyGrid() const { return m_showPropertyGrid; }
    void PropertyInspectorPrefab::SetShowAddPropertyButton(bool show) { m_showAddPropertyButton = show; }
    bool PropertyInspectorPrefab::GetShowAddPropertyButton() const { return m_showAddPropertyButton; }

    void PropertyInspectorPrefab::SetReadOnly(bool readOnly) { m_readOnly = readOnly; }
    bool PropertyInspectorPrefab::IsReadOnly() const { return m_readOnly; }

    void PropertyInspectorPrefab::AddProperty(const std::string& key, const std::string& value) { (void)key; (void)value; }
    void PropertyInspectorPrefab::RemoveProperty(const std::string& key) { (void)key; }
    void PropertyInspectorPrefab::SetProperty(const std::string& key, const std::string& value) { (void)key; (void)value; }
    std::string PropertyInspectorPrefab::GetProperty(const std::string& key) const { (void)key; return ""; }

    void PropertyInspectorPrefab::AddComponent(const std::string& componentType) { (void)componentType; }
    void PropertyInspectorPrefab::RemoveComponent(const std::string& componentType) { (void)componentType; }
    void PropertyInspectorPrefab::EnableComponent(const std::string& componentType, bool enabled) { (void)componentType; (void)enabled; }

    bool PropertyInspectorPrefab::ValidatePropertyValue(const std::string& propertyName, const std::string& value) const { (void)propertyName; (void)value; return true; }
    bool PropertyInspectorPrefab::ValidateAllProperties() const { return true; }

    void PropertyInspectorPrefab::RenderComponentInfo() { }
    void PropertyInspectorPrefab::RenderPropertyGrid() { }
    void PropertyInspectorPrefab::RenderAddPropertyPanel() { }
    void PropertyInspectorPrefab::RenderComponentList() { }

    void PropertyInspectorPrefab::OnPropertyChanged(const std::string& key, const std::string& value) { (void)key; (void)value; }
    void PropertyInspectorPrefab::OnComponentAdded(const std::string& componentType) { (void)componentType; }
    void PropertyInspectorPrefab::OnComponentRemoved(const std::string& componentType) { (void)componentType; }

} // namespace Olympe
