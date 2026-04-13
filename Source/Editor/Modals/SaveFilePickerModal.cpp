/**
 * @file SaveFilePickerModal.cpp
 * @brief Implementation of SaveFilePickerModal (Phase 40 Enhancement).
 * @author Olympe Engine
 * @date 2026-03-21
 */

#include "SaveFilePickerModal.h"
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
#include <cstring>
#include <fstream>

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

SaveFilePickerModal::SaveFilePickerModal(SaveFileType fileType)
    : m_fileType(fileType)
{
    m_currentPath = GetDefaultDirectory();
    strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
    RefreshFileList();
}

// ============================================================================
// Modal Lifecycle
// ============================================================================

void SaveFilePickerModal::Open(const std::string& directory, const std::string& suggestedFilename)
{
    m_isOpen = true;
    m_confirmed = false;
    m_selectedFile = "";
    m_showOverwriteConfirm = false;

    if (!directory.empty())
    {
        m_currentPath = directory;
        strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), directory.c_str(), _TRUNCATE);
    }
    else
    {
        m_currentPath = GetDefaultDirectory();
        strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), m_currentPath.c_str(), _TRUNCATE);
    }

    if (!suggestedFilename.empty())
    {
        strncpy_s(m_filenameBuffer, sizeof(m_filenameBuffer), suggestedFilename.c_str(), _TRUNCATE);
    }
    else
    {
        memset(m_filenameBuffer, 0, sizeof(m_filenameBuffer));
    }

    RefreshFileList();
}

void SaveFilePickerModal::Close()
{
    m_isOpen = false;
    m_confirmed = false;
    m_selectedFile = "";
    m_showOverwriteConfirm = false;
}

void SaveFilePickerModal::Render()
{
    if (!m_isOpen)
        return;

    // Show overwrite confirmation dialog if needed
    if (m_showOverwriteConfirm)
    {
        RenderOverwriteConfirmation();
        return;
    }

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
        // Files and Folders (Split Panel)
        // ====================================================================

        ImGui::BeginChild("##file_browser", ImVec2(0, 250), true);
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
            RenderFolderList();

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
        // Filename Input
        // ====================================================================

        RenderFilenameInput();

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
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string SaveFilePickerModal::GetDefaultDirectory() const
{
    switch (m_fileType)
    {
        case SaveFileType::BehaviorTree:
            return "./Gamedata";
        case SaveFileType::Blueprint:
            return "Blueprints";
        case SaveFileType::EntityPrefab:
            return "./Gamedata/Prefabs";
        case SaveFileType::Audio:
            return "./Gamedata/Audio";
        default:
            return "./Gamedata";
    }
}

std::string SaveFilePickerModal::GetFileExtension() const
{
    switch (m_fileType)
    {
        case SaveFileType::BehaviorTree:
            return ".bt.json";
        case SaveFileType::Blueprint:
            return ".ats";
        case SaveFileType::EntityPrefab:
            return ".pref.json";
        case SaveFileType::Audio:
            return ".ogg";
        default:
            return "";
    }
}

std::string SaveFilePickerModal::GetModalTitle() const
{
    switch (m_fileType)
    {
        case SaveFileType::BehaviorTree:
            return "Save BehaviorTree As##save_bt";
        case SaveFileType::Blueprint:
            return "Save Blueprint As##save_ats";
        case SaveFileType::EntityPrefab:
            return "Save Entity Prefab As##save_pref";
        case SaveFileType::Audio:
            return "Save Audio As##save_audio";
        default:
            return "Save File As##save_file";
    }
}

std::string SaveFilePickerModal::GetDescriptionText() const
{
    switch (m_fileType)
    {
        case SaveFileType::BehaviorTree:
            return "Save your BehaviorTree with a new name";
        case SaveFileType::Blueprint:
            return "Save your Blueprint with a new name";
        case SaveFileType::EntityPrefab:
            return "Save your Entity Prefab with a new name";
        case SaveFileType::Audio:
            return "Save your audio file with a new name";
        default:
            return "Save your file with a new name";
    }
}

void SaveFilePickerModal::RefreshFileList()
{
    m_fileList.clear();
    m_folderList.clear();

    std::string extension = GetFileExtension();

#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = m_currentPath + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        SYSTEM_LOG << "[SaveFilePickerModal] Directory not found or inaccessible: " << m_currentPath << "\n";
        return;
    }

    do
    {
        std::string filename = findData.cFileName;

        // Skip "." and ".."
        if (filename == "." || filename == "..")
            continue;

        // Separate folders and files
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            m_folderList.push_back(filename);
        }
        else
        {
            // Only show files with matching extension
            if (filename.length() >= extension.length() &&
                filename.substr(filename.length() - extension.length()) == extension)
            {
                m_fileList.push_back(filename);
            }
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);

    std::sort(m_fileList.begin(), m_fileList.end());
    std::sort(m_folderList.begin(), m_folderList.end());

    SYSTEM_LOG << "[SaveFilePickerModal] Found " << m_fileList.size() 
               << " files and " << m_folderList.size()
               << " folders in " << m_currentPath << "\n";
#else
    DIR* dir = opendir(m_currentPath.c_str());
    if (!dir)
    {
        SYSTEM_LOG << "[SaveFilePickerModal] Directory not found or inaccessible: " << m_currentPath << "\n";
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename = entry->d_name;
        
        if (filename == "." || filename == "..")
            continue;

        std::string fullPath = m_currentPath + "/" + filename;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        {
            m_folderList.push_back(filename);
        }
        else
        {
            // Only show files with matching extension
            if (filename.length() >= extension.length() &&
                filename.substr(filename.length() - extension.length()) == extension)
            {
                m_fileList.push_back(filename);
            }
        }
    }

    closedir(dir);

    std::sort(m_fileList.begin(), m_fileList.end());
    std::sort(m_folderList.begin(), m_folderList.end());

    SYSTEM_LOG << "[SaveFilePickerModal] Found " << m_fileList.size() 
               << " files and " << m_folderList.size()
               << " folders in " << m_currentPath << "\n";
#endif
}

