/**
 * @file VisualScriptEditorPanel_Canvas.cpp
 * @brief Canvas rendering, node palette, and context menus for VisualScriptEditorPanel.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details Extracted from VisualScriptEditorPanel.cpp (Phase 9 refactoring).
 * Contains: RenderCanvas, RenderNodePalette, RenderContextMenus.
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"
#include "BBVariableRegistry.h"
#include "MathOpOperand.h"
#include "TabManager.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../json_helper.h"
#include "../TaskSystem/TaskGraphLoader.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <limits>
#include <unordered_set>

namespace Olympe {

// ============================================================================
// Canvas Rendering
// ============================================================================

void VisualScriptEditorPanel::RenderCanvas()
{
    // Switch to this panel's dedicated ImNodes context so that node positions
    // and canvas panning are preserved independently for each open tab.
    if (m_imnodesContext)
        ImNodes::EditorContextSet(m_imnodesContext);

    // Publish the exact Visual Script viewport to the shared canvas adapter
    // every frame.  Grid, minimap, coordinate conversions and reset-view now
    // operate on this same region instead of an inferred parent window.
    if (m_canvasEditor)
    {
        m_canvasEditor->SetCanvasScreenPos(ImGui::GetCursorScreenPos());
        m_canvasEditor->SetCanvasSize(ImGui::GetContentRegionAvail());
        m_canvasEditor->BeginRender();
        m_canvasEditor->RenderGrid(CanvasGridRenderer::Style_VisualScript);
    }
    else
    {
        ImNodes::BeginNodeEditor();
    }

    // IMPORTANT: RenderCanvas() is called on every frame while the tab is active.
    // A tab switch must NOT be treated as a reload. The stored positions are
    // only injected once per actual LoadTemplate()/CreateNew() cycle.

    // On the first render after LoadTemplate(), push the stored (posX, posY)
    // of each node into ImNodes so the canvas matches the saved layout.
    // BUG-003 Fix: positions are stored in grid space; use SetNodeGridSpacePos
    // to restore them pan-independently (avoids double-offset with viewport pan).
    if (m_needsPositionSync && m_editorNodes.size() > 0)
    {
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            ImNodes::SetNodeGridSpacePos(
                m_editorNodes[i].nodeID,
                ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
        }
        m_needsPositionSync = false;
    }

    // Phase 21-B: focus/scroll to a node requested from the verification panel
    if (m_focusNodeID >= 0)
    {
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            if (m_editorNodes[i].nodeID == m_focusNodeID)
            {
                // BUG-003 Fix: restore in grid space for pan-independent positioning.
                ImNodes::SetNodeGridSpacePos(
                    m_focusNodeID,
                    ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
                break;
            }
        }
        m_focusNodeID = -1;
    }

    // NOTE: Right-click context menu detection is deferred until after
    // ImNodes::EndNodeEditor() below so that IsNodeHovered() / IsLinkHovered()
    // (which require ImNodesScope_None) can be used to determine what was clicked.

    // Build connected attribute IDs set from current editor links.
    // Pins whose attribute ID is in this set will be rendered filled;
    // unconnected pins are rendered outlined (empty).
    std::unordered_set<int> connectedAttrIDs;
    for (size_t li = 0; li < m_editorLinks.size(); ++li)
    {
        connectedAttrIDs.insert(m_editorLinks[li].srcAttrID);
        connectedAttrIDs.insert(m_editorLinks[li].dstAttrID);
    }

    // Render all nodes
    int activeNodeID = DebugController::Get().GetCurrentNodeID();

    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        VSEditorNode& eNode = m_editorNodes[i];

        bool hasBreakpoint = DebugController::Get().HasBreakpoint(
            0 /* graphID placeholder */, eNode.nodeID);
        bool isActive = (eNode.nodeID == activeNodeID &&
                         DebugController::Get().IsDebugging());

        // Phase 21-B: highlight nodes that have Error issues in the verification result
        bool hasVerifError = false;
        if (m_verificationDone)
        {
            for (size_t vi = 0; vi < m_verificationResult.issues.size(); ++vi)
            {
                if (m_verificationResult.issues[vi].nodeID == eNode.nodeID &&
                    m_verificationResult.issues[vi].severity == VSVerificationSeverity::Error)
                {
                    hasVerifError = true;
                    break;
                }
            }
        }
        if (hasVerifError)
        {
            ImNodes::PushColorStyle(ImNodesCol_NodeBackground,
                                    IM_COL32(120, 30, 30, 230));
            ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundHovered,
                                    IM_COL32(120, 30, 30, 230));
            ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundSelected,
                                    IM_COL32(120, 30, 30, 230));
        }

        auto execIn  = GetExecInputPins(eNode.def.Type);
        auto execOut = GetExecOutputPinsForNode(eNode.def);

        // Phase 24.2 FIX: Ensure data-pure nodes have DataPins initialized
        // This handles both newly created nodes AND nodes loaded from blueprints

        // Initialize DataPins for GetBBValue (Variable) nodes
        if (eNode.def.Type == TaskNodeType::GetBBValue && eNode.def.DataPins.empty())
        {
            DataPinDefinition pinOut;
            pinOut.PinName = "Value";
            pinOut.Dir     = DataPinDir::Output;
            pinOut.PinType = VariableType::Float;  // Will be resolved at runtime based on actual variable
            eNode.def.DataPins.push_back(pinOut);
            std::cerr << "[VSEditor] Initialized DataPins for GetBBValue (Variable) node #" << eNode.nodeID << "\n";
        }

        // Initialize DataPins for MathOp nodes
        if (eNode.def.Type == TaskNodeType::MathOp && eNode.def.DataPins.empty())
        {
            // Initialize DataPins for this MathOp node if not already present
            DataPinDefinition pinA;
            pinA.PinName = "A";
            pinA.Dir     = DataPinDir::Input;
            pinA.PinType = VariableType::Float;
            eNode.def.DataPins.push_back(pinA);

            DataPinDefinition pinB;
            pinB.PinName = "B";
            pinB.Dir     = DataPinDir::Input;
            pinB.PinType = VariableType::Float;
            eNode.def.DataPins.push_back(pinB);

            DataPinDefinition pinResult;
            pinResult.PinName = "Result";
            pinResult.Dir     = DataPinDir::Output;
            pinResult.PinType = VariableType::Float;
            eNode.def.DataPins.push_back(pinResult);

            std::cerr << "[VSEditor] Initialized DataPins for MathOp node #" << eNode.nodeID << "\n";
        }

        // Initialize DataPins for SetBBValue nodes
        if (eNode.def.Type == TaskNodeType::SetBBValue && eNode.def.DataPins.empty())
        {
            DataPinDefinition pinIn;
            pinIn.PinName = "Value";
            pinIn.Dir     = DataPinDir::Input;
            pinIn.PinType = VariableType::Float;  // Will be resolved at runtime based on target variable
            eNode.def.DataPins.push_back(pinIn);
            std::cerr << "[VSEditor] Initialized DataPins for SetBBValue node #" << eNode.nodeID << "\n";
        }

        std::vector<std::pair<std::string, VariableType>> dataIn, dataOut;
        for (size_t p = 0; p < eNode.def.DataPins.size(); ++p)
        {
            const DataPinDefinition& pin = eNode.def.DataPins[p];
            if (pin.Dir == DataPinDir::Input)
                dataIn.push_back({pin.PinName, pin.PinType});
            else
                dataOut.push_back({pin.PinName, pin.PinType});
        }

        // Phase 24 — Dispatcher: Branch nodes use specialized renderer
        if (eNode.def.Type == TaskNodeType::Branch && m_branchRenderer)
        {
            // Convert TaskNodeDefinition to NodeBranchData for specialized rendering
            NodeBranchData branchData;
            branchData.nodeID        = eNode.nodeID;  // int nodeID for ImNodes attribute UIDs
            branchData.nodeName      = eNode.def.NodeName;
            branchData.conditionRefs = eNode.def.conditionRefs;
            branchData.dynamicPins   = eNode.def.dynamicPins;
            branchData.breakpoint    = hasBreakpoint;

            // Render via NodeBranchRenderer (4-section layout)
            // Must be wrapped with ImNodes::BeginNode/EndNode just like generic renderer
            ImNodes::BeginNode(eNode.nodeID);
            m_branchRenderer->RenderNode(branchData, connectedAttrIDs);
            ImNodes::EndNode();
        }
        else
        {
            // Use generic renderer for all other node types
            VisualScriptNodeRenderer::RenderNode(
                eNode.nodeID,
                eNode.nodeID,
                0 /* graphID placeholder */,
                eNode.def,
                hasBreakpoint,
                isActive,
                execIn, execOut,
                dataIn, dataOut,
                [](int nid, void* ud) {
                    VisualScriptEditorPanel* panel =
                        static_cast<VisualScriptEditorPanel*>(ud);
                    panel->m_pendingAddPin       = true;
                    panel->m_pendingAddPinNodeID = nid;
                },
                this,
                [](int nid, int dynIdx, void* ud) {
                    VisualScriptEditorPanel* panel =
                        static_cast<VisualScriptEditorPanel*>(ud);
                    panel->m_pendingRemovePin       = true;
                    panel->m_pendingRemovePinNodeID = nid;
                    panel->m_pendingRemovePinDynIdx = dynIdx;
                },
                this,
                connectedAttrIDs);
        }

        if (hasVerifError)
        {
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
        }

        // Breakpoint / active overlays
        if (hasBreakpoint)
            VisualScriptNodeRenderer::RenderBreakpointIndicator(eNode.nodeID);
        if (isActive)
            VisualScriptNodeRenderer::RenderActiveNodeGlow(eNode.nodeID);

        // Phase 33 — Selection glow (MUST BE INSIDE BeginNodeEditor/EndNodeEditor scope)
        // Draw selection glow for this node if it's selected
        // Glow color adapts to the node's visual style
        if (ImNodes::IsNodeSelected(eNode.nodeID))
        {
            ImVec2 nodeScreenPos = ImNodes::GetNodeScreenSpacePos(eNode.nodeID);
            ImVec2 nodeSize = ImNodes::GetNodeDimensions(eNode.nodeID);

            ImVec2 screenMin = nodeScreenPos;
            ImVec2 screenMax(nodeScreenPos.x + nodeSize.x, nodeScreenPos.y + nodeSize.y);

            // Adapt glow color to node type
            VSNodeStyle nodeStyle = GetNodeStyle(eNode.def.Type);
            unsigned int glowColor = GetNodeTitleColor(nodeStyle);

            // Use SelectionEffectRenderer with node-type-specific color
            m_selectionRenderer.RenderCompleteSelection(
                screenMin,
                screenMax,
                glowColor,      // Glow color matches node type
                2.0f,           // baseWidth
                1.0f,           // zoom (imnodes fixed 1.0x)
                1.0f,           // nodeScale
                5.0f            // cornerRadius
            );
        }

        // Mark this node as rendered so position sync is safe
        m_positionedNodes.insert(eNode.nodeID);
     }

    // Render links
    for (size_t i = 0; i < m_editorLinks.size(); ++i)
    {
        const VSEditorLink& link = m_editorLinks[i];
        if (link.isData)
            ImNodes::PushColorStyle(ImNodesCol_Link, SystemColors::DATA_CONNECTION_COLOR);
        else
            ImNodes::PushColorStyle(ImNodesCol_Link, SystemColors::EXEC_CONNECTION_COLOR);
        ImNodes::Link(link.linkID, link.srcAttrID, link.dstAttrID);
        ImNodes::PopColorStyle();
    }

    // Phase 37 — Render minimap overlay on canvas
    if (m_canvasEditor)
    {
        m_canvasEditor->RenderMinimap();
    }

    if (m_canvasEditor)
        m_canvasEditor->EndRender();
    else
        ImNodes::EndNodeEditor();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
    {
        m_pendingSelectAll = true;
    }
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
    {
        m_pendingCopySelection = true;
    }
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
    {
        m_pendingPaste = true;
    }

    // Canvas commands are deliberately processed outside the ImNodes editor
    // scope. This is the same safe point used for deferred node creation and
    // avoids mutating ImNodes selection while it is laying out nodes.
    if (m_pendingResetView)
    {
        if (m_canvasEditor)
            m_canvasEditor->ResetView();
        m_pendingResetView = false;
    }

    if (m_pendingSelectAll)
    {
        ImNodes::ClearNodeSelection();
        ImNodes::ClearLinkSelection();
        for (size_t nodeIndex = 0; nodeIndex < m_editorNodes.size(); ++nodeIndex)
            ImNodes::SelectNode(m_editorNodes[nodeIndex].nodeID);
        for (size_t linkIndex = 0; linkIndex < m_editorLinks.size(); ++linkIndex)
            ImNodes::SelectLink(m_editorLinks[linkIndex].linkID);

        m_selectedNodeID = m_editorNodes.empty() ? -1 : m_editorNodes.front().nodeID;
        m_pendingSelectAll = false;
    }

    if (m_pendingCopySelection)
    {
        CopySelectedNodesToFrameworkClipboard();
        m_pendingCopySelection = false;
    }

    if (m_pendingPaste)
    {
        ImVec2 pastePosition(m_contextMenuX, m_contextMenuY);
        if (m_canvasEditor)
            pastePosition = m_canvasEditor->ScreenToCanvas(ImGui::GetMousePos());
        PasteFrameworkClipboard(pastePosition.x, pastePosition.y);
        m_pendingPaste = false;
    }

    // FIX 4: Skip position sync if undo/redo just executed.
    // SyncEditorNodesFromTemplate() has already written the correct undo-target
    // positions into m_editorNodes and SetNodeEditorSpacePos() has pushed them
    // to ImNodes. Reading them back here (before ImNodes has rendered the new
    // positions once) would overwrite the correct values with stale ImNodes state.
    if (m_skipPositionSyncNextFrame)
    {
        m_skipPositionSyncNextFrame = false;
    }
    else
    {
        SyncNodePositionsFromImNodes();
    }

    // ========================================================================
    // Context menu dispatch (requires ImNodesScope_None, i.e. after EndNodeEditor)
    // Priority: node hover > link hover > canvas background.
    // ========================================================================
    {
        int  hoveredNode = -1;
        int  hoveredLink = -1;
        bool nodeHovered = ImNodes::IsNodeHovered(&hoveredNode);
        bool linkHovered = ImNodes::IsLinkHovered(&hoveredLink);

        // PHASE 1: Detect right-click and open the appropriate popup.
        // Use ImNodes::IsEditorHovered() for canvas background detection so
        // that the check works even when ImNodes has captured mouse focus.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            if (nodeHovered)
            {
                m_contextNodeID = hoveredNode;
                ImGui::OpenPopup("VSNodeContextMenu");
                SYSTEM_LOG << "[VSEditor] Opened context menu on NODE #" << hoveredNode << "\n";
            }
            else if (linkHovered)
            {
                m_contextLinkID = hoveredLink;
                ImGui::OpenPopup("VSLinkContextMenu");
                SYSTEM_LOG << "[VSEditor] Opened context menu on LINK #" << hoveredLink << "\n";
            }
            else if (m_canvasEditor
                         ? m_canvasEditor->IsPointInCanvas(ImGui::GetMousePos())
                         : ImNodes::IsEditorHovered())
            {
                // The framework owns the canonical screen-to-canvas transform.
                // This stays reliable even when an ImGui drag/drop overlay owns
                // the immediate hover state for the same region.
                const ImVec2 canvasPoint = m_canvasEditor
                    ? m_canvasEditor->ScreenToCanvas(ImGui::GetMousePos())
                    : ImGui::GetMousePos();
                m_contextMenuX = canvasPoint.x;
                m_contextMenuY = canvasPoint.y;
                ImGui::OpenPopup("VSNodePalette");
                SYSTEM_LOG << "[VSEditor] Opened context menu on CANVAS at ("
                           << m_contextMenuX << ", " << m_contextMenuY << ")\n";
            }
        }

        // PHASE 1.5: Detect left-click (for double-click tracking).
        // Used to detect when user double-clicks on a node (e.g., SubGraph).
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            m_canvasEditor &&
            m_canvasEditor->IsPointInCanvas(ImGui::GetMousePos()))
        {
            if (nodeHovered)
            {
                float currentTime = static_cast<float>(ImGui::GetTime());

                // Check if this is a double-click (same node + within threshold)
                if (m_lastClickNodeID == hoveredNode &&
                    (currentTime - m_lastClickTime) < DOUBLE_CLICK_THRESHOLD)
                {
                    // Double-click detected!
                    OnNodeDoubleClicked(hoveredNode);
                    m_lastClickNodeID = -1;  // Reset to prevent triple-click
                }
                else
                {
                    // First click (or click on different node)
                    m_lastClickNodeID = hoveredNode;
                    m_lastClickTime = currentTime;
                }
            }
            else
            {
                // Clicked on empty canvas, reset double-click state
                m_lastClickNodeID = -1;
            }
        }

        // PHASE 2: Render popups in the same ImGui window scope.
        RenderContextMenus();
        RenderNodePalette();

        // ========================================================================
        // Hover visual effects: nodes and links (apply BT-style hover to VS)
        // - Node hover: render a subtle glow + border using SelectionEffectRenderer
        // - Link hover: draw an overlay bezier with highlight color between node centers
        // ========================================================================
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Node hover effect
            if (nodeHovered && hoveredNode >= 0)
            {
                ImVec2 nodeScreenPos = ImNodes::GetNodeScreenSpacePos(hoveredNode);
                ImVec2 nodeSize = ImNodes::GetNodeDimensions(hoveredNode);
                ImVec2 screenMin = nodeScreenPos;
                ImVec2 screenMax(nodeScreenPos.x + nodeSize.x, nodeScreenPos.y + nodeSize.y);

                // Use gold accent for hover
                unsigned int glowColor = IM_COL32(255, 200, 80, 200);
                m_selectionRenderer.RenderCompleteSelection(
                    screenMin,
                    screenMax,
                    glowColor,
                    1.5f,   // baseWidth
                    1.0f,   // zoom
                    1.0f,   // nodeScale
                    5.0f    // cornerRadius
                );
            }

            // Link hover effect: draw a bright bezier overlay between node centers
            if (linkHovered && hoveredLink >= 0 && hoveredLink < static_cast<int>(m_editorLinks.size()))
            {
                const VSEditorLink& link = m_editorLinks[hoveredLink];
                // derive node ids from attribute IDs by integer division (attribute scheme: nodeUID * 10000 + offset)
                int srcNodeID = -1;
                int dstNodeID = -1;

                if (link.srcAttrID >= 0) srcNodeID = link.srcAttrID / 10000;
                if (link.dstAttrID >= 0) dstNodeID = link.dstAttrID / 10000;

                if (srcNodeID != -1 && dstNodeID != -1)
                {
                    ImVec2 srcPos = ImNodes::GetNodeScreenSpacePos(srcNodeID);
                    ImVec2 dstPos = ImNodes::GetNodeScreenSpacePos(dstNodeID);
                    ImVec2 srcDim = ImNodes::GetNodeDimensions(srcNodeID);
                    ImVec2 dstDim = ImNodes::GetNodeDimensions(dstNodeID);

                    ImVec2 from = ImVec2(srcPos.x + srcDim.x * 0.5f, srcPos.y + srcDim.y * 0.5f);
                    ImVec2 to   = ImVec2(dstPos.x + dstDim.x * 0.5f, dstPos.y + dstDim.y * 0.5f);

                    float controlOffset = (to.x - from.x) * 0.4f;
                    ImVec2 cp1(from.x + controlOffset, from.y);
                    ImVec2 cp2(to.x - controlOffset, to.y);

                    ImU32 lineColor = ImGui::GetColorU32(ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
                    drawList->AddBezierCubic(from, cp1, cp2, to, lineColor, 4.0f, 24);

                    // endpoints
                    drawList->AddCircleFilled(from, 4.0f, lineColor);
                    drawList->AddCircleFilled(to, 4.0f, lineColor);
                }
            }
        }
    }

    // ========================================================================
    // PHASE 2: Detect drag & drop (store pending node creation).
    // AddNode() must NOT be called here — ImNodes' internal state is still
    // being finalised at this point and SetNodeEditorSpacePos would assert.
    // ========================================================================
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VS_NODE_TYPE_ENUM");
        if (payload && payload->Data && payload->DataSize == sizeof(uint8_t))
        {
            uint8_t enumValue = *static_cast<const uint8_t*>(payload->Data);
            TaskNodeType nodeType = static_cast<TaskNodeType>(enumValue);

            // Get mouse position in canvas space
            ImVec2 mousePos  = ImGui::GetMousePos();
            ImVec2 canvasPos = ImNodes::EditorContextGetPanning();
            float  zoom      = 1.0f;  // ImNodes doesn't expose zoom yet

            // Convert screen space to canvas space
            ImVec2 windowPos = ImGui::GetWindowPos();
            float canvasX = (mousePos.x - windowPos.x - canvasPos.x) / zoom;
            float canvasY = (mousePos.y - windowPos.y - canvasPos.y) / zoom;

            // CRITICAL: Don't call AddNode() here — just store the request.
            // The node will be created in Phase 2 below, safely outside the
            // ImNodes editor scope.
            m_pendingNodeDrop = true;
            m_pendingNodeType = nodeType;
            m_pendingNodeX    = canvasX;
            m_pendingNodeY    = canvasY;
        }
        ImGui::EndDragDropTarget();
    }

    // ========================================================================
    // PHASE 2: Process pending node creation (outside editor scope).
    // AddNode() and SetNodeEditorSpacePos() are both safe here — the editor
    // context is fully closed (ImNodesScope_None).
    // ========================================================================
    if (m_pendingNodeDrop)
    {
        // Ensure positions are not garbage values (defend against FLT_MAX or corrupted memory)
        float safeX = m_pendingNodeX;
        float safeY = m_pendingNodeY;
        if (!std::isfinite(safeX) || !std::isfinite(safeY) ||
            safeX < -100000.0f || safeX > 100000.0f ||
            safeY < -100000.0f || safeY > 100000.0f)
        {
            safeX = 0.0f;
            safeY = 0.0f;
            SYSTEM_LOG << "[VSEditor] Warning: pending node position was garbage; reset to (0, 0)\n";
        }

        int newNodeID = AddNode(m_pendingNodeType, safeX, safeY);

        // Pre-register the position so ImNodes places the node correctly
        // on the very first frame it is rendered (next frame).
        ImNodes::SetNodeEditorSpacePos(newNodeID, ImVec2(safeX, safeY));

        m_dirty           = true;
        m_pendingNodeDrop = false;

        std::cout << "[VisualScriptEditorPanel] Node created: ID=" << newNodeID
                  << " type=" << static_cast<int>(m_pendingNodeType)
                  << " at (" << safeX << ", " << safeY << ")"
                  << std::endl;
    }

    // ========================================================================
    // PHASE 2: Process pending dynamic pin addition (outside editor scope).
    // The [+] button callback on VSSequence/Switch stores the request here; we
    // process it after EndNodeEditor so that AddDynamicPinCommand can safely
    // modify the template and trigger RebuildLinks().
    //
    // Phase 3 FIX (Switch nodes): For Switch nodes, instead of direct
    // DynamicExecOutputPins modification, open the modal for safe editing.
    // ========================================================================
    if (m_pendingAddPin)
    {
        m_pendingAddPin = false;

        VSEditorNode* eNode = nullptr;
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            if (m_editorNodes[i].nodeID == m_pendingAddPinNodeID)
            {
                eNode = &m_editorNodes[i];
                break;
            }
        }

        if (eNode != nullptr)
        {
            // Phase 3 FIX: For Switch nodes, open modal instead of direct modification
            if (eNode->def.Type == TaskNodeType::Switch)
            {
                if (!m_switchCaseModal)
                    m_switchCaseModal = std::make_unique<SwitchCaseEditorModal>();
                m_switchCaseModal->Open(eNode->def.switchCases);
                m_selectedNodeID = m_pendingAddPinNodeID;  // Ensure UI knows which node to edit
                SYSTEM_LOG << "[VSEditor] Switch node #" << m_pendingAddPinNodeID
                           << ": opened modal for safe case editing (Phase 3 FIX)\n";
            }
            else if (eNode->def.Type == TaskNodeType::VSSequence)
            {
                // VSSequence: proceed with direct pin addition (unchanged behavior)
                int pinIdx = static_cast<int>(eNode->def.DynamicExecOutputPins.size()) + 1;
                std::string pinName = "Out_" + std::to_string(pinIdx);

                // Update editor-side def immediately
                eNode->def.DynamicExecOutputPins.push_back(pinName);

                // Push undo command (also updates template)
                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(
                        new AddDynamicPinCommand(m_pendingAddPinNodeID, pinName)),
                    m_template);

                RebuildLinks();
                m_dirty = true;
                SYSTEM_LOG << "[VSEditor] AddDynamicPin: VSSequence node #" << m_pendingAddPinNodeID
                           << " added pin '" << pinName << "'\n";
            }
        }
    }

    // ========================================================================
    // PHASE 2: Process pending dynamic pin removal (outside editor scope).
    // The [-] button callback on dynamic pins stores the request here; we
    // process it after EndNodeEditor so that RemoveExecPinCommand can safely
    // modify the template and trigger RebuildLinks().
    //
    // Phase 3 FIX (Switch nodes): For Switch nodes, instead of direct
    // DynamicExecOutputPins modification, open the modal for safe editing.
    // ========================================================================
    if (m_pendingRemovePin)
    {
        m_pendingRemovePin = false;

        VSEditorNode* eNode = nullptr;
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            if (m_editorNodes[i].nodeID == m_pendingRemovePinNodeID)
            {
                eNode = &m_editorNodes[i];
                break;
            }
        }

        if (eNode != nullptr)
        {
            // Phase 3 FIX: For Switch nodes, open modal instead of direct removal
            if (eNode->def.Type == TaskNodeType::Switch)
            {
                if (!m_switchCaseModal)
                    m_switchCaseModal = std::make_unique<SwitchCaseEditorModal>();
                m_switchCaseModal->Open(eNode->def.switchCases);
                m_selectedNodeID = m_pendingRemovePinNodeID;  // Ensure UI knows which node to edit
                SYSTEM_LOG << "[VSEditor] Switch node #" << m_pendingRemovePinNodeID
                           << ": opened modal for safe case removal (Phase 3 FIX)\n";
            }
            else if (eNode->def.Type == TaskNodeType::VSSequence &&
                     m_pendingRemovePinDynIdx >= 0 &&
                     m_pendingRemovePinDynIdx < static_cast<int>(eNode->def.DynamicExecOutputPins.size()))
            {
                // VSSequence: proceed with direct pin removal (unchanged behavior)
                const std::string pinName =
                    eNode->def.DynamicExecOutputPins[static_cast<size_t>(m_pendingRemovePinDynIdx)];

                // Find any outgoing link from this pin in the template
                int32_t linkedTargetNodeID = -1;
                std::string linkedTargetPinName;
                for (size_t c = 0; c < m_template.ExecConnections.size(); ++c)
                {
                    const ExecPinConnection& ec = m_template.ExecConnections[c];
                    if (ec.SourceNodeID == m_pendingRemovePinNodeID &&
                        ec.SourcePinName == pinName)
                    {
                        linkedTargetNodeID  = ec.TargetNodeID;
                        linkedTargetPinName = ec.TargetPinName;
                        break;
                    }
                }

                // Update editor-side def immediately
                eNode->def.DynamicExecOutputPins.erase(
                    eNode->def.DynamicExecOutputPins.begin() + m_pendingRemovePinDynIdx);

                // Push undo command (also updates template)
                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(
                        new RemoveExecPinCommand(m_pendingRemovePinNodeID,
                                                 pinName,
                                                 m_pendingRemovePinDynIdx,
                                                 linkedTargetNodeID,
                                                 linkedTargetPinName)),
                    m_template);

                RebuildLinks();
                m_dirty = true;
                SYSTEM_LOG << "[VSEditor] RemoveDynamicPin: VSSequence node #" << m_pendingRemovePinNodeID
                           << " removed pin '" << pinName << "'\n";
            }
        }
    }

    // Track node moves for undo/redo using MoveNodeCommand.
    // Phase 19 — snapshot-at-click approach:
    //   Step 1 (MouseClicked)  : snapshot current eNode.posX/Y for all positioned nodes.
    //   Step 2 (MouseDown)     : keep eNode.posX/Y in sync with ImNodes live positions.
    //   Step 3 (MouseReleased) : for each snapshotted node, push MoveNodeCommand if
    //                            final position differs from snapshot by more than 1px.
    //
    // Only query nodes that have been rendered at least once (present in
    // m_positionedNodes) to avoid ImNodes assertions for brand-new nodes.
    {
        // Skip movement detection for one frame immediately after an undo/redo
        // so that stale ImNodes positions (not yet updated by the new render
        // cycle) are not mistaken for user-initiated drag-start positions.
        if (m_justPerformedUndoRedo)
        {
            m_justPerformedUndoRedo = false;
        }
        else
        {
        // Step 1: snapshot only a click that begins inside the canvas.  A
        // property-panel click must never create a MoveNode history entry.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            m_canvasEditor &&
            m_canvasEditor->IsPointInCanvas(ImGui::GetMousePos()))
        {
            m_nodeDragStartPositions.clear();
            for (size_t i = 0; i < m_editorNodes.size(); ++i)
            {
                const VSEditorNode& eNode = m_editorNodes[i];
                if (m_positionedNodes.count(eNode.nodeID) == 0)
                    continue;
                m_nodeDragStartPositions[eNode.nodeID] =
                    std::make_pair(eNode.posX, eNode.posY);
            }
            //SYSTEM_LOG << "[VSEditor] Mouse clicked: snapshot " << static_cast<size_t>(m_nodeDragStartPositions.size()) << " node positions\n";
        }

        // Step 2: while mouse is held, keep eNode.posX/Y current (live Save support).
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            for (size_t i = 0; i < m_editorNodes.size(); ++i)
            {
                VSEditorNode& eNode = m_editorNodes[i];
                if (m_positionedNodes.count(eNode.nodeID) == 0)
                    continue;
                const ImVec2 pos = ImNodes::GetNodeEditorSpacePos(eNode.nodeID);
                eNode.posX = pos.x;
                eNode.posY = pos.y;
            }
        }

        // Step 3: on release, push MoveNodeCommand for any node that moved > 1px.
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            for (const auto& entry : m_nodeDragStartPositions)
            {
                const int   nodeID = entry.first;
                const float startX = entry.second.first;
                const float startY = entry.second.second;

                // CRITICAL FIX: Check if node still exists before querying ImNodes
                // The node could have been deleted or the canvas reloaded between mouse click and release.
                // Without this check, GetNodeEditorSpacePos() will assert on a non-existent node.
                if (m_positionedNodes.count(nodeID) == 0)
                    continue;  // Skip this node, it was deleted or canvas state changed

                const ImVec2 finalPos = ImNodes::GetNodeEditorSpacePos(nodeID);

                // Update eNode with final position
                for (size_t i = 0; i < m_editorNodes.size(); ++i)
                {
                    if (m_editorNodes[i].nodeID == nodeID)
                    {
                        m_editorNodes[i].posX = finalPos.x;
                        m_editorNodes[i].posY = finalPos.y;
                        break;
                    }
                }

                if (std::abs(finalPos.x - startX) > 1.0f ||
                    std::abs(finalPos.y - startY) > 1.0f)
                {
                    m_undoStack.PushCommand(
                        std::unique_ptr<ICommand>(
                            new MoveNodeCommand(nodeID,
                                                startX,    startY,
                                                finalPos.x, finalPos.y)),
                        m_template);
                    //SYSTEM_LOG << "[VSEditor] MoveNodeCommand pushed node #" << nodeID
                    //           << " (" << startX << "," << startY
                    //           << ") -> (" << finalPos.x << "," << finalPos.y
                    //           << ") [UNDOABLE]\n";
                    m_dirty = true;
                }
                //else
                //{
                //    SYSTEM_LOG << "[VSEditor] Node #" << nodeID
                //               << " not moved (delta < 1px), skipping\n";
                //}
            }
            m_nodeDragStartPositions.clear();
        }
        } // end !m_justPerformedUndoRedo
    }

    // Hover tooltip — ImNodes::IsNodeHovered() requires ImNodesScope_None,
    // so it must be called here (after EndNodeEditor), never inside the
    // BeginNodeEditor/EndNodeEditor block.
    {
        int hoveredNode = -1;
        if (ImNodes::IsNodeHovered(&hoveredNode))
        {
            auto it = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                   [hoveredNode](const VSEditorNode& n) {
                                       return n.nodeID == hoveredNode;
                                   });
            if (it != m_editorNodes.end())
            {
                const char* tip = GetNodeTypeLabel(it->def.Type);
                if (tip && tip[0] != '\0')
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(tip);
                    ImGui::EndTooltip();
                }
            }
        }
    }

    // Handle new link creation
    int startAttr = -1, endAttr = -1;
    if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
    {
        int startOffset = startAttr % 10000;
        int endOffset   = endAttr   % 10000;

        // Classify pin directions by offset range:
        //   0      -> exec-in  (Input)
        //   100–199 -> exec-out (Output)
        //   200–299 -> data-in  (Input)
        //   300–399 -> data-out (Output)
        bool startIsOutput = (startOffset >= 100 && startOffset < 200) ||
                             (startOffset >= 300 && startOffset < 400);
        bool endIsInput    = (endOffset == 0) ||
                             (endOffset >= 200 && endOffset < 300);

        // Auto-swap if user dragged backwards (Input -> Output).
        // ImNodes normalises the direction automatically (Output pin is always
        // returned as startAttr), so this branch fires only in edge cases where
        // the pin type could not be determined by ImNodes.
        if (!startIsOutput && endIsInput)
        {
            std::swap(startAttr, endAttr);
            startOffset   = startAttr % 10000;
            endOffset     = endAttr   % 10000;
            // Recalculate flags from the new offsets after swap.
            startIsOutput = (startOffset >= 100 && startOffset < 200) ||
                            (startOffset >= 300 && startOffset < 400);
            endIsInput    = (endOffset == 0) ||
                            (endOffset >= 200 && endOffset < 300);
        }

        if (startIsOutput && endIsInput)
        {
            const bool isExecLink = (startOffset >= 100 && startOffset < 200);
            const bool isDataLink = (startOffset >= 300 && startOffset < 400);
            const int  srcNodeID  = startAttr / 10000;
            const int  dstNodeID  = endAttr   / 10000;

            // Phase 24: CRITICAL - Prevent data links from connecting to exec-in pin (offset 0)
            // If this is a data link and destination is exec-in, find the first available data-in pin instead
            if (isDataLink && endOffset == 0)
            {
                // Data-to-exec mismatch detected. Try to find the first available data-in pin.
                auto dstIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [dstNodeID](const VSEditorNode& n) {
                                              return n.nodeID == dstNodeID;
                                          });

                if (dstIt != m_editorNodes.end() &&
                    dstIt->def.Type == TaskNodeType::Branch &&
                    !dstIt->def.dynamicPins.empty())
                {
                    // Force endAttr to point to the first dynamic data-in pin (offset 200)
                    endAttr = dstNodeID * 10000 + 200;
                    endOffset = 200;
                }
                else
                {
                    // No valid data-in pins available, reject this link
                    m_dirty = false;
                    // Skip link creation
                    startIsOutput = false;
                    endIsInput = false;
                }
            }

            // Only proceed if we still have valid pins to connect
            if (!startIsOutput || !endIsInput)
                return;

            if (isExecLink)
            {
                // Resolve source exec-out pin name from its index
                const int srcPinIndex = startOffset - 100;
                std::string srcPinName = "Out";

                auto srcIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [srcNodeID](const VSEditorNode& n) {
                                              return n.nodeID == srcNodeID;
                                          });
                if (srcIt != m_editorNodes.end())
                {
                    auto outPins = GetExecOutputPinsForNode(srcIt->def);
                    if (srcPinIndex < static_cast<int>(outPins.size()))
                        srcPinName = outPins[srcPinIndex];
                }

                if (VSConnectionValidator::IsExecConnectionValid(m_template, srcNodeID, srcPinName, dstNodeID))
                {
                    ConnectExec(srcNodeID, srcPinName, dstNodeID, "In");
                    SYSTEM_LOG << "[VSEditor] Created exec link: node #" << srcNodeID
                               << "." << srcPinName << " -> node #" << dstNodeID << ".In\n";
                    m_dirty = true;
                }
                else
                {
                    SYSTEM_LOG << "[VSEditor] Exec link validation failed: node #" << srcNodeID
                               << "." << srcPinName << " -> node #" << dstNodeID << ".In\n";
                }
            }
            else if (isDataLink)
            {
                // Resolve source data-out and destination data-in pin names
                int srcPinIndex = startOffset - 300;
                int dstPinIndex = endOffset   - 200;
                std::string srcPinName = "Value";
                std::string dstPinName = "Value";

                auto srcIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [srcNodeID](const VSEditorNode& n) {
                                              return n.nodeID == srcNodeID;
                                          });
                auto dstIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [dstNodeID](const VSEditorNode& n) {
                                              return n.nodeID == dstNodeID;
                                          });

                if (srcIt != m_editorNodes.end())
                {
                    // Try static pin list first, then fall back to DataPins vector
                    auto outPins = GetDataOutputPins(srcIt->def.Type);
                    if (srcPinIndex < static_cast<int>(outPins.size()))
                    {
                        srcPinName = outPins[srcPinIndex];
                    }
                    else
                    {
                        int outIdx = 0;
                        for (size_t p = 0; p < srcIt->def.DataPins.size(); ++p)
                        {
                            if (srcIt->def.DataPins[p].Dir == DataPinDir::Output)
                            {
                                if (outIdx == srcPinIndex)
                                {
                                    srcPinName = srcIt->def.DataPins[p].PinName;
                                    break;
                                }
                                ++outIdx;
                            }
                        }
                    }
                }

                if (dstIt != m_editorNodes.end())
                {
                    // Phase 24: Check if destination is a Branch node with dynamic pins
                    if (dstIt->def.Type == TaskNodeType::Branch)
                    {
                        // For Branch nodes, data-in pins start at offset 200
                        // If dstPinIndex is negative or out of range, force it to 0 (first pin)
                        if (dstPinIndex < 0 || dstPinIndex >= static_cast<int>(dstIt->def.dynamicPins.size()))
                        {
                            if (!dstIt->def.dynamicPins.empty())
                            {
                                dstPinIndex = 0;  // Force first available data-in pin
                                std::cerr << "[VSEditor] Data-in pin index corrected to 0 (first available)\n";
                            }
                        }

                        if (dstPinIndex >= 0 && dstPinIndex < static_cast<int>(dstIt->def.dynamicPins.size()))
                        {
                            // Use the dynamic pin's ID as the target pin name
                            dstPinName = dstIt->def.dynamicPins[dstPinIndex].id;
                        }
                        else
                        {
                            std::cerr << "[VSEditor] Cannot find valid data-in pin on Branch node\n";
                            return;  // Skip this link
                        }
                    }
                    else
                    {
                        // Fall back to static data pins
                        auto inPins = GetDataInputPins(dstIt->def.Type);
                        if (dstPinIndex < static_cast<int>(inPins.size()))
                        {
                            dstPinName = inPins[dstPinIndex];
                        }
                        else
                        {
                            int inIdx = 0;
                            for (size_t p = 0; p < dstIt->def.DataPins.size(); ++p)
                            {
                                if (dstIt->def.DataPins[p].Dir == DataPinDir::Input)
                                {
                                    if (inIdx == dstPinIndex)
                                    {
                                        dstPinName = dstIt->def.DataPins[p].PinName;
                                        break;
                                    }
                                    ++inIdx;
                                }
                            }
                        }
                    }
                }

                ConnectData(srcNodeID, srcPinName, dstNodeID, dstPinName);
                std::cout << "[VisualScriptEditorPanel] Created data link: node"
                          << srcNodeID << "." << srcPinName
                          << " -> node" << dstNodeID << "." << dstPinName << "\n";
                m_dirty = true;
            }
            else
            {
                std::cerr << "[VisualScriptEditorPanel] Cannot create link"
                             " — incompatible pin types (exec/data mismatch)\n";
            }
        }
        else
        {
            std::cerr << "[VisualScriptEditorPanel] Cannot create link"
                         " — incompatible pin types (both inputs or both outputs)\n";
        }
    }

    // Handle link deletion (triggered when the user Ctrl+clicks a link in ImNodes)
    int destroyedLink = -1;
    if (ImNodes::IsLinkDestroyed(&destroyedLink))
    {
        // Delegate to RemoveLink() so that:
        //   1. The underlying template connection is removed (not just the
        //      visual m_editorLinks entry).  Without this the connection would
        //      reappear as a "ghost" link the next time RebuildLinks() is called
        //      (e.g. after any undo/redo).
        //   2. A DeleteLinkCommand is pushed onto the undo stack, making the
        //      deletion reversible via Ctrl+Z.
        RemoveLink(destroyedLink);
    }

    // Handle node selection
    if (ImNodes::NumSelectedNodes() == 1)
    {
        int selNodes[1] = {-1};
        ImNodes::GetSelectedNodes(selNodes);
        m_selectedNodeID = selNodes[0];
    }
    else if (ImNodes::NumSelectedNodes() == 0)
    {
        m_selectedNodeID = -1;
    }

    // F9 = toggle breakpoint on selected node
    if (ImGui::IsKeyPressed(ImGuiKey_F9) && m_selectedNodeID >= 0)
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                "Node " + std::to_string(m_selectedNodeID));
    }

    // Delete key = remove all selected nodes and links
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && ImGui::IsWindowFocused())
    {
        int numSelectedNodes = ImNodes::NumSelectedNodes();
        if (numSelectedNodes > 0)
        {
            if (numSelectedNodes > 5)
            {
                std::cout << "[VSEditor] Warning: Deleting " << numSelectedNodes
                          << " nodes" << std::endl;
            }

            std::vector<int> selectedNodes(static_cast<size_t>(numSelectedNodes));
            ImNodes::GetSelectedNodes(selectedNodes.data());

            for (int nodeID : selectedNodes)
            {
                if (m_selectedNodeID == nodeID)
                    m_selectedNodeID = -1;
                RemoveNode(nodeID);
                std::cout << "[VSEditor] Deleted node " << nodeID << std::endl;
            }

            m_dirty = true;
        }

        int numSelectedLinks = ImNodes::NumSelectedLinks();
        if (numSelectedLinks > 0)
        {
            std::vector<int> selectedLinks(static_cast<size_t>(numSelectedLinks));
            ImNodes::GetSelectedLinks(selectedLinks.data());

            for (int linkID : selectedLinks)
            {
                RemoveLink(linkID);
                std::cout << "[VSEditor] Deleted link " << linkID << std::endl;
            }

            m_dirty = true;
        }
    }

    RenderValidationOverlay();
}

