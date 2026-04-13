/**
 * @file BehaviorTreeFilePickerModal.cpp
 * @brief Implementation of BehaviorTreeFilePickerModal (Phase C).
 * @author Olympe Engine
 * @date 2026-03-15
 */

#include "BehaviorTreeFilePickerModal.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_consts.h"
#include "../../system/system_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cstring>

namespace Olympe {

BehaviorTreeFilePickerModal::BehaviorTreeFilePickerModal()
{
    // Initialize path to behavior tree directory
    m_currentPath = "./Gamedata";// / BehaviorTree";
    strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
    RefreshFileList();
}

// ============================================================================
// Modal Lifecycle
// ============================================================================

void BehaviorTreeFilePickerModal::Open(const std::string& currentPath)
{
    m_isOpen = true;
    m_confirmed = false;
    m_selectedFile = "";
    m_selectedIndex = -1;

    if (!currentPath.empty())
    {
        m_currentPath = currentPath;
        strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), currentPath.c_str(), _TRUNCATE);
    }

    RefreshFileList();
    memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
}

void BehaviorTreeFilePickerModal::Close()
{
    m_isOpen = false;
    m_confirmed = false;
    m_selectedFile = "";
}

void BehaviorTreeFilePickerModal::Render()
{
    if (!m_isOpen)
        return;

    // Center the modal on screen
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700.0f, 500.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(500.0f, 300.0f), ImVec2(1200.0f, 800.0f));

    bool open = true;
    if (ImGui::BeginPopupModal("Select BehaviorTree File##modal", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "Select a BehaviorTree file (.bt.json) to link with this component");
        ImGui::Separator();

        // ====================================================================
        // Path Navigation
        // ====================================================================

        ImGui::TextDisabled("Path:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-100.0f);
        if (ImGui::InputText("##path", m_pathBuffer, sizeof(m_pathBuffer)))
        {
            m_currentPath = m_pathBuffer;
            RefreshFileList();
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh##refresh", ImVec2(90, 0)))
        {
            RefreshFileList();
        }

        ImGui::Separator();

        // ====================================================================
        // Search Filter
        // ====================================================================

        ImGui::TextDisabled("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##search", m_searchBuffer, sizeof(m_searchBuffer));

        ImGui::Separator();

        // ====================================================================
        // File List
        // ====================================================================

        RenderFileList();

        ImGui::Separator();

        // ====================================================================
        // Selected File Display
        // ====================================================================

        ImGui::TextDisabled("Selected:");
        ImGui::SameLine();
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_behaviorTreeFiles.size()))
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "%s", m_behaviorTreeFiles[m_selectedIndex].c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "(none)");
        }

        ImGui::Separator();

        // ====================================================================
        // Action Buttons
        // ====================================================================

        RenderActionButtons();

        ImGui::EndPopup();
    }

    if (!open)
    {
        m_isOpen = false;
        ImGui::OpenPopup("Select BehaviorTree File##modal");
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void BehaviorTreeFilePickerModal::RefreshFileList()
{
    m_behaviorTreeFiles.clear();
    m_selectedIndex = -1;

#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = m_currentPath + "\\*.bt.json";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        SYSTEM_LOG << "[BehaviorTreeFilePicker] Directory not found or inaccessible: " << m_currentPath << "\n";
        return;
    }

    do
    {
        std::string filename = findData.cFileName;

        // Skip directories, only get files
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            m_behaviorTreeFiles.push_back(filename);
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);

    // Sort alphabetically
    std::sort(m_behaviorTreeFiles.begin(), m_behaviorTreeFiles.end());

    SYSTEM_LOG << "[BehaviorTreeFilePicker] Found " << m_behaviorTreeFiles.size() 
               << " behavior tree files (.bt.json) in " << m_currentPath << "\n";
#else
    SYSTEM_LOG << "[BehaviorTreeFilePicker] Platform not supported (Windows only)\n";
#endif
}

void BehaviorTreeFilePickerModal::RenderFileList()
{
    std::vector<std::string> filteredFiles = GetFilteredFiles();

    ImGui::TextDisabled("Available BehaviorTrees:");

    ImGui::BeginChild("##file_list", ImVec2(0, 250), true);

    for (int i = 0; i < static_cast<int>(filteredFiles.size()); ++i)
    {
        const std::string& filename = filteredFiles[i];
        
        // Find the actual index in the unfiltered list
        int actualIndex = -1;
        for (int j = 0; j < static_cast<int>(m_behaviorTreeFiles.size()); ++j)
        {
            if (m_behaviorTreeFiles[j] == filename)
            {
                actualIndex = j;
                break;
            }
        }

        bool isSelected = (actualIndex == m_selectedIndex);

        ImGui::PushID(i);

        if (ImGui::Selectable(filename.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups))
        {
            m_selectedIndex = actualIndex;
        }

        ImGui::PopID();
    }

    if (filteredFiles.empty())
    {
        ImGui::TextDisabled("(no behavior tree files found)");
    }

    ImGui::EndChild();
}

void BehaviorTreeFilePickerModal::RenderActionButtons()
{
    bool canSelect = m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_behaviorTreeFiles.size());

    if (!canSelect)
        ImGui::BeginDisabled(true);

    if (ImGui::Button("Select##select", ImVec2(100, 0)))
    {
        if (canSelect)
        {
            // Build full path: currentPath / filename
            m_selectedFile = m_currentPath + "/" + m_behaviorTreeFiles[m_selectedIndex];
            m_confirmed = true;
            m_isOpen = false;
            ImGui::CloseCurrentPopup();
        }
    }

    if (!canSelect)
        ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel##cancel", ImVec2(100, 0)))
    {
        m_isOpen = false;
        m_confirmed = false;
        ImGui::CloseCurrentPopup();
    }
}

std::vector<std::string> BehaviorTreeFilePickerModal::GetFilteredFiles() const
{
    std::string searchLower(m_searchBuffer);
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    std::vector<std::string> filtered;

    for (const auto& filename : m_behaviorTreeFiles)
    {
        std::string filenameLower(filename);
        std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);

        if (searchLower.empty() || filenameLower.find(searchLower) != std::string::npos)
        {
            filtered.push_back(filename);
        }
    }

    return filtered;
}

}  // namespace Olympe
