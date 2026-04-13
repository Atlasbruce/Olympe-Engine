/**
 * @file BTNodePropertyPanel.cpp
 * @brief Property editor panel implementation for BehaviorTree nodes
 * @author Olympe Engine
 * @date 2026-04-08
 */

#include "BTNodePropertyPanel.h"
#include "BTNodeGraphManager.h"
#include "../third_party/imgui/imgui.h"
#include "../DataManager.h"
#include "../AI/BehaviorTree.h"
#include "../system/system_utils.h"
#include "../Editor/Modals/FilePickerModal.h"
#include "Framework/CanvasModalRenderer.h"
#include <cstring>

namespace Olympe {

BTNodePropertyPanel::BTNodePropertyPanel()
    : m_activeGraphId(-1)
    , m_selectedNodeId(-1)
{
    std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
    std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
}

void BTNodePropertyPanel::Initialize()
{
    // Nothing to initialize currently
}

void BTNodePropertyPanel::SetSelectedNode(int graphId, int nodeId)
{
    m_activeGraphId = graphId;
    m_selectedNodeId = nodeId;

    if (graphId >= 0 && nodeId >= 0)
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(graphId);
        if (graph)
        {
            const GraphNode* node = graph->GetNode(nodeId);
            if (node)
            {
                strncpy_s(m_nodeNameBuffer, sizeof(m_nodeNameBuffer), node->name.c_str(), _TRUNCATE);
            }
        }
    }
}

void BTNodePropertyPanel::ClearSelection()
{
    m_activeGraphId = -1;
    m_selectedNodeId = -1;
    std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
    std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
}

void BTNodePropertyPanel::Render()
{
    if (m_activeGraphId < 0 || m_selectedNodeId < 0)
    {
        ImGui::TextDisabled("No node selected");
        return;
    }

    NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_activeGraphId);
    if (!graph)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Graph not found");
        ClearSelection();
        return;
    }

    GraphNode* node = graph->GetNode(m_selectedNodeId);
    if (!node)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Node not found");
        ClearSelection();
        return;
    }

    ImGui::Text("Node ID: %d", node->id);
    ImGui::Separator();

    RenderNodeBasicInfo(node);
    ImGui::Separator();

    // Phase 39c: Check if this is a SubGraph node
    if (node->type == NodeType::BT_SubGraph)
    {
        RenderSubGraphControls(node);
        ImGui::Separator();
    }

    RenderNodeParameters(node);
}

void BTNodePropertyPanel::RenderNodeBasicInfo(const GraphNode* node)
{
    ImGui::Text("Node Type: %s", NodeTypeToString(node->type));
    
    // Node name editing
    if (ImGui::InputText("##NodeName", m_nodeNameBuffer, sizeof(m_nodeNameBuffer)))
    {
        // Will apply on next ApplyNodeChanges call
    }
    
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_activeGraphId);
        if (graph)
        {
            GraphNode* mutableNode = graph->GetNode(m_selectedNodeId);
            if (mutableNode)
            {
                mutableNode->name = m_nodeNameBuffer;
                SYSTEM_LOG << "[BTNodePropertyPanel] Updated node name to: " << m_nodeNameBuffer << "\n";
            }
        }
    }

    // Position info (read-only for now)
    ImGui::Text("Position: (%.1f, %.1f)", node->posX, node->posY);
}

void BTNodePropertyPanel::RenderNodeParameters(const GraphNode* node)
{
    ImGui::PushID(node->id);

    // For BT nodes, parameters are stored in a generic map
    if (node->parameters.empty())
    {
        ImGui::TextDisabled("No parameters");
        ImGui::PopID();
        return;
    }

    ImGui::TextUnformatted("Parameters:");
    for (const auto& param : node->parameters)
    {
        ImGui::BulletText("%s: %s", param.first.c_str(), param.second.c_str());
    }

    ImGui::PopID();
}

void BTNodePropertyPanel::ApplyNodeChanges(GraphNode* node)
{
    if (!node)
        return;

    node->name = m_nodeNameBuffer;
    SYSTEM_LOG << "[BTNodePropertyPanel] Applied changes to node: " << node->id << "\n";
}