void VisualScriptEditorPanel::CopySelectedNodesToFrameworkClipboard()
{
    if (!m_framework || ImNodes::NumSelectedNodes() <= 0)
        return;

    const int selectedCount = ImNodes::NumSelectedNodes();
    std::vector<int> selectedNodes(static_cast<size_t>(selectedCount));
    ImNodes::GetSelectedNodes(selectedNodes.data());
    std::unordered_set<int> selectedIds(selectedNodes.begin(), selectedNodes.end());

    std::shared_ptr<VisualScriptGraphClipboardPayload> payload(
        new VisualScriptGraphClipboardPayload());

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    for (size_t index = 0; index < m_editorNodes.size(); ++index)
    {
        const VSEditorNode& editorNode = m_editorNodes[index];
        if (selectedIds.count(editorNode.nodeID) == 0)
            continue;
        minX = std::min(minX, editorNode.posX);
        minY = std::min(minY, editorNode.posY);
    }

    if (minX == std::numeric_limits<float>::max())
        return;

    for (size_t index = 0; index < m_editorNodes.size(); ++index)
    {
        const VSEditorNode& editorNode = m_editorNodes[index];
        if (selectedIds.count(editorNode.nodeID) == 0)
            continue;

        VisualScriptGraphClipboardPayload::NodeEntry entry;
        entry.definition = editorNode.def;
        // Visual Script v4 uses explicit exec/data connections.  Legacy
        // structural IDs would point back into the source graph after paste,
        // so rebuild the copied subgraph solely from the remapped links below.
        entry.definition.ChildrenIDs.clear();
        entry.definition.NextOnSuccess = NODE_INDEX_NONE;
        entry.definition.NextOnFailure = NODE_INDEX_NONE;
        entry.definition.EditorPosX = editorNode.posX;
        entry.definition.EditorPosY = editorNode.posY;
        entry.definition.HasEditorPos = true;
        entry.relativeX = editorNode.posX - minX;
        entry.relativeY = editorNode.posY - minY;
        payload->nodes.push_back(entry);
    }

    for (size_t index = 0; index < m_template.ExecConnections.size(); ++index)
    {
        const ExecPinConnection& connection = m_template.ExecConnections[index];
        if (selectedIds.count(connection.SourceNodeID) != 0 &&
            selectedIds.count(connection.TargetNodeID) != 0)
        {
            payload->execConnections.push_back(connection);
        }
    }
    for (size_t index = 0; index < m_template.DataConnections.size(); ++index)
    {
        const DataPinConnection& connection = m_template.DataConnections[index];
        if (selectedIds.count(connection.SourceNodeID) != 0 &&
            selectedIds.count(connection.TargetNodeID) != 0)
        {
            payload->dataConnections.push_back(connection);
        }
    }

    // Presets and Blackboard declarations are graph-scoped dependencies. They
    // are copied with the selection so a pasted Branch or data node remains
    // immediately valid in another VS tab.
    payload->presets = m_template.Presets;
    payload->localVariables = m_template.LocalVariables;
    payload->blackboardEntries = m_template.Blackboard;
    payload->globalVariableValues = m_template.GlobalVariableValues;

    m_framework->SetClipboardPayload(payload);
    SYSTEM_LOG << "[VSEditor] Copied " << payload->nodes.size() << " node(s), "
               << payload->execConnections.size() << " exec link(s), "
               << payload->dataConnections.size() << " data link(s)\n";
}

