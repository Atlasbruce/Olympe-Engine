/**
 * @file SubGraphFilePickerModal.cpp
 * @brief Implementation of SubGraphFilePickerModal (Phase 26).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "SubGraphFilePickerModal.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_consts.h"
#include "../../system/system_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cstring>

namespace Olympe {

SubGraphFilePickerModal::SubGraphFilePickerModal()
{
    // Initialize path to blueprints directory
    m_currentPath = "Blueprints";
    strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
    RefreshFileList();
}

// ============================================================================
// Modal Lifecycle
// ============================================================================

void SubGraphFilePickerModal::Open(const std::string& currentPath)
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

void SubGraphFilePickerModal::Close()
{
    m_isOpen = false;
    m_confirmed = false;
    m_selectedFile = "";
}

void SubGraphFilePickerModal::Render()
{
    if (!m_isOpen)
        return;

    // Center the modal on screen
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(700.0f, 500.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(500.0f, 300.0f), ImVec2(1200.0f, 800.0f));

    bool open = true;
    if (ImGui::BeginPopupModal("Select SubGraph File##modal", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "Select a Blueprint file (.ats) to use as SubGraph");
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
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_blueprintFiles.size()))
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "%s", m_blueprintFiles[m_selectedIndex].c_str());
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
        ImGui::OpenPopup("Select SubGraph File##modal");
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void SubGraphFilePickerModal::RefreshFileList()
{
    m_blueprintFiles.clear();
    m_selectedIndex = -1;

#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = m_currentPath + "\\*.ats";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        SYSTEM_LOG << "[SubGraphFilePicker] Directory not found or inaccessible: " << m_currentPath << "\n";
        return;
    }

    do
    {
        std::string filename = findData.cFileName;

        // Skip directories, only get files
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            m_blueprintFiles.push_back(filename);
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);

    // Sort alphabetically
    std::sort(m_blueprintFiles.begin(), m_blueprintFiles.end());

    SYSTEM_LOG << "[SubGraphFilePicker] Found " << m_blueprintFiles.size() 
               << " blueprint files (.ats) in " << m_currentPath << "\n";
#else
    SYSTEM_LOG << "[SubGraphFilePicker] Platform not supported (Windows only)\n";
#endif
}

void SubGraphFilePickerModal::RenderFileList()
{
    std::vector<std::string> filteredFiles = GetFilteredFiles();

    ImGui::TextDisabled("Available Blueprints:");

    ImGui::BeginChild("##file_list", ImVec2(0, 250), true);

    for (int i = 0; i < static_cast<int>(filteredFiles.size()); ++i)
    {
        const std::string& filename = filteredFiles[i];
        
        // Find the actual index in the unfiltered list
        int actualIndex = -1;
        for (int j = 0; j < static_cast<int>(m_blueprintFiles.size()); ++j)
        {
            if (m_blueprintFiles[j] == filename)
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
        ImGui::TextDisabled("(no blueprint files found)");
    }

    ImGui::EndChild();
}

void SubGraphFilePickerModal::RenderActionButtons()
{
    bool canSelect = m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_blueprintFiles.size());

    if (!canSelect)
        ImGui::BeginDisabled(true);

    if (ImGui::Button("Select##select", ImVec2(100, 0)))
    {
        if (canSelect)
        {
            // Build full path: currentPath / filename
            m_selectedFile = m_currentPath + "/" + m_blueprintFiles[m_selectedIndex];
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

std::vector<std::string> SubGraphFilePickerModal::GetFilteredFiles() const
{
    std::string searchLower(m_searchBuffer);
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    std::vector<std::string> filtered;

    for (const auto& filename : m_blueprintFiles)
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
