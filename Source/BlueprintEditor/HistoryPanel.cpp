/*
 * Olympe Blueprint Editor - History Panel Implementation
 */

#include "HistoryPanel.h"
#include "BlueprintEditor.h"
#include "BPCommandSystem.h"
#include "../third_party/imgui/imgui.h"

namespace Olympe
{
    HistoryPanel::HistoryPanel()
        : m_ShowPanel(false)
    {
    }

    HistoryPanel::~HistoryPanel()
    {
    }

    void HistoryPanel::Initialize()
    {
        m_ShowPanel = false;
    }

    void HistoryPanel::Shutdown()
    {
    }

    void HistoryPanel::Render()
    {
        if (!m_ShowPanel)
        {
            return;
        }

        ImGui::Begin("History", &m_ShowPanel);

        auto& backend = BlueprintEditor::Get();
        Blueprint::CommandStack* cmdStack = backend.GetCommandStack();

        if (!cmdStack)
        {
            ImGui::Text("Command stack not initialized");
            ImGui::End();
            return;
        }

        // Display command stack info
        ImGui::Text("Undo Stack Size: %zu", cmdStack->GetUndoStackSize());
        ImGui::Text("Redo Stack Size: %zu", cmdStack->GetRedoStackSize());

        ImGui::Separator();

        // Undo stack section
        ImGui::Text("Undo History:");
        ImGui::BeginChild("UndoHistory", ImVec2(0, 200), true);
        
        auto undoDescriptions = cmdStack->GetUndoStackDescriptions();
        for (int i = static_cast<int>(undoDescriptions.size()) - 1; i >= 0; i--)
        {
            ImGui::PushID(i);
            
            // Highlight the most recent command
            if (i == static_cast<int>(undoDescriptions.size()) - 1)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                ImGui::Text("-> %s", undoDescriptions[i].c_str());
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("   %s", undoDescriptions[i].c_str());
            }
            
            ImGui::PopID();
        }
        
        if (undoDescriptions.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No commands to undo");
        }
        
        ImGui::EndChild();

        ImGui::Separator();

        // Redo stack section
        ImGui::Text("Redo History:");
        ImGui::BeginChild("RedoHistory", ImVec2(0, 150), true);
        
        auto redoDescriptions = cmdStack->GetRedoStackDescriptions();
        for (int i = static_cast<int>(redoDescriptions.size()) - 1; i >= 0; i--)
        {
            ImGui::PushID(i + 1000);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "   %s", redoDescriptions[i].c_str());
            ImGui::PopID();
        }
        
        if (redoDescriptions.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No commands to redo");
        }
        
        ImGui::EndChild();

        ImGui::Separator();

        // Action buttons
        ImGui::BeginDisabled(!backend.CanUndo());
        if (ImGui::Button("Undo", ImVec2(120, 0)))
        {
            backend.Undo();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!backend.CanRedo());
        if (ImGui::Button("Redo", ImVec2(120, 0)))
        {
            backend.Redo();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Clear History", ImVec2(120, 0)))
        {
            if (cmdStack)
            {
                cmdStack->Clear();
            }
        }

        ImGui::End();
    }
}
