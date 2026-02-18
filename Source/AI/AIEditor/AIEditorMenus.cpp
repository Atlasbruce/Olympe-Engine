/**
 * @file AIEditorMenus.cpp
 * @brief Implementation of AI Editor menus
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "AIEditorMenus.h"
#include "AIEditorGUI.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"

namespace Olympe {
namespace AI {

// ============================================================================
// Menu Rendering
// ============================================================================

void AIEditorMenus::RenderFileMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Behavior Tree", "Ctrl+N")) {
            editor->MenuAction_NewBT();
        }
        if (ImGui::MenuItem("New HFSM", "Ctrl+Shift+N")) {
            editor->MenuAction_NewHFSM();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Open", "Ctrl+O")) {
            editor->MenuAction_Open();
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            editor->MenuAction_Save();
        }
        if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
            editor->MenuAction_SaveAs();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Close", "Ctrl+W")) {
            editor->MenuAction_Close();
        }
        ImGui::EndMenu();
    }
}

void AIEditorMenus::RenderEditMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu("Edit")) {
        bool canUndo = editor->GetCommandStack().CanUndo();
        bool canRedo = editor->GetCommandStack().CanRedo();
        
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo)) {
            editor->MenuAction_Undo();
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) {
            editor->MenuAction_Redo();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            editor->MenuAction_Cut();
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            editor->MenuAction_Copy();
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            editor->MenuAction_Paste();
        }
        if (ImGui::MenuItem("Delete", "Delete")) {
            editor->MenuAction_Delete();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select All", "Ctrl+A")) {
            editor->MenuAction_SelectAll();
        }
        ImGui::EndMenu();
    }
}

void AIEditorMenus::RenderViewMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Show Node Palette")) {
            editor->MenuAction_ShowNodePalette();
        }
        if (ImGui::MenuItem("Show Blackboard")) {
            editor->MenuAction_ShowBlackboard();
        }
        if (ImGui::MenuItem("Show Senses Panel")) {
            editor->MenuAction_ShowSensesPanel();
        }
        if (ImGui::MenuItem("Show Runtime Debug")) {
            editor->MenuAction_ShowRuntimeDebug();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout")) {
            editor->MenuAction_ResetLayout();
        }
        ImGui::EndMenu();
    }
}

void AIEditorMenus::RenderHelpMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About")) {
            editor->MenuAction_About();
        }
        ImGui::EndMenu();
    }
    (void)editor;
}

// ============================================================================
// Dialogs
// ============================================================================

void AIEditorMenus::ShowNewBTDialog(AIEditorGUI* editor)
{
    // TODO: Implement new BT dialog with templates
    SYSTEM_LOG << "[AIEditorMenus] New BT dialog (not yet implemented)" << std::endl;
    (void)editor;
}

void AIEditorMenus::ShowOpenDialog(AIEditorGUI* editor)
{
    // TODO: Implement file browser dialog
    SYSTEM_LOG << "[AIEditorMenus] Open dialog (not yet implemented)" << std::endl;
    (void)editor;
}

void AIEditorMenus::ShowSaveAsDialog(AIEditorGUI* editor)
{
    // TODO: Implement save as dialog
    SYSTEM_LOG << "[AIEditorMenus] Save As dialog (not yet implemented)" << std::endl;
    (void)editor;
}

void AIEditorMenus::ShowAboutDialog()
{
    ImGui::OpenPopup("About AI Editor");
    
    if (ImGui::BeginPopupModal("About AI Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("AI Editor v1.0");
        ImGui::Text("Phase 1.3 Implementation");
        ImGui::Separator();
        ImGui::Text("Olympe Engine");
        ImGui::Text("(c) 2026");
        
        ImGui::Separator();
        
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

} // namespace AI
} // namespace Olympe
