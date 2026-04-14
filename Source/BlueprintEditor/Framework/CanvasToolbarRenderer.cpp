/**
 * @file CanvasToolbarRenderer.cpp
 * @brief Implementation of unified toolbar renderer for all graph types (Phase 41)
 * @author Olympe Engine
 * @date 2026-03-25
 */

#include "CanvasToolbarRenderer.h"
#include "IGraphDocument.h"
#include "CanvasModalRenderer.h"
#include "../../third_party/imgui/imgui.h"
#include "../../Editor/Modals/FilePickerModal.h"
#include "../../system/system_utils.h"
#include "../../system/system_consts.h"

#include <sstream>

namespace Olympe {

// ============================================================================
// Constructor / Destructor
// ============================================================================

CanvasToolbarRenderer::CanvasToolbarRenderer(IGraphDocument* document)
    : m_document(document)
    , m_browseModal(nullptr)
    , m_isDirty(false)
{
    if (!m_document)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] WARNING: Constructed with null document\n";
        return;
    }

    SYSTEM_LOG << "[CanvasToolbarRenderer] Created for document: "
               << m_document->GetName() << " (Type: "
               << m_document->GetTypeName() << ")\n";
}

CanvasToolbarRenderer::~CanvasToolbarRenderer()
{
    // Phase 44.1: SaveFile modal managed by dispatcher, Browse modal local
    if (m_browseModal)
    {
        delete m_browseModal;
        m_browseModal = nullptr;
    }
}

// ============================================================================
// Public Interface
// ============================================================================

void CanvasToolbarRenderer::Render()
{
    if (!m_document)
        return;

    // Update cached state
    bool currentDirty = m_document->IsDirty();
    if (currentDirty != m_isDirty)
    {
        m_isDirty = currentDirty;
    }

    // Render toolbar layout
    ImGui::BeginGroup();
    {
        // Render buttons: [Save] [SaveAs] | [Browse] | [Path]
        RenderButtons();
        ImGui::SameLine(0.0f, 20.0f);  // Spacing between button groups
        ImGui::Separator();
        ImGui::SameLine(0.0f, 20.0f);
        RenderPathDisplay();
    }
    ImGui::EndGroup();
}

void CanvasToolbarRenderer::RenderModals()
{
    if (!m_document)
        return;

    // Render SaveAs modal if open
    if (m_showSaveAsModal)
    {
        // Determine file type based on document type
        SaveFileType saveType = SaveFileType::Blueprint;
        if (m_document)
        {
            switch (m_document->GetType())
            {
                case DocumentType::BEHAVIOR_TREE:
                    saveType = SaveFileType::BehaviorTree;
                    break;
                case DocumentType::ENTITY_PREFAB:
                    saveType = SaveFileType::EntityPrefab;
                    break;
                case DocumentType::VISUAL_SCRIPT:
                    saveType = SaveFileType::Blueprint;
                    break;
                default:
                    saveType = SaveFileType::Blueprint;
                    break;
            }
        }

        std::string initDir = GetInitialDirectory();
        std::string suggestedName = GetSuggestedFilename();

        // Check if modal is not already open, then open it
        if (!CanvasModalRenderer::Get().IsSaveFileModalOpen())
        {
            CanvasModalRenderer::Get().OpenSaveFilePickerModal(initDir, suggestedName, saveType);
        }

        // Render handled by TabManager calling CanvasModalRenderer::RenderSaveFilePickerModal()

        if (CanvasModalRenderer::Get().IsSaveFileModalConfirmed())
        {
            std::string selectedPath = CanvasModalRenderer::Get().GetSelectedSaveFilePath();
            OnSaveAsComplete(selectedPath);
            m_showSaveAsModal = false;
            CanvasModalRenderer::Get().CloseSaveFileModal();
        }
        else if (!CanvasModalRenderer::Get().IsSaveFileModalOpen())
        {
            m_showSaveAsModal = false;
            CanvasModalRenderer::Get().CloseSaveFileModal();
        }
    }

    // Render Browse modal if open
    if (m_showBrowseModal)
    {
        if (!m_browseModal)
        {
            // Determine file type based on document type
            FilePickerType browseType = FilePickerType::SubGraph;
            if (m_document)
            {
                switch (m_document->GetType())
                {
                    case DocumentType::BEHAVIOR_TREE:
                        browseType = FilePickerType::BehaviorTree;
                        break;
                    case DocumentType::VISUAL_SCRIPT:
                        browseType = FilePickerType::SubGraph;
                        break;
                    case DocumentType::ENTITY_PREFAB:
                        browseType = FilePickerType::SubGraph;  // Temporary: may need EntityPrefab type
                        break;
                    default:
                        browseType = FilePickerType::SubGraph;
                        break;
                }
            }

            std::string initDir = GetInitialDirectory();
            m_browseModal = new FilePickerModal(browseType);
            m_browseModal->Open(initDir);
        }

        m_browseModal->Render();

        if (m_browseModal->IsConfirmed())
        {
            std::string selectedPath = m_browseModal->GetSelectedFile();
            OnBrowseComplete(selectedPath);
            m_showBrowseModal = false;
            delete m_browseModal;
            m_browseModal = nullptr;
        }
        else if (!m_browseModal->IsOpen())
        {
            m_showBrowseModal = false;
            delete m_browseModal;
            m_browseModal = nullptr;
        }
    }
}

