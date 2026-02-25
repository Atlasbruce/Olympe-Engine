/**
 * @file Clipboard.h
 * @brief Node-graph clipboard: copy/paste selected nodes via ImGui system clipboard.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * NodeGraphClipboard serialises selected nodes (including their type, sub-type,
 * parameters and relative positions) to a compact JSON string stored in the
 * system clipboard via ImGui::SetClipboardText / ImGui::GetClipboardText.
 *
 * The JSON payload is prefixed with the marker OLYMPE_NG_CLIP: so that foreign
 * clipboard contents are silently ignored on paste.
 *
 * Multi-node copy is supported: all currently selected nodes are written.
 * On paste the nodes are positioned relative to the current mouse cursor
 * position, preserving their layout offsets.
 *
 * Design notes
 * ------------
 * - The class is a singleton for convenience; each editor window uses the same
 *   shared clipboard.
 * - No heap allocations are kept between Copy() and Paste() calls; the JSON is
 *   stored solely in the system clipboard string.
 * - C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>
#include "BTNodeGraphManager.h"

namespace Olympe
{

/**
 * @class NodeGraphClipboard
 * @brief Singleton clipboard for node-graph copy / paste operations.
 */
class NodeGraphClipboard
{
public:
    /// Returns the singleton instance.
    static NodeGraphClipboard& Get();

    /**
     * @brief Serialise currently selected nodes to JSON and write to the system
     *        clipboard.
     *
     * @param graph    Active NodeGraph (source of node data and selection).
     * @param graphID  Active graph ID (used to resolve ImNodes global UIDs).
     *
     * If no nodes are selected the clipboard is left unchanged.
     */
    void CopySelectedNodes(NodeGraph* graph, int graphID);

    /**
     * @brief Read the system clipboard, deserialise nodes and create them in
     *        the active graph under the current mouse cursor.
     *
     * @param graph       Active NodeGraph (destination).
     * @param mousePosX   Paste anchor X in canvas (grid) space.
     * @param mousePosY   Paste anchor Y in canvas (grid) space.
     * @param snapToGrid  When true, each pasted node position is snapped to the
     *                    nearest grid cell of size @p snapGridSize.
     * @param snapGridSize Grid cell size used when @p snapToGrid is true.
     *
     * If the clipboard does not contain a valid Olympe payload this is a no-op.
     */
    void PasteNodes(NodeGraph* graph, float mousePosX, float mousePosY,
                    bool snapToGrid = false, float snapGridSize = 16.0f);

private:
    NodeGraphClipboard() = default;

    /// Prefix that marks Olympe node-graph clipboard payloads.
    static const char* k_ClipPrefix;
};

} // namespace Olympe
