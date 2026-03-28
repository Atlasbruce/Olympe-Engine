// ============================================================================
// VisualScriptEditorPanel_Verification.cpp
// Phase 21-B: Graph Verification & Phase 24.3: Execution Testing
// ============================================================================
//
// This file contains graph validation, verification panel rendering, and
// Phase 24.3 execution test integration for the Blueprint Editor.
//
// Methods:
// - RenderValidationOverlay()          : Overlay validation checks
// - RunVerification()                  : Run full graph verification
// - RenderVerificationPanel()          : Render verification UI panel
// - RenderVerificationLogsPanel()      : Render verification logs output
// - RenderConditionEditor()            : Condition expression editor
// - RenderVariableSelector()           : Variable dropdown selector
// - RenderConstValueInput()            : Const value input field
// - RenderPinSelector()                : Pin selection dropdown
// - BuildConditionPreview()            : Build human-readable condition preview
//
// Integration Points:
// - VSGraphVerifier                   : Graph verification engine
// - ExecutionTestPanel                : Phase 24.3 execution testing UI
// - m_template                        : Graph to verify
// - m_verificationResult              : Cached verification state
// ============================================================================

#include "VisualScriptEditorPanel.h"
#include <sstream>

namespace Olympe {

// ============================================================================
// Validation Overlay - Basic Checks
// ============================================================================

void VisualScriptEditorPanel::RenderValidationOverlay()
{
    m_validationWarnings.clear();
    m_validationErrors.clear();

    // Check: every non-EntryPoint node should have at least one exec-in connection
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        const VSEditorNode& eNode = m_editorNodes[i];
        if (eNode.def.Type == TaskNodeType::EntryPoint)
            continue;

        bool hasExecIn = false;
        for (size_t c = 0; c < m_template.ExecConnections.size(); ++c)
        {
            if (m_template.ExecConnections[c].TargetNodeID == eNode.nodeID)
            {
                hasExecIn = true;
                break;
            }
        }
        if (!hasExecIn)
        {
            m_validationErrors.push_back(
                "Node " + std::to_string(eNode.nodeID) + " (" +
                eNode.def.NodeName + "): no exec-in connection");
        }

        // SubGraph path validation
        if (eNode.def.Type == TaskNodeType::SubGraph &&
            eNode.def.SubGraphPath.empty())
        {
            m_validationWarnings.push_back(
                "Node " + std::to_string(eNode.nodeID) +
                " (SubGraph): SubGraphPath is empty");
        }
    }
}

// ============================================================================
// Graph Verification - Phase 21-B
// ============================================================================

void VisualScriptEditorPanel::RunVerification()
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() called for graph '"
               << m_template.Name << "'\n";
    m_verificationResult = VSGraphVerifier::Verify(m_template);
    m_verificationDone   = true;

    // Phase 24.3 — Populate verification logs for display in the output panel
    m_verificationLogs.clear();
    for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
    {
        const VSVerificationIssue& issue = m_verificationResult.issues[i];
        std::string logEntry;

        // Format: "[SEVERITY] message (Node: nodeID)"
        if (issue.severity == VSVerificationSeverity::Error)
            logEntry = "[ERROR] ";
        else if (issue.severity == VSVerificationSeverity::Warning)
            logEntry = "[WARN] ";
        else
            logEntry = "[INFO] ";

        logEntry += issue.message;
        if (issue.nodeID >= 0)
            logEntry += " (Node: " + std::to_string(issue.nodeID) + ")";

        m_verificationLogs.push_back(logEntry);
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() done: "
               << m_verificationResult.issues.size() << " issue(s), "
               << "errors=" << (m_verificationResult.HasErrors()   ? "yes" : "no") << ", "
               << "warnings=" << (m_verificationResult.HasWarnings() ? "yes" : "no") << "\n";
}

// ============================================================================
// Verification Panel - UI Display
// ============================================================================

