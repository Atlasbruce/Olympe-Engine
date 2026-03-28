/**
 * @file VisualScriptNodeRenderer.h
 * @brief Node style rendering helpers for VisualScriptEditorPanel (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Provides stateless helpers for rendering VS node styles (colours, pin shapes,
 * hover tooltips) on top of ImNodes.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <utility>
#include <unordered_set>
#include "../TaskSystem/TaskGraphTypes.h"
#include "../TaskSystem/TaskGraphTemplate.h"

// Forward declarations (avoid pulling in heavy headers)
struct ImVec2;

namespace Olympe {

// ============================================================================
// Node visual style
// ============================================================================

/**
 * @enum VSNodeStyle
 * @brief Visual style category for a VS node.
 */
enum class VSNodeStyle {
    EntryPoint,   ///< Green  — single "Out" exec pin
    FlowControl,  ///< Blue   — Branch, Sequence, While, ForEach, DoOnce
    Action,       ///< Orange — AtomicTask
    Data,         ///< Purple — GetBBValue, SetBBValue, MathOp
    SubGraph,     ///< Teal   — SubGraph call
    Delay         ///< Yellow — Delay timer
};

/**
 * @brief Returns the VSNodeStyle appropriate for a given node type.
 */
VSNodeStyle GetNodeStyle(TaskNodeType type);

/**
 * @brief Returns the title-bar RGBA colour for a given style.
 */
unsigned int GetNodeTitleColor(VSNodeStyle style);

/**
 * @brief Returns the title-bar hovered RGBA colour.
 */
unsigned int GetNodeTitleHoveredColor(VSNodeStyle style);

/**
 * @brief Returns the RGBA colour for a data pin by variable type.
 */
unsigned int GetDataPinColor(VariableType type);

/**
 * @brief Returns a human-readable label for a TaskNodeType.
 */
const char* GetNodeTypeLabel(TaskNodeType type);

/**
 * @brief Returns a human-readable label for a VariableType.
 */
const char* GetVariableTypeLabel(VariableType type);

// ============================================================================
// VisualScriptNodeRenderer
// ============================================================================

/**
 * @class VisualScriptNodeRenderer
 * @brief Stateless helper class for rendering VS nodes via ImNodes.
 *
 * @details
 * All methods are static — this class has no instance state.  The renderer
 * pushes/pops ImNodes colour styles and delegates actual node/pin calls to
 * ImNodes directly.
 */
class VisualScriptNodeRenderer {
public:

    /**
     * @brief Renders a complete VS node (title + exec pins + data pins).
     *
     * Must be called inside an ImNodes::BeginNodeEditor() … EndNodeEditor() scope.
     *
     * @param nodeUID      Global ImNodes node UID.
     * @param nodeID       Graph-local node ID (for breakpoint lookup).
     * @param graphID      Graph ID (for breakpoint lookup).
     * @param nodeName     Display name.
     * @param type         Node type (determines style).
     * @param hasBreakpoint  Whether a breakpoint is set on this node.
     * @param isActive     Whether this node is currently executing (debug).
     * @param execInputPins  Names of exec-in pins (usually just {"In"}).
     * @param execOutputPins Names of exec-out pins (e.g. {"True","False"}).
     * @param dataInputPins  (name, type) pairs for data-in pins.
     * @param dataOutputPins (name, type) pairs for data-out pins.
     *
     * @note Tooltip display is handled by the caller after EndNodeEditor()
     *       because ImNodes::IsNodeHovered() requires ImNodesScope_None.
     */
    static void RenderNode(
        int                                           nodeUID,
        int                                           nodeID,
        int                                           graphID,
        const std::string&                            nodeName,
        TaskNodeType                                  type,
        bool                                          hasBreakpoint,
        bool                                          isActive,
        const std::vector<std::string>&               execInputPins,
        const std::vector<std::string>&               execOutputPins,
        const std::vector<std::pair<std::string, VariableType>>& dataInputPins,
        const std::vector<std::pair<std::string, VariableType>>& dataOutputPins,
        const std::unordered_set<int>&                connectedAttrIDs = {});

    /**
     * @brief Extended RenderNode with inline parameter display and optional Add[+]/Remove[-] callbacks.
     *
     * Displays key parameters inline in the node body (between title bar and pins).
     * For VSSequence and Switch nodes, renders a [+] button below the last exec-out pin
     * that invokes onAddPin(nodeID, onAddPinUserData).
     * For each dynamic exec-out pin (all but the base pin), renders a [-] button inline
     * that invokes onRemovePin(nodeID, dynamicPinIndex, onRemovePinUserData).
     *
     * @param nodeUID        Global ImNodes node UID.
     * @param nodeID         Graph-local node ID.
     * @param graphID        Graph ID (breakpoint lookup).
     * @param def            Full node definition (for inline parameter display).
     * @param hasBreakpoint  Whether a breakpoint is set on this node.
     * @param isActive       Whether this node is executing (debug).
     * @param execInputPins  Names of exec-in pins.
     * @param execOutputPins Names of exec-out pins (includes dynamic ones for VSSequence/Switch).
     * @param dataInputPins  (name, type) pairs for data-in pins.
     * @param dataOutputPins (name, type) pairs for data-out pins.
     * @param onAddPin       Optional callback invoked when user clicks [+] on a VSSequence/Switch.
     *                       Receives the nodeID and onAddPinUserData. Pass nullptr to disable.
     * @param onAddPinUserData  User data passed to onAddPin.
     * @param onRemovePin    Optional callback invoked when user clicks [-] on a dynamic pin.
     *                       Receives nodeID, 0-based dynamic pin index, and onRemovePinUserData.
     *                       Pass nullptr to disable.
     * @param onRemovePinUserData  User data passed to onRemovePin.
     */
    static void RenderNode(
        int                                                      nodeUID,
        int                                                      nodeID,
        int                                                      graphID,
        const TaskNodeDefinition&                                def,
        bool                                                     hasBreakpoint,
        bool                                                     isActive,
        const std::vector<std::string>&                          execInputPins,
        const std::vector<std::string>&                          execOutputPins,
        const std::vector<std::pair<std::string, VariableType>>& dataInputPins,
        const std::vector<std::pair<std::string, VariableType>>& dataOutputPins,
        void (*onAddPin)(int nodeID, void* userData),
        void* onAddPinUserData,
        void (*onRemovePin)(int nodeID, int dynamicPinIndex, void* userData) = nullptr,
        void* onRemovePinUserData = nullptr,
        const std::unordered_set<int>& connectedAttrIDs = {});

    /**
     * @brief Renders a breakpoint indicator (red circle) next to a node.
     *
     * Must be called inside an ImNodes::BeginNodeEditor() … EndNodeEditor() scope,
     * and after ImNodes::EndNode() so that GetNodeEditorSpacePos() is valid.
     *
     * @param nodeUID  Global ImNodes node UID.
     */
    static void RenderBreakpointIndicator(int nodeUID);

    /**
     * @brief Renders a "currently executing" glow overlay around a node.
     *
     * @param nodeUID  Global ImNodes node UID.
     */
    static void RenderActiveNodeGlow(int nodeUID);

private:
    VisualScriptNodeRenderer() = delete;
};

} // namespace Olympe
