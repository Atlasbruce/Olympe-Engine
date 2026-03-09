/**
 * @file DebugController.h
 * @brief Runtime debug controller for ATS Visual Scripting (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * DebugController is a singleton state machine that manages breakpoints,
 * debug flow (Continue, Pause, Step Next/Into/Out), and the call stack for
 * SubGraph traversal.  It is designed to be called from both the editor UI
 * thread and the task execution thread, so all methods are mutex-protected.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "../TaskSystem/LocalBlackboard.h"
#include "../TaskSystem/TaskGraphTypes.h"

namespace Olympe {

// ============================================================================
// Supporting structures
// ============================================================================

/**
 * @struct BreakpointKey
 * @brief Uniquely identifies a breakpoint by graphID + nodeID.
 */
struct BreakpointKey {
    int graphID = -1;
    int nodeID  = -1;

    bool operator==(const BreakpointKey& o) const
    {
        return graphID == o.graphID && nodeID == o.nodeID;
    }
};

/**
 * @struct BreakpointInfo
 * @brief Describes a single breakpoint.
 */
struct BreakpointInfo {
    int         graphID   = -1;
    int         nodeID    = -1;
    std::string graphName;    ///< Human-readable graph file name
    std::string nodeName;     ///< Human-readable node name
    bool        enabled   = true;
};

/**
 * @struct SubGraphStackFrame
 * @brief A single frame in the debugger's SubGraph call stack.
 */
struct SubGraphStackFrame {
    int         graphID  = -1;
    int         nodeID   = -1;
    std::string graphName;
    std::string nodeName;
    bool        isCurrent = false;  ///< true for the top-most (active) frame
};

// ============================================================================
// DebugState enum
// ============================================================================

/**
 * @enum DebugState
 * @brief States of the debug controller state machine.
 */
enum class DebugState {
    NotDebugging,   ///< No active debug session
    Running,        ///< Normal execution
    Paused,         ///< Stopped at a breakpoint or manual pause
    StepNext,       ///< Execute next node then pause
    StepInto,       ///< Step into a SubGraph on next SubGraph node
    StepOut         ///< Run until the current SubGraph returns
};

// ============================================================================
// DebugController
// ============================================================================

/**
 * @class DebugController
 * @brief Singleton that manages runtime debugging of VS graphs.
 *
 * @details
 * Usage:
 * @code
 *   // Editor side
 *   DebugController::Get().SetBreakpoint(0, 3);
 *   DebugController::Get().StartDebugging(0);
 *
 *   // TaskSystem side (called from ExecuteFrame)
 *   DebugController::Get().OnNodeExecuting(graphID, nodeID, &blackboard);
 * @endcode
 *
 * Meyers singleton pattern (local static in Get()).
 * Thread-safe via std::mutex.
 */
class DebugController {
public:

    /**
     * @brief Returns the singleton instance (Meyers pattern).
     */
    static DebugController& Get();

    // -----------------------------------------------------------------------
    // Breakpoints
    // -----------------------------------------------------------------------

    /**
     * @brief Registers a breakpoint at (graphID, nodeID).
     * Safe to call multiple times — idempotent.
     */
    void SetBreakpoint(int graphID, int nodeID,
                       const std::string& graphName = "",
                       const std::string& nodeName  = "");

    /**
     * @brief Removes the breakpoint at (graphID, nodeID) if it exists.
     */
    void ClearBreakpoint(int graphID, int nodeID);

    /**
     * @brief Toggles the breakpoint at (graphID, nodeID).
     * Adds it if absent, removes it if present.
     */
    void ToggleBreakpoint(int graphID, int nodeID,
                          const std::string& graphName = "",
                          const std::string& nodeName  = "");

    /**
     * @brief Returns true if an enabled breakpoint exists at (graphID, nodeID).
     */
    bool HasBreakpoint(int graphID, int nodeID) const;

    /**
     * @brief Enables or disables an existing breakpoint.
     */
    void SetBreakpointEnabled(int graphID, int nodeID, bool enabled);

    /**
     * @brief Returns all registered breakpoints for @p graphID.
     */
    std::vector<BreakpointInfo> GetBreakpoints(int graphID) const;

    /**
     * @brief Returns all registered breakpoints across all graphs.
     */
    std::vector<BreakpointInfo> GetAllBreakpoints() const;

    /**
     * @brief Removes all breakpoints.
     */
    void ClearAllBreakpoints();

