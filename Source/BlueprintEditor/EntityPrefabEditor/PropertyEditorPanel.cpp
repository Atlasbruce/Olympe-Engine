#include "PropertyEditorPanel.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../../system/system_utils.h"
#include <unordered_map>

namespace Olympe
{
    PropertyEditorPanel::PropertyEditorPanel()
    {
        m_behaviorTreeModal = std::make_unique<BehaviorTreeFilePickerModal>();
    }

    PropertyEditorPanel::~PropertyEditorPanel() { }

    void PropertyEditorPanel::Initialize() 
    { 
        SYSTEM_LOG << "[PropertyEditorPanel] Initializing...\n";
    }

    void PropertyEditorPanel::Render(EntityPrefabGraphDocument* document)
    {
        if (!document) { return; }
        m_document = document;

        if (m_selectedNodeId == InvalidNodeId)
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No node selected");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a node to edit component properties");
        }
        else
        {
            const ComponentNode* node = document->GetNode(m_selectedNodeId);
            if (node)
            {
                RenderNodeInfo();
                ImGui::Separator();
                RenderNodeProperties();
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Selected node not found!");
                m_selectedNodeId = InvalidNodeId;
            }
        }
    }

    void PropertyEditorPanel::SetSelectedNode(NodeId nodeId)
    {
        m_selectedNodeId = nodeId;
        // SYSTEM_LOG line removed to eliminate spam - logs occur every frame during rendering
    }

    NodeId PropertyEditorPanel::GetSelectedNode() const
    {
        return m_selectedNodeId;
    }

    void PropertyEditorPanel::ClearSelection()
    {
        m_selectedNodeId = InvalidNodeId;
    }

    bool PropertyEditorPanel::HasSelectedNode() const
    {
        return m_selectedNodeId != InvalidNodeId;
    }

    void PropertyEditorPanel::ApplyChanges()
    {
        if (m_document && m_selectedNodeId != InvalidNodeId)
        {
            m_document->SetDirty(true);
        }
    }

    void PropertyEditorPanel::RenderNodeInfo()
    {
        // This method is kept for backward compatibility but now only shows
        // component name since node metadata is now in NodePropertiesPanel
        const ComponentNode* node = m_document->GetNode(m_selectedNodeId);
        if (!node) { return; }

        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "Component:");
        ImGui::SameLine();
        ImGui::TextUnformatted(node->componentType.c_str());
    }

    void PropertyEditorPanel::RenderNodeProperties()
    {
        ComponentNode* node = m_document->GetNode(m_selectedNodeId);
        if (!node) { return; }

        ImGui::TextUnformatted("Properties");
        ImGui::Spacing();

        if (node->properties.size() > 0)
        {
            for (auto it = node->properties.begin(); it != node->properties.end(); ++it)
            {
                ImGui::PushID(it->first.c_str());

                std::string label = it->first;
                std::string value = it->second;

                // Special handling for BehaviorTree path field
                if (label == "behaviorType")
                {
                    // Text input for BehaviorTree path (read-only display)
                    static std::unordered_map<int, std::string> btPathBufferCache;
                    if (btPathBufferCache.find(m_selectedNodeId) == btPathBufferCache.end())
                    {
                        btPathBufferCache[m_selectedNodeId] = value;
                    }
                    std::string& cachedPath = btPathBufferCache[m_selectedNodeId];
                    char pathBuffer[512] = "";
                    strncpy_s(pathBuffer, sizeof(pathBuffer), cachedPath.c_str(), _TRUNCATE);

                    // Render input field with Browse button
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "BehaviorTree Path");
                    ImGui::SetNextItemWidth(-80.0f);
                    if (ImGui::InputText("##behaviorTreePath_input", pathBuffer, sizeof(pathBuffer)))
                    {
                        std::string newPath(pathBuffer);
                        node->SetProperty(it->first, newPath);
                        cachedPath = newPath;
                        ApplyChanges();
                    }

                    // Browse button to open file picker
                    ImGui::SameLine();
                    if (ImGui::Button("Browse##bt_browse", ImVec2(75, 0)))
                    {
                        m_behaviorTreeModal->Open("./Gamedata");
                    }

                    // Render modal every frame
                    if (m_behaviorTreeModal)
                    {
                        m_behaviorTreeModal->Render();

                        // Handle file selection from modal
                        if (m_behaviorTreeModal->IsConfirmed())
                        {
                            std::string selectedFile = m_behaviorTreeModal->GetSelectedFile();

                            // Extract relative path by removing "Gamedata/BehaviorTree/" prefix
                            std::string relativePath = selectedFile;
                            const std::string btPrefix = "Gamedata/";
                            const std::string btPrefixWin = "Gamedata\\";

                            if (relativePath.find(btPrefix) == 0)
                            {
                                relativePath = relativePath.substr(btPrefix.length());
                            }
                            else if (relativePath.find(btPrefixWin) == 0)
                            {
                                relativePath = relativePath.substr(btPrefixWin.length());
                            }

                            // Normalize path: forward slashes to backslashes for consistency
                            for (char& c : relativePath)
                            {
                                if (c == '/')
                                    c = '\\';
                            }

                            // Update node property and cache
                            node->SetProperty(it->first, relativePath);
                            cachedPath = relativePath;
                            ApplyChanges();

                            SYSTEM_LOG << "[PropertyEditorPanel] Selected BehaviorTree: " << relativePath << "\n";
                        }
                    }
                }
                else
                {
                    // Standard text input for other properties
                    static char buf[256];
                    strncpy_s(buf, sizeof(buf), value.c_str(), _TRUNCATE);

                    if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
                    {
                        node->SetProperty(it->first, std::string(buf));
                        ApplyChanges();
                    }
                }

                ImGui::PopID();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No properties defined for this component");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), "(Check ComponentsParameters.json)");
        }
    }

} // namespace Olympe
