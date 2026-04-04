#include "PropertyEditorPanel.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../../system/system_utils.h"

namespace Olympe
{
    PropertyEditorPanel::PropertyEditorPanel()
    {
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

        ImGui::BeginChild("##PropertyPanel", ImVec2(0, 0), true);

        if (m_selectedNodeId == InvalidNodeId)
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No node selected");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a node to edit properties");
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

        ImGui::EndChild();
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
        const ComponentNode* node = m_document->GetNode(m_selectedNodeId);
        if (!node) { return; }

        ImGui::TextUnformatted("Node Info");
        ImGui::Spacing();

        ImGui::BulletText("Node ID: %d", node->nodeId);
        ImGui::BulletText("Component Type: %s", node->componentType.c_str());
        ImGui::BulletText("Component Name: %s", node->componentName.c_str());
        ImGui::BulletText("Position: (%.1f, %.1f, %.1f)", node->position.x, node->position.y, node->position.z);
        ImGui::BulletText("Size: (%.1f, %.1f, %.1f)", node->size.x, node->size.y, node->size.z);
        ImGui::BulletText("Enabled: %s", node->enabled ? "Yes" : "No");
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

                // Simple text input for now
                static char buf[256];
                strncpy_s(buf, sizeof(buf), value.c_str(), _TRUNCATE);

                if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
                {
                    node->SetProperty(it->first, std::string(buf));
                    ApplyChanges();
                }

                ImGui::PopID();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No properties in this node");
        }
    }

} // namespace Olympe