void VisualScriptEditorPanel::PasteFrameworkClipboard(float canvasX, float canvasY)
{
    if (!m_framework || !m_framework->HasClipboardPayload("VisualScript"))
        return;

    const GraphClipboardPayloadPtr genericPayload = m_framework->GetClipboardPayload();
    const std::shared_ptr<const VisualScriptGraphClipboardPayload> payload =
        std::dynamic_pointer_cast<const VisualScriptGraphClipboardPayload>(genericPayload);
    if (!payload || payload->nodes.empty())
        return;

    for (size_t index = 0; index < payload->presets.size(); ++index)
    {
        const ConditionPreset& preset = payload->presets[index];
        if (!m_presetRegistry.GetPreset(preset.id))
            m_presetRegistry.CreatePreset(preset);

        const bool storedInTemplate = std::find_if(
            m_template.Presets.begin(), m_template.Presets.end(),
            [&preset](const ConditionPreset& existing) { return existing.id == preset.id; })
            != m_template.Presets.end();
        if (!storedInTemplate)
            m_template.Presets.push_back(preset);
    }

    for (size_t index = 0; index < payload->localVariables.size(); ++index)
    {
        const VariableDefinition& variable = payload->localVariables[index];
        const bool alreadyDeclared = std::find_if(
            m_template.LocalVariables.begin(), m_template.LocalVariables.end(),
            [&variable](const VariableDefinition& existing) { return existing.Name == variable.Name; })
            != m_template.LocalVariables.end();
        if (!alreadyDeclared)
            m_template.LocalVariables.push_back(variable);
    }

    for (size_t index = 0; index < payload->blackboardEntries.size(); ++index)
    {
        const BlackboardEntry& entry = payload->blackboardEntries[index];
        const bool alreadyDeclared = std::find_if(
            m_template.Blackboard.begin(), m_template.Blackboard.end(),
            [&entry](const BlackboardEntry& existing) {
                return existing.Key == entry.Key && existing.IsGlobal == entry.IsGlobal;
            }) != m_template.Blackboard.end();
        if (!alreadyDeclared)
            m_template.Blackboard.push_back(entry);
    }

    if (payload->globalVariableValues.is_object())
    {
        for (nlohmann::json::const_iterator it = payload->globalVariableValues.begin();
             it != payload->globalVariableValues.end(); ++it)
        {
            if (!m_template.GlobalVariableValues.contains(it.key()))
                m_template.GlobalVariableValues[it.key()] = it.value();
        }
    }

    std::unordered_map<int32_t, int32_t> remappedIds;
    std::vector<int> pastedNodeIds;
    for (size_t index = 0; index < payload->nodes.size(); ++index)
    {
        const VisualScriptGraphClipboardPayload::NodeEntry& entry = payload->nodes[index];
        TaskNodeDefinition definition = entry.definition;
        const int32_t oldId = definition.NodeID;
        const int32_t newId = AllocNodeID();
        const float newX = canvasX + entry.relativeX;
        const float newY = canvasY + entry.relativeY;

        definition.NodeID = newId;
        definition.NodeName += " (Copy)";
        definition.EditorPosX = newX;
        definition.EditorPosY = newY;
        definition.HasEditorPos = true;

        ParameterBinding posX;
        posX.Type = ParameterBindingType::Literal;
        posX.LiteralValue = TaskValue(newX);
        ParameterBinding posY;
        posY.Type = ParameterBindingType::Literal;
        posY.LiteralValue = TaskValue(newY);
        definition.Parameters["__posX"] = posX;
        definition.Parameters["__posY"] = posY;

        VSEditorNode editorNode;
        editorNode.nodeID = newId;
        editorNode.posX = newX;
        editorNode.posY = newY;
        editorNode.def = definition;
        m_editorNodes.push_back(editorNode);
        m_undoStack.PushCommand(
            std::unique_ptr<ICommand>(new AddNodeCommand(definition)), m_template);
        ImNodes::SetNodeGridSpacePos(newId, ImVec2(newX, newY));
        remappedIds[oldId] = newId;
        pastedNodeIds.push_back(newId);
    }

    for (size_t index = 0; index < payload->execConnections.size(); ++index)
    {
        const ExecPinConnection& source = payload->execConnections[index];
        ConnectExec(remappedIds[source.SourceNodeID], source.SourcePinName,
                    remappedIds[source.TargetNodeID], source.TargetPinName);
    }
    for (size_t index = 0; index < payload->dataConnections.size(); ++index)
    {
        const DataPinConnection& source = payload->dataConnections[index];
        ConnectData(remappedIds[source.SourceNodeID], source.SourcePinName,
                    remappedIds[source.TargetNodeID], source.TargetPinName);
    }

    // A paste hands the interaction over to the newly-created subgraph. This
    // makes immediate drag, duplicate and delete actions operate on the pasted
    // nodes rather than on the source selection left in the destination tab.
    ImNodes::ClearNodeSelection();
    ImNodes::ClearLinkSelection();
    for (size_t index = 0; index < pastedNodeIds.size(); ++index)
        ImNodes::SelectNode(pastedNodeIds[index]);
    m_selectedNodeID = pastedNodeIds.empty() ? -1 : pastedNodeIds.front();

    m_dirty = true;
    m_verificationDone = false;
    SYSTEM_LOG << "[VSEditor] Pasted " << payload->nodes.size() << " node(s)\n";
}


