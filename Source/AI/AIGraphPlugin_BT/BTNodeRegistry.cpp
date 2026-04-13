/**
 * @file BTNodeRegistry.cpp
 * @brief Implementation of BTNodeRegistry
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "BTNodeRegistry.h"
#include "../../system/system_utils.h"
#include "../../BlueprintEditor/ColorScheme.h"

namespace Olympe {
namespace AI {

// Helper to convert IM_COL32 (RRGGBBAA) to registry color format (AABBGGRR)
inline uint32_t ConvertColorFormat(ImU32 imguiColor) {
    uint32_t r = (imguiColor >> 0) & 0xFF;
    uint32_t g = (imguiColor >> 8) & 0xFF;
    uint32_t b = (imguiColor >> 16) & 0xFF;
    uint32_t a = (imguiColor >> 24) & 0xFF;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

BTNodeRegistry& BTNodeRegistry::Get() {
    static BTNodeRegistry instance;
    return instance;
}

BTNodeRegistry::BTNodeRegistry() {
    InitializeBuiltInTypes();
}

void BTNodeRegistry::InitializeBuiltInTypes() {
    // ========================================================================
    // COMPOSITES (Flow Control)
    // ========================================================================
    
    RegisterNodeType({
        "BT_Selector",
        "Selector",
        "Executes children until one succeeds (OR logic)",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::FlowControl_Primary),
        "+",         // Plus symbol
        1,           // Min 1 child
        -1,          // Unlimited children
        true,        // Allows decorator
        {}           // No parameters
    });
    
    RegisterNodeType({
        "BT_Sequence",
        "Sequence",
        "Executes children until one fails (AND logic)",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::FlowControl_Primary),
        "->",        // Arrow
        1,           // Min 1 child
        -1,          // Unlimited children
        true,        // Allows decorator
        {}           // No parameters
    });
    
    RegisterNodeType({
        "BT_Parallel",
        "Parallel",
        "Executes all children simultaneously",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::Composite_Primary),
        "||",        // Parallel bars
        2,           // Min 2 children
        -1,          // Unlimited children
        true,        // Allows decorator
        {}           // No parameters
    });
    
    // ========================================================================
    // DECORATORS (Modifiers)
    // ========================================================================
    
    RegisterNodeType({
        "BT_Inverter",
        "Inverter",
        "Inverts child result (SUCCESS <-> FAILURE)",
        BTNodeCategory::Decorator,
        ConvertColorFormat(ColorScheme::Inverter_Primary),
        "!",         // Not symbol
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on decorator
        {}           // No parameters
    });
    
    RegisterNodeType({
        "BT_Repeater",
        "Repeater",
        "Repeats child N times",
        BTNodeCategory::Decorator,
        ConvertColorFormat(ColorScheme::Repeater_Primary),
        "@",         // At symbol
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on decorator
        {"repeatCount"}
    });
    
    RegisterNodeType({
        "BT_UntilSuccess",
        "Until Success",
        "Repeats child until it succeeds",
        BTNodeCategory::Decorator,
        ConvertColorFormat(ColorScheme::Repeater_Primary),
        "^",         // Up arrow
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on decorator
        {}
    });
    
    RegisterNodeType({
        "BT_UntilFailure",
        "Until Failure",
        "Repeats child until it fails",
        BTNodeCategory::Decorator,
        ConvertColorFormat(ColorScheme::Repeater_Primary),
        "v",         // Down arrow
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on decorator
        {}
    });
    
    RegisterNodeType({
        "BT_Cooldown",
        "Cooldown",
        "Limits execution frequency",
        BTNodeCategory::Decorator,
        ConvertColorFormat(ColorScheme::Inverter_Primary),
        "#",         // Hash symbol
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on decorator
        {"cooldownDuration"}
    });
    
    // ========================================================================
    // CONDITIONS (Boolean Checks)
    // ========================================================================
    
    RegisterNodeType({
        "BT_CheckBlackboardValue",
        "Check Blackboard Value",
        "Compares blackboard value against expected value",
        BTNodeCategory::Condition,
        ConvertColorFormat(ColorScheme::FlowControl_Primary),
        "?",         // Question mark
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"key", "operator", "value"}
    });
    
    RegisterNodeType({
        "BT_HasTarget",
        "Has Target",
        "Checks if entity has a target",
        BTNodeCategory::Condition,
        ConvertColorFormat(ColorScheme::FlowControl_Primary),
        "T",         // Target symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {}
    });
    
    RegisterNodeType({
        "BT_IsTargetInRange",
        "Is Target In Range",
        "Checks if target is within specified distance",
        BTNodeCategory::Condition,
        ConvertColorFormat(ColorScheme::FlowControl_Primary),
        "R",         // Range symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"distance"}
    });
    
    RegisterNodeType({
        "BT_CanSeeTarget",
        "Can See Target",
        "Checks line of sight to target",
        BTNodeCategory::Condition,
        ConvertColorFormat(ColorScheme::FlowControl_Primary),
        "E",         // Eye symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {}
    });
    
    // ========================================================================
    // ACTIONS (Leaf Execution Nodes)
    // ========================================================================
    
    RegisterNodeType({
        "BT_Wait",
        "Wait",
        "Waits for N seconds",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "W",         // Wait symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"duration"}
    });
    
    RegisterNodeType({
        "BT_WaitRandomTime",
        "Wait Random Time",
        "Waits for random duration",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "w",         // Small wait
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"minDuration", "maxDuration"}
    });
    
    RegisterNodeType({
        "BT_SetBlackboardValue",
        "Set Blackboard Value",
        "Modifies blackboard value",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "=",         // Equals symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"key", "value"}
    });
    
    RegisterNodeType({
        "BT_MoveToTarget",
        "Move To Target",
        "Moves entity towards target",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "M",         // Move symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"speed"}
    });
    
    RegisterNodeType({
        "BT_MoveToPosition",
        "Move To Position",
        "Moves entity to specific position",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "P",         // Position symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"x", "y"}
    });
    
    RegisterNodeType({
        "BT_AttackTarget",
        "Attack Target",
        "Attacks current target",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "A",         // Attack symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {}
    });
    
    RegisterNodeType({
        "BT_PlayAnimation",
        "Play Animation",
        "Plays specified animation",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "*",         // Star symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"animationName"}
    });
    
    RegisterNodeType({
        "BT_EmitSound",
        "Emit Sound",
        "Plays sound effect",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "S",         // Sound symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"soundId"}
    });

    // ========================================================================
    // ROOT NODE (Entry Point)
    // ========================================================================

    RegisterNodeType({
        "BT_Root",
        "Root",
        "Entry point for behavior tree execution (auto-created, undeletable)",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::EntryPoint_Primary),
        "▶",         // Play symbol
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on root
        {}           // No parameters
    });

    // ========================================================================
    // EVENT NODES (Event-Driven Execution)
    // ========================================================================

    RegisterNodeType({
        "BT_OnEvent",
        "On Event",
        "Entry point triggered by EventQueue message (parameterized by event type)",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::EntryPoint_Primary),
        "📨",        // Envelope symbol
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on event root
        {"eventType", "eventMessage"}  // Filter by event type and optional message
    });

    // ========================================================================
    // NEW COMPOSITE NODES
    // ========================================================================

    RegisterNodeType({
        "BT_RandomSelector",
        "Random Selector",
        "Selects a random child to execute (non-deterministic)",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::Composite_Primary),
        "?",         // Question mark
        1,           // Min 1 child
        -1,          // Unlimited children
        true,        // Allows decorator
        {}           // No parameters
    });

    RegisterNodeType({
        "BT_ParallelThreshold",
        "Parallel Threshold",
        "Executes children in parallel with success/failure thresholds",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::Composite_Primary),
        "≈",         // Threshold symbol
        2,           // Min 2 children
        -1,          // Unlimited children
        true,        // Allows decorator
        {"successThreshold", "failureThreshold"}  // Number of children that must succeed/fail
    });

    // ========================================================================
    // NEW DECORATOR NODES
    // ========================================================================

    RegisterNodeType({
        "BT_Monitor",
        "Monitor",
        "Continuously re-evaluates condition during child execution",
        BTNodeCategory::Decorator,
        ConvertColorFormat(ColorScheme::Inverter_Primary),
        "◉",         // Circle symbol (monitoring)
        1,           // Exactly 1 child
        1,           // Exactly 1 child
        false,       // No decorator on decorator
        {"checkInterval"}  // Re-evaluation interval in milliseconds
    });

    // ========================================================================
    // MESSAGE SENDING ACTION
    // ========================================================================

    RegisterNodeType({
        "BT_SendMessage",
        "Send Message",
        "Emit event to EventQueue for other systems to receive",
        BTNodeCategory::Action,
        ConvertColorFormat(ColorScheme::Action_Primary),
        "→",         // Arrow (message send)
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"eventType", "domain", "param1", "param2", "state"}  // Message parameters
    });

    // ========================================================================
    // SUBGRAPH NODES (Behavior Composition & Reusability)
    // ========================================================================

    RegisterNodeType({
        "BT_SubGraph",
        "SubGraph",
        "Encapsulates and executes another behavior tree (enables reusability and modularity)",
        BTNodeCategory::Composite,
        ConvertColorFormat(ColorScheme::SubGraph_Primary),
        "◈",         // Diamond symbol (encapsulation)
        0,           // No children (children come from loaded SubGraph)
        0,           // No children
        true,        // Allows decorator
        {"subgraphPath", "inputParamCount", "outputParamCount", "inputBindingCount", "outputBindingCount"}
    });

    SYSTEM_LOG << "[BTNodeRegistry] Initialized " << m_nodeTypes.size() << " built-in node types" << std::endl;
}

void BTNodeRegistry::RegisterNodeType(const BTNodeTypeInfo& info) {
    m_nodeTypes[info.typeName] = info;
}

const BTNodeTypeInfo* BTNodeRegistry::GetNodeTypeInfo(const std::string& typeName) const {
    auto it = m_nodeTypes.find(typeName);
    return (it != m_nodeTypes.end()) ? &it->second : nullptr;
}

std::vector<std::string> BTNodeRegistry::GetAllNodeTypes() const {
    std::vector<std::string> result;
    result.reserve(m_nodeTypes.size());
    
    for (auto it = m_nodeTypes.begin(); it != m_nodeTypes.end(); ++it) {
        result.push_back(it->first);
    }
    
    return result;
}

std::vector<std::string> BTNodeRegistry::GetNodeTypesByCategory(BTNodeCategory category) const {
    std::vector<std::string> result;
    
    for (auto it = m_nodeTypes.begin(); it != m_nodeTypes.end(); ++it) {
        if (it->second.category == category) {
            result.push_back(it->first);
        }
    }
    
    return result;
}

bool BTNodeRegistry::IsValidNodeType(const std::string& typeName) const {
    return m_nodeTypes.find(typeName) != m_nodeTypes.end();
}

bool BTNodeRegistry::CanHaveChildren(const std::string& typeName) const {
    const BTNodeTypeInfo* info = GetNodeTypeInfo(typeName);
    if (info == nullptr) {
        return false;
    }
    
    return info->maxChildren != 0;
}

int BTNodeRegistry::GetMinChildren(const std::string& typeName) const {
    const BTNodeTypeInfo* info = GetNodeTypeInfo(typeName);
    if (info == nullptr) {
        return 0;
    }
    
    return info->minChildren;
}

int BTNodeRegistry::GetMaxChildren(const std::string& typeName) const {
    const BTNodeTypeInfo* info = GetNodeTypeInfo(typeName);
    if (info == nullptr) {
        return 0;
    }
    
    return info->maxChildren;
}

} // namespace AI
} // namespace Olympe
