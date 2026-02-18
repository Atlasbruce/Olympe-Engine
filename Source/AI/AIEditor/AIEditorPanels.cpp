/**
 * @file AIEditorPanels.cpp
 * @brief Implementation of specialized AI Editor panels
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "AIEditorPanels.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"
#include <cstring>

namespace Olympe {
namespace AI {

// ============================================================================
// BlackboardInspectorPanel
// ============================================================================

BlackboardInspectorPanel::BlackboardInspectorPanel()
    : m_showAddDialog(false)
{
    std::memset(m_newVarName, 0, sizeof(m_newVarName));
}

void BlackboardInspectorPanel::Render(NodeGraph::GraphDocument* activeGraph)
{
    if (activeGraph == nullptr) {
        ImGui::Text("No active graph");
        return;
    }
    
    ImGui::Text("Blackboard Variables");
    ImGui::Separator();
    
    if (ImGui::Button("Add Variable")) {
        m_showAddDialog = true;
    }
    
    ImGui::Separator();
    
    RenderVariableList(activeGraph);
    
    if (m_showAddDialog) {
        RenderAddVariableDialog();
    }
}

void BlackboardInspectorPanel::RenderVariableList(NodeGraph::GraphDocument* doc)
{
    // TODO: Implement blackboard variable list from graph metadata
    ImGui::Text("(Variable list not yet implemented)");
    (void)doc;
}

void BlackboardInspectorPanel::RenderAddVariableDialog()
{
    ImGui::OpenPopup("Add Blackboard Variable");
    
    if (ImGui::BeginPopupModal("Add Blackboard Variable", &m_showAddDialog)) {
        ImGui::InputText("Name", m_newVarName, sizeof(m_newVarName));
        
        if (ImGui::Button("Add")) {
            SYSTEM_LOG << "[BlackboardPanel] Add variable: " << m_newVarName << std::endl;
            m_showAddDialog = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel")) {
            m_showAddDialog = false;
        }
        
        ImGui::EndPopup();
    }
}

void BlackboardInspectorPanel::RenderVariableEditor(const std::string& varName)
{
    ImGui::Text("Editing: %s", varName.c_str());
    // TODO: Implement variable editor
}

// ============================================================================
// SensesDebugPanel
// ============================================================================

SensesDebugPanel::SensesDebugPanel()
{
}

void SensesDebugPanel::Render()
{
    ImGui::Text("AI Senses Debug");
    ImGui::Separator();
    
    RenderEntitySensesList();
}

void SensesDebugPanel::RenderEntitySensesList()
{
    ImGui::Text("Entities with AI Senses:");
    ImGui::Separator();
    
    // TODO: List entities with AIComponent
    ImGui::Text("(Entity list not yet implemented)");
}

void SensesDebugPanel::RenderSenseDetails()
{
    // TODO: Show sense details for selected entity
}

void SensesDebugPanel::RenderVisionCone()
{
    // TODO: Visualize vision cone
}

void SensesDebugPanel::RenderDetectedTargets()
{
    // TODO: Show detected targets
}

// ============================================================================
// RuntimeDebugPanel
// ============================================================================

RuntimeDebugPanel::RuntimeDebugPanel()
    : m_selectedEntity(-1)
{
}

void RuntimeDebugPanel::Render()
{
    ImGui::Text("Runtime Debug");
    ImGui::Separator();
    
    // Split panel
    ImGui::BeginChild("EntityList", ImVec2(200, 0), true);
    RenderEntityList();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("ExecutionView", ImVec2(0, 0), true);
    if (m_selectedEntity >= 0) {
        RenderExecutionGraph();
        ImGui::Separator();
        RenderExecutionLog();
        ImGui::Separator();
        RenderBlackboardValues();
    } else {
        ImGui::Text("Select an entity to view execution");
    }
    ImGui::EndChild();
}

void RuntimeDebugPanel::RenderEntityList()
{
    ImGui::Text("Entities with BT:");
    ImGui::Separator();
    
    // TODO: List entities with BehaviorTreeRuntime component
    ImGui::Text("(Entity list not yet implemented)");
}

void RuntimeDebugPanel::RenderExecutionGraph()
{
    ImGui::Text("Execution Graph");
    // TODO: Show graph with executing nodes highlighted
}

void RuntimeDebugPanel::RenderExecutionLog()
{
    ImGui::Text("Execution Log");
    // TODO: Show execution log entries
}

void RuntimeDebugPanel::RenderBlackboardValues()
{
    ImGui::Text("Blackboard Values");
    // TODO: Show current blackboard values
}

} // namespace AI
} // namespace Olympe
