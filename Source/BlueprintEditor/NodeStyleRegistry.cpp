/**
 * @file NodeStyleRegistry.cpp
 * @brief Implementation of NodeStyleRegistry.
 * @author Olympe Engine
 *
 * @details
 * C++14 compliant - no C++17/20 features.
 */

#include "NodeStyleRegistry.h"
#include "ColorScheme.h"

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
    // ---- BT_Sequence: composite (dark blue) -------------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Composite_Primary;
        s.headerHoveredColor  = ColorScheme::Composite_Hovered;
        s.headerSelectedColor = ColorScheme::Composite_Selected;
        s.icon = "->";
        m_styles[static_cast<int>(NodeType::BT_Sequence)] = s;
    }

    // ---- BT_Selector: composite (dark blue) -----------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Composite_Primary;
        s.headerHoveredColor  = ColorScheme::Composite_Hovered;
        s.headerSelectedColor = ColorScheme::Composite_Selected;
        s.icon = "?";
        m_styles[static_cast<int>(NodeType::BT_Selector)] = s;
    }

    // ---- BT_Action: action (orange) ------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Action_Primary;
        s.headerHoveredColor  = ColorScheme::Action_Hovered;
        s.headerSelectedColor = ColorScheme::Action_Selected;
        s.icon = "A";
        m_styles[static_cast<int>(NodeType::BT_Action)] = s;
    }

    // ---- BT_Condition: flow control (purple) ---------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::FlowControl_Primary;
        s.headerHoveredColor  = ColorScheme::FlowControl_Hovered;
        s.headerSelectedColor = ColorScheme::FlowControl_Selected;
        s.icon = "C";
        m_styles[static_cast<int>(NodeType::BT_Condition)] = s;
    }

    // ---- BT_Decorator: inverter (purple-plum) ----------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Inverter_Primary;
        s.headerHoveredColor  = ColorScheme::Inverter_Hovered;
        s.headerSelectedColor = ColorScheme::Inverter_Selected;
        s.icon = "D";
        m_styles[static_cast<int>(NodeType::BT_Decorator)] = s;
    }

    // ---- BT_Root: entry point (green) ----------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::EntryPoint_Primary;
        s.headerHoveredColor  = ColorScheme::EntryPoint_Hovered;
        s.headerSelectedColor = ColorScheme::EntryPoint_Selected;
        s.icon = "R";
        m_styles[static_cast<int>(NodeType::BT_Root)] = s;
    }

    // ---- BT_OnEvent: entry point (green) ------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::EntryPoint_Primary;
        s.headerHoveredColor  = ColorScheme::EntryPoint_Hovered;
        s.headerSelectedColor = ColorScheme::EntryPoint_Selected;
        s.icon = "E";
        m_styles[static_cast<int>(NodeType::BT_OnEvent)] = s;
    }

    // ---- BT_Parallel: composite (dark blue) --------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Composite_Primary;
        s.headerHoveredColor  = ColorScheme::Composite_Hovered;
        s.headerSelectedColor = ColorScheme::Composite_Selected;
        s.icon = "||";
        m_styles[static_cast<int>(NodeType::BT_Parallel)] = s;
    }

    // ---- BT_RandomSelector: composite (dark blue) --------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Composite_Primary;
        s.headerHoveredColor  = ColorScheme::Composite_Hovered;
        s.headerSelectedColor = ColorScheme::Composite_Selected;
        s.icon = "R?";
        m_styles[static_cast<int>(NodeType::BT_RandomSelector)] = s;
    }

    // ---- BT_ParallelThreshold: composite (dark blue) -----------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Composite_Primary;
        s.headerHoveredColor  = ColorScheme::Composite_Hovered;
        s.headerSelectedColor = ColorScheme::Composite_Selected;
        s.icon = "≈";
        m_styles[static_cast<int>(NodeType::BT_ParallelThreshold)] = s;
    }

    // ---- BT_Inverter: inverter (purple-plum) --------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Inverter_Primary;
        s.headerHoveredColor  = ColorScheme::Inverter_Hovered;
        s.headerSelectedColor = ColorScheme::Inverter_Selected;
        s.icon = "!";
        m_styles[static_cast<int>(NodeType::BT_Inverter)] = s;
    }

    // ---- BT_Monitor: inverter (purple-plum) --------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Inverter_Primary;
        s.headerHoveredColor  = ColorScheme::Inverter_Hovered;
        s.headerSelectedColor = ColorScheme::Inverter_Selected;
        s.icon = "◉";
        m_styles[static_cast<int>(NodeType::BT_Monitor)] = s;
    }

    // ---- BT_Repeater: repeater (yellow) ------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Repeater_Primary;
        s.headerHoveredColor  = ColorScheme::Repeater_Hovered;
        s.headerSelectedColor = ColorScheme::Repeater_Selected;
        s.icon = "∞";
        m_styles[static_cast<int>(NodeType::BT_Repeater)] = s;
    }

    // ---- BT_UntilSuccess: repeater (yellow) --------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Repeater_Primary;
        s.headerHoveredColor  = ColorScheme::Repeater_Hovered;
        s.headerSelectedColor = ColorScheme::Repeater_Selected;
        s.icon = "▲";
        m_styles[static_cast<int>(NodeType::BT_UntilSuccess)] = s;
    }

    // ---- BT_UntilFailure: repeater (yellow) --------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Repeater_Primary;
        s.headerHoveredColor  = ColorScheme::Repeater_Hovered;
        s.headerSelectedColor = ColorScheme::Repeater_Selected;
        s.icon = "▼";
        m_styles[static_cast<int>(NodeType::BT_UntilFailure)] = s;
    }

    // ---- BT_Cooldown: inverter (purple-plum) --------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Inverter_Primary;
        s.headerHoveredColor  = ColorScheme::Inverter_Hovered;
        s.headerSelectedColor = ColorScheme::Inverter_Selected;
        s.icon = "⏱";
        m_styles[static_cast<int>(NodeType::BT_Cooldown)] = s;
    }

    // ---- BT_SendMessage: action (orange) -----------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Action_Primary;
        s.headerHoveredColor  = ColorScheme::Action_Hovered;
        s.headerSelectedColor = ColorScheme::Action_Selected;
        s.icon = "→";
        m_styles[static_cast<int>(NodeType::BT_SendMessage)] = s;
    }

    // ---- BT_SubGraph: subgraph (teal) --------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::SubGraph_Primary;
        s.headerHoveredColor  = ColorScheme::SubGraph_Hovered;
        s.headerSelectedColor = ColorScheme::SubGraph_Selected;
        s.icon = "⊕";
        m_styles[static_cast<int>(NodeType::BT_SubGraph)] = s;
    }

    // ---- HFSM_State: subgraph (teal) -----------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::SubGraph_Primary;
        s.headerHoveredColor  = ColorScheme::SubGraph_Hovered;
        s.headerSelectedColor = ColorScheme::SubGraph_Selected;
        s.icon = "S";
        m_styles[static_cast<int>(NodeType::HFSM_State)] = s;
    }

    // ---- HFSM_Transition: action (orange) ------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Action_Primary;
        s.headerHoveredColor  = ColorScheme::Action_Hovered;
        s.headerSelectedColor = ColorScheme::Action_Selected;
        s.icon = "T";
        m_styles[static_cast<int>(NodeType::HFSM_Transition)] = s;
    }

    // ---- Comment: comment (gray) ------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = ColorScheme::Comment_Primary;
        s.headerHoveredColor  = ColorScheme::Comment_Hovered;
        s.headerSelectedColor = ColorScheme::Comment_Selected;
        s.icon = "//";
        m_styles[static_cast<int>(NodeType::Comment)] = s;
    }

    // =========================================================================
    // VS node type styles (keyed by string name)
    // =========================================================================

    // ---- EntryPoint: dark violet -------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(90,  30, 140, 255);
        s.headerHoveredColor  = IM_COL32(115, 55, 165, 255);
        s.headerSelectedColor = IM_COL32(140, 80, 190, 255);
        s.icon = ">";
        m_vsStyles["EntryPoint"] = s;
    }

    // ---- Branch: orange ----------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(160,  90, 10, 255);
        s.headerHoveredColor  = IM_COL32(185, 115, 35, 255);
        s.headerSelectedColor = IM_COL32(210, 140, 60, 255);
        s.icon = "?";
        m_vsStyles["Branch"] = s;
    }

    // ---- Switch: dark orange -----------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(140, 70, 10, 255);
        s.headerHoveredColor  = IM_COL32(165, 95, 35, 255);
        s.headerSelectedColor = IM_COL32(190, 120, 60, 255);
        s.icon = "/";
        m_vsStyles["Switch"] = s;
    }

    // ---- VSSequence: light blue --------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(30, 100, 170, 255);
        s.headerHoveredColor  = IM_COL32(55, 125, 195, 255);
        s.headerSelectedColor = IM_COL32(80, 150, 220, 255);
        s.icon = "->";
        m_vsStyles["VSSequence"] = s;
    }

    // ---- While: dark green -------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(20, 110,  60, 255);
        s.headerHoveredColor  = IM_COL32(45, 135,  85, 255);
        s.headerSelectedColor = IM_COL32(70, 160, 110, 255);
        s.icon = "@";
        m_vsStyles["While"] = s;
    }

    // ---- ForEach: medium green ---------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(30, 130,  70, 255);
        s.headerHoveredColor  = IM_COL32(55, 155,  95, 255);
        s.headerSelectedColor = IM_COL32(80, 180, 120, 255);
        s.icon = "#";
        m_vsStyles["ForEach"] = s;
    }

    // ---- DoOnce: dark grey -------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(80, 80, 80, 255);
        s.headerHoveredColor  = IM_COL32(105, 105, 105, 255);
        s.headerSelectedColor = IM_COL32(130, 130, 130, 255);
        s.icon = "1";
        m_vsStyles["DoOnce"] = s;
    }

    // ---- Delay: blue-violet ------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(60, 60, 160, 255);
        s.headerHoveredColor  = IM_COL32(85, 85, 185, 255);
        s.headerSelectedColor = IM_COL32(110, 110, 210, 255);
        s.icon = "T";
        m_vsStyles["Delay"] = s;
    }

    // ---- GetBBValue: turquoise ---------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(20, 130, 130, 255);
        s.headerHoveredColor  = IM_COL32(45, 155, 155, 255);
        s.headerSelectedColor = IM_COL32(70, 180, 180, 255);
        s.icon = "R";
        m_vsStyles["GetBBValue"] = s;
    }

    // ---- SetBBValue: dark teal ---------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(20, 110, 110, 255);
        s.headerHoveredColor  = IM_COL32(45, 135, 135, 255);
        s.headerSelectedColor = IM_COL32(70, 160, 160, 255);
        s.icon = "W";
        m_vsStyles["SetBBValue"] = s;
    }

    // ---- MathOp: pink ------------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(160,  40, 100, 255);
        s.headerHoveredColor  = IM_COL32(185,  65, 125, 255);
        s.headerSelectedColor = IM_COL32(210,  90, 150, 255);
        s.icon = "=";
        m_vsStyles["MathOp"] = s;
    }

    // ---- SubGraph: golden --------------------------------------------------
    {
        NodeStyle s;
        s.headerColor         = IM_COL32(160, 130,  20, 255);
        s.headerHoveredColor  = IM_COL32(185, 155,  45, 255);
        s.headerSelectedColor = IM_COL32(210, 180,  70, 255);
        s.icon = "G";
        m_vsStyles["SubGraph"] = s;
    }

    // ---- AtomicTask (VS): reuse BT_Action style (orange) -------------------
    m_vsStyles["AtomicTask"] = m_styles[static_cast<int>(NodeType::BT_Action)];
}

// ============================================================================
// GetStyleByVSTypeName
// ============================================================================

const NodeStyle& NodeStyleRegistry::GetStyleByVSTypeName(const std::string& vsTypeName) const
{
    auto it = m_vsStyles.find(vsTypeName);
    if (it != m_vsStyles.end())
        return it->second;
    return m_defaultStyle;
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
    return IM_COL32(180, 100, 200, 255);
}

} // namespace Olympe