void VisualScriptEditorPanel::RenderVerificationPanel()
{
    ImGui::Separator();
    ImGui::TextDisabled("Graph Verification");

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("Click 'Verify' in toolbar to run verification.");
        return;
    }

    // Global status line
    if (m_verificationResult.HasErrors())
    {
        int errorCount = 0;
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            if (m_verificationResult.issues[i].severity == VSVerificationSeverity::Error)
                ++errorCount;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                           "Errors found: %d", errorCount);
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "OK — warnings present");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "OK — no issues");
    }

    if (m_verificationResult.issues.empty())
        return;

    // List issues grouped: Errors first, then Warnings, then Info
    const VSVerificationSeverity orderedSev[3] = {
        VSVerificationSeverity::Error,
        VSVerificationSeverity::Warning,
        VSVerificationSeverity::Info
    };

    for (int s = 0; s < 3; ++s)
    {
        VSVerificationSeverity sev = orderedSev[s];
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            const VSVerificationIssue& issue = m_verificationResult.issues[i];
            if (issue.severity != sev)
                continue;

            ImGui::PushID(static_cast<int>(i));

            if (sev == VSVerificationSeverity::Error)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[E]");
            else if (sev == VSVerificationSeverity::Warning)
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[W]");
            else
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[I]");

            ImGui::SameLine();
            ImGui::Text("%s: %s", issue.ruleID.c_str(), issue.message.c_str());

            if (issue.nodeID >= 0)
            {
                ImGui::SameLine();
                std::string btnLabel = "Go##go" + std::to_string(i);
                if (ImGui::SmallButton(btnLabel.c_str()))
                {
                    m_focusNodeID    = issue.nodeID;
                    m_selectedNodeID = issue.nodeID;
                }
            }

            ImGui::PopID();
        }
    }
}

// ============================================================================
// Verification Logs Panel - Phase 24.3
// ============================================================================

void VisualScriptEditorPanel::RenderVerificationLogsPanel()
{
    // Note: The header "Verification Output" is rendered by the container (BlueprintEditorGUI),
    // so we only render the content here (status + logs).

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("(Click 'Verify' button to run verification)");
        return;
    }

    // Display verification result summary
    ImGui::Spacing();

    // Debug: Show issue count
    ImGui::TextDisabled("Issues found: %zu", m_verificationResult.issues.size());

    // Status line with color coding
    if (m_verificationResult.HasErrors())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                          "[ERROR] Graph has %d error(s)", 
                          (int)m_verificationResult.issues.size());
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), 
                          "[WARNING] Graph is valid but has warnings");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), 
                          "[OK] Graph is valid - no issues found");
    }

    ImGui::Separator();

    // Display issues grouped by severity
    ImGui::BeginChild("VerificationLogsChild", ImVec2(0, 0), true);

    const VSVerificationSeverity orderedSev[3] = {
        VSVerificationSeverity::Error,
        VSVerificationSeverity::Warning,
        VSVerificationSeverity::Info
    };

    const char* sevLabels[3] = { "[ERROR]", "[WARN]", "[INFO]" };
    ImVec4      sevColors[3] = {
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),  // Error: red
        ImVec4(1.0f, 0.85f, 0.0f, 1.0f), // Warning: yellow
        ImVec4(0.5f, 0.8f, 1.0f, 1.0f)   // Info: light blue
    };

    for (int s = 0; s < 3; ++s)
    {
        VSVerificationSeverity sev = orderedSev[s];
        bool hasThisSeverity = false;

        // Count issues of this severity
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            if (m_verificationResult.issues[i].severity == sev)
            {
                hasThisSeverity = true;
                break;
            }
        }

        if (!hasThisSeverity)
            continue;

        // Display header for this severity level
        ImGui::TextColored(sevColors[s], "%s", sevLabels[s]);

        // Display all issues with this severity
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            const VSVerificationIssue& issue = m_verificationResult.issues[i];
            if (issue.severity != sev)
                continue;

            // Format message: "[SEVERITY] message (NodeID: xxx)"
            std::string message = issue.message;
            if (issue.nodeID >= 0)
            {
                message += " (Node: " + std::to_string(issue.nodeID) + ")";
            }

            ImGui::BulletText("%s", message.c_str());
        }

        ImGui::Spacing();
    }

    ImGui::EndChild();
}

// ============================================================================
// Condition Editor - Phase 23-B.4
// ============================================================================

