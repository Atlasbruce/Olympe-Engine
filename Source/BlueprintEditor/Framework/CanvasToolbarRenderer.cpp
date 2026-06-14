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
#include "../TabManager.h"

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

bool CanvasToolbarRenderer::IsModalOpen() const
{
    return m_showSaveAsModal || CanvasModalRenderer::Get().IsSaveFileModalOpen() || m_browseModal != nullptr;
}

std::string CanvasToolbarRenderer::GetStatusText() const
{
    if (!m_document)
        return "No Document";

    std::string path = m_document->GetFilePath();
    if (path.empty())
        return "Untitled (Modified)";

    std::string status = path;
    if (m_document->IsDirty())
        status += " *";

    return status;
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
        return "*.ats.json";
    case DocumentType::BEHAVIOR_TREE:
        return "*.bt.json";
    case DocumentType::ENTITY_PREFAB:
        return "*.pref.json";
    default:
        return "*.json";
    }
}

std::string CanvasToolbarRenderer::GetDefaultSaveDirectory() const
{
    if (!m_document)
        return "./Gamedata/";

    switch (m_document->GetType())
    {
    case DocumentType::VISUAL_SCRIPT:
        return "./Gamedata/VisualScript/";
    case DocumentType::BEHAVIOR_TREE:
        return "./Gamedata/BehaviorTree/";
    case DocumentType::ENTITY_PREFAB:
        return "./Gamedata/EntityPrefab/";
    default:
        return "./Gamedata/";
    }
}

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

    // Phase 62 FIX: Removed diagnostic logs (were 60 FPS spam - Phase 44.4)
    // Render toolbar layout
    ImGui::BeginGroup();
    {
        // Render buttons: [Save] [SaveAs] | [Browse] | [Path]
        RenderButtons();
        ImGui::SameLine(0.0f, 20.0f);  // Spacing between button groups
        ImGui::Separator();
        ImGui::SameLine(0.0f, 20.0f);
        RenderPathDisplay();

        // Phase 71: Render unified Minimap controls if supported
        IGraphRenderer* renderer = m_document->GetRenderer();
        if (renderer && renderer->SupportsMinimap())
        {
            ImGui::SameLine(0.0f, 20.0f);
            ImGui::Separator();
            ImGui::SameLine(0.0f, 20.0f);

            bool minimapVisible = renderer->IsMinimapVisible();
            if (ImGui::Checkbox("Minimap##unified_tb", &minimapVisible))
            {
                renderer->SetMinimapVisible(minimapVisible);
            }

            if (minimapVisible)
            {
                ImGui::SameLine();
                float minimapSize = renderer->GetMinimapSize();
                ImGui::SetNextItemWidth(80.0f);
                if (ImGui::SliderFloat("Size##unified_tb", &minimapSize, 0.05f, 0.5f, "%.2f"))
                {
                    renderer->SetMinimapSize(minimapSize);
                }

                ImGui::SameLine();
                int currentPos = renderer->GetMinimapPosition();
                ImGui::SetNextItemWidth(100.0f);
                const char* positions[] = { "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right" };
                if (ImGui::Combo("Position##unified_tb", &currentPos, positions, 4))
                {
                    renderer->SetMinimapPosition(currentPos);
                }
            }
        }
    }
    ImGui::EndGroup();
}

void CanvasToolbarRenderer::RenderModals()
{
    if (m_showSaveAsModal)
    {
        m_showSaveAsModal = false;

        Olympe::SaveFileType fileType = Olympe::SaveFileType::Blueprint;
        if (m_document)
        {
            switch (m_document->GetType())
            {
            case DocumentType::BEHAVIOR_TREE:
                fileType = Olympe::SaveFileType::BehaviorTree;
                break;
            case DocumentType::ENTITY_PREFAB:
                fileType = Olympe::SaveFileType::EntityPrefab;
                break;
            case DocumentType::VISUAL_SCRIPT:
            default:
                fileType = Olympe::SaveFileType::Blueprint;
                break;
            }
        }

        CanvasModalRenderer::Get().OpenSaveFilePickerModal(
            GetDefaultSaveDirectory(),
            GetSuggestedFilename(),
            fileType);
    }

    CanvasModalRenderer::Get().RenderSaveFilePickerModal();

    if (CanvasModalRenderer::Get().IsSaveFileModalConfirmed())
    {
        const std::string selectedFile = CanvasModalRenderer::Get().GetSelectedSaveFilePath();
        if (!selectedFile.empty())
        {
            OnSaveAsComplete(selectedFile);
        }

        CanvasModalRenderer::Get().CloseSaveFileModal();
    }
}

