/**
 * @file CanvasToolbarRenderer.cpp
 * @brief Implementation of unified toolbar renderer for all graph types (Phase 41)
 * @author Olympe Engine
 * @date 2026-03-25
 */

#include "CanvasToolbarRenderer.h"
#include "IGraphDocument.h"
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
    }
    ImGui::EndGroup();
}

void CanvasToolbarRenderer::RenderModals()
{
    if (!m_document)
        return;

    // Generate unique modal ID based on document pointer to avoid conflicts
    // when multiple renderers are active
    std::string modalIdStr = "SaveAsPopup_" + std::to_string(reinterpret_cast<uintptr_t>(m_document));
    const char* modalId = modalIdStr.c_str();

    // ========== PHASE 50: Simple SaveAs Modal (replaces complex SaveFilePickerModal) ==========
    // Opens a simple text input popup for filepath entry (like legacy NodeGraphPanel)
    if (m_showSaveAsModal)
    {
        // Clear buffer when popup first opens (like legacy code)
        if (!m_saveAsPopupOpen)
        {
            m_saveAsFileBuffer[0] = '\0';
            m_saveAsPopupOpen = true;
        }

        ImGui::OpenPopup(modalId);
    }

    // Render the simple SaveAs popup modal
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(modalId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save document as:");
        ImGui::Separator();

        ImGui::SetNextItemWidth(300.0f);
        ImGui::InputText("##filepath_input", m_saveAsFileBuffer, FILEPATH_BUFFER_SIZE);

        ImGui::Separator();

        // Save button
        if (ImGui::Button("Save", ImVec2(100, 0)))
        {
            std::string filepath(m_saveAsFileBuffer);
            if (!filepath.empty())
            {
                // Ensure .json extension (like legacy code)
                if (filepath.size() < 5 || filepath.substr(filepath.size() - 5) != ".json")
                    filepath += ".json";

                // Proceed with save
                OnSaveAsComplete(filepath);
                m_showSaveAsModal = false;
                m_saveAsPopupOpen = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        // Cancel button
        if (ImGui::Button("Cancel", ImVec2(100, 0)))
        {
            m_showSaveAsModal = false;
            m_saveAsPopupOpen = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    else
    {
        // Popup closed - reset state
        if (m_saveAsPopupOpen)
        {
            m_saveAsPopupOpen = false;
            m_showSaveAsModal = false;
        }
    }

    // ========== Browse Modal (unchanged) ==========
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
                        browseType = FilePickerType::SubGraph;
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
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::RenderButtons] ERROR: m_document is null!\n";
        return;
    }

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
    // Phase 44.4: Direct call to legacy Behavior Tree save without framework
    // NodeGraphPanel's SaveActiveGraph handles all the I/O logic
    // This ensures file persistence works while framework is incomplete

    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] ENTER - User clicked Save button\n";

    if (!m_document)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] ERROR: No document loaded!\n";
        return;
    }

    // Phase 50.1.3: Generalized save for all graph types (BehaviorTree, VisualScript, EntityPrefab)
    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] Document type: " << (int)m_document->GetType() << "\n";

    // Get the filepath from the document
    std::string currentPath = m_document->GetFilePath();
    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] Current filepath: '" << currentPath << "'\n";

    if (currentPath.empty())
    {
        // No path yet - need SaveAs
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] No path yet - redirecting to SaveAs\n";
        OnSaveAsClicked();
        return;
    }

    // Call the proven save logic (works for all types)
    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] About to call ExecuteSave()\n";
    try
    {
        if (ExecuteSave(currentPath))
        {
            SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] ✓ ExecuteSave returned true\n";
            SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] Calling TabManager::OnGraphDocumentSaved()\n";
            TabManager::Get().OnGraphDocumentSaved(m_document, currentPath);
            SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] Calling m_onSaveComplete callback\n";
            if (m_onSaveComplete)
            {
                m_onSaveComplete(currentPath);
                SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] Callback executed\n";
            }
            SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] EXIT - Save complete\n";
        }
        else
        {
            SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] ✗ ExecuteSave returned false!\n";
        }
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveClicked] EXCEPTION: " << e.what() << "\n";
    }
}

void CanvasToolbarRenderer::OnSaveAsClicked()
{
    // Phase 50.1.3: Generalized SaveAs for all graph types (BehaviorTree, VisualScript, EntityPrefab)
    // Shows modal and saves when confirmed

    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsClicked] ENTER - User clicked SaveAs button\n";

    if (!m_document)
    {
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsClicked] ERROR: No document loaded!\n";
        return;
    }

    // Open SaveAs dialog via framework (modal will be rendered separately)
    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsClicked] Setting m_showSaveAsModal = true\n";
    m_showSaveAsModal = true;
    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsClicked] EXIT - Modal flag set\n";
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

    SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] SaveAs path validated, about to call ExecuteSave()\n";

    if (ExecuteSave(filePath))
    {
        m_selectedFilePath = filePath;
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] ✓ ExecuteSave succeeded\n";
        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] Calling TabManager::OnGraphDocumentSaved()\n";

        // Phase 44.2.1: Notify TabManager to update UI (tab name, dirty flag)
        TabManager::Get().OnGraphDocumentSaved(m_document, filePath);

        SYSTEM_LOG << "[CanvasToolbarRenderer::OnSaveAsComplete] Calling m_onSaveComplete callback\n";
        if (m_onSaveComplete)
        {
            m_onSaveComplete(filePath);
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
