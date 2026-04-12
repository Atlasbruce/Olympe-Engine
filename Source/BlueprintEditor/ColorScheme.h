#pragma once

#include <cstdint>
#include "../third_party/imgui/imgui.h"

// Centralized color scheme for BehaviorTree nodes
// Provides semantic color mapping for all node types

namespace ColorScheme
{
    // ========== Entry Point Nodes (Green Tones) ==========
    // Root and OnEvent are entry points - they have no input pins
    constexpr ImU32 EntryPoint_Primary         = IM_COL32(0, 200, 80, 255);      // Darker green
    constexpr ImU32 EntryPoint_Hovered         = IM_COL32(0, 255, 100, 255);     // Brighter green (hovered)
    constexpr ImU32 EntryPoint_Selected        = IM_COL32(100, 255, 150, 255);   // Light green (selected)

    // ========== Composite Nodes (Dark Blue Tones) ==========
    // Selector, Sequence, Parallel - composite nodes that manage children
    constexpr ImU32 Composite_Primary          = IM_COL32(40, 80, 140, 255);     // Dark blue
    constexpr ImU32 Composite_Hovered          = IM_COL32(70, 110, 180, 255);    // Lighter dark blue (hovered)
    constexpr ImU32 Composite_Selected         = IM_COL32(100, 140, 210, 255);   // Very light dark blue (selected)

    // ========== Flow Control Nodes (Purple/Violet Tones) ==========
    // Condition nodes - control execution flow
    constexpr ImU32 FlowControl_Primary        = IM_COL32(100, 50, 140, 255);    // Purple
    constexpr ImU32 FlowControl_Hovered        = IM_COL32(130, 70, 180, 255);    // Lighter purple (hovered)
    constexpr ImU32 FlowControl_Selected       = IM_COL32(160, 100, 210, 255);   // Very light purple (selected)

    // ========== Action Nodes (Orange Tones) ==========
    // Action nodes are leaf execution nodes
    constexpr ImU32 Action_Primary             = IM_COL32(200, 120, 40, 255);    // Orange
    constexpr ImU32 Action_Hovered             = IM_COL32(220, 140, 60, 255);    // Lighter orange (hovered)
    constexpr ImU32 Action_Selected            = IM_COL32(240, 160, 80, 255);    // Very light orange (selected)

    // ========== Inverter Nodes (Purple-Plum Tones) ==========
    // Inverter nodes negate execution results
    constexpr ImU32 Inverter_Primary           = IM_COL32(140, 60, 120, 255);    // Purple-plum
    constexpr ImU32 Inverter_Hovered           = IM_COL32(160, 80, 140, 255);    // Lighter purple-plum (hovered)
    constexpr ImU32 Inverter_Selected          = IM_COL32(180, 100, 160, 255);   // Very light purple-plum (selected)

    // ========== Repeater Nodes (Yellow/Ochre Tones) ==========
    // Repeater nodes loop execution
    constexpr ImU32 Repeater_Primary           = IM_COL32(180, 150, 40, 255);    // Yellow/ochre
    constexpr ImU32 Repeater_Hovered           = IM_COL32(210, 180, 60, 255);    // Lighter yellow/ochre (hovered)
    constexpr ImU32 Repeater_Selected          = IM_COL32(240, 210, 80, 255);    // Very light yellow/ochre (selected)

    // ========== Special Nodes (Legacy Support) ==========
    // SubGraph nodes
    constexpr ImU32 SubGraph_Primary           = IM_COL32(80, 100, 140, 255);    // Steel blue
    constexpr ImU32 SubGraph_Hovered           = IM_COL32(110, 130, 170, 255);   // Lighter steel blue (hovered)
    constexpr ImU32 SubGraph_Selected          = IM_COL32(140, 160, 200, 255);   // Very light steel blue (selected)

    // Comment nodes (HFSM-specific)
    constexpr ImU32 Comment_Primary            = IM_COL32(100, 100, 100, 255);   // Neutral gray
    constexpr ImU32 Comment_Hovered            = IM_COL32(130, 130, 130, 255);   // Lighter gray (hovered)
    constexpr ImU32 Comment_Selected           = IM_COL32(160, 160, 160, 255);   // Very light gray (selected)

} // namespace ColorScheme
