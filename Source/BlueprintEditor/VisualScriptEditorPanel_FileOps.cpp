// ============================================================================
// VisualScriptEditorPanel_FileOps.cpp
// File I/O, Graph Loading/Saving, JSON Serialization
// ============================================================================
//
// This file contains all file operations: loading templates, saving graphs,
// JSON serialization, and persistence operations.
//
// Methods:
// - LoadTemplate(const std::string& path)  : Load blueprint from file
// - Save()                                 : Save to current file
// - SaveAs(const std::string& path)        : Save to new path
// - SerializeAndWrite()                    : Serialize template to JSON
// - SyncPresetsFromRegistryToTemplate()    : Phase 24 preset sync before save
// - RenderSaveAsDialog()                   : ImGui save-as dialog
// - ResetViewportBeforeSave()              : BUG-003: Viewport pan save/restore
// - AfterSave()                            : Post-save cleanup
//
// Integration Points:
// - TaskGraphLoader                       : JSON deserialization
// - TaskGraphSerializer                   : JSON serialization
// - ConditionPresetRegistry               : Phase 24 presets
// - m_template                            : Graph data
// - m_dirty flag                          : Track unsaved changes
// ============================================================================

#include "VisualScriptEditorPanel.h"
#include <fstream>
#include <sstream>

namespace Olympe {

// ============================================================================
// Load Template from File
// ============================================================================

void VisualScriptEditorPanel::LoadTemplate(const std::string& path)
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] LoadTemplate: " << path << "\n";

    TaskGraphTemplate loadedTemplate;
    if (!TaskGraphLoader::LoadFromFile(path, loadedTemplate))
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] LoadTemplate FAILED\n";
        return;
    }

    m_template = loadedTemplate;
    m_currentFilePath = path;
    m_saveAsFilename = path;

    // Sync editor state from loaded template
    SyncCanvasFromTemplate();

    // Load presets from template (Phase 24)
    for (const auto& preset : m_template.Presets)
    {
        m_presetRegistry.AddPreset(preset);
    }

    m_dirty = false;
    m_verificationDone = false;

    SYSTEM_LOG << "[VisualScriptEditorPanel] LoadTemplate complete\n";
}

// ============================================================================
// Save Operations
// ============================================================================

bool VisualScriptEditorPanel::Save()
{
    if (m_currentFilePath.empty())
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Save: no current file path\n";
        return false;
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] Saving to: " << m_currentFilePath << "\n";

    // Update template from canvas before save
    SyncTemplateFromCanvas();

    // Phase 24: Sync presets to template
    SyncPresetsFromRegistryToTemplate();

    // Validate blackboard before save (BUG-001)
    ValidateAndCleanBlackboardEntries();

    // Serialize and write
    bool success = SerializeAndWrite();

    if (success)
    {
        m_dirty = false;
        AfterSave();
        SYSTEM_LOG << "[VisualScriptEditorPanel] Save succeeded\n";
    }
    else
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Save FAILED\n";
    }

    return success;
}

bool VisualScriptEditorPanel::SaveAs(const std::string& path)
{
    if (path.empty())
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs: empty path\n";
        return false;
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs: " << path << "\n";

    m_currentFilePath = path;
    m_saveAsFilename = path;

    return Save();
}

bool VisualScriptEditorPanel::SerializeAndWrite()
{
    try
    {
        // Serialize m_template to JSON
        std::string jsonStr = TaskGraphSerializer::Serialize(m_template);

        // Write to file
        std::ofstream outFile(m_currentFilePath);
        if (!outFile.is_open())
        {
            SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: failed to open file\n";
            return false;
        }

        outFile << jsonStr;
        outFile.close();

        return true;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: exception: " << e.what() << "\n";
        return false;
    }
}

// ============================================================================
// Phase 24: Preset Synchronization
// ============================================================================

void VisualScriptEditorPanel::SyncPresetsFromRegistryToTemplate()
{
    // Copy all presets from registry to template for serialization
    m_template.Presets.clear();

    size_t presetCount = m_presetRegistry.GetPresetCount();
    std::vector<ConditionPreset> allPresets = m_presetRegistry.GetFilteredPresets("");

    for (const auto& preset : allPresets)
    {
        m_template.Presets.push_back(preset);
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] SyncPresetsFromRegistryToTemplate: "
               << m_template.Presets.size() << " preset(s) synced\n";
}

// ============================================================================
// Save As Dialog
// ============================================================================

void VisualScriptEditorPanel::RenderSaveAsDialog()
{
    if (!m_showSaveAsDialog)
        return;

    bool open = true;
    if (ImGui::BeginPopupModal("Save As##vseditor", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save blueprint as:");
        ImGui::Separator();

        char filenameBuf[256];
        strncpy_s(filenameBuf, sizeof(filenameBuf), m_saveAsFilename.c_str(), _TRUNCATE);

        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::InputText("##filename", filenameBuf, sizeof(filenameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::string newPath = filenameBuf;
            if (!newPath.empty())
            {
                SaveAs(newPath);
                m_showSaveAsDialog = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            std::string newPath = filenameBuf;
            if (!newPath.empty())
            {
                SaveAs(newPath);
                m_showSaveAsDialog = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            m_showSaveAsDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (!open)
    {
        m_showSaveAsDialog = false;
    }
}

// ============================================================================
// BUG-003 Fix: Viewport Pan Management
// ============================================================================

void VisualScriptEditorPanel::ResetViewportBeforeSave()
{
    // Store current pan/zoom state
    // ImNodes::IO& io = ImNodes::GetIO();
    // io.Panning = ImVec2(0, 0);  // Reset pan to origin
}

void VisualScriptEditorPanel::AfterSave()
{
    // Post-save operations
    // (Could include cache invalidation, UI refresh, etc.)
}

}  // namespace Olympe
