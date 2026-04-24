/**
 * @file Clipboard.cpp
 * @brief Implementation of NodeGraphClipboard.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * Serialisation format (JSON, one object per copied node):
 * {
 *   "nodes": [
 *     {
 *       "type": "BT_Action",
 *       "name": "Move",
 *       "actionType": "MoveToLocation",
 *       "conditionType": "",
 *       "decoratorType": "",
 *       "params": { "speed": "5.0" },
 *       "dx": 0.0,
 *       "dy": 0.0
 *     }
 *   ]
 * }
 *
 * "dx"/"dy" are position offsets relative to the top-left bounding corner of
 * the copied selection, so pasted nodes reproduce the original spatial layout.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Clipboard.h"
#include "../system/system_utils.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../third_party/nlohmann/json.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <cfloat>
#include <algorithm>

using json = nlohmann::json;

namespace Olympe
{

// ============================================================================
// Constants
// ============================================================================

const char* NodeGraphClipboard::k_ClipPrefix = "OLYMPE_NG_CLIP:";

// Multiplier used to convert local node IDs to ImNodes global UIDs.
// Must match the value in NodeGraphPanel.cpp.
static constexpr int CLIP_GRAPH_ID_MULTIPLIER = 10000;

// ============================================================================
// Singleton
// ============================================================================

NodeGraphClipboard& NodeGraphClipboard::Get()
{
    static NodeGraphClipboard s_instance;
    return s_instance;
}

// ============================================================================
// CopySelectedNodes
// ============================================================================

// TODO: Phase 50.4 - Reimplement with modern GraphDocument
void NodeGraphClipboard::CopySelectedNodes()
{
    // Deprecated - NodeGraph class removed (Phase 50.3 namespace collision fix)
}

// ============================================================================
// PasteNodes
// ============================================================================

// TODO: Phase 50.4 - Reimplement with modern GraphDocument
void NodeGraphClipboard::PasteNodes(void* graph, int graphID,
                                     float mousePosX, float mousePosY,
                                     bool snapToGrid, float snapGridSize)
{
    // Deprecated - NodeGraph class removed in Phase 50.3 namespace collision fix
    // Will be reimplemented in Phase 50.4 with GraphDocument
}

} // namespace Olympe
