/**
 * @file AIEditorPanels.h
 * @brief Specialized panels for AI Editor
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * AI-specific panels:
 * - BlackboardInspectorPanel: Edit blackboard variables
 * - SensesDebugPanel: Debug AI senses
 * - RuntimeDebugPanel: Runtime execution visualization
 */

#pragma once

#include "../../NodeGraphCore/GraphDocument.h"
#include <string>

namespace Olympe {
namespace AI {

/**
 * @class BlackboardInspectorPanel
 * @brief Panel for editing blackboard variables
 */
class BlackboardInspectorPanel {
public:
    BlackboardInspectorPanel();
    ~BlackboardInspectorPanel() = default;
    
    /**
     * @brief Render the panel
     * @param activeGraph Active graph document
     */
    void Render(NodeGraph::GraphDocument* activeGraph);
    
private:
    void RenderVariableList(NodeGraph::GraphDocument* doc);
    void RenderAddVariableDialog();
    void RenderVariableEditor(const std::string& varName);
    
    bool m_showAddDialog;
    char m_newVarName[256];
};

/**
 * @class SensesDebugPanel
 * @brief Panel for debugging AI senses
 */
class SensesDebugPanel {
public:
    SensesDebugPanel();
    ~SensesDebugPanel() = default;
    
    /**
     * @brief Render the panel
     */
    void Render();
    
private:
    void RenderEntitySensesList();
    void RenderSenseDetails();
    void RenderVisionCone();
    void RenderDetectedTargets();
};

/**
 * @class RuntimeDebugPanel
 * @brief Panel for runtime execution debugging
 */
class RuntimeDebugPanel {
public:
    RuntimeDebugPanel();
    ~RuntimeDebugPanel() = default;
    
    /**
     * @brief Render the panel
     */
    void Render();
    
private:
    void RenderEntityList();
    void RenderExecutionGraph();
    void RenderExecutionLog();
    void RenderBlackboardValues();
    
    int m_selectedEntity;
};

} // namespace AI
} // namespace Olympe
