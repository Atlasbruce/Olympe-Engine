/**
 * @file TaskExecutionBridge.h
 * @brief Runtime bridge: routes live TaskRunner state to the Editor UI.
 * @author Olympe Engine
 * @date 2026-02-25
 *
 * @details
 * TaskExecutionBridge is a lightweight mediator between the TaskSystem and the
 * Editor layer.  It registers a callback with TaskSystem::SetEditorPublishCallback
 * and forwards the published runner state to Editor-side hooks (function
 * pointers) that have been registered by the Editor at startup.
 *
 * Design goals
 * ------------
 * - TaskExecutionBridge lives in Source/TaskSystem/ and has zero compile-time
 *   dependency on any Editor or ImGui headers.
 * - The Editor registers its static methods (e.g. InspectorPanel::SetDebugBlackboard
 *   and NodeGraphPanel::SetActiveDebugNode) as function-pointer hooks via Install().
 * - Non-owning semantics: the LocalBlackboard pointer forwarded to the BB hook is
 *   valid only for the duration of the callback invocation.
 * - Calls are guarded: if no hooks are installed the callback is a no-op.
 *
 * Usage (Editor startup)
 * ----------------------
 * @code
 *   // In WorldBridge or Editor initialization code:
 *   TaskExecutionBridge::Install(
 *       &NodeGraphPanel::SetActiveDebugNode,    // BridgeSetNodeFn
 *       &InspectorPanel::SetDebugBlackboardRaw  // BridgeSetBBFn
 *   );
 * @endcode
 *
 * Usage (Editor shutdown)
 * -----------------------
 * @code
 *   TaskExecutionBridge::Uninstall();
 * @endcode
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include "LocalBlackboard.h"
#include "../ECS_Entity.h"

namespace Olympe {

/// Callback type: receives the local node index being executed.
using BridgeSetNodeFn = void(*)(int nodeIndex);

/// Callback type: receives a non-owning pointer to the frame blackboard.
using BridgeSetBBFn   = void(*)(const LocalBlackboard* bb);

/**
 * @class TaskExecutionBridge
 * @brief Mediator that publishes per-frame task-runner state to the Editor.
 *
 * @details
 * All members are static; the class is not instantiable.  This mirrors the
 * pattern used by TaskSystem::SetEditorPublishCallback.
 */
class TaskExecutionBridge
{
public:

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Install the bridge and register Editor-side hooks.
     *
     * Registers a static callback with TaskSystem::SetEditorPublishCallback
     * and stores the provided hook functions.  Safe to call multiple times;
     * subsequent calls replace the previous hooks.
     *
     * @param nodeFn  Called each frame with the active local node index.
     *                Pass nullptr to skip node-highlight updates.
     * @param bbFn    Called each frame with a pointer to the live blackboard.
     *                Pass nullptr to skip blackboard updates.
     */
    static void Install(BridgeSetNodeFn nodeFn, BridgeSetBBFn bbFn);

    /**
     * @brief Uninstall the bridge.
     *
     * Clears the hooks and passes nullptr to
     * TaskSystem::SetEditorPublishCallback to disable publishing.
     */
    static void Uninstall();

    /**
     * @brief Returns true when the bridge is currently installed.
     */
    static bool IsInstalled();

private:

    /**
     * @brief The actual callback registered with TaskSystem.
     *
     * Forwards @p nodeIndex to s_NodeFn and @p bb to s_BBFn when they are
     * non-null.  A frame-local copy of the blackboard is made before calling
     * s_BBFn so the pointer remains valid for the duration of the call.
     */
    static void s_OnPublish(EntityID entity, int nodeIndex, const LocalBlackboard* bb);

    /// Stored Editor-side node hook.  nullptr = no-op.
    static BridgeSetNodeFn s_NodeFn;

    /// Stored Editor-side blackboard hook.  nullptr = no-op.
    static BridgeSetBBFn   s_BBFn;

    /// Whether the bridge is currently registered with TaskSystem.
    static bool s_Installed;
};

} // namespace Olympe
