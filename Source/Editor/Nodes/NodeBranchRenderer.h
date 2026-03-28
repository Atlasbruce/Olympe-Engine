/**
 * @file NodeBranchRenderer.h
 * @brief ImGui renderer for NodeBranch nodes with 4 distinct sections (Phase 24-REFONTE).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * NodeBranchRenderer draws a single NodeBranch inside the ImGui / ImNodes
 * graph canvas.  The node is divided into four visual sections separated by
 * horizontal dividers, matching the validated mockup:
 *
 * @code
 * ┌─────────────────────────────────────────────────┐
 * │  Section 1 — Title bar (blue background)        │
 * │    Is Health Critical?                          │
 * ├─────────────────────────────────────────────────┤
 * │  Section 2 — Exec pins (static, never editable) │
 * │    >> In                         Then >>        │
 * │                                  Else >>        │
 * ├─────────────────────────────────────────────────┤
 * │  Section 3 — Conditions preview (READ-ONLY)     │
 * │      [mHealth] <= [2]       (green, monospace)  │
 * │  And [mSpeed] <= [100.00]                       │
 * │  Or  [mSpeed] == [Pin : 1]                      │
 * ├─────────────────────────────────────────────────┤
 * │  Section 4 — Dynamic data pins (yellow, cond.)  │
 * │  * In #1: [mSpeed] == [Pin : 1]                │
 * └─────────────────────────────────────────────────┘
 * @endcode
 *
 * Interaction:
 *   - Hover over a condition row → tooltip showing preset details.
 *   - Click a condition row → fires OnConditionClicked(conditionIndex).
 *   - Dynamic pins support drag/connect via ImNodes.
 *   - Section 4 is only rendered when at least one dynamic pin exists.
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
#include <unordered_set>

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
    int                           nodeID;           ///< Numeric node identifier (for ImNodes attribute UIDs)
    std::string                   nodeName;         ///< Display name (e.g. "Branch #3")
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
     * @param data             Current snapshot of the node's data.
     * @param connectedAttrIDs Set of attribute IDs that are currently connected.
     *                         Pins in this set are rendered filled; others outlined.
     */
    void RenderNode(const NodeBranchData& data,
                    const std::unordered_set<int>& connectedAttrIDs = {});

    // -----------------------------------------------------------------------
    // Section renderers (also exposed for unit testing / partial renders)
    // -----------------------------------------------------------------------

    /**
     * @brief Renders Section 1: title bar with blue background.
     *
     * Displays the node name on a blue-highlighted background row.
     * @param data  Node data.
     */
    void RenderTitleSection(const NodeBranchData& data);

    /**
     * @brief Renders Section 2: static exec pins (In / Then / Else).
     *
     * Shows "In" on the left and "Then" / "Else" on the right.
     * These pins are NEVER editable from this section.
     * @param data  Node data.
     */
    void RenderExecPinsSection(const NodeBranchData& data);

    /**
     * @brief Renders Section 3: read-only conditions preview (green text).
     *
     * Each condition is displayed in the format:
     *   "    [mHealth] <= [2]"
     *   "And [mSpeed] <= [100.00]"
     * Hovering shows a tooltip; clicking fires OnConditionClicked.
     * @param data  Node data.
     */
    void RenderConditionsSection(const NodeBranchData& data);

    /**
     * @brief Renders Section 4: dynamic data pins (yellow, conditional).
     *
     * Only rendered when data.dynamicPins is non-empty.
     * Format: "In #<idx>[L/R]: <condPreview>"
     * @param data             Node data.
     * @param connectedAttrIDs Set of connected attribute IDs (for filled/outlined shape).
     */
    void RenderDynamicPinsSection(const NodeBranchData& data,
                                   const std::unordered_set<int>& connectedAttrIDs = {});

    // -----------------------------------------------------------------------
    // ImNodes connector setup
    // -----------------------------------------------------------------------

    /**
     * @brief Sets up ImNodes connectors for each dynamic pin.
     *
     * Call this within an ImNodes::BeginNode() / ImNodes::EndNode() block so
     * each yellow dynamic pin becomes drag-connectable in the graph editor.
     * Each pin receives a connector ID derived from pin.id.
     *
     * In headless / test builds this is a no-op.
     *
     * @param data  Node data (dynamicPins are iterated).
     */
    void SetupDynamicPinConnectors(const NodeBranchData& data);

    // -----------------------------------------------------------------------
    // Pin regeneration (Modal-to-Canvas workflow)
    // -----------------------------------------------------------------------

    /**
     * @brief Regenerates dynamic data pins from an updated condition list.
     *
     * Call this after the user confirms changes in the edit modal so the
     * canvas reflects the new pin set on the next render frame.
     *
     * Delegates to DynamicDataPinManager::RegeneratePinsFromConditions() so
     * the host does not need a direct reference to the pin manager.
     *
     * @param conditionRefs  Node's condition list (modified in-place to store
     *                       updated pin IDs).
     */
    void TriggerPinRegeneration(std::vector<NodeConditionRef>& conditionRefs);

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
    // ImGui color constants (matching mockup spec)
    // -----------------------------------------------------------------------

    /// @brief Blue title bar background color (#0066CC equivalent).
    static constexpr float kTitleR = 0.0f;
    static constexpr float kTitleG = 0.4f;
    static constexpr float kTitleB = 0.8f;

    /// @brief Yellow dynamic-pin label color (#FFD700 equivalent).
    static constexpr float kDynPinR = 1.0f;
    static constexpr float kDynPinG = 0.843f;
    static constexpr float kDynPinB = 0.0f;

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
