/**
 * @file DebugController.cpp
 * @brief Implementation of the ATS VS debug controller (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "DebugController.h"

#include <chrono>
#include <thread>
#include <iostream>

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

DebugController& DebugController::Get()
{
    static DebugController instance;
    return instance;
}

DebugController::DebugController()
{
}

DebugController::~DebugController()
{
}

// ============================================================================
// Internal helpers
// ============================================================================

int DebugController::MakeBreakpointKey(int graphID, int nodeID) const
{
    // Combine graphID and nodeID into a single integer key.
    // Assumes nodeID < 100000 and graphID < 100000.
    return graphID * 100000 + nodeID;
}

bool DebugController::HasBreakpointLocked(int graphID, int nodeID) const
{
    int key = MakeBreakpointKey(graphID, nodeID);
    auto it = m_breakpoints.find(key);
    if (it == m_breakpoints.end())
        return false;
    return it->second.enabled;
}

// ============================================================================
// Breakpoints
// ============================================================================

void DebugController::SetBreakpoint(int graphID, int nodeID,
                                    const std::string& graphName,
                                    const std::string& nodeName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int key = MakeBreakpointKey(graphID, nodeID);
    BreakpointInfo bp;
    bp.graphID   = graphID;
    bp.nodeID    = nodeID;
    bp.graphName = graphName;
    bp.nodeName  = nodeName;
    bp.enabled   = true;
    m_breakpoints[key] = bp;
}

void DebugController::ClearBreakpoint(int graphID, int nodeID)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int key = MakeBreakpointKey(graphID, nodeID);
    m_breakpoints.erase(key);
}

void DebugController::ToggleBreakpoint(int graphID, int nodeID,
                                       const std::string& graphName,
                                       const std::string& nodeName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int key = MakeBreakpointKey(graphID, nodeID);
    auto it = m_breakpoints.find(key);
    if (it != m_breakpoints.end())
    {
        m_breakpoints.erase(it);
    }
    else
    {
        BreakpointInfo bp;
        bp.graphID   = graphID;
        bp.nodeID    = nodeID;
        bp.graphName = graphName;
        bp.nodeName  = nodeName;
        bp.enabled   = true;
        m_breakpoints[key] = bp;
    }
}

bool DebugController::HasBreakpoint(int graphID, int nodeID) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return HasBreakpointLocked(graphID, nodeID);
}

void DebugController::SetBreakpointEnabled(int graphID, int nodeID, bool enabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int key = MakeBreakpointKey(graphID, nodeID);
    auto it = m_breakpoints.find(key);
    if (it != m_breakpoints.end())
    {
        it->second.enabled = enabled;
    }
}

std::vector<BreakpointInfo> DebugController::GetBreakpoints(int graphID) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<BreakpointInfo> result;
    for (auto& kv : m_breakpoints)
    {
        if (kv.second.graphID == graphID)
            result.push_back(kv.second);
    }
    return result;
}

std::vector<BreakpointInfo> DebugController::GetAllBreakpoints() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<BreakpointInfo> result;
    for (auto& kv : m_breakpoints)
    {
        result.push_back(kv.second);
    }
    return result;
}

void DebugController::ClearAllBreakpoints()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_breakpoints.clear();
}

// ============================================================================
// Debug flow
// ============================================================================

void DebugController::StartDebugging(int graphID)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state          = DebugState::Running;
    m_currentGraphID = graphID;
    m_currentNodeID  = -1;
    m_callStack.clear();
}

void DebugController::StopDebugging()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state          = DebugState::NotDebugging;
    m_currentGraphID = -1;
    m_currentNodeID  = -1;
    m_callStack.clear();
}

void DebugController::Continue()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state == DebugState::Paused)
    {
        m_state = DebugState::Running;
    }
}

void DebugController::Pause()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state == DebugState::Running)
    {
        m_state = DebugState::Paused;
    }
}

void DebugController::StepNext()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state == DebugState::Paused)
    {
        m_state = DebugState::StepNext;
    }
}

void DebugController::StepInto()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state == DebugState::Paused)
    {
        m_state = DebugState::StepInto;
    }
}

void DebugController::StepOut()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state == DebugState::Paused)
    {
        m_state = DebugState::StepOut;
    }
}

// ============================================================================
// Runtime state accessors
// ============================================================================

DebugState DebugController::GetState() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

bool DebugController::IsDebugging() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state != DebugState::NotDebugging;
}

int DebugController::GetCurrentGraphID() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentGraphID;
}

int DebugController::GetCurrentNodeID() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentNodeID;
}

LocalBlackboard DebugController::GetCurrentBlackboard() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bbSnapshot;
}

std::vector<SubGraphStackFrame> DebugController::GetCallStack() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_callStack;
}

// ============================================================================
// Hooks
// ============================================================================

void DebugController::OnNodeExecuting(int graphID, int nodeID,
                                      const LocalBlackboard* bb)
{
    // Update current node and blackboard snapshot under lock, then check
    // whether to pause.
    DebugState stateSnapshot;
    bool shouldPause = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_state == DebugState::NotDebugging)
            return;

        m_currentGraphID = graphID;
        m_currentNodeID  = nodeID;

        if (bb != nullptr)
            m_bbSnapshot = *bb;

        stateSnapshot = m_state;

        // Breakpoint hit in Running state
        if (stateSnapshot == DebugState::Running &&
            HasBreakpointLocked(graphID, nodeID))
        {
            m_state   = DebugState::Paused;
            shouldPause = true;
        }
        // StepNext: pause immediately after this call
        else if (stateSnapshot == DebugState::StepNext)
        {
            m_state   = DebugState::Paused;
            shouldPause = true;
        }
        // StepInto: pause (entering a SubGraph will be handled separately)
        else if (stateSnapshot == DebugState::StepInto)
        {
            m_state   = DebugState::Paused;
            shouldPause = true;
        }
    }

    // Busy-wait loop — only runs when not in headless/test mode
    if (shouldPause && !m_headlessMode)
    {
        while (true)
        {
            DebugState cur;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                cur = m_state;
            }
            if (cur != DebugState::Paused)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void DebugController::PushCallFrame(int graphID, int nodeID,
                                    const std::string& graphName,
                                    const std::string& nodeName)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Mark previous top frame as non-current
    if (!m_callStack.empty())
        m_callStack.back().isCurrent = false;

    SubGraphStackFrame frame;
    frame.graphID   = graphID;
    frame.nodeID    = nodeID;
    frame.graphName = graphName;
    frame.nodeName  = nodeName;
    frame.isCurrent = true;
    m_callStack.push_back(frame);
}

void DebugController::PopCallFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_callStack.empty())
        m_callStack.pop_back();

    if (!m_callStack.empty())
        m_callStack.back().isCurrent = true;

    // If we were stepping out and we've now popped back, pause.
    if (m_state == DebugState::StepOut)
    {
        m_state = DebugState::Paused;
    }
}

// ============================================================================
// Headless mode
// ============================================================================

void DebugController::SetHeadlessMode(bool headless)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_headlessMode = headless;
}

} // namespace Olympe
