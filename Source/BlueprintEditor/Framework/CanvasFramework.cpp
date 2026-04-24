/**
 * @file CanvasFramework.cpp
 * @brief Implementation of unified canvas framework orchestrator (Phase 41)
 * @author Olympe Engine
 * @date 2026-03-25
 */

#include "CanvasFramework.h"
#include "IGraphDocument.h"
#include "CanvasToolbarRenderer.h"
#include "../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Constructor / Destructor
// ============================================================================

CanvasFramework::CanvasFramework(IGraphDocument* document)
    : m_document(document)
    , m_toolbar(nullptr)
{
    SYSTEM_LOG << "[CanvasFramework] Created\n";
    
    if (m_document)
    {
        InitializeToolbar();
        SYSTEM_LOG << "[CanvasFramework] Initialized for document: "
                   << m_document->GetName() << "\n";
    }
    else
    {
        SYSTEM_LOG << "[CanvasFramework] WARNING: Constructed with null document\n";
    }
}

CanvasFramework::~CanvasFramework()
{
    if (m_toolbar)
    {
        delete m_toolbar;
        m_toolbar = nullptr;
    }
    SYSTEM_LOG << "[CanvasFramework] Destroyed\n";
}

// ============================================================================
// Lifecycle Methods
// ============================================================================

bool CanvasFramework::HasDocument() const
{
    return m_document != nullptr;
}

void CanvasFramework::SetDocument(IGraphDocument* document)
{
    if (m_document == document)
        return;

    SYSTEM_LOG << "[CanvasFramework] Switching document\n";

    m_document = document;

    // Reinitialize toolbar for new document
    if (m_toolbar)
    {
        delete m_toolbar;
        m_toolbar = nullptr;
    }

    if (m_document)
    {
        InitializeToolbar();
        SYSTEM_LOG << "[CanvasFramework] Document switched to: "
                   << m_document->GetName() << "\n";
    }
    else
    {
        SYSTEM_LOG << "[CanvasFramework] Document cleared\n";
    }
}

IGraphDocument* CanvasFramework::GetDocument() const
{
    return m_document;
}

// ============================================================================
// Rendering Methods
// ============================================================================

void CanvasFramework::Render()
{
    if (!m_document)
    {
        ImGui::Text("No document loaded");
        return;
    }

    // Render toolbar at top
    RenderToolbar();

    // Render main canvas area
    ImGui::Spacing();
    RenderCanvas();

    // Render modals (overlays)
    RenderModals();

    // Render status bar if enabled
    if (m_showStatusBar)
    {
        ImGui::Spacing();
        RenderStatusBar();
    }
}

void CanvasFramework::RenderToolbar()
{
    if (!m_toolbar || !m_document)
        return;

    m_toolbar->Render();
}

void CanvasFramework::RenderCanvas()
{
    if (!m_document)
        return;

    IGraphRenderer* renderer = m_document->GetRenderer();
    if (!renderer)
    {
        ImGui::TextWrapped("Renderer not available for this document type");
        return;
    }

    // TODO: Call renderer's Render() method
    // Note: Specific renderer rendering depends on document type
    // EntityPrefab: renderer->Render()
    // VisualScript: imnodes rendering (different pattern)
    // BehaviorTree: imnodes rendering (different pattern)
    
    SYSTEM_LOG << "[CanvasFramework] Canvas rendering delegated to document renderer\n";
}

void CanvasFramework::RenderModals()
{
    if (!m_toolbar)
    {
        SYSTEM_LOG << "[CanvasFramework::RenderModals] ERROR: m_toolbar is null!\n";
        return;
    }

    //SYSTEM_LOG << "[CanvasFramework::RenderModals] Delegating to toolbar->RenderModals()\n";
    m_toolbar->RenderModals();
}

void CanvasFramework::SetCanvasLayout(float x, float y, float width, float height)
{
    m_canvasX = x;
    m_canvasY = y;
    m_canvasWidth = width;
    m_canvasHeight = height;
    m_customCanvasLayout = true;

    SYSTEM_LOG << "[CanvasFramework] Canvas layout set: ("
               << x << ", " << y << ") " << width << "x" << height << "\n";
}

// ============================================================================
// Document Operations
// ============================================================================

bool CanvasFramework::SaveDocument(const std::string& filePath)
{
    if (!m_document)
    {
        SetStatusMessage("Error: No document to save", 3.0f);
        SYSTEM_LOG << "[CanvasFramework] ERROR: Cannot save - no document\n";
        return false;
    }

    bool success = m_document->Save(filePath);

    if (success)
    {
        SetStatusMessage("Document saved: " + filePath, 3.0f);
        SYSTEM_LOG << "[CanvasFramework] Successfully saved document\n";
    }
    else
    {
        SetStatusMessage("Error: Failed to save document", 3.0f);
        SYSTEM_LOG << "[CanvasFramework] ERROR: Save failed\n";
    }

    return success;
}

bool CanvasFramework::LoadDocument(const std::string& filePath)
{
    if (!m_document)
    {
        SetStatusMessage("Error: No document to load", 3.0f);
        SYSTEM_LOG << "[CanvasFramework] ERROR: Cannot load - no document context\n";
        return false;
    }

    bool success = m_document->Load(filePath);

    if (success)
    {
        SetStatusMessage("Document loaded: " + filePath, 3.0f);
        SYSTEM_LOG << "[CanvasFramework] Successfully loaded document\n";
    }
    else
    {
        SetStatusMessage("Error: Failed to load document", 3.0f);
        SYSTEM_LOG << "[CanvasFramework] ERROR: Load failed\n";
    }

    return success;
}

