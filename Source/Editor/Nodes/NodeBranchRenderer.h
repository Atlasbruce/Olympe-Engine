/**
 * @file NodeBranchRenderer.h
 * @brief ImGui renderer for NodeBranch nodes with 4 distinct sections (Phase 24.4).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * NodeBranchRenderer draws a single NodeBranch inside the ImGui / ImNodes
 * graph canvas.  The node is divided into four visual sections separated by
 * horizontal dividers:
 *
 * @code
 * ┌─────────────────────────────────────────────────┐
 * │  Section 1 — Static pins                        │
 * │    ● In (input execution)                       │
 * ├─────────────────────────────────────────────────┤
 * │  Section 2 — Conditions (read-only)             │
 * │    [Start] [mHealth] <= [2]  (Condition #1)     │
 * │    [And  ] [mSpeed]  > [0]   (Fast)             │
 * ├─────────────────────────────────────────────────┤
 * │  Section 3 — Dynamic data pins                  │
 * │    ● In #1L: [mHealth] <= [Pin:1]  (yellow)    │
 * ├─────────────────────────────────────────────────┤
 * │  Section 4 — Execution flow pins                │
 * │    ● Then (true branch)                         │
 * │    ● Else (false branch)                        │
 * └─────────────────────────────────────────────────┘
 * @endcode
 *
 * Interaction:
 *   - Hover over a condition row → tooltip showing preset details.
 *   - Click a condition row → fires OnConditionClicked(conditionIndex).
 *   - Dynamic pins support drag/connect via ImNodes.
 *
 * Auto-update:
 *   Call NotifyPresetChanged(presetID) whenever a preset is updated so that
 *   the cached preview strings are refreshed.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../ConditionPreset/ConditionPreset.h"
#include "../ConditionPreset/ConditionPresetRegistry.h"
#include "../ConditionPreset/NodeConditionRef.h"
#include "../ConditionPreset/DynamicDataPin.h"
#include "../ConditionPreset/DynamicDataPinManager.h"

namespace Olympe {

/**
 * @struct NodeBranchData
 * @brief Lightweight snapshot of a NodeBranch required for rendering.
 *
 * @details
 * The renderer does not own the node; it receives a NodeBranchData reference
 * each frame so it remains stateless with respect to node ownership.
 */
struct NodeBranchData {
    std::string                   nodeID;          ///< Unique node identifier
    std::string                   nodeName;        ///< Display name (e.g. "Branch #3")
    std::vector<NodeConditionRef> conditionRefs;   ///< Ordered condition references
    std::vector<DynamicDataPin>   dynamicPins;     ///< Generated dynamic data pins
    bool                          breakpoint = false; ///< Whether a breakpoint is set
};

/**
 * @class NodeBranchRenderer
 * @brief Renders a NodeBranch using ImGui/ImNodes with 4 sections.
 *
 * @details
 * The renderer is stateless with respect to node data: it reads from the
 * NodeBranchData passed each frame.  Internal state is limited to hover/click
 * tracking for interaction.
 *
 * Usage:
 * @code
 *   NodeBranchRenderer renderer(registry, pinManager);
 *   // In the ImNodes frame:
 *   renderer.RenderNode(nodeID, branchData);
 * @endcode
 */
class NodeBranchRenderer {
public:

    /**
     * @brief Constructs the renderer with its dependencies.
     * @param registry    Global ConditionPresetRegistry.
     * @param pinManager  DynamicDataPinManager for the node being rendered.
     */
    NodeBranchRenderer(ConditionPresetRegistry& registry,
                       DynamicDataPinManager&   pinManager);

    ~NodeBranchRenderer() = default;

    // Non-copyable
    NodeBranchRenderer(const NodeBranchRenderer&) = delete;
    NodeBranchRenderer& operator=(const NodeBranchRenderer&) = delete;

    // -----------------------------------------------------------------------
    // Main render entry point
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the full NodeBranch node in the current ImNodes context.
     *
     * Must be called between ImNodes::BeginNode() and ImNodes::EndNode().
     * In headless builds this is a no-op.
     *
     * @param data  Current snapshot of the node's data.
     */
    void RenderNode(const NodeBranchData& data);

    // -----------------------------------------------------------------------
    // Section renderers (also exposed for unit testing / partial renders)
    // -----------------------------------------------------------------------

    /**
     * @brief Renders Section 1: static execution-input pin ("In").
     * @param data  Node data.
     */
    void RenderStaticInputPin(const NodeBranchData& data);

    /**
     * @brief Renders Section 2: read-only condition list.
     * @param data  Node data.
     */
    void RenderConditionsSection(const NodeBranchData& data);

    /**
     * @brief Renders Section 3: dynamic data pins (yellow).
     * @param data  Node data.
     */
    void RenderDynamicPinsSection(const NodeBranchData& data);

    /**
     * @brief Renders Section 4: execution flow output pins (Then / Else).
     * @param data  Node data.
     */
    void RenderExecutionFlowPins(const NodeBranchData& data);

    // -----------------------------------------------------------------------
    // Interaction state
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the index of the last condition row that was clicked, or
     *        -1 if none.
     */
    int GetLastClickedConditionIndex() const { return m_lastClickedCondition; }

    /**
     * @brief Resets the last-clicked condition index to -1.
     */
    void ClearLastClickedCondition() { m_lastClickedCondition = -1; }

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    /**
     * @brief Fired when the user clicks on a condition row.
     * Arg: zero-based condition index.
     */
    std::function<void(int)> OnConditionClicked;

    // -----------------------------------------------------------------------
    // Preset change notification
    // -----------------------------------------------------------------------

    /**
     * @brief Notifies the renderer that a preset has been updated.
     *
     * Marks the node as needing a label refresh.  The next call to RenderNode()
     * will recompute preview strings from the registry.
     *
     * @param changedPresetID  UUID of the modified preset.
     */
    void NotifyPresetChanged(const std::string& changedPresetID);

    // -----------------------------------------------------------------------
    // Testable state helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if any preset change notification is pending (dirty).
     */
    bool IsRefreshPending() const { return m_refreshPending; }

    /**
     * @brief Clears the refresh-pending flag (called after refresh is complete).
     */
    void ClearRefreshPending() { m_refreshPending = false; }

private:

    // -----------------------------------------------------------------------
    // Section helpers
    // -----------------------------------------------------------------------

    void RenderSectionHeader(const char* label);
    void RenderSectionSeparator();

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    ConditionPresetRegistry& m_registry;          ///< Shared global registry
    DynamicDataPinManager&   m_pinManager;        ///< Shared dynamic pin manager
    int                      m_lastClickedCondition = -1; ///< Interaction state
    bool                     m_refreshPending    = false;  ///< Dirty flag
};

} // namespace Olympe