void BTNodePropertyPanel::RenderSubGraphControls(GraphNode* node)
{
    if (!node)
        return;

    ImGui::TextUnformatted("SubGraph Configuration:");
    ImGui::Indent();

    // Display SubGraph path with file browser button
    auto pathIt = node->parameters.find("subgraphPath");
    std::string currentPath = (pathIt != node->parameters.end()) ? pathIt->second : "";

    if (!currentPath.empty())
    {
        ImGui::TextWrapped("Path: %s", currentPath.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓");
    }
    else
    {
        ImGui::TextDisabled("(No SubGraph path specified)");
    }

    // File browser button - Phase 42: Using unified CanvasModalRenderer
    if (ImGui::Button("Browse...##subgraphPath", ImVec2(-1, 0)))
    {
        // Extract directory only from the full path (if it's a file path)
        std::string directory = "Blueprints/";
        if (!currentPath.empty())
        {
            size_t lastSlash = currentPath.find_last_of("/\\");
            if (lastSlash != std::string::npos)
            {
                directory = currentPath.substr(0, lastSlash + 1);
            }
            else
            {
                // No slash found - treat as directory
                directory = currentPath;
            }
        }
        CanvasModalRenderer::Get().OpenSubGraphFilePickerModal(directory);
    }

    // Phase 42: Check if modal has closed with a selection
    // This pattern detects when modal was open last frame but is now closed
    static bool wasFilePickerOpen = false;
    bool isFilePickerOpen = CanvasModalRenderer::Get().IsSubGraphModalOpen();

    if (wasFilePickerOpen && !isFilePickerOpen)
    {
        // Modal just closed - check if user selected a file
        std::string selectedFile = CanvasModalRenderer::Get().GetSelectedSubGraphFile();
        if (!selectedFile.empty())
        {
            // Update the node's subgraphPath parameter with the selected file
            node->parameters["subgraphPath"] = selectedFile;
            SYSTEM_LOG << "[BTNodePropertyPanel] Updated SubGraph path: " << selectedFile << "\n";
        }
    }
    wasFilePickerOpen = isFilePickerOpen;

    // Phase 39c Step 6: Validation error display
    ImGui::Spacing();
    std::vector<std::string> validationErrors = BehaviorTreeManager::Get().GetValidationErrors(m_activeGraphId);

    if (!validationErrors.empty())
    {
        // Display error section with red highlighting
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); // Red
        ImGui::TextWrapped("❌ Validation Errors:");
        ImGui::PopStyleColor();

        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); // Lighter red
        for (const auto& error : validationErrors)
        {
            ImGui::BulletText("%s", error.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::Unindent();

        // Add visual separator
        ImGui::Spacing();
        ImGui::Separator();
    }
    else if (!currentPath.empty())
    {
        // Show success message only if path is specified
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f)); // Green
        ImGui::TextWrapped("✓ No validation errors");
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    ImGui::Separator();

    // Display input parameters
    auto inputCountIt = node->parameters.find("inputParamCount");
    int inputCount = 0;
    if (inputCountIt != node->parameters.end())
    {
        try
        {
            inputCount = std::stoi(inputCountIt->second);
        }
        catch (const std::exception&)
        {
            inputCount = 0;
        }
    }

    if (inputCount > 0)
    {
        ImGui::TextUnformatted("Input Parameters:");
        ImGui::Indent();
        for (int i = 0; i < inputCount; ++i)
        {
            char keyBuf[64];
            snprintf(keyBuf, sizeof(keyBuf), "inputParam_%d", i);
            auto it = node->parameters.find(keyBuf);
            if (it != node->parameters.end())
            {
                ImGui::BulletText("%s", it->second.c_str());
            }
        }
        ImGui::Unindent();
    }
    else
    {
        ImGui::TextDisabled("(No input parameters)");
    }

    ImGui::Separator();

    // Display output parameters
    auto outputCountIt = node->parameters.find("outputParamCount");
    int outputCount = 0;
    if (outputCountIt != node->parameters.end())
    {
        try
        {
            outputCount = std::stoi(outputCountIt->second);
        }
        catch (const std::exception&)
        {
            outputCount = 0;
        }
    }

    if (outputCount > 0)
    {
        ImGui::TextUnformatted("Output Parameters:");
        ImGui::Indent();
        for (int i = 0; i < outputCount; ++i)
        {
            char keyBuf[64];
            snprintf(keyBuf, sizeof(keyBuf), "outputParam_%d", i);
            auto it = node->parameters.find(keyBuf);
            if (it != node->parameters.end())
            {
                ImGui::BulletText("%s", it->second.c_str());
            }
        }
        ImGui::Unindent();
    }
    else
    {
        ImGui::TextDisabled("(No output parameters)");
    }

    ImGui::Unindent();
    ImGui::Separator();

    // Phase 39c Step 4: Parameter binding editor
    RenderSubGraphBindingEditor(node);
}

