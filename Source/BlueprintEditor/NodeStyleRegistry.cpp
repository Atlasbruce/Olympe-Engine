/**
 * @file NodeStyleRegistry.cpp
 * @brief Implementation of NodeStyleRegistry.
 * @author Olympe Engine
 *
 * @details
 * C++14 compliant - no C++17/20 features.
 */

#include "NodeStyleRegistry.h"

namespace Olympe
{

// ============================================================================
// Singleton
// ============================================================================

NodeStyleRegistry& NodeStyleRegistry::Get()
{
    static NodeStyleRegistry s_instance;
    return s_instance;
}

// ============================================================================
// Constructor - populate style table
// ============================================================================

NodeStyleRegistry::NodeStyleRegistry()
{
    // ---- BT_Sequence: blue ------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(41,  74, 122, 255);
        s.headerHoveredColor  = IM_COL32(66,  99, 149, 255);
        s.headerSelectedColor = IM_COL32(91, 124, 176, 255);
        s.icon = "->";
        m_styles[static_cast<int>(NodeType::BT_Sequence)] = s;
    }

    // ---- BT_Selector: green -----------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(41, 122,  74, 255);
        s.headerHoveredColor  = IM_COL32(66, 149,  99, 255);
        s.headerSelectedColor = IM_COL32(91, 176, 124, 255);
        s.icon = "?";
        m_styles[static_cast<int>(NodeType::BT_Selector)] = s;
    }

    // ---- BT_Action: orange ------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(150, 80,  20, 255);
        s.headerHoveredColor  = IM_COL32(180, 105, 45, 255);
        s.headerSelectedColor = IM_COL32(210, 130, 70, 255);
        s.icon = "A";
        m_styles[static_cast<int>(NodeType::BT_Action)] = s;
    }

    // ---- BT_Condition: yellow ---------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(140, 120, 20, 255);
        s.headerHoveredColor  = IM_COL32(168, 148, 45, 255);
        s.headerSelectedColor = IM_COL32(196, 176, 70, 255);
        s.icon = "C";
        m_styles[static_cast<int>(NodeType::BT_Condition)] = s;
    }

    // ---- BT_Decorator: purple ---------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(100, 40, 140, 255);
        s.headerHoveredColor  = IM_COL32(125, 65, 165, 255);
        s.headerSelectedColor = IM_COL32(150, 90, 190, 255);
        s.icon = "D";
        m_styles[static_cast<int>(NodeType::BT_Decorator)] = s;
    }

    // ---- HFSM_State: teal -------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(30, 110, 120, 255);
        s.headerHoveredColor  = IM_COL32(55, 135, 145, 255);
        s.headerSelectedColor = IM_COL32(80, 160, 170, 255);
        s.icon = "S";
        m_styles[static_cast<int>(NodeType::HFSM_State)] = s;
    }

    // ---- HFSM_Transition: red ---------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(140, 40,  40, 255);
        s.headerHoveredColor  = IM_COL32(165, 65,  65, 255);
        s.headerSelectedColor = IM_COL32(190, 90,  90, 255);
        s.icon = "T";
        m_styles[static_cast<int>(NodeType::HFSM_Transition)] = s;
    }

    // ---- Comment: dark grey -----------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(60, 60, 60, 255);
        s.headerHoveredColor  = IM_COL32(80, 80, 80, 255);
        s.headerSelectedColor = IM_COL32(100, 100, 100, 255);
        s.icon = "//";
        m_styles[static_cast<int>(NodeType::Comment)] = s;
    }
}

// ============================================================================
// GetStyle
// ============================================================================

const NodeStyle& NodeStyleRegistry::GetStyle(NodeType type) const
{
    auto it = m_styles.find(static_cast<int>(type));
    if (it != m_styles.end())
        return it->second;
    return m_defaultStyle;
}

// ============================================================================
// GetStyleByTaskID
// ============================================================================

const NodeStyle& NodeStyleRegistry::GetStyleByTaskID(const std::string& /*taskId*/) const
{
    // All atomic tasks use the Action style.
    auto it = m_styles.find(static_cast<int>(NodeType::BT_Action));
    if (it != m_styles.end())
        return it->second;
    return m_defaultStyle;
}

// ============================================================================
// Pin colours
// ============================================================================

ImU32 NodeStyleRegistry::GetExecPinColor()
{
    return IM_COL32(255, 255, 255, 255);
}

ImU32 NodeStyleRegistry::GetDataPinColor()
{
    return IM_COL32(100, 200, 100, 255);
}

} // namespace Olympe
