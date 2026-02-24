/**
 * @file NodeStyleRegistry.h
 * @brief Per-NodeType visual style registry (header colour, icon, pin colours).
 * @author Olympe Engine
 *
 * @details
 * NodeStyleRegistry provides a singleton that maps NodeType values to
 * NodeStyle descriptors used when rendering nodes in the editor.  The
 * registry is populated once at construction and queried every frame; no
 * heap allocations occur during rendering.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <unordered_map>

#include "../third_party/imgui/imgui.h"
#include "BTNodeGraphManager.h"

namespace Olympe
{

/**
 * @struct NodeStyle
 * @brief Visual descriptor for a single node type.
 */
struct NodeStyle
{
    /// Title-bar background colour (ImNodes TitleBar colour slot).
    ImU32 headerColor         = IM_COL32(80, 80, 80, 255);
    /// Title-bar colour when the node is hovered.
    ImU32 headerHoveredColor  = IM_COL32(100, 100, 100, 255);
    /// Title-bar colour when the node is selected.
    ImU32 headerSelectedColor = IM_COL32(120, 120, 120, 255);
    /// Short ASCII icon displayed before the node title (no emoji/extended chars).
    const char* icon = "";
};

/**
 * @class NodeStyleRegistry
 * @brief Singleton providing NodeStyle descriptors per NodeType.
 *
 * @details
 * Usage:
 * @code
 *   const NodeStyle& style = NodeStyleRegistry::Get().GetStyle(node->type);
 *   ImNodes::PushColorStyle(ImNodesCol_TitleBar,         style.headerColor);
 *   ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered,  style.headerHoveredColor);
 *   ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, style.headerSelectedColor);
 *   ImNodes::BeginNode(id);
 *   // ...
 *   ImNodes::EndNode();
 *   ImNodes::PopColorStyle();
 *   ImNodes::PopColorStyle();
 *   ImNodes::PopColorStyle();
 * @endcode
 *
 * For exec-flow pins use GetExecPinColor(); for data pins GetDataPinColor().
 */
class NodeStyleRegistry
{
public:
    /// Returns the singleton instance.
    static NodeStyleRegistry& Get();

    /**
     * @brief Returns the style for the given node type.
     * Falls back to the default grey style for unknown types.
     */
    const NodeStyle& GetStyle(NodeType type) const;

    /**
     * @brief Returns the style to use for an atomic-task node identified by ID.
     * Currently returns the BT_Action style for all task IDs.
     */
    const NodeStyle& GetStyleByTaskID(const std::string& taskId) const;

    /// Colour for execution-flow (exec) pins — white.
    static ImU32 GetExecPinColor();

    /// Colour for data pins — green.
    static ImU32 GetDataPinColor();

private:
    NodeStyleRegistry();

    NodeStyle m_defaultStyle;

    /// Key is the underlying int value of NodeType.
    std::unordered_map<int, NodeStyle> m_styles;
};

} // namespace Olympe
