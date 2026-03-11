/**
 * @file AIEditorMenus.cpp
 * @brief Implementation of AI Editor menus
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "AIEditorMenus.h"
#include "AIEditorGUI.h"
#include "../../Core/IconsFontAwesome6.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"

namespace Olympe {
namespace AI {

// ============================================================================
// Menu Rendering
// ============================================================================

void AIEditorMenus::RenderFileMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu(ICON_FA_FILE " File")) {
        if (ImGui::MenuItem(ICON_FA_BRAIN " New Behavior Tree", "Ctrl+N")) {
            editor->MenuAction_NewBT();
        }
        if (ImGui::MenuItem(ICON_FA_DIAGRAM_PROJECT " New HFSM", "Ctrl+Shift+N")) {
            editor->MenuAction_NewHFSM();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open", "Ctrl+O")) {
            editor->MenuAction_Open();
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save", "Ctrl+S")) {
            editor->MenuAction_Save();
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save As", "Ctrl+Shift+S")) {
            editor->MenuAction_SaveAs();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_XMARK " Close", "Ctrl+W")) {
            editor->MenuAction_Close();
        }
        ImGui::EndMenu();
    }
}

void AIEditorMenus::RenderEditMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu(ICON_FA_PEN_TO_SQUARE " Edit")) {
        bool canUndo = editor->GetCommandStack().CanUndo();
        bool canRedo = editor->GetCommandStack().CanRedo();

        if (ImGui::MenuItem(ICON_FA_ROTATE_LEFT " Undo", "Ctrl+Z", false, canUndo)) {
            editor->MenuAction_Undo();
        }
        if (ImGui::MenuItem(ICON_FA_ROTATE_RIGHT " Redo", "Ctrl+Y", false, canRedo)) {
            editor->MenuAction_Redo();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_SCISSORS " Cut", "Ctrl+X")) {
            editor->MenuAction_Cut();
        }
        if (ImGui::MenuItem(ICON_FA_COPY " Copy", "Ctrl+C")) {
            editor->MenuAction_Copy();
        }
        if (ImGui::MenuItem(ICON_FA_CLIPBOARD " Paste", "Ctrl+V")) {
            editor->MenuAction_Paste();
        }
        if (ImGui::MenuItem(ICON_FA_TRASH_CAN " Delete", "Delete")) {
            editor->MenuAction_Delete();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_CHECK " Select All", "Ctrl+A")) {
            editor->MenuAction_SelectAll();
        }
        ImGui::EndMenu();
    }
}

void AIEditorMenus::RenderViewMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu(ICON_FA_EYE " View")) {
        if (ImGui::MenuItem(ICON_FA_CUBES " Show Node Palette")) {
            editor->MenuAction_ShowNodePalette();
        }
        if (ImGui::MenuItem(ICON_FA_TABLE " Show Blackboard")) {
            editor->MenuAction_ShowBlackboard();
        }
        if (ImGui::MenuItem(ICON_FA_MICROCHIP " Show Senses Panel")) {
            editor->MenuAction_ShowSensesPanel();
        }
        if (ImGui::MenuItem(ICON_FA_BUG " Show Runtime Debug")) {
            editor->MenuAction_ShowRuntimeDebug();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_WINDOW_RESTORE " Reset Layout")) {
            editor->MenuAction_ResetLayout();
        }
        ImGui::EndMenu();
    }
}

void AIEditorMenus::RenderHelpMenu(AIEditorGUI* editor)
{
    if (ImGui::BeginMenu(ICON_FA_CIRCLE_QUESTION " Help")) {
        if (ImGui::MenuItem(ICON_FA_CIRCLE_INFO " About")) {
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
