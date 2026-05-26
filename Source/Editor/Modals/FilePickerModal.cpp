/**
 * @file FilePickerModal.cpp
 * @brief Implementation of FilePickerModal (Phase 40).
 * @author Olympe Engine
 * @date 2026-03-20
 */

#include "FilePickerModal.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_consts.h"
#include "../../system/system_utils.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include <algorithm>
#include <functional>
#include <cstring>

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

FilePickerModal::FilePickerModal(FilePickerType fileType)
    : m_fileType(fileType)
{
    // Initialize path to default directory for this file type
    m_currentPath = GetDefaultDirectory();
    strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
    RefreshFileList();
}

// ============================================================================
// Modal Lifecycle
// ============================================================================

void FilePickerModal::Open(const std::string& currentPath)
{
    m_isOpen = true;
    m_confirmed = false;
    m_selectedFile = "";
    m_selectedIndex = -1;
    m_selectedFilterIndex = 0;
    m_currentFilter = GetFilePattern();

    if (!currentPath.empty())
    {
        m_currentPath = currentPath;
        strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), currentPath.c_str(), _TRUNCATE);
    }
    else
    {
        m_currentPath = GetDefaultDirectory();
        strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
    }

    RefreshFileList();
    memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
}

void FilePickerModal::Close()
{
    m_isOpen = false;
    m_confirmed = false;
    m_selectedFile = "";
}

void FilePickerModal::Render()
{
    if (!m_isOpen)
        return;

    // Center the modal on screen
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900.0f, 600.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(600.0f, 400.0f), ImVec2(1400.0f, 900.0f));

    bool open = true;
    std::string title = GetModalTitle();
    if (ImGui::BeginPopupModal(title.c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Description
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "%s", GetDescriptionText().c_str());
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
        // Filter Dropdown & Search Filter
        // ====================================================================

        ImGui::TextDisabled("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);

        // Build filter options based on file type
        const char* filterOptions[] = { "All (*.*)", "Type 1", "Type 2", "Type 3" };
        int filterCount = 1;

        if (m_fileType == FilePickerType::BehaviorTree)
        {
            filterOptions[1] = "BehaviorTree (*.bt.json)";
            filterOptions[2] = "Subgraph (*.ats.json)";
            filterOptions[3] = "All Files (*.*)";
            filterCount = 4;
        }
        else if (m_fileType == FilePickerType::SubGraph)
        {
            filterOptions[1] = "Blueprint (*.ats)";
            filterOptions[2] = "All Files (*.*)";
            filterCount = 3;
        }

        if (ImGui::Combo("##filter", &m_selectedFilterIndex, filterOptions, filterCount))
        {
            // Update filter based on selection
            if (m_fileType == FilePickerType::BehaviorTree)
            {
                if (m_selectedFilterIndex == 0) m_currentFilter = "*";
                else if (m_selectedFilterIndex == 1) m_currentFilter = ".bt.json";
                else if (m_selectedFilterIndex == 2) m_currentFilter = ".ats.json";
                else m_currentFilter = "*";
            }
            else if (m_fileType == FilePickerType::SubGraph)
            {
                if (m_selectedFilterIndex == 0) m_currentFilter = "*";
                else if (m_selectedFilterIndex == 1) m_currentFilter = ".ats";
                else m_currentFilter = "*";
            }
            else
            {
                m_currentFilter = "*";
            }
            RefreshFileList();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Search:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##search", m_searchBuffer, sizeof(m_searchBuffer));

        ImGui::Separator();

        // ====================================================================
        // Files and Folders (Split Panel)
        // ====================================================================

        ImGui::BeginChild("##file_browser", ImVec2(0, 300), true);
        {
            // Left column: Folders
            float folderWidth = 150.0f;
            ImGui::BeginChild("##folders", ImVec2(folderWidth, -1), true);
            ImGui::TextDisabled("Folders:");

            // Parent directory ".."
            if (ImGui::Selectable("..", false))
            {
                size_t lastSlash = m_currentPath.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                {
                    m_currentPath = m_currentPath.substr(0, lastSlash);
                    strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
                    RefreshFileList();
                }
            }

            // List subdirectories
            for (const auto& folder : m_folderList)
            {
                if (ImGui::Selectable(folder.c_str(), false))
                {
                    // Build path safely - avoid double slashes
                    if (!m_currentPath.empty() && m_currentPath.back() != '/' && m_currentPath.back() != '\\')
                    {
                        m_currentPath += "/";
                    }
                    m_currentPath += folder;
                    strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
                    RefreshFileList();
                }
            }
            ImGui::EndChild();
        }

        // Right column: Files
        ImGui::SameLine();
        ImGui::BeginChild("##files", ImVec2(0, -1), true);
        ImGui::TextDisabled("Available Files:");

        RenderFileList();

        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::Separator();

        // ====================================================================
        // Selected File Display
        // ====================================================================

        ImGui::TextDisabled("Selected:");
        ImGui::SameLine();
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_fileList.size()))
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "%s", m_fileList[m_selectedIndex].c_str());
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
        ImGui::OpenPopup(title.c_str());
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string FilePickerModal::GetDefaultDirectory() const
{
    switch (m_fileType)
    {
        case FilePickerType::BehaviorTree:
            return "./Gamedata/BehaviorTree";
        case FilePickerType::SubGraph:
            return "./Gamedata/VisualScript";  // Changed from "Blueprints" to match new structure
        case FilePickerType::EntityPrefab:
            return "./Gamedata/EntityPrefab";
        case FilePickerType::Audio:
            return "./Gamedata/Audio";
        case FilePickerType::Tileset:
            return "./Gamedata/Tilesets";
        default:
            return "./Gamedata";
    }
}

