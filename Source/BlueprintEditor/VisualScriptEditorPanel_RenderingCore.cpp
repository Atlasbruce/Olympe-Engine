/**
 * @file VisualScriptEditorPanel_RenderingCore.cpp
 * @brief Core rendering and undo/redo operations for VisualScriptEditorPanel.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details Extracted from VisualScriptEditorPanel.cpp (Phase 8 refactoring).
 * Contains: PerformUndo, PerformRedo, Render, RenderContent, RenderToolbar, RenderSaveAsDialog.
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"
#include "BBVariableRegistry.h"
#include "MathOpOperand.h"
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
#include <unordered_set>

namespace Olympe {

// ============================================================================
// Undo/Redo wrappers
// ============================================================================

void VisualScriptEditorPanel::PerformUndo()
{
    if (!m_undoStack.CanUndo())
        return;

    std::string desc = m_undoStack.PeekUndoDescription();
    SYSTEM_LOG << "[VSEditor] UNDO: " << desc << "\n";
    m_undoStack.Undo(m_template);
    SyncEditorNodesFromTemplate();
    RebuildLinks();

    // Force-push the restored positions into ImNodes so that the next
    // BeginNode()/EndNode() cycle renders them at the correct location.
    // BUG-003 Fix: positions stored in m_editorNodes are grid-space
    // (written by SyncNodePositionsFromImNodes via GetNodeGridSpacePos),
    // so use SetNodeGridSpacePos to restore them pan-independently.
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        ImNodes::SetNodeGridSpacePos(
            m_editorNodes[i].nodeID,
            ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
    }

    // Block position sync and movement tracking for 1 frame so that stale
    // ImNodes state cannot overwrite the correct undo-target positions before
    // ImNodes has rendered the new layout at least once.
    m_justPerformedUndoRedo      = true;
    m_skipPositionSyncNextFrame  = true;
    m_nodeDragStartPositions.clear();
    m_dirty = true;
    m_verificationDone = false;
    SYSTEM_LOG << "[VSEditor] Undo complete. Template now has "
               << m_template.Nodes.size() << " nodes, "
               << m_template.ExecConnections.size() << " exec connections\n";
}

void VisualScriptEditorPanel::PerformRedo()
{
    if (!m_undoStack.CanRedo())
        return;

    std::string desc = m_undoStack.PeekRedoDescription();
    SYSTEM_LOG << "[VSEditor] REDO: " << desc << "\n";
    m_undoStack.Redo(m_template);
    SyncEditorNodesFromTemplate();
    RebuildLinks();

    // Same treatment as PerformUndo().
    // BUG-003 Fix: use SetNodeGridSpacePos (grid-space) for pan-independent restore.
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        ImNodes::SetNodeGridSpacePos(
            m_editorNodes[i].nodeID,
            ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
    }

    m_justPerformedUndoRedo      = true;
    m_skipPositionSyncNextFrame  = true;
    m_nodeDragStartPositions.clear();
    m_dirty = true;
    m_verificationDone = false;
    SYSTEM_LOG << "[VSEditor] Redo complete. Template now has "
               << m_template.Nodes.size() << " nodes, "
               << m_template.ExecConnections.size() << " exec connections\n";
}

// ============================================================================
// Rendering
// ============================================================================

void VisualScriptEditorPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("VS Graph Editor", &m_visible);
    RenderContent();
    ImGui::End();

    // Render the condition preset library panel (Phase 24 UI integration)
    m_libraryPanel->Render();
}

void VisualScriptEditorPanel::RenderContent()
{
    // Auto-initialize verification on first render (Phase 24.3)
    // This ensures logs panel displays without requiring "Verify" button click first
    if (!m_verificationDone && m_template.Nodes.size() > 0)
    {
        RunVerification();
    }

    RenderToolbar();
    RenderSaveAsDialog();
    ImGui::Separator();

    // Two-column layout: canvas (left) | resize handle | properties panel (right, 3 sub-panels)
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // Initialize panel width to default 28% on first use
    if (m_propertiesPanelWidth <= 0.0f)
        m_propertiesPanelWidth = totalWidth * 0.28f;

    // Clamp to a sensible range
    if (m_propertiesPanelWidth < 200.0f) m_propertiesPanelWidth = 200.0f;
    if (m_propertiesPanelWidth > totalWidth * 0.60f) m_propertiesPanelWidth = totalWidth * 0.60f;

    float handleWidth = 6.0f;
    float canvasWidth = totalWidth - m_propertiesPanelWidth - handleWidth;

    ImGui::BeginChild("VSCanvas", ImVec2(canvasWidth, 0), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderCanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    // UX Fix #3: Drag-to-resize handle between canvas and properties panel
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##vsresize", ImVec2(handleWidth, -1.0f));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_propertiesPanelWidth -= ImGui::GetIO().MouseDelta.x;
        if (m_propertiesPanelWidth < 200.0f)          m_propertiesPanelWidth = 200.0f;
        if (m_propertiesPanelWidth > totalWidth * 0.60f) m_propertiesPanelWidth = totalWidth * 0.60f;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Right panel container with 3 vertical sub-panels (A: Node Props | B: Preset Bank | C: Local Vars)
    ImGui::BeginChild("VSRightPanel", ImVec2(m_propertiesPanelWidth, 0), true);

    float rightPanelHeight = ImGui::GetContentRegionAvail().y;
    float splitterHeight = 4.0f;

    // Initialize sub-panel heights on first use (equal thirds for 3 panels)
    if (m_nodePropertiesPanelHeight <= 0.0f)
    {
        m_nodePropertiesPanelHeight = (rightPanelHeight - splitterHeight * 2) / 3.0f;
        m_presetBankPanelHeight = (rightPanelHeight - splitterHeight * 2) / 3.0f;
    }

    // Clamp heights to reasonable ranges
    float minPanelHeight = 50.0f;
    if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;
    if (m_presetBankPanelHeight < minPanelHeight) m_presetBankPanelHeight = minPanelHeight;

    float localVarHeight = rightPanelHeight - m_nodePropertiesPanelHeight - m_presetBankPanelHeight - splitterHeight * 2;
    if (localVarHeight < minPanelHeight) localVarHeight = minPanelHeight;

    // ---- Part A: Node Properties Panel ----
    ImGui::BeginChild("Part_A_NodeProps", ImVec2(0, m_nodePropertiesPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderNodePropertiesPanel();
    ImGui::EndChild();

    // ---- Splitter 1 (between Part A and Part B) ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##splitter1", ImVec2(-1.0f, splitterHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_nodePropertiesPanelHeight += ImGui::GetIO().MouseDelta.y;
        if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;
    }
    ImGui::PopStyleColor(3);

    // ---- Part B: Preset Bank Panel ----
    ImGui::BeginChild("Part_B_PresetBank", ImVec2(0, m_presetBankPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderPresetBankPanel();
    ImGui::EndChild();

    // ---- Splitter 2 (between Part B and Part C) ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##splitter2", ImVec2(-1.0f, splitterHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_presetBankPanelHeight += ImGui::GetIO().MouseDelta.y;
        if (m_presetBankPanelHeight < minPanelHeight) m_presetBankPanelHeight = minPanelHeight;
    }
    ImGui::PopStyleColor(3);

    // ---- Part C: Local/Global Variables Panel (with tab selection) ----
    ImGui::BeginChild("Part_C_Blackboard", ImVec2(0, localVarHeight), false,
                      ImGuiWindowFlags_NoScrollbar);

    // Tab selector for Local vs Global variables
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
    ImGui::RadioButton("Local Variables", &m_blackboardTabSelection, 0);
    ImGui::SameLine(150.0f);
    ImGui::RadioButton("Global Variables", &m_blackboardTabSelection, 1);
    ImGui::PopStyleVar();
    ImGui::Separator();

    // Render appropriate panel based on tab selection
    if (m_blackboardTabSelection == 0)
        RenderLocalVariablesPanel();
    else
        RenderGlobalVariablesPanel();

    ImGui::EndChild();

    ImGui::EndChild();  // End VSRightPanel
}

void VisualScriptEditorPanel::RenderToolbar()
{
    // Title
    const char* title = m_currentPath.empty()
                        ? "Untitled VS Graph"
                        : m_currentPath.c_str();
    ImGui::TextDisabled("%s%s", title, m_dirty ? " *" : "");

    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Save button clicked. m_currentPath='"
                   << m_currentPath << "'\n";
        if (m_currentPath.empty())
        {
            m_showSaveAsDialog = true;
        }
        else if (!Save())
        {
            ImGui::OpenPopup("SaveError");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As"))
    {
        m_showSaveAsDialog = true;
    }
    ImGui::SameLine();
    // Phase 24.3 — Removed "New Graph" button as requested
    // Users must create new graphs through the file browser instead

    if (ImGui::Button("Verify##gvs"))
    {
        RunVerification();
    }
    ImGui::SameLine();
    if (ImGui::Button("Run Graph##sim"))
    {
        RunGraphSimulation();
    }
    ImGui::SameLine();
    if (ImGui::Button("Condition Presets"))
    {
        m_libraryPanel->Open();
    }
    ImGui::SameLine();
    if (m_verificationDone)
    {
        if (m_verificationResult.HasErrors())
        {
            int errorCount = 0;
            for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
            {
                if (m_verificationResult.issues[i].severity == VSVerificationSeverity::Error)
                    ++errorCount;
            }
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                               "[%d error(s)]", errorCount);
        }
        else if (m_verificationResult.HasWarnings())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[OK - warnings]");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[OK]");
        }
    }

    // Keyboard shortcuts
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_S) &&
            ImGui::GetIO().KeyCtrl)
        {
            SYSTEM_LOG << "[VisualScriptEditorPanel] Ctrl+S pressed. m_currentPath='"
                       << m_currentPath << "'\n";
            if (m_currentPath.empty())
            {
                m_showSaveAsDialog = true;
            }
            else if (!Save())
            {
                ImGui::OpenPopup("SaveError");
            }
        }

        // Undo (Ctrl+Z)
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) &&
            m_undoStack.CanUndo())
        {
            PerformUndo();
        }

        // Redo (Ctrl+Y)
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) &&
            m_undoStack.CanRedo())
        {
            PerformRedo();
        }
    }

    if (ImGui::BeginPopup("SaveError"))
    {
        ImGui::TextColored(ImVec4(1,0,0,1), "Save failed — check file path.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void VisualScriptEditorPanel::RenderSaveAsDialog()
{
    if (m_showSaveAsDialog)
    {
        ImGui::OpenPopup("SaveAsDialog");
        m_showSaveAsDialog = false;

        // Derive the save extension from the currently loaded file so we
        // preserve .json files as .json (instead of silently renaming to .ats).
        // Fall back to .ats for new/untitled graphs.
        m_saveAsExtension = ".ats";
        if (!m_currentPath.empty())
        {
            size_t dotPos = m_currentPath.rfind('.');
            if (dotPos != std::string::npos)
                m_saveAsExtension = m_currentPath.substr(dotPos);
        }

        // Pre-fill the filename from the current path so the user doesn't have
        // to retype a name they already gave the graph.
        if (!m_currentPath.empty())
        {
            size_t lastSlash = m_currentPath.find_last_of("/\\");
            std::string fname = (lastSlash != std::string::npos)
                                ? m_currentPath.substr(lastSlash + 1)
                                : m_currentPath;
            // Strip extension
            size_t dotPos = fname.rfind('.');
            if (dotPos != std::string::npos)
                fname = fname.substr(0, dotPos);

            strncpy_s(m_saveAsFilename, sizeof(m_saveAsFilename), fname.c_str(), _TRUNCATE);
        }
        // else: keep whatever is already in the buffer (set in constructor or
        //       carried over from a previous dialog invocation).
    }

    if (ImGui::BeginPopupModal("SaveAsDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save Visual Script As");
        ImGui::Separator();

        // Directory dropdown
        ImGui::Text("Directory:");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##SaveDir", m_saveAsDirectory.c_str()))
        {
            static const char* dirs[] = {
                "Blueprints/AI",
                "Blueprints/AI/Tests",
                "Gamedata/TaskGraph/Examples",
                "Gamedata/TaskGraph/Templates"
            };
            for (int i = 0; i < static_cast<int>(sizeof(dirs) / sizeof(dirs[0])); ++i)
            {
                bool selected = (m_saveAsDirectory == dirs[i]);
                if (ImGui::Selectable(dirs[i], selected))
                    m_saveAsDirectory = dirs[i];
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Filename input
        ImGui::Text("Filename:");
        ImGui::SameLine();
        ImGui::InputText("##FileName", m_saveAsFilename, sizeof(m_saveAsFilename));

        // Full path preview
        ImGui::TextDisabled("Full path: %s/%s%s",
                            m_saveAsDirectory.c_str(),
                            m_saveAsFilename,
                            m_saveAsExtension.c_str());

        ImGui::Separator();

        // Save / Cancel buttons
        bool filenameEmpty = (std::strlen(m_saveAsFilename) == 0);
        if (filenameEmpty)
            ImGui::BeginDisabled();
        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            std::string fullPath = m_saveAsDirectory + "/" +
                                   std::string(m_saveAsFilename) + m_saveAsExtension;
            SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs dialog confirmed. fullPath='"
                       << fullPath << "'\n";
            if (SaveAs(fullPath))
            {
                std::cout << "[VisualScriptEditorPanel] Saved to: " << fullPath << std::endl;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                ImGui::OpenPopup("SaveAsError");
            }
        }
        if (filenameEmpty)
            ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        // Nested error popup
        if (ImGui::BeginPopupModal("SaveAsError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Save failed — check directory and permissions.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace Olympe
