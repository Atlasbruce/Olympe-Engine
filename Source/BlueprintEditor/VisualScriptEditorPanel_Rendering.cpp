// ============================================================================
// VisualScriptEditorPanel_Rendering.cpp
// Main UI Rendering: Toolbar, Node Palette, Context Menus
// ============================================================================
//
// This file contains all main UI rendering including toolbar, node selection
// palette, context menus, and dialog windows.
//
// Methods:
// - RenderToolbar()               : Top toolbar with buttons
// - RenderNodePalette()           : Node selection panel (drag-drop)
// - RenderContextMenus()          : Right-click context menus
//
// Integration Points:
// - ImGui rendering                : UI framework
// - m_selectedNodeID               : Selection state
// - m_dirty flag                   : Modification tracking
// - undo/redo stack               : Edit history
// ============================================================================

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


void VisualScriptEditorPanel::RenderNodePalette()
{
    if (!ImGui::BeginPopup("VSNodePalette"))
        return;

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

// ============================================================================
// Branch / While node — dedicated Properties panel renderer
// ============================================================================


} // namespace Olympe