std::string FilePickerModal::GetFilePattern() const
{
    switch (m_fileType)
    {
        case FilePickerType::BehaviorTree:
            return ".bt.json";
        case FilePickerType::SubGraph:
            return ".ats.json";  // Changed to include .json extension
        case FilePickerType::EntityPrefab:
            return ".prefab.json";
        case FilePickerType::Audio:
            return ".ogg";
        case FilePickerType::Tileset:
            return ".tsj";
        default:
            return "*";
    }
}

std::string FilePickerModal::GetModalTitle() const
{
    switch (m_fileType)
    {
        case FilePickerType::BehaviorTree:
            return "Select BehaviorTree File##filepicker_bt";
        case FilePickerType::SubGraph:
            return "Select VisualScript File##filepicker_ats";
        case FilePickerType::EntityPrefab:
            return "Select EntityPrefab File##filepicker_prefab";
        case FilePickerType::Audio:
            return "Select Audio File##filepicker_audio";
        case FilePickerType::Tileset:
            return "Select Tileset File##filepicker_tileset";
        default:
            return "Select File##filepicker";
    }
}

std::string FilePickerModal::GetDescriptionText() const
{
    switch (m_fileType)
    {
        case FilePickerType::BehaviorTree:
            return "Select a BehaviorTree file (.bt.json) to link with this component";
        case FilePickerType::SubGraph:
            return "Select a VisualScript file (.ats.json) to use as SubGraph";
        case FilePickerType::EntityPrefab:
            return "Select an EntityPrefab file (.prefab.json) to load";
        case FilePickerType::Audio:
            return "Select an Audio file (.ogg)";
        case FilePickerType::Tileset:
            return "Select a Tileset file (.tsj)";
        default:
            return "Select a file";
    }
}

void FilePickerModal::RefreshFileList()
{
    m_fileList.clear();
    m_folderList.clear();
    m_selectedIndex = -1;

    std::string pattern = m_currentFilter.empty() ? GetFilePattern() : m_currentFilter;

    // For SubGraph type, scan multiple root directories recursively
    if (m_fileType == FilePickerType::SubGraph)
    {
        ScanDirectoriesRecursively("./GameData", pattern);
        ScanDirectoriesRecursively("./Blueprint", pattern);
    }
    else
    {
        // For other types, scan the current path recursively
        ScanDirectoriesRecursively(m_currentPath, pattern);
    }

    // Sort alphabetically
    std::sort(m_fileList.begin(), m_fileList.end());
    std::sort(m_folderList.begin(), m_folderList.end());

    SYSTEM_LOG << "[FilePickerModal] Found " << m_fileList.size() 
               << " files matching " << pattern << " and " << m_folderList.size()
               << " folders in " << (m_fileType == FilePickerType::SubGraph ? "GameData+Blueprint" : m_currentPath) << "\n";
}

void FilePickerModal::ScanDirectoriesRecursively(const std::string& rootPath, const std::string& pattern)
{
    ScanDirectoriesRecursivelyHelper(rootPath, pattern, rootPath);
}

