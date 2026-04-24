#include "PlaceholderPropertyEditorPanel.h"
#include "PlaceholderGraphDocument.h"
#include "../../third_party/imgui/imgui.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Olympe {

PlaceholderPropertyEditorPanel::PlaceholderPropertyEditorPanel()
    : m_document(nullptr), m_selectionCount(0)
{
}

PlaceholderPropertyEditorPanel::~PlaceholderPropertyEditorPanel()
{
}

void PlaceholderPropertyEditorPanel::Initialize(PlaceholderGraphDocument* document)
{
    m_document = document;
}

void PlaceholderPropertyEditorPanel::Render(const std::vector<int>& selectedNodeIds)
{
    m_selectedNodeIds = selectedNodeIds;
    m_selectionCount = static_cast<int>(selectedNodeIds.size());

    if (m_selectionCount == 0)
    {
        RenderEmptyState();
        return;
    }

    // Collect values from first selected node
    CollectPropertyValues();

    // Detect differences if multi-select
    if (m_selectionCount > 1)
    {
        DetectPropertyDifferences();
    }

    RenderNodeInfo();
    ImGui::Separator();
    RenderNodeProperties();

    ImGui::Spacing();
    ImGui::Separator();

    // Apply/Reset buttons
    bool hasChanges = HasChanges();
    if (ImGui::Button("Apply", ImVec2(120, 0)) && hasChanges)
    {
        ApplyChanges();
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(120, 0)))
    {
        ResetChanges();
    }
}

void PlaceholderPropertyEditorPanel::ApplyChanges()
{
    if (!m_document || m_selectedNodeIds.empty())
        return;

    // Apply changes to all selected nodes
    for (int nodeId : m_selectedNodeIds)
    {
        PlaceholderNode* node = m_document->GetNode(nodeId);
        if (!node)
            continue;

        // Apply Title
        if (m_titleEdit.isEditing)
        {
            m_document->SetNodeTitle(nodeId, m_titleEdit.GetValue());
        }

        // Apply Filepath
        if (m_filepathEdit.isEditing)
        {
            m_document->SetNodeFilepath(nodeId, m_filepathEdit.GetValue());
        }

        // Apply Position X
        if (m_posXEdit.isEditing)
        {
            float x = ParseFloat(m_posXEdit.GetValue(), node->posX);
            m_document->SetNodePosition(nodeId, x, node->posY);
        }

        // Apply Position Y
        if (m_posYEdit.isEditing)
        {
            float y = ParseFloat(m_posYEdit.GetValue(), node->posY);
            m_document->SetNodePosition(nodeId, node->posX, y);
        }

        // Apply Width
        if (m_widthEdit.isEditing)
        {
            node->width = static_cast<int>(ParseFloat(m_widthEdit.GetValue(), static_cast<float>(node->width)));
        }

        // Apply Height
        if (m_heightEdit.isEditing)
        {
            node->height = static_cast<int>(ParseFloat(m_heightEdit.GetValue(), static_cast<float>(node->height)));
        }

        // Apply Enabled
        if (m_enabledEdit.isEditing)
        {
            bool enabled = m_enabledEdit.GetValue() != "0";
            m_document->SetNodeEnabled(nodeId, enabled);
        }
    }

    m_document->SetDirty(true);
    ResetChanges();
}

void PlaceholderPropertyEditorPanel::ResetChanges()
{
    m_titleEdit.isEditing = false;
    m_filepathEdit.isEditing = false;
    m_posXEdit.isEditing = false;
    m_posYEdit.isEditing = false;
    m_widthEdit.isEditing = false;
    m_heightEdit.isEditing = false;
    m_enabledEdit.isEditing = false;
}

void PlaceholderPropertyEditorPanel::Clear()
{
    m_selectedNodeIds.clear();
    m_selectionCount = 0;
    ResetChanges();
}

bool PlaceholderPropertyEditorPanel::HasChanges() const
{
    return m_titleEdit.isEditing || m_filepathEdit.isEditing ||
           m_posXEdit.isEditing || m_posYEdit.isEditing ||
           m_widthEdit.isEditing || m_heightEdit.isEditing ||
           m_enabledEdit.isEditing;
}

void PlaceholderPropertyEditorPanel::RenderEmptyState()
{
    ImGui::TextDisabled("(No nodes selected)");
}

void PlaceholderPropertyEditorPanel::RenderNodeInfo()
{
    ImGui::Text("Selected: %d node(s)", m_selectionCount);
    if (m_selectionCount == 1)
    {
        ImGui::Text("Node ID: %d", m_selectedNodeIds[0]);
    }
}