void SaveFilePickerModal::RenderFileList()
{
    for (const auto& filename : m_fileList)
    {
        if (ImGui::Selectable(filename.c_str(), false, ImGuiSelectableFlags_DontClosePopups))
        {
            // Remove extension and populate filename buffer
            std::string filenameNoExt = filename;
            std::string ext = GetFileExtension();
            if (filenameNoExt.length() > ext.length() &&
                filenameNoExt.substr(filenameNoExt.length() - ext.length()) == ext)
            {
                filenameNoExt = filenameNoExt.substr(0, filenameNoExt.length() - ext.length());
            }
            strncpy_s(m_filenameBuffer, sizeof(m_filenameBuffer), filenameNoExt.c_str(), _TRUNCATE);
        }
    }

    if (m_fileList.empty())
    {
        ImGui::TextDisabled("(no files found)");
    }
}

void SaveFilePickerModal::RenderFolderList()
{
    for (const auto& folder : m_folderList)
    {
        if (ImGui::Selectable(folder.c_str(), false, ImGuiSelectableFlags_DontClosePopups))
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
}

void SaveFilePickerModal::RenderFilenameInput()
{
    ImGui::TextDisabled("Filename:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##filename", m_filenameBuffer, sizeof(m_filenameBuffer));

    // Display full path preview
    ImGui::TextDisabled("Full path: %s/%s%s",
                        m_currentPath.c_str(),
                        m_filenameBuffer,
                        GetFileExtension().c_str());
}

void SaveFilePickerModal::RenderActionButtons()
{
    bool filenameEmpty = (std::strlen(m_filenameBuffer) == 0);

    if (!filenameEmpty)
    {
        if (ImGui::Button("Save##save", ImVec2(120, 0)))
        {
            std::string fullPath = m_currentPath + "/" + 
                                   std::string(m_filenameBuffer) + 
                                   GetFileExtension();

            if (FileExists(fullPath))
            {
                // Show overwrite confirmation
                m_selectedFile = fullPath;
                m_showOverwriteConfirm = true;
            }
            else
            {
                // Save immediately
                m_selectedFile = fullPath;
                m_confirmed = true;
                m_isOpen = false;
                ImGui::CloseCurrentPopup();
                SYSTEM_LOG << "[SaveFilePickerModal] File save confirmed: " << fullPath << "\n";
            }
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Button("Save##save", ImVec2(120, 0));
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel##cancel", ImVec2(120, 0)))
    {
        m_isOpen = false;
        m_confirmed = false;
        m_showOverwriteConfirm = false;
        ImGui::CloseCurrentPopup();
    }
}

bool SaveFilePickerModal::FileExists(const std::string& path) const
{
    std::ifstream file(path);
    return file.good();
}

void SaveFilePickerModal::RenderOverwriteConfirmation()
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Overwrite File?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("File already exists. Overwrite?");
        ImGui::Text("%s", m_selectedFile.c_str());
        ImGui::Spacing();

        if (ImGui::Button("Yes, Overwrite##yes", ImVec2(120, 0)))
        {
            m_confirmed = true;
            m_isOpen = false;
            m_showOverwriteConfirm = false;
            ImGui::CloseCurrentPopup();
            SYSTEM_LOG << "[SaveFilePickerModal] File overwrite confirmed: " << m_selectedFile << "\n";
        }

        ImGui::SameLine();

        if (ImGui::Button("No, Cancel##no", ImVec2(120, 0)))
        {
            m_selectedFile = "";
            m_showOverwriteConfirm = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    else
    {
        ImGui::OpenPopup("Overwrite File?");
    }
}

}  // namespace Olympe
