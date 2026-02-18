/**
 * @file BTNodeRegistry.cpp
 * @brief Implementation of BTNodeRegistry
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "BTNodeRegistry.h"
#include "../../system/system_utils.h"

namespace Olympe {
namespace AI {

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
        0xFF4488FF,  // Orange
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
        0xFF88FF44,  // Green
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
        0xFFFF8844,  // Cyan
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
        0xFFFF44FF,  // Magenta
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
        0xFFFF44FF,  // Magenta
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
        0xFFFF44FF,  // Magenta
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
        0xFFFF44FF,  // Magenta
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
        0xFFFF44FF,  // Magenta
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
        0xFF4444FF,  // Blue
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
        0xFF4444FF,  // Blue
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
        0xFF4444FF,  // Blue
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
        0xFF4444FF,  // Blue
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
        0xFF44FF44,  // Yellow
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
        0xFF44FF44,  // Yellow
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
        0xFF44FF44,  // Yellow
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
        0xFF44FF44,  // Yellow
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
        0xFF44FF44,  // Yellow
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
        0xFF44FF44,  // Yellow
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
        0xFF44FF44,  // Yellow
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
        0xFF44FF44,  // Yellow
        "S",         // Sound symbol
        0,           // No children
        0,           // No children
        true,        // Allows decorator
        {"soundId"}
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