    // -----------------------------------------------------------------------
    // Debug flow
    // -----------------------------------------------------------------------

    /**
     * @brief Starts a debug session for the given graph.
     * Transitions from NotDebugging → Running.
     */
    void StartDebugging(int graphID);

    /**
     * @brief Stops the current debug session.
     * Transitions to NotDebugging from any state.
     */
    void StopDebugging();

    /**
     * @brief Resumes execution from Paused state.
     * Transitions Paused → Running.
     */
    void Continue();

    /**
     * @brief Pauses execution.
     * Transitions Running → Paused.
     */
    void Pause();

    /**
     * @brief Executes the next node then pauses.
     * Transitions Paused → StepNext.
     */
    void StepNext();

    /**
     * @brief Steps into a SubGraph if the next node is a SubGraph node.
     * Transitions Paused → StepInto.
     */
    void StepInto();

    /**
     * @brief Runs until the current SubGraph returns.
     * Transitions Paused → StepOut.
     */
    void StepOut();

    // -----------------------------------------------------------------------
    // Runtime state
    // -----------------------------------------------------------------------

    /** @brief Returns the current debug state. */
    DebugState GetState() const;

    /** @brief Returns true when a debug session is active. */
    bool IsDebugging() const;

    /** @brief Returns the currently debugged graphID (-1 if none). */
    int GetCurrentGraphID() const;

    /** @brief Returns the node being executed (-1 if none). */
    int GetCurrentNodeID() const;

    /**
     * @brief Returns a copy of the blackboard snapshot from the last
     *        OnNodeExecuting() call.
     */
    LocalBlackboard GetCurrentBlackboard() const;

    /**
     * @brief Returns the current SubGraph call stack (most recent at front).
     */
    std::vector<SubGraphStackFrame> GetCallStack() const;

    // -----------------------------------------------------------------------
    // Hooks called by TaskGraphExecutor / VSGraphExecutor
    // -----------------------------------------------------------------------

    /**
     * @brief Called by VSGraphExecutor before executing a node.
     *
     * Updates the current node/graph and blackboard snapshot.
     * If a breakpoint is hit and state is Running, transitions to Paused.
     * If state is StepNext, transitions to Paused after the first call.
     *
     * @note This method may busy-wait when the state is Paused (in debug
     *       mode with the UI thread calling Continue/StepNext).  In a
     *       headless test context, busy-wait is skipped.
     *
     * @param graphID   Graph being executed.
     * @param nodeID    Node about to be executed.
     * @param bb        Live blackboard pointer (snapshot is copied).
     */
    void OnNodeExecuting(int graphID, int nodeID, const LocalBlackboard* bb);

    /**
     * @brief Push a SubGraph frame onto the call stack.
     * Called by VSGraphExecutor when entering a SubGraph.
     */
    void PushCallFrame(int graphID, int nodeID,
                       const std::string& graphName = "",
                       const std::string& nodeName  = "");

    /**
     * @brief Pop the top SubGraph frame from the call stack.
     * Called by VSGraphExecutor when returning from a SubGraph.
     */
    void PopCallFrame();

    // -----------------------------------------------------------------------
    // Headless / test mode
    // -----------------------------------------------------------------------

    /**
     * @brief When true, OnNodeExecuting() never busy-waits on Paused.
     * Useful for unit tests where there is no UI thread to call Continue().
     */
    void SetHeadlessMode(bool headless);

private:

    DebugController();
    ~DebugController();

    // Non-copyable, non-movable
    DebugController(const DebugController&) = delete;
    DebugController& operator=(const DebugController&) = delete;

    // -----------------------------------------------------------------------
    // Internal helpers (called with mutex held)
    // -----------------------------------------------------------------------

    int MakeBreakpointKey(int graphID, int nodeID) const;
    bool HasBreakpointLocked(int graphID, int nodeID) const;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    mutable std::mutex m_mutex;

    DebugState m_state         = DebugState::NotDebugging;
    int        m_currentGraphID = -1;
    int        m_currentNodeID  = -1;
    bool       m_headlessMode   = false;

    LocalBlackboard m_bbSnapshot;

    /// Breakpoints keyed by (graphID * 100000 + nodeID)
    std::unordered_map<int, BreakpointInfo> m_breakpoints;

    /// Call stack (index 0 = bottom / entry, back = current)
    std::vector<SubGraphStackFrame> m_callStack;
};

} // namespace Olympe