void VisualScriptEditorPanel::RenderConditionEditor(
    Condition& condition,
    int conditionIndex,
    const std::vector<BlackboardEntry>& allVars,
    const std::vector<std::string>& availablePins)
{
    ImGui::PushID(conditionIndex);
    ImGui::Separator();
    ImGui::Text("Condition #%d", conditionIndex + 1);

    // -- LEFT SIDE --
    ImGui::Text("Left:");
    ImGui::SameLine();

    const bool isLeftPin   = (condition.leftMode == "Pin");
    const bool isLeftVar   = (condition.leftMode == "Variable");
    const bool isLeftConst = (condition.leftMode == "Const");

    if (ImGui::Button(isLeftPin ? "[PIN]" : "Pin", ImVec2(55, 0)))
    {
        condition.leftMode = "Pin";
        condition.leftPin  = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isLeftVar ? "[VAR]" : "Var", ImVec2(55, 0)))
    {
        condition.leftMode     = "Variable";
        condition.leftVariable = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isLeftConst ? "[CST]" : "Const", ImVec2(55, 0)))
    {
        condition.leftMode = "Const";
        m_dirty = true;
    }

    ImGui::Indent();
    if (condition.leftMode == "Pin")
        RenderPinSelector(condition.leftPin, availablePins, "##leftpin");
    else if (condition.leftMode == "Variable")
        RenderVariableSelector(condition.leftVariable, allVars,
                               condition.compareType, "##leftvar");
    else
        RenderConstValueInput(condition.leftConstValue,
                              condition.compareType, "##leftconst");
    ImGui::Unindent();

    // -- OPERATOR --
    ImGui::Text("Op:");
    ImGui::SameLine();
    const char* operators[] = { "==", "!=", "<", ">", "<=", ">=" };
    int opIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (condition.operatorStr == operators[i])
        {
            opIdx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(70.0f);
    if (ImGui::Combo("##op", &opIdx, operators, 6))
    {
        condition.operatorStr = operators[opIdx];
        m_dirty = true;
    }

    // -- RIGHT SIDE --
    ImGui::Text("Right:");
    ImGui::SameLine();

    const bool isRightPin   = (condition.rightMode == "Pin");
    const bool isRightVar   = (condition.rightMode == "Variable");
    const bool isRightConst = (condition.rightMode == "Const");

    if (ImGui::Button(isRightPin ? "[PIN]##r" : "Pin##r", ImVec2(55, 0)))
    {
        condition.rightMode = "Pin";
        condition.rightPin  = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isRightVar ? "[VAR]##r" : "Var##r", ImVec2(55, 0)))
    {
        condition.rightMode     = "Variable";
        condition.rightVariable = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isRightConst ? "[CST]##r" : "Const##r", ImVec2(55, 0)))
    {
        condition.rightMode = "Const";
        m_dirty = true;
    }

    ImGui::Indent();
    if (condition.rightMode == "Pin")
        RenderPinSelector(condition.rightPin, availablePins, "##rightpin");
    else if (condition.rightMode == "Variable")
        RenderVariableSelector(condition.rightVariable, allVars,
                               condition.compareType, "##rightvar");
    else
        RenderConstValueInput(condition.rightConstValue,
                              condition.compareType, "##rightconst");
    ImGui::Unindent();

    // -- TYPE HINT --
    ImGui::Text("Type:");
    ImGui::SameLine();
    const char* types[] = { "None", "Bool", "Int", "Float", "String", "Vector" };
    const VariableType typeValues[] = {
        VariableType::None, VariableType::Bool, VariableType::Int,
        VariableType::Float, VariableType::String, VariableType::Vector
    };
    int typeIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (condition.compareType == typeValues[i])
        {
            typeIdx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::Combo("##cmptype", &typeIdx, types, 6))
    {
        condition.compareType = typeValues[typeIdx];
        m_dirty = true;
    }

    // -- PREVIEW --
    const std::string preview = BuildConditionPreview(condition);
    ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f),
                       "Preview: %s", preview.c_str());

    ImGui::PopID();
}

// ============================================================================
// Variable Selector - Dropdown UI
// ============================================================================

void VisualScriptEditorPanel::RenderVariableSelector(
    std::string& selectedVar,
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType,
    const char* label)
{
    // Filter by type (if a type is specified)
    std::vector<std::string> names;
    for (size_t i = 0; i < allVars.size(); ++i)
    {
        if (expectedType == VariableType::None || allVars[i].Type == expectedType)
        {
            if (!allVars[i].Key.empty())
                names.push_back(allVars[i].Key);
        }
    }

    if (names.empty())
    {
        ImGui::TextDisabled("(no variables)");
        return;
    }

    // BUG-029 Fix: auto-initialise to the first available variable when the
    // selection is empty (e.g. right after switching to Variable mode).
    // Without this the combo visually shows the first item but selectedVar
    // remains "" so BuildConditionPreview displays "[Var: ?]".
    if (selectedVar.empty())
    {
        selectedVar = names[0];
        m_dirty = true;
    }

    int selected = 0;
    for (int i = 0; i < static_cast<int>(names.size()); ++i)
    {
        if (names[static_cast<size_t>(i)] == selectedVar)
        {
            selected = i;
            break;
        }
    }

    std::vector<const char*> cstrs;
    cstrs.reserve(names.size());
    for (size_t i = 0; i < names.size(); ++i)
        cstrs.push_back(names[i].c_str());

    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo(label, &selected, cstrs.data(), static_cast<int>(cstrs.size())))
    {
        selectedVar = names[static_cast<size_t>(selected)];
        m_dirty = true;
    }
}

}  // namespace Olympe
