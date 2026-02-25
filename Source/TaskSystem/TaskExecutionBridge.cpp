/**
 * @file TaskExecutionBridge.cpp
 * @brief Implementation of TaskExecutionBridge.
 * @author Olympe Engine
 * @date 2026-02-25
 *
 * @details
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskExecutionBridge.h"
#include "TaskSystem.h"
#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Static member definitions
// ============================================================================

BridgeSetNodeFn TaskExecutionBridge::s_NodeFn    = nullptr;
BridgeSetBBFn   TaskExecutionBridge::s_BBFn      = nullptr;
bool            TaskExecutionBridge::s_Installed  = false;

// ============================================================================
// s_OnPublish (private static callback forwarded to TaskSystem)
// ============================================================================

void TaskExecutionBridge::s_OnPublish(EntityID /*entity*/,
                                      int nodeIndex,
                                      const LocalBlackboard* bb)
{
    if (s_NodeFn != nullptr)
    {
        s_NodeFn(nodeIndex);
    }

    if (s_BBFn != nullptr)
    {
        s_BBFn(bb);
    }
}

// ============================================================================
// Install
// ============================================================================

void TaskExecutionBridge::Install(BridgeSetNodeFn nodeFn, BridgeSetBBFn bbFn)
{
    s_NodeFn    = nodeFn;
    s_BBFn      = bbFn;
    s_Installed = true;

    TaskSystem::SetEditorPublishCallback(&TaskExecutionBridge::s_OnPublish);

    SYSTEM_LOG << "[TaskExecutionBridge] Installed (nodeFn="
               << (nodeFn != nullptr ? "set" : "null")
               << ", bbFn="
               << (bbFn != nullptr ? "set" : "null")
               << ")\n";
}

// ============================================================================
// Uninstall
// ============================================================================

void TaskExecutionBridge::Uninstall()
{
    TaskSystem::SetEditorPublishCallback(nullptr);
    s_NodeFn    = nullptr;
    s_BBFn      = nullptr;
    s_Installed = false;

    SYSTEM_LOG << "[TaskExecutionBridge] Uninstalled.\n";
}

// ============================================================================
// IsInstalled
// ============================================================================

bool TaskExecutionBridge::IsInstalled()
{
    return s_Installed;
}

} // namespace Olympe
