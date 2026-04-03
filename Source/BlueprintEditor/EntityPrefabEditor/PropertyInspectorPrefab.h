#pragma once

#include "ComponentNodeData.h"
#include "PrefabLoader.h"
#include <string>
#include <map>
#include <functional>
#include <memory>

namespace OlympeEngine {

// ============================================================================
// PropertyInspectorPrefab - Property editing UI panel
// ============================================================================

class PropertyInspectorPrefab {
public:
    PropertyInspectorPrefab();
    ~PropertyInspectorPrefab();

    // Rendering
    void Render(float width, float height);
    void RenderComponentName();
    void RenderComponentType();
    void RenderComponentProperties();
    void RenderEntityCenterProperties();

    // Selection Management
    void SetSelectedNode(const ComponentNodeData& nodeData);
    void SetSelectedNode(const std::string& componentType, const std::string& componentName);
    void ClearSelection();
    bool HasSelection() const { return m_hasSelection; }

    // Property Management
    void SetProperty(const std::string& propertyName, const std::string& value);
    std::string GetProperty(const std::string& propertyName) const;
    const std::map<std::string, std::string>& GetAllProperties() const { return m_properties; }

    // Callbacks
    using OnPropertyChangedCallback = std::function<void(
        const std::string& propertyName,
        const std::string& oldValue,
        const std::string& newValue
    )>;

    void SetOnPropertyChangedCallback(OnPropertyChangedCallback callback);

    // UI State
    bool IsExpanded() const { return m_isExpanded; }
    void SetExpanded(bool expanded) { m_isExpanded = expanded; }

    // Component Description Display
    void DisplayComponentDescription(const std::string& componentType);
    void DisplayComponentDocumentation();

    // Property Type-Specific Rendering
    void RenderProperty(
        const std::string& propertyName,
        const std::string& propertyType,
        const std::string& currentValue
    );

    void RenderPropertyInt(
        const std::string& label,
        const std::string& currentValue
    );

    void RenderPropertyFloat(
        const std::string& label,
        const std::string& currentValue
    );

    void RenderPropertyBool(
        const std::string& label,
        const std::string& currentValue
    );

    void RenderPropertyString(
        const std::string& label,
        const std::string& currentValue
    );

    void RenderPropertyVector3(
        const std::string& label,
        const std::string& currentValue
    );

    void RenderPropertyEnum(
        const std::string& label,
        const std::string& currentValue,
        const std::vector<std::string>& enumValues
    );

    // Reset & Apply
    void ApplyChanges();
    void DiscardChanges();
    void ResetToDefaults();

    // Read-Only Mode (for Phase 1)
    void SetReadOnly(bool readOnly) { m_readOnly = readOnly; }
    bool IsReadOnly() const { return m_readOnly; }

private:
    bool m_hasSelection = false;
    bool m_isExpanded = true;
    bool m_readOnly = false;

    // Selected component info
    std::string m_selectedComponentType;
    std::string m_selectedComponentName;

    // Property buffers (for ImGui editing)
    std::map<std::string, std::string> m_properties;
    std::map<std::string, std::string> m_originalProperties;  // For discard

    // Callback
    OnPropertyChangedCallback m_onPropertyChanged;

    // Helper methods
    void RenderPropertyLabel(const std::string& label, const std::string& description = "");
    void RenderPropertyInput(
        const std::string& propertyName,
        const std::string& propertyType,
        std::string& value
    );

    bool ValidatePropertyValue(
        const std::string& propertyType,
        const std::string& value
    ) const;

    std::string FormatPropertyValue(
        const std::string& propertyType,
        const std::string& value
    ) const;

    // ImGui Input Wrappers
    bool InputIntProperty(const char* label, std::string& value);
    bool InputFloatProperty(const char* label, std::string& value);
    bool InputTextProperty(const char* label, std::string& value);
    bool InputBoolProperty(const char* label, std::string& value);
    bool InputVector3Property(const char* label, std::string& value);
};

}  // namespace OlympeEngine