void BTNodePropertyPanel::RenderSubGraphBindingEditor(GraphNode* node)
{
    if (!node)
        return;

    ImGui::TextUnformatted("Parameter Bindings:");
    ImGui::Indent();

    // ===== INPUT BINDINGS =====
    ImGui::TextUnformatted("Input Bindings (Parent → Child):");
    ImGui::Indent();

    // Extract input binding count
    auto inputBindingCountIt = node->parameters.find("inputBindingCount");
    int inputBindingCount = 0;
    if (inputBindingCountIt != node->parameters.end())
    {
        try
        {
            inputBindingCount = std::stoi(inputBindingCountIt->second);
        }
        catch (const std::exception&)
        {
            inputBindingCount = 0;
        }
    }

    // Display existing input bindings in table format
    bool hasInputBindings = inputBindingCount > 0;
    if (hasInputBindings)
    {
        ImGui::BeginTable("##inputBindings", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Child Param", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Parent Param", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < inputBindingCount; ++i)
        {
            ImGui::TableNextRow();

            // Child param name (column 0)
            char childKeyBuf[64];
            snprintf(childKeyBuf, sizeof(childKeyBuf), "inputBinding_%d_child", i);
            auto childIt = node->parameters.find(childKeyBuf);
            std::string childParam = (childIt != node->parameters.end()) ? childIt->second : "";

            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("%s", childParam.c_str());

            // Parent param name (column 1) - editable
            char parentKeyBuf[64];
            snprintf(parentKeyBuf, sizeof(parentKeyBuf), "inputBinding_%d_parent", i);
            auto parentIt = node->parameters.find(parentKeyBuf);
            std::string currentParentParam = (parentIt != node->parameters.end()) ? parentIt->second : "";

            ImGui::TableSetColumnIndex(1);
            char parentBuf[256];
            strncpy_s(parentBuf, sizeof(parentBuf), currentParentParam.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText(("##inputBinding_" + std::to_string(i) + "_parent").c_str(), parentBuf, sizeof(parentBuf)))
            {
                node->parameters[parentKeyBuf] = parentBuf;
            }

            // Delete button (column 2)
            ImGui::TableSetColumnIndex(2);
            if (ImGui::Button(("Del##input_" + std::to_string(i)).c_str(), ImVec2(-1, 0)))
            {
                // Mark for deletion by shifting remaining bindings
                for (int j = i; j < inputBindingCount - 1; ++j)
                {
                    char srcChild[64], srcParent[64];
                    char dstChild[64], dstParent[64];
                    snprintf(srcChild, sizeof(srcChild), "inputBinding_%d_child", j + 1);
                    snprintf(srcParent, sizeof(srcParent), "inputBinding_%d_parent", j + 1);
                    snprintf(dstChild, sizeof(dstChild), "inputBinding_%d_child", j);
                    snprintf(dstParent, sizeof(dstParent), "inputBinding_%d_parent", j);

                    auto srcChildIt = node->parameters.find(srcChild);
                    auto srcParentIt = node->parameters.find(srcParent);

                    if (srcChildIt != node->parameters.end())
                        node->parameters[dstChild] = srcChildIt->second;
                    if (srcParentIt != node->parameters.end())
                        node->parameters[dstParent] = srcParentIt->second;
                }
                inputBindingCount--;
                node->parameters["inputBindingCount"] = std::to_string(inputBindingCount);

                // Clean up last binding
                char lastChild[64], lastParent[64];
                snprintf(lastChild, sizeof(lastChild), "inputBinding_%d_child", inputBindingCount);
                snprintf(lastParent, sizeof(lastParent), "inputBinding_%d_parent", inputBindingCount);
                node->parameters.erase(lastChild);
                node->parameters.erase(lastParent);
            }
        }
        ImGui::EndTable();
    }
    else
    {
        ImGui::TextDisabled("(No input bindings)");
    }

    // Add input binding button
    if (ImGui::Button("+ Add Input Binding##input", ImVec2(-1, 0)))
    {
        char childKeyBuf[64], parentKeyBuf[64];
        snprintf(childKeyBuf, sizeof(childKeyBuf), "inputBinding_%d_child", inputBindingCount);
        snprintf(parentKeyBuf, sizeof(parentKeyBuf), "inputBinding_%d_parent", inputBindingCount);
        node->parameters[childKeyBuf] = "newParam";
        node->parameters[parentKeyBuf] = "";
        inputBindingCount++;
        node->parameters["inputBindingCount"] = std::to_string(inputBindingCount);
    }

    ImGui::Unindent();
    ImGui::Separator();

    // ===== OUTPUT BINDINGS =====
    ImGui::TextUnformatted("Output Bindings (Child → Parent):");
    ImGui::Indent();

    // Extract output binding count
    auto outputBindingCountIt = node->parameters.find("outputBindingCount");
    int outputBindingCount = 0;
    if (outputBindingCountIt != node->parameters.end())
    {
        try
        {
            outputBindingCount = std::stoi(outputBindingCountIt->second);
        }
        catch (const std::exception&)
        {
            outputBindingCount = 0;
        }
    }

    // Display existing output bindings in table format
    bool hasOutputBindings = outputBindingCount > 0;
    if (hasOutputBindings)
    {
        ImGui::BeginTable("##outputBindings", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Parent Param", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Child Param", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < outputBindingCount; ++i)
        {
            ImGui::TableNextRow();

            // Parent param name (column 0) - editable
            char parentKeyBuf[64];
            snprintf(parentKeyBuf, sizeof(parentKeyBuf), "outputBinding_%d_parent", i);
            auto parentIt = node->parameters.find(parentKeyBuf);
            std::string currentParentParam = (parentIt != node->parameters.end()) ? parentIt->second : "";

            ImGui::TableSetColumnIndex(0);
            char parentBuf[256];
            strncpy_s(parentBuf, sizeof(parentBuf), currentParentParam.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText(("##outputBinding_" + std::to_string(i) + "_parent").c_str(), parentBuf, sizeof(parentBuf)))
            {
                node->parameters[parentKeyBuf] = parentBuf;
            }

            // Child param name (column 1)
            char childKeyBuf[64];
            snprintf(childKeyBuf, sizeof(childKeyBuf), "outputBinding_%d_child", i);
            auto childIt = node->parameters.find(childKeyBuf);
            std::string childParam = (childIt != node->parameters.end()) ? childIt->second : "";

            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled("%s", childParam.c_str());

            // Delete button (column 2)
            ImGui::TableSetColumnIndex(2);
            if (ImGui::Button(("Del##output_" + std::to_string(i)).c_str(), ImVec2(-1, 0)))
            {
                // Mark for deletion by shifting remaining bindings
                for (int j = i; j < outputBindingCount - 1; ++j)
                {
                    char srcParent[64], srcChild[64];
                    char dstParent[64], dstChild[64];
                    snprintf(srcParent, sizeof(srcParent), "outputBinding_%d_parent", j + 1);
                    snprintf(srcChild, sizeof(srcChild), "outputBinding_%d_child", j + 1);
                    snprintf(dstParent, sizeof(dstParent), "outputBinding_%d_parent", j);
                    snprintf(dstChild, sizeof(dstChild), "outputBinding_%d_child", j);

                    auto srcParentIt = node->parameters.find(srcParent);
                    auto srcChildIt = node->parameters.find(srcChild);

                    if (srcParentIt != node->parameters.end())
                        node->parameters[dstParent] = srcParentIt->second;
                    if (srcChildIt != node->parameters.end())
                        node->parameters[dstChild] = srcChildIt->second;
                }
                outputBindingCount--;
                node->parameters["outputBindingCount"] = std::to_string(outputBindingCount);

                // Clean up last binding
                char lastParent[64], lastChild[64];
                snprintf(lastParent, sizeof(lastParent), "outputBinding_%d_parent", outputBindingCount);
                snprintf(lastChild, sizeof(lastChild), "outputBinding_%d_child", outputBindingCount);
                node->parameters.erase(lastParent);
                node->parameters.erase(lastChild);
            }
        }
        ImGui::EndTable();
    }
    else
    {
        ImGui::TextDisabled("(No output bindings)");
    }

    // Add output binding button
    if (ImGui::Button("+ Add Output Binding##output", ImVec2(-1, 0)))
    {
        char parentKeyBuf[64], childKeyBuf[64];
        snprintf(parentKeyBuf, sizeof(parentKeyBuf), "outputBinding_%d_parent", outputBindingCount);
        snprintf(childKeyBuf, sizeof(childKeyBuf), "outputBinding_%d_child", outputBindingCount);
        node->parameters[parentKeyBuf] = "";
        node->parameters[childKeyBuf] = "newParam";
        outputBindingCount++;
        node->parameters["outputBindingCount"] = std::to_string(outputBindingCount);
    }

    ImGui::Unindent();
}

} // namespace Olympe
