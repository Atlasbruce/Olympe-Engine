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
#include "../DataManager.h"

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
    // Phase 26 — Performance Optimization: Verification is now MANUAL, not automatic
    // Previously: Auto-verification was called every frame (CRITICAL BOTTLENECK)
    // Now: Verification runs only when user clicks "Verify" button or saves
    // This removes the O(n²) verification workload from the hot render path
    // Impact: ~60 FPS frame drops → smooth UI, verification runs on-demand

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

    // Right panel container with tab-based layout
    // Part A: Node Properties (top, resizable)
    // Part B: Tab system for Presets, Local Variables, Global Variables
    ImGui::BeginChild("VSRightPanel", ImVec2(m_propertiesPanelWidth, 0), true);

    float rightPanelHeight = ImGui::GetContentRegionAvail().y;
    float splitterHeight = 4.0f;

    // Initialize sub-panel heights on first use
    if (m_nodePropertiesPanelHeight <= 0.0f)
    {
        // Part A gets 35% of height, Part B (tabbed) gets remaining 65%
        m_nodePropertiesPanelHeight = rightPanelHeight * 0.35f;
    }

    // Clamp heights to reasonable ranges
    float minPanelHeight = 50.0f;
    if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;

    float tabbedPanelHeight = rightPanelHeight - m_nodePropertiesPanelHeight - splitterHeight;
    if (tabbedPanelHeight < minPanelHeight) tabbedPanelHeight = minPanelHeight;

    // ---- Part A: Node Properties Panel ----
    ImGui::BeginChild("Part_A_NodeProps", ImVec2(0, m_nodePropertiesPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderNodePropertiesPanel();
    ImGui::EndChild();

    // ---- Splitter (between Part A and Part B) ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##splitter_nodeprops_tabs", ImVec2(-1.0f, splitterHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_nodePropertiesPanelHeight += ImGui::GetIO().MouseDelta.y;
        if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;
    }
    ImGui::PopStyleColor(3);

    // ---- Part B: Tab-based panel (Presets, Local Variables, Global Variables) ----
    ImGui::BeginChild("Part_B_TabbedPanel", ImVec2(0, tabbedPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);

    // Phase 26: Render tab bar
    RenderRightPanelTabs();

    // Phase 26: Render active tab content
    RenderRightPanelTabContent();

    ImGui::EndChild();

    ImGui::EndChild();  // End VSRightPanel
}

void VisualScriptEditorPanel::RenderToolbar()
{
	// buttons: Save, Save As, Verify, Run Simulation
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
	//ImGui::SameLine(); // useless since the Panel proeprties button is now in the right panel tab bar
	//if (ImGui::Button("Condition Presets"))
	//{
	//    m_libraryPanel->Open();
	//}
	ImGui::SameLine();

	// Phase 37 — Minimap controls for VisualScript canvas
	if (m_canvasEditor)
	{
		// Checkbox to toggle minimap visibility
		if (ImGui::Checkbox("Minimap##vs", &m_minimapVisible))
		{
			m_canvasEditor->SetMinimapVisible(m_minimapVisible);
		}
		ImGui::SameLine();

		// DragFloat to control minimap size (0.05 to 0.5 of canvas)
		if (ImGui::DragFloat("Size##minimap_vs", &m_minimapSize, 0.01f, 0.05f, 0.5f, "%.2f"))
		{
			m_minimapSize = std::max(0.05f, std::min(0.5f, m_minimapSize));
			m_canvasEditor->SetMinimapSize(m_minimapSize);
		}
		ImGui::SameLine();

		// Combo for minimap position
		const char* positionLabels[] = { "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right" };
		if (ImGui::Combo("Position##minimap_vs", &m_minimapPosition, positionLabels, 4))
		{
			m_canvasEditor->SetMinimapPosition(m_minimapPosition);
		}
		ImGui::SameLine();
	}

	// Title
	const char* title = m_currentPath.empty()
		? "Untitled VS Graph"
		: m_currentPath.c_str();
	ImGui::TextDisabled("%s%s", title, m_dirty ? " *" : "");

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
        // Use centralized SaveFilePickerModal via DataManager (Phase 40)
        DataManager& dm = DataManager::Get();
        std::string suggestedName = m_currentPath.empty() 
                                    ? "graph" 
                                    : m_currentPath.substr(m_currentPath.find_last_of("/\\") + 1);

        // Remove extension from suggested name
        size_t dotPos = suggestedName.rfind('.');
        if (dotPos != std::string::npos)
            suggestedName = suggestedName.substr(0, dotPos);

        dm.OpenSaveFilePickerModal(Olympe::SaveFileType::Blueprint, "Gamedata/TaskGraph", suggestedName);
        m_showSaveAsDialog = false;
    }

    // Render centralized save modal
    DataManager& dm = DataManager::Get();
    dm.RenderSaveFilePickerModal();

    // Handle modal result
    if (dm.IsSaveFilePickerModalOpen() == false) {
        std::string selectedFile = dm.GetSelectedSaveFile();
        if (!selectedFile.empty()) {
            SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs dialog confirmed. fullPath='"
                       << selectedFile << "'\n";
            if (SaveAs(selectedFile)) {
                std::cout << "[VisualScriptEditorPanel] Saved to: " << selectedFile << std::endl;
                m_currentPath = selectedFile;
            } else {
                SYSTEM_LOG << "[VisualScriptEditorPanel] Save failed - check directory and permissions\n";
            }
        }
    }
}

} // namespace Olympe