// ============================================================================
// Node Palette
// ============================================================================

void VisualScriptEditorPanel::RenderNodePalette()
{
    if (!ImGui::BeginPopup("VSNodePalette"))
        return;

    ImGui::TextDisabled("Canvas Actions");
    if (ImGui::MenuItem("Select All", "Ctrl+A"))
    {
        m_pendingSelectAll = true;
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::MenuItem("Reset View"))
    {
        m_pendingResetView = true;
        ImGui::CloseCurrentPopup();
    }
    // Selection is queried only after EndNodeEditor(); querying ImNodes here
    // would violate its active-editor-scope contract.
    if (ImGui::MenuItem("Copy Selection", "Ctrl+C"))
    {
        m_pendingCopySelection = true;
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::MenuItem("Paste", "Ctrl+V", false,
                        m_framework && m_framework->HasClipboardPayload("VisualScript")))
    {
        m_pendingPaste = true;
        ImGui::CloseCurrentPopup();
    }

    ImGui::Separator();
    ImGui::TextDisabled("Add Node");
    ImGui::Separator();

    // Flow Control
    if (ImGui::BeginMenu("Flow Control"))
    {
        auto addFlowNode = [&](TaskNodeType type, const char* label) {
            if (ImGui::MenuItem(label))
            {
                AddNode(type, m_contextMenuX, m_contextMenuY);
                ImGui::CloseCurrentPopup();
            }
        };
        addFlowNode(TaskNodeType::EntryPoint, "EntryPoint");
        addFlowNode(TaskNodeType::Branch,     "Branch");
        addFlowNode(TaskNodeType::VSSequence, "Sequence");
        addFlowNode(TaskNodeType::While,      "While");
        addFlowNode(TaskNodeType::ForEach,    "ForEach");
        addFlowNode(TaskNodeType::DoOnce,     "DoOnce");
        addFlowNode(TaskNodeType::Delay,      "Delay");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Actions"))
    {
        if (ImGui::MenuItem("AtomicTask"))
        {
            AddNode(TaskNodeType::AtomicTask, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Data"))
    {
        if (ImGui::MenuItem("GetBBValue"))
        {
            AddNode(TaskNodeType::GetBBValue, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("SetBBValue"))
        {
            AddNode(TaskNodeType::SetBBValue, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("MathOp"))
        {
            AddNode(TaskNodeType::MathOp, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("SubGraph"))
    {
        if (ImGui::MenuItem("SubGraph"))
        {
            AddNode(TaskNodeType::SubGraph, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

// ============================================================================
// Context Menus
// ============================================================================

void VisualScriptEditorPanel::RenderContextMenus()
{
    // ========================================================================
    // Node context menu
    // ========================================================================
    if (ImGui::BeginPopup("VSNodeContextMenu"))
    {
        if (ImGui::MenuItem("Edit Properties"))
        {
            m_selectedNodeID = m_contextNodeID;
            SYSTEM_LOG << "[VSEditor] Selected node #" << m_contextNodeID
                       << " for editing\n";
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete Node"))
        {
            RemoveNode(m_contextNodeID);
            if (m_selectedNodeID == m_contextNodeID)
                m_selectedNodeID = -1;
            m_dirty = true;
            SYSTEM_LOG << "[VSEditor] Deleted node #" << m_contextNodeID
                       << " via context menu\n";
        }

        ImGui::Separator();

        {
            bool hasBP = DebugController::Get().HasBreakpoint(0, m_contextNodeID);
            if (ImGui::MenuItem(hasBP ? "Remove Breakpoint (F9)" : "Add Breakpoint (F9)"))
            {
                DebugController::Get().ToggleBreakpoint(0, m_contextNodeID,
                                                        m_template.Name,
                                                        "Node " + std::to_string(m_contextNodeID));
                SYSTEM_LOG << "[VSEditor] Toggled breakpoint on node #"
                           << m_contextNodeID << " -> "
                           << (hasBP ? "OFF" : "ON") << "\n";
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Duplicate"))
        {
            auto it = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                [this](const VSEditorNode& n) { return n.nodeID == m_contextNodeID; });
            if (it != m_editorNodes.end())
            {
                TaskNodeDefinition newDef = it->def;
                newDef.NodeID    = AllocNodeID();
                newDef.NodeName += " (Copy)";
                newDef.EditorPosX = it->posX + 50.0f;
                newDef.EditorPosY = it->posY + 50.0f;
                newDef.HasEditorPos = true;

                VSEditorNode eNew;
                eNew.nodeID = newDef.NodeID;
                eNew.posX   = newDef.EditorPosX;
                eNew.posY   = newDef.EditorPosY;
                eNew.def    = newDef;
                m_editorNodes.push_back(eNew);

                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(new AddNodeCommand(newDef)),
                    m_template);
                m_dirty = true;
                SYSTEM_LOG << "[VSEditor] Node " << m_contextNodeID
                           << " duplicated as #" << newDef.NodeID << "\n";
            }
        }

        ImGui::EndPopup();
    }

    // ========================================================================
    // Link context menu
    // ========================================================================
    if (ImGui::BeginPopup("VSLinkContextMenu"))
    {
        if (ImGui::MenuItem("Delete Connection"))
        {
            RemoveLink(m_contextLinkID);
            m_dirty = true;
            SYSTEM_LOG << "[VSEditor] Deleted link #" << m_contextLinkID
                       << " via context menu\n";
        }
        ImGui::EndPopup();
    }
}

} // namespace Olympe