void FilePickerModal::ScanDirectoriesRecursivelyHelper(const std::string& rootPath, const std::string& pattern, const std::string& currentPath)
{
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = currentPath + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        std::string filename = findData.cFileName;

        // Skip "." and ".."
        if (filename == "." || filename == "..")
            continue;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // For SubGraph, collect relative paths for navigation
            if (m_fileType == FilePickerType::SubGraph)
            {
                std::string relativePath = currentPath.substr(rootPath.length());
                if (!relativePath.empty() && relativePath[0] == '\\') relativePath = relativePath.substr(1);
                relativePath += "\\" + filename;
                m_folderList.push_back(relativePath);
            }
            else
            {
                m_folderList.push_back(filename);
            }
            // Recurse into subdirectory
            ScanDirectoriesRecursivelyHelper(rootPath, pattern, currentPath + "\\" + filename);
        }
        else
        {
            // Check if file matches pattern
            if (pattern == "*" || filename.find(pattern) != std::string::npos)
            {
                if (m_fileType == FilePickerType::SubGraph)
                {
                    // Store full relative path for SubGraph
                    std::string relativePath = currentPath.substr(rootPath.length());
                    if (!relativePath.empty() && relativePath[0] == '\\') relativePath = relativePath.substr(1);
                    if (!relativePath.empty()) relativePath += "\\";
                    relativePath += filename;
                    m_fileList.push_back(relativePath);
                }
                else
                {
                    m_fileList.push_back(filename);
                }
            }
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
#else
    DIR* dir = opendir(currentPath.c_str());
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename = entry->d_name;

        // Skip "." and ".."
        if (filename == "." || filename == "..")
            continue;

        std::string fullPath = currentPath + "/" + filename;

        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == 0)
        {
            if (S_ISDIR(statbuf.st_mode))
            {
                // For SubGraph, collect relative paths
                if (m_fileType == FilePickerType::SubGraph)
                {
                    std::string relativePath = currentPath.substr(rootPath.length());
                    if (!relativePath.empty() && relativePath[0] == '/') relativePath = relativePath.substr(1);
                    relativePath += "/" + filename;
                    m_folderList.push_back(relativePath);
                }
                else
                {
                    m_folderList.push_back(filename);
                }
                // Recurse
                ScanDirectoriesRecursivelyHelper(rootPath, pattern, fullPath);
            }
            else
            {
                // Check pattern
                if (pattern == "*" || filename.find(pattern) != std::string::npos)
                {
                    if (m_fileType == FilePickerType::SubGraph)
                    {
                        std::string relativePath = currentPath.substr(rootPath.length());
                        if (!relativePath.empty() && relativePath[0] == '/') relativePath = relativePath.substr(1);
                        if (!relativePath.empty()) relativePath += "/";
                        relativePath += filename;
                        m_fileList.push_back(relativePath);
                    }
                    else
                    {
                        m_fileList.push_back(filename);
                    }
                }
            }
        }
    }

    closedir(dir);
#endif
}

void FilePickerModal::RenderFileList()
{
    std::vector<std::string> filteredFiles = GetFilteredFiles();

    ImGui::TextDisabled("Available Files:");

    ImGui::BeginChild("##file_list", ImVec2(0, 250), true);

    for (int i = 0; i < static_cast<int>(filteredFiles.size()); ++i)
    {
        const std::string& filename = filteredFiles[i];
        
        // Find the actual index in the unfiltered list
        int actualIndex = -1;
        for (int j = 0; j < static_cast<int>(m_fileList.size()); ++j)
        {
            if (m_fileList[j] == filename)
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
        ImGui::TextDisabled("(no files found)");
    }

    ImGui::EndChild();
}

void FilePickerModal::RenderActionButtons()
{
    bool canSelect = m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_fileList.size());

    if (!canSelect)
        ImGui::BeginDisabled(true);

    if (ImGui::Button("Select##select", ImVec2(100, 0)))
    {
        if (canSelect)
        {
            // Build full path: currentPath / filename
            m_selectedFile = m_currentPath + "/" + m_fileList[m_selectedIndex];
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

std::vector<std::string> FilePickerModal::GetFilteredFiles() const
{
    std::string searchLower(m_searchBuffer);
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    std::vector<std::string> filtered;

    for (const auto& filename : m_fileList)
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
