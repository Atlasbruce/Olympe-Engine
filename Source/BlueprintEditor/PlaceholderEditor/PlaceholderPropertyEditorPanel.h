#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace Olympe {

class PlaceholderGraphDocument;
class PlaceholderNode;

/**
 * @class PlaceholderPropertyEditorPanel
 * @brief Property editor panel for Placeholder graph nodes
 * 
 * Supports single-node and multi-node property editing with
 * automatic difference detection for conflicting values.
 * Provides Apply/Reset buttons for batch editing operations.
 *
 * Features:
 * - Single-node: Show all 7 properties (Title, Filepath, Position X/Y, Width, Height, Enabled)
 * - Multi-node: Show common properties, indicate differences
 * - Batch editing: Apply property changes to multiple nodes simultaneously
 *
 * Integration:
 * - Phase 3: Uses GetSelectedNodeIds() from GraphEditorBase
 * - Updates via UpdateSelectedNodesProperty() framework callback
 * - Dirty flag tracking via SetDirty()
 *
 * C++14 compliant
 */
class PlaceholderPropertyEditorPanel
{
public:
    PlaceholderPropertyEditorPanel();
    ~PlaceholderPropertyEditorPanel();

    /// Initialize panel with document reference
    void Initialize(PlaceholderGraphDocument* document);

    /// Render property editor UI
    /// @param selectedNodeIds - Vector of currently selected node IDs from framework
    void Render(const std::vector<int>& selectedNodeIds);

    /// Apply pending property changes to all selected nodes
    void ApplyChanges();

    /// Reset property edit buffers to last saved values
    void ResetChanges();

    /// Clear all editing state and selection
    void Clear();

    /// Check if any properties have been edited
    bool HasChanges() const;

private:
    /// Structure for managing ImGui input state for a single property
    struct PropertyEditState
    {
        static const int BUFFER_SIZE = 256;
        char buffer[BUFFER_SIZE];
        bool isEditing;
        bool isDifferent;

        PropertyEditState() : isEditing(false), isDifferent(false)
        {
            buffer[0] = '\0';
        }

        std::string GetValue() const { return std::string(buffer); }

        void SetValue(const std::string& val)
        {
            strncpy_s(buffer, val.c_str(), BUFFER_SIZE - 1);
            buffer[BUFFER_SIZE - 1] = '\0';
        }
    };

    // ========== Rendering Methods ==========

    /// Render empty state when no nodes selected
    void RenderEmptyState();

    /// Render node info header (selection count, node ID)
    void RenderNodeInfo();

    /// Render all 7 property fields
    void RenderNodeProperties();

    /// Render single property field with ImGui
    void RenderPropertyField(const std::string& label, PropertyEditState& state);

    /// Render boolean property with checkbox
    void RenderBoolProperty(const std::string& label, PropertyEditState& state);

    // ========== Data Management ==========

    /// Collect property values from first selected node
    void CollectPropertyValues();

    /// Detect property differences across all selected nodes
    void DetectPropertyDifferences();

    /// Format float value for display (remove trailing zeros)
    static std::string FormatFloat(float value, int precision = 2);

    /// Parse float value from string with error handling
    static float ParseFloat(const std::string& str, float defaultValue = 0.0f);

    // ========== Member Variables ==========

    PlaceholderGraphDocument* m_document;
    std::vector<int> m_selectedNodeIds;
    int m_selectionCount;

    // Property edit state for each of 7 properties
    PropertyEditState m_titleEdit;
    PropertyEditState m_filepathEdit;
    PropertyEditState m_posXEdit;
    PropertyEditState m_posYEdit;
    PropertyEditState m_widthEdit;
    PropertyEditState m_heightEdit;
    PropertyEditState m_enabledEdit;
};

} // namespace Olympe