bool CanvasToolbarRenderer::IsModalOpen() const
{
    return m_showSaveAsModal || m_showBrowseModal;
}

std::string CanvasToolbarRenderer::GetSelectedFilePath() const
{
    return m_selectedFilePath;
}

std::string CanvasToolbarRenderer::GetStatusText() const
{
    if (!m_document)
        return "No document";

    std::string path = m_document->GetFilePath();
    if (path.empty())
        return "Untitled (unsaved)";

    if (m_document->IsDirty())
        return path + " *";
    else
        return path;
}

void CanvasToolbarRenderer::Invalidate()
{
    m_isDirty = m_document ? m_document->IsDirty() : false;
}

void CanvasToolbarRenderer::SetOnSaveComplete(SaveCallback callback)
{
    m_onSaveComplete = callback;
}

void CanvasToolbarRenderer::SetOnBrowseComplete(BrowseCallback callback)
{
    m_onBrowseComplete = callback;
}

std::string CanvasToolbarRenderer::GetSaveFileFilter() const
{
    if (!m_document)
        return "*.json";

    switch (m_document->GetType())
    {
        case DocumentType::VISUAL_SCRIPT:
            return "*.vs.json";
        case DocumentType::BEHAVIOR_TREE:
            return "*.bt.json";
        case DocumentType::ENTITY_PREFAB:
            return "*.prefab.json";
        default:
            return "*.json";
    }
}

std::string CanvasToolbarRenderer::GetBrowseFileFilter() const
{
    return GetSaveFileFilter();
}

// ============================================================================
// Private Methods - UI Rendering
// ============================================================================

void CanvasToolbarRenderer::RenderButtons()
{
    if (!m_document)
        return;

    // [Save] button
    bool canSave = m_document->IsDirty();
    if (!canSave)
        ImGui::BeginDisabled();

    if (ImGui::Button("Save", ImVec2(80.0f, 0.0f)))
    {
        OnSaveClicked();
    }

    if (!canSave)
        ImGui::EndDisabled();

    ImGui::SetItemTooltip("Save document to current path");

    ImGui::SameLine();

    // [SaveAs] button
    if (ImGui::Button("Save As", ImVec2(80.0f, 0.0f)))
    {
        OnSaveAsClicked();
    }
    ImGui::SetItemTooltip("Save document to new location");

    ImGui::SameLine(0.0f, 20.0f);  // Spacing between button groups

    // [Browse] button
    if (ImGui::Button("Browse", ImVec2(80.0f, 0.0f)))
    {
        OnBrowseClicked();
    }
    ImGui::SetItemTooltip("Open existing document");
}

void CanvasToolbarRenderer::RenderPathDisplay()
{
    if (!m_document)
        return;

    std::string statusText = GetStatusText();

    ImGui::TextDisabled("File: ");
    ImGui::SameLine();

    // Display path with color coding
    if (m_document->IsDirty())
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", statusText.c_str());
    }
    else if (!m_document->GetFilePath().empty())
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", statusText.c_str());
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", statusText.c_str());
    }
}