void PlaceholderPropertyEditorPanel::RenderNodeProperties()
{
    ImGui::BeginGroup();

    RenderPropertyField("Title", m_titleEdit);
    RenderPropertyField("Filepath", m_filepathEdit);
    RenderPropertyField("Position X", m_posXEdit);
    RenderPropertyField("Position Y", m_posYEdit);
    RenderPropertyField("Width", m_widthEdit);
    RenderPropertyField("Height", m_heightEdit);
    RenderBoolProperty("Enabled", m_enabledEdit);

    ImGui::EndGroup();
}

void PlaceholderPropertyEditorPanel::RenderPropertyField(const std::string& label, PropertyEditState& state)
{
    // Highlight differences in multi-select with orange background
    if (m_selectionCount > 1 && state.isDifferent)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.6f, 0.4f, 0.0f, 0.5f));
    }

    ImGui::InputText(label.c_str(), state.buffer, PropertyEditState::BUFFER_SIZE);
    state.isEditing = ImGui::IsItemEdited();

    if (m_selectionCount > 1 && state.isDifferent)
    {
        ImGui::PopStyleColor();
        ImGui::TextDisabled("(multiple values)");
    }
}

void PlaceholderPropertyEditorPanel::RenderBoolProperty(const std::string& label, PropertyEditState& state)
{
    bool boolValue = state.GetValue() != "0";
    ImGui::Checkbox(label.c_str(), &boolValue);
    state.isEditing = ImGui::IsItemEdited();
    state.SetValue(boolValue ? "1" : "0");
}

void PlaceholderPropertyEditorPanel::CollectPropertyValues()
{
    if (m_selectedNodeIds.empty() || !m_document)
        return;

    PlaceholderNode* firstNode = m_document->GetNode(m_selectedNodeIds[0]);
    if (!firstNode)
        return;

    m_titleEdit.SetValue(firstNode->title);
    m_filepathEdit.SetValue(firstNode->filepath);
    m_posXEdit.SetValue(FormatFloat(firstNode->posX));
    m_posYEdit.SetValue(FormatFloat(firstNode->posY));
    m_widthEdit.SetValue(FormatFloat(static_cast<float>(firstNode->width)));
    m_heightEdit.SetValue(FormatFloat(static_cast<float>(firstNode->height)));
    m_enabledEdit.SetValue(firstNode->enabled ? "1" : "0");

    // Clear difference flags initially
    m_titleEdit.isDifferent = false;
    m_filepathEdit.isDifferent = false;
    m_posXEdit.isDifferent = false;
    m_posYEdit.isDifferent = false;
    m_widthEdit.isDifferent = false;
    m_heightEdit.isDifferent = false;
    m_enabledEdit.isDifferent = false;
}

void PlaceholderPropertyEditorPanel::DetectPropertyDifferences()
{
    if (!m_document || m_selectionCount <= 1)
        return;

    const float EPSILON = 0.001f;
    PlaceholderNode* firstNode = m_document->GetNode(m_selectedNodeIds[0]);
    if (!firstNode)
        return;

    // Compare each property with first node across all selected nodes
    for (int i = 1; i < m_selectionCount; ++i)
    {
        PlaceholderNode* node = m_document->GetNode(m_selectedNodeIds[i]);
        if (!node)
            continue;

        // Check Title
        if (node->title != firstNode->title)
            m_titleEdit.isDifferent = true;

        // Check Filepath
        if (node->filepath != firstNode->filepath)
            m_filepathEdit.isDifferent = true;

        // Check Position X (with epsilon for float comparison)
        if (std::fabs(node->posX - firstNode->posX) > EPSILON)
            m_posXEdit.isDifferent = true;

        // Check Position Y
        if (std::fabs(node->posY - firstNode->posY) > EPSILON)
            m_posYEdit.isDifferent = true;

        // Check Width
        if (node->width != firstNode->width)
            m_widthEdit.isDifferent = true;

        // Check Height
        if (node->height != firstNode->height)
            m_heightEdit.isDifferent = true;

        // Check Enabled
        if (node->enabled != firstNode->enabled)
            m_enabledEdit.isDifferent = true;
    }
}

std::string PlaceholderPropertyEditorPanel::FormatFloat(float value, int precision)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    std::string result = oss.str();

    // Remove trailing zeros after decimal point
    if (result.find('.') != std::string::npos)
    {
        result.erase(result.find_last_not_of('0') + 1, std::string::npos);
        if (result.back() == '.')
            result.pop_back();
    }

    return result;
}

float PlaceholderPropertyEditorPanel::ParseFloat(const std::string& str, float defaultValue)
{
    try
    {
        return std::stof(str);
    }
    catch (...)
    {
        return defaultValue;
    }
}

} // namespace Olympe