void CanvasToolbarRenderer::RenderButtons()
{
    if (!m_document)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::RenderButtons] ERROR: m_document is null!\n";
        return;
    }

    // [Save] button
    // A document can always be saved if it's dirty, or if it has never been saved yet (empty path)
    bool canSave = m_document->IsDirty() || m_document->GetFilePath().empty();

    if (!canSave)
        ImGui::BeginDisabled();

    if (ImGui::Button("Save", ImVec2(80.0f, 0.0f)))
    {
        OnSaveClicked();
    }

    if (!canSave)
        ImGui::EndDisabled();

    ImGui::SetItemTooltip("Save document to current path (Ctrl+S)");

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

    // Phase 55: Unified Verify/Run buttons
    IGraphRenderer* renderer = m_document->GetRenderer();
    if (renderer)
    {
        if (renderer->SupportsVerification())
        {
            ImGui::SameLine(0.0f, 20.0f);
            if (ImGui::Button("Verify", ImVec2(80.0f, 0.0f)))
            {
                renderer->VerifyGraph();
            }
            ImGui::SetItemTooltip("Verify graph logic and connectivity");
        }

        if (renderer->SupportsExecution())
        {
            ImGui::SameLine();
            if (ImGui::Button("Run", ImVec2(80.0f, 0.0f)))
            {
                renderer->RunGraph();
            }
            ImGui::SetItemTooltip("Execute/Simulate graph logic");
        }
    }
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
    SYSTEM_LOG << "[CanvasToolbarRenderer] OnSaveClicked - Delegating to TabManager for unified path\n";
    TabManager::Get().SaveActiveTab();
}

void CanvasToolbarRenderer::OnSaveAsClicked()
{
    SYSTEM_LOG << "[CanvasToolbarRenderer] OnSaveAsClicked - opening centralized Save As modal\n";
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
    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] ENTER - Modal confirmed with path: '" << filePath << "'\n";

    if (!m_document || filePath.empty())
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] ERROR: SaveAs cancelled or invalid path\n";
        return;
    }

    // Phase 64: Path normalization and directory enforcement
    // If user provided a path without a directory, prepend the default directory
    std::string finalPath = filePath;
    bool hasDirectory = (finalPath.find('/') != std::string::npos || finalPath.find('\\') != std::string::npos);
    
    if (!hasDirectory)
    {
        std::string defaultDir = GetDefaultSaveDirectory();
        finalPath = defaultDir + finalPath;
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] No directory found in user input, prepending default: " << defaultDir << "\n";
    }

    // Ensure extension is correct even if user manually changed it or forgot it
    std::string extension = GetSaveFileFilter();
    if (extension.find("*") == 0) extension = extension.substr(1);
    
    if (finalPath.size() < extension.size() || finalPath.substr(finalPath.size() - extension.size()) != extension)
    {
        // If it already has .json but not .bt.json (example), we append or replace
        // For simplicity, just append if it doesn't end with the specific extension
        finalPath += extension;
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] Appending required extension: " << extension << "\n";
    }

    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] Final normalized path: '" << finalPath << "'\n";

    if (ExecuteSave(finalPath))
    {
        m_selectedFilePath = finalPath;
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] ✓ ExecuteSave succeeded\n";
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] Calling TabManager::OnGraphDocumentSaved()\n";

        // Phase 44.2.1: Notify TabManager to update UI (tab name, dirty flag)
        TabManager::Get().OnGraphDocumentSaved(m_document, finalPath);

        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] Calling m_onSaveComplete callback\n";
        if (m_onSaveComplete)
        {
            m_onSaveComplete(finalPath);
            SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] Callback executed\n";
        }
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] EXIT - SaveAs complete\n";
    }
    else
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] ✗ ERROR: ExecuteSave failed!\n";
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
    SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] ENTER - filePath='" << filePath << "'\n";

    if (!m_document)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] ERROR: m_document is null! Cannot save.\n";
        return false;
    }

    SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] Document name: " << m_document->GetName() << "\n";
    SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] Document type: " << m_document->GetTypeName() << "\n";
    SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] Dirty state BEFORE save: " << (m_document->IsDirty() ? "TRUE" : "FALSE") << "\n";
    SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] About to invoke m_document->Save(filepath)...\n";

    bool success = m_document->Save(filePath);

    if (success)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] ✓ m_document->Save() returned TRUE\n";
        SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] Dirty state AFTER save: " << (m_document->IsDirty() ? "TRUE" : "FALSE") << "\n";
    }
    else
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] ✗ m_document->Save() returned FALSE - save FAILED!\n";
        SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] Dirty state AFTER failed save: " << (m_document->IsDirty() ? "TRUE" : "FALSE") << "\n";
    }

    SYSTEM_LOG << "[CanvasToolbarRenderer::ExecuteSave] EXIT - returning " << (success ? "true" : "false") << "\n";
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