bool CanvasFramework::IsDirty() const
{
    if (!m_document)
        return false;

    return m_document->IsDirty();
}

std::string CanvasFramework::GetCurrentPath() const
{
    if (!m_document)
        return "";

    return m_document->GetFilePath();
}

std::string CanvasFramework::GetDocumentName() const
{
    if (!m_document)
        return "No document";

    return m_document->GetName();
}

std::string CanvasFramework::GetDocumentTypeName() const
{
    if (!m_document)
        return "Unknown";

    return m_document->GetTypeName();
}

// ========== MODAL STATE ==========

bool CanvasFramework::IsModalOpen() const
{
    if (!m_toolbar)
        return false;

    return m_toolbar->IsModalOpen();
}

void CanvasFramework::OpenSaveAsDialog()
{
    if (!m_toolbar)
    {
        SYSTEM_LOG << "[CanvasFramework] ERROR: No toolbar to open SaveAs dialog\n";
        return;
    }

    SYSTEM_LOG << "[CanvasFramework] Opening SaveAs dialog\n";
    m_toolbar->SetOnSaveComplete(
        std::bind(&CanvasFramework::OnToolbarSaveComplete, this, std::placeholders::_1)
    );
    // Trigger SaveAs through toolbar (would need public method for this)
}

void CanvasFramework::OpenBrowseDialog()
{
    if (!m_toolbar)
    {
        SYSTEM_LOG << "[CanvasFramework] ERROR: No toolbar to open Browse dialog\n";
        return;
    }

    SYSTEM_LOG << "[CanvasFramework] Opening Browse dialog\n";
    m_toolbar->SetOnBrowseComplete(
        std::bind(&CanvasFramework::OnToolbarBrowseComplete, this, std::placeholders::_1)
    );
    // Trigger Browse through toolbar (would need public method for this)
}

// ========== FEATURE QUERIES ==========

CanvasToolbarRenderer* CanvasFramework::GetToolbar()
{
    return m_toolbar;
}

const CanvasToolbarRenderer* CanvasFramework::GetToolbar() const
{
    return m_toolbar;
}

IGraphRenderer* CanvasFramework::GetCanvasRenderer()
{
    if (!m_document)
        return nullptr;

    return m_document->GetRenderer();
}

const IGraphRenderer* CanvasFramework::GetCanvasRenderer() const
{
    if (!m_document)
        return nullptr;

    return m_document->GetRenderer();
}

// ============================================================================
// Private Methods
// ============================================================================

void CanvasFramework::InitializeToolbar()
{
    if (!m_document)
        return;

    if (m_toolbar)
    {
        delete m_toolbar;
    }

    m_toolbar = new CanvasToolbarRenderer(m_document);

    // Setup callbacks
    m_toolbar->SetOnSaveComplete(
        std::bind(&CanvasFramework::OnToolbarSaveComplete, this, std::placeholders::_1)
    );
    m_toolbar->SetOnBrowseComplete(
        std::bind(&CanvasFramework::OnToolbarBrowseComplete, this, std::placeholders::_1)
    );

    SYSTEM_LOG << "[CanvasFramework] Toolbar initialized\n";
}

void CanvasFramework::OnToolbarSaveComplete(const std::string& filePath)
{
    SYSTEM_LOG << "[CanvasFramework] Toolbar save completed: " << filePath << "\n";
    SetStatusMessage("Saved: " + filePath, 3.0f);
}

void CanvasFramework::OnToolbarBrowseComplete(const std::string& filePath)
{
    SYSTEM_LOG << "[CanvasFramework] Toolbar browse completed: " << filePath << "\n";
    
    // Load the selected file
    if (m_document && m_document->Load(filePath))
    {
        SetStatusMessage("Loaded: " + filePath, 3.0f);
        SYSTEM_LOG << "[CanvasFramework] Document loaded successfully\n";
    }
    else
    {
        SetStatusMessage("Error: Failed to load file", 3.0f);
        SYSTEM_LOG << "[CanvasFramework] ERROR: Failed to load file\n";
    }
}

void CanvasFramework::RenderStatusBar()
{
    ImGui::Separator();
    
    // Display status message if active
    if (m_statusMessageTimeout > 0.0f)
    {
        ImGui::TextDisabled("[Status] %s", m_statusMessage.c_str());
        m_statusMessageTimeout -= ImGui::GetIO().DeltaTime;
    }
    else
    {
        // Display document state
        ImGui::TextDisabled("[Info] Document: %s | Type: %s | Status: %s",
                           GetDocumentName().c_str(),
                           GetDocumentTypeName().c_str(),
                           IsDirty() ? "Modified" : "Saved");
    }
}

void CanvasFramework::SetStatusMessage(const std::string& message, float durationSeconds)
{
    m_statusMessage = message;
    m_statusMessageTimeout = durationSeconds;

    SYSTEM_LOG << "[CanvasFramework] Status: " << message << "\n";
}

} // namespace Olympe