// ============================================================================
// Private Methods - Button Callbacks
// ============================================================================

void CanvasToolbarRenderer::OnSaveClicked()
{
    if (!m_document)
        return;

    SYSTEM_LOG << "[CanvasToolbarRenderer] Save button clicked\n";

    std::string currentPath = m_document->GetFilePath();

    // If no path, show SaveAs dialog
    if (currentPath.empty())
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] No current path, opening SaveAs modal\n";
        OnSaveAsClicked();
        return;
    }

    // Save to current path
    if (ExecuteSave(currentPath))
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] Successfully saved to: " << currentPath << "\n";
        if (m_onSaveComplete)
            m_onSaveComplete(currentPath);
    }
    else
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] ERROR: Failed to save to: " << currentPath << "\n";
    }
}

void CanvasToolbarRenderer::OnSaveAsClicked()
{
    if (!m_document)
        return;

    SYSTEM_LOG << "[CanvasToolbarRenderer] SaveAs button clicked\n";
    m_showSaveAsModal = true;
}

void CanvasToolbarRenderer::OnBrowseClicked()
{
    if (!m_document)
        return;

    SYSTEM_LOG << "[CanvasToolbarRenderer] Browse button clicked\n";
    m_showBrowseModal = true;
}

void CanvasToolbarRenderer::OnSaveAsComplete(const std::string& filePath)
{
    if (!m_document || filePath.empty())
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] SaveAs cancelled or invalid path\n";
        return;
    }

    SYSTEM_LOG << "[CanvasToolbarRenderer] SaveAs selected: " << filePath << "\n";

    if (ExecuteSave(filePath))
    {
        m_selectedFilePath = filePath;
        SYSTEM_LOG << "[CanvasToolbarRenderer] Successfully saved to: " << filePath << "\n";
        if (m_onSaveComplete)
            m_onSaveComplete(filePath);
    }
    else
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] ERROR: Failed to save to: " << filePath << "\n";
    }
}

void CanvasToolbarRenderer::OnBrowseComplete(const std::string& filePath)
{
    if (!m_document || filePath.empty())
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] Browse cancelled or invalid path\n";
        return;
    }

    SYSTEM_LOG << "[CanvasToolbarRenderer] Browse selected: " << filePath << "\n";

    if (ExecuteLoad(filePath))
    {
        m_selectedFilePath = filePath;
        SYSTEM_LOG << "[CanvasToolbarRenderer] Successfully loaded from: " << filePath << "\n";
        if (m_onBrowseComplete)
            m_onBrowseComplete(filePath);
    }
    else
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] ERROR: Failed to load from: " << filePath << "\n";
    }
}

// ============================================================================
// Private Methods - Helper Functions
// ============================================================================

std::string CanvasToolbarRenderer::GetInitialDirectory() const
{
    if (!m_document)
        return ".";

    std::string path = m_document->GetFilePath();
    if (path.empty())
        return ".";

    // Extract directory from path
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash == std::string::npos)
        return ".";

    return path.substr(0, lastSlash);
}

std::string CanvasToolbarRenderer::GetSuggestedFilename() const
{
    if (!m_document)
        return "Untitled.json";

    std::string path = m_document->GetFilePath();
    if (path.empty())
        return "Untitled.json";

    // Extract filename from path
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash == std::string::npos)
        return path;

    return path.substr(lastSlash + 1);
}

bool CanvasToolbarRenderer::ExecuteSave(const std::string& filePath)
{
    if (!m_document)
        return false;

    SYSTEM_LOG << "[CanvasToolbarRenderer] Executing save to: " << filePath << "\n";

    bool success = m_document->Save(filePath);

    if (success)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] Save succeeded\n";
    }
    else
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] Save failed (check logs above)\n";
    }

    return success;
}

bool CanvasToolbarRenderer::ExecuteLoad(const std::string& filePath)
{
    if (!m_document)
        return false;

    SYSTEM_LOG << "[CanvasToolbarRenderer] Executing load from: " << filePath << "\n";

    bool success = m_document->Load(filePath);

    if (success)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] Load succeeded\n";
    }
    else
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer] Load failed (check logs above)\n";
    }

    return success;
}

} // namespace Olympe
