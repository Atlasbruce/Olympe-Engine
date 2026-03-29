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
// - RenderSaveAsDialog()          : (Moved to FileOps)
// - RenderValidationOverlay()     : (Moved to Verification)
// - RenderMainMenu()              : Main application menu
// - RenderStatusBar()             : Bottom status bar
//
// Integration Points:
// - ImGui rendering                : UI framework
// - m_selectedNodeID               : Selection state
// - m_dirty flag                   : Modification tracking
// - undo/redo stack               : Edit history
// ============================================================================

#include "VisualScriptEditorPanel.h"

namespace Olympe {

// ============================================================================
// Toolbar Rendering
// ============================================================================

void VisualScriptEditorPanel::RenderToolbar()
{
    // Toolbar buttons row
    ImGui::BeginChild("Toolbar", ImVec2(0, 40), true);

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("File:");
    ImGui::SameLine();

    // Load button
    if (ImGui::Button("Load##vsload", ImVec2(50, 0)))
    {
        // Trigger file open dialog (platform-specific)
        SYSTEM_LOG << "[VSEditor] Load button clicked\n";
    }
    ImGui::SameLine();

    // Save button
    if (ImGui::Button("Save##vssave", ImVec2(50, 0)))
    {
        Save();
    }
    ImGui::SameLine();

    // Save As button
    if (ImGui::Button("Save As##vssaveas", ImVec2(70, 0)))
    {
        m_showSaveAsDialog = true;
        ImGui::OpenPopup("Save As##vseditor");
    }
    ImGui::SameLine();

    ImGui::Separator();
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Edit:");
    ImGui::SameLine();

    // Undo button
    if (ImGui::Button("Undo##vsundo", ImVec2(50, 0)))
    {
        PerformUndo();
    }
    ImGui::SameLine();

    // Redo button
    if (ImGui::Button("Redo##vsredo", ImVec2(50, 0)))
    {
        PerformRedo();
    }
    ImGui::SameLine();

    ImGui::Separator();
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Verify:");
    ImGui::SameLine();

    // Verification button
    if (ImGui::Button("Verify##vsverify", ImVec2(55, 0)))
    {
        RunVerification();
    }
    ImGui::SameLine();

    // Test Execution button (Phase 24.3)
    if (ImGui::Button("Test##vstest", ImVec2(50, 0)))
    {
        SYSTEM_LOG << "[VSEditor] Test Execution clicked\n";
        // Would trigger ExecutionTestPanel rendering
    }

    ImGui::EndChild();
}

// ============================================================================
// Node Palette - Drag-drop Node Creation
// ============================================================================

void VisualScriptEditorPanel::RenderNodePalette()
{
    ImGui::BeginChild("NodePalette", ImVec2(200, 0), true);

    ImGui::TextDisabled("Node Palette");
    ImGui::Separator();

    // List of available node types
    const char* nodeTypeNames[] = {
        "Entry Point",
        "Branch",
        "Switch",
        "While",
        "ForEach",
        "DoOnce",
        "Delay",
        "Sequence",
        "SubGraph",
        "Atomic Task",
        "Get Variable",
        "Set Variable",
        "Math Op",
    };

    const TaskNodeType nodeTypeValues[] = {
        TaskNodeType::EntryPoint,
        TaskNodeType::Branch,
        TaskNodeType::Switch,
        TaskNodeType::While,
        TaskNodeType::ForEach,
        TaskNodeType::DoOnce,
        TaskNodeType::Delay,
        TaskNodeType::VSSequence,
        TaskNodeType::SubGraph,
        TaskNodeType::AtomicTask,
        TaskNodeType::GetBBValue,
        TaskNodeType::SetBBValue,
        TaskNodeType::MathOp,
    };

    for (int i = 0; i < 13; ++i)
    {
        ImGui::PushID(i);

        // Make selectable items draggable
        if (ImGui::Selectable(nodeTypeNames[i]))
        {
            // Node selected (could trigger drag)
        }

        // Drag-drop support
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            TaskNodeType nodeType = nodeTypeValues[i];
            ImGui::SetDragDropPayload("NODE_TYPE", &nodeType, sizeof(TaskNodeType));
            ImGui::Text("Create: %s", nodeTypeNames[i]);
            ImGui::EndDragDropSource();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

// ============================================================================
// Context Menus - Right-click Actions
// ============================================================================

void VisualScriptEditorPanel::RenderContextMenus()
{
    // Node context menu
    if (ImGui::BeginPopupContextItem("NodeContext##vsed"))
    {
        if (m_selectedNodeID >= 0)
        {
            ImGui::TextDisabled("Node %d", m_selectedNodeID);
            ImGui::Separator();

            if (ImGui::MenuItem("Duplicate"))
            {
                // Duplicate selected node
            }
            if (ImGui::MenuItem("Delete"))
            {
                RemoveNode(m_selectedNodeID);
            }
            if (ImGui::MenuItem("Goto properties"))
            {
                m_focusNodeID = m_selectedNodeID;
            }
        }

        ImGui::EndPopup();
    }

    // Canvas context menu
    if (ImGui::BeginPopupContextVoid("CanvasContext##vsed"))
    {
        if (ImGui::MenuItem("Add Node..."))
        {
            // Open node palette
        }
        if (ImGui::MenuItem("Paste"))
        {
            // Paste copied nodes
        }

        ImGui::EndPopup();
    }
}

// ============================================================================
// Main Menu
// ============================================================================

void VisualScriptEditorPanel::RenderMainMenu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load"))
            {
                SYSTEM_LOG << "[VSEditor] Load menu clicked\n";
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                Save();
            }
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S"))
            {
                m_showSaveAsDialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close"))
            {
                // Close current blueprint
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
            {
                PerformUndo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y"))
            {
                PerformRedo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A"))
            {
                // Select all nodes
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Node Palette", nullptr, &m_showNodePalette);
            ImGui::MenuItem("Properties", nullptr, &m_showProperties);
            ImGui::MenuItem("Verification", nullptr, &m_showVerification);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

// ============================================================================
// Status Bar
// ============================================================================

void VisualScriptEditorPanel::RenderStatusBar()
{
    ImGui::BeginChild("StatusBar", ImVec2(0, 25), true);

    // Dirty indicator
    if (m_dirty)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "●");
        ImGui::SameLine();
    }

    ImGui::TextDisabled("Nodes: %zu | Links: %zu",
                       m_editorNodes.size(),
                       m_editorLinks.size());

    ImGui::SameLine(ImGui::GetWindowWidth() - 150);

    if (m_verificationDone)
    {
        if (m_verificationResult.HasErrors())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "❌ Errors");
        }
        else if (m_verificationResult.HasWarnings())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "⚠ Warnings");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "✓ OK");
        }
    }
    else
    {
        ImGui::TextDisabled("(not verified)");
    }

    ImGui::EndChild();
}

}  // namespace Olympe
