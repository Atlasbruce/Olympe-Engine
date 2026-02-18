/**
 * @file BTNodePalette.h
 * @brief ImGui palette for dragging BT nodes
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Provides an ImGui window showing all available BT node types organized
 * by category. Supports search filtering and drag-and-drop functionality.
 */

#pragma once

#include <string>

namespace Olympe {
namespace AI {

// Forward declarations
enum class BTNodeCategory : uint8_t;

/**
 * @class BTNodePalette
 * @brief UI palette for BT node selection
 *
 * @details
 * Renders an ImGui window with all registered BT node types. Users can
 * search and drag nodes to create them in the editor.
 */
class BTNodePalette {
public:
    BTNodePalette();
    ~BTNodePalette() = default;
    
    /**
     * @brief Render the palette window
     * @param isOpen Pointer to bool controlling window visibility
     */
    void Render(bool* isOpen);
    
    /**
     * @brief Get the node type being dragged (if any)
     * @return Type name or empty string
     */
    std::string GetDraggedNodeType() const { return m_draggedNodeType; }
    
    /**
     * @brief Check if a drag operation is in progress
     * @return true if dragging
     */
    bool IsDragging() const { return m_isDragging; }
    
    /**
     * @brief Reset drag state (call after drop)
     */
    void ResetDrag() { m_isDragging = false; m_draggedNodeType.clear(); }
    
private:
    /**
     * @brief Render a category section
     * @param categoryName Display name of category
     * @param category Category enum
     */
    void RenderCategory(const std::string& categoryName, BTNodeCategory category);
    
    /**
     * @brief Render a single node button
     * @param typeName Node type identifier
     */
    void RenderNodeButton(const std::string& typeName);
    
    std::string m_draggedNodeType;
    bool m_isDragging;
    char m_searchFilter[256];
};

} // namespace AI
} // namespace Olympe
