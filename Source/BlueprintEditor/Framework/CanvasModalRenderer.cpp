/**
 * @file CanvasModalRenderer.cpp
 * @brief Implementation of centralized modal management for canvas editors.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * Phase 42 Framework Unification - Modal Layer Implementation
 *
 * Provides single rendering path for all graph editor modals to fix
 * BehaviorTree modal duplicate UI zones (was: DataManager rendering twice per frame).
 *
 * Phase 44: Consolidated SubGraphFilePickerModal → FilePickerModal (with folder tree)
 * This ensures all property panels show consistent UI with folder navigation.
 *
 * Delegates to FilePickerModal for actual modal logic.
 * This class provides unified interface and prevents duplicate rendering.
 *
 * C++14 compliant.
 */

#include "CanvasModalRenderer.h"

namespace Olympe {

// ============================================================================
// Singleton
// ============================================================================

CanvasModalRenderer& CanvasModalRenderer::Get()
{
    static CanvasModalRenderer instance;
    return instance;
}

CanvasModalRenderer::CanvasModalRenderer()
    : m_subGraphConfirmed(false),
      m_behaviorTreeConfirmed(false),
      m_saveConfirmed(false)
{
    // All modals initialized via member initializers
}

// ============================================================================
// SubGraph File Picker Modal - Public Interface
// ============================================================================

void CanvasModalRenderer::OpenSubGraphFilePickerModal(const std::string& initialPath)
{
    // Delegate to wrapped SubGraphFilePickerModal
    // This opens the modal for display in next Render() call
    m_subGraphModal.Open(initialPath);
    m_subGraphConfirmed = false;
    m_cachedSubGraphSelection = "";
}

void CanvasModalRenderer::RenderSubGraphFilePickerModal()
{
    // Single rendering point per frame
    // This ensures no duplicate modal zones (fixes BehaviorTree bug)
    m_subGraphModal.Render();

    // Cache confirmation state from wrapped modal
    // Clear m_subGraphConfirmed after one read to prevent multiple processing
    if (!m_subGraphConfirmed && m_subGraphModal.IsConfirmed())
    {
        m_subGraphConfirmed = true;
        m_cachedSubGraphSelection = m_subGraphModal.GetSelectedFile();
    }
}

bool CanvasModalRenderer::IsSubGraphModalConfirmed() const
{
    // Return cached confirmation state
    // This allows caller to check if modal was just confirmed
    return m_subGraphConfirmed;
}

std::string CanvasModalRenderer::GetSelectedSubGraphFile() const
{
    // Return cached selection from wrapped modal
    // Valid only after IsSubGraphModalConfirmed() returned true
    return m_cachedSubGraphSelection;
}

bool CanvasModalRenderer::IsSubGraphModalOpen() const
{
    // Query wrapped modal directly for current open state
    return m_subGraphModal.IsOpen();
}

void CanvasModalRenderer::CloseSubGraphModal()
{
    // Close wrapped modal without selection
    m_subGraphModal.Close();
    m_subGraphConfirmed = false;
    m_cachedSubGraphSelection = "";
}

// ============================================================================
// BehaviorTree File Picker Modal - Public Interface
// ============================================================================

void CanvasModalRenderer::OpenBehaviorTreeFilePickerModal(const std::string& initialPath)
{
    // Phase 44.1: Unified from PropertyEditorPanel::m_behaviorTreeModal
    m_behaviorTreeModal.Open(initialPath);
    m_behaviorTreeConfirmed = false;
    m_cachedBehaviorTreeSelection = "";
}

void CanvasModalRenderer::RenderBehaviorTreeFilePickerModal()
{
    // Single rendering point per frame
    m_behaviorTreeModal.Render();

    // Cache confirmation state
    if (!m_behaviorTreeConfirmed && m_behaviorTreeModal.IsConfirmed())
    {
        m_behaviorTreeConfirmed = true;
        m_cachedBehaviorTreeSelection = m_behaviorTreeModal.GetSelectedFile();
    }
}

bool CanvasModalRenderer::IsBehaviorTreeModalConfirmed() const
{
    return m_behaviorTreeConfirmed;
}

std::string CanvasModalRenderer::GetSelectedBehaviorTreeFile() const
{
    return m_cachedBehaviorTreeSelection;
}

bool CanvasModalRenderer::IsBehaviorTreeModalOpen() const
{
    return m_behaviorTreeModal.IsOpen();
}

void CanvasModalRenderer::CloseBehaviorTreeModal()
{
    m_behaviorTreeModal.Close();
    m_behaviorTreeConfirmed = false;
    m_cachedBehaviorTreeSelection = "";
}

// ============================================================================
// Save File Picker Modal - Public Interface
// ============================================================================

void CanvasModalRenderer::OpenSaveFilePickerModal(const std::string& initialPath,
                                                   const std::string& suggestedName,
                                                   SaveFileType fileType)
{
    // Phase 44.1: Unified from CanvasToolbarRenderer::m_saveModal
    // Note: fileType parameter provided for future use or caller context
    // The modal instance (m_saveFileModal) is initialized with SaveFileType::Blueprint
    m_saveFileModal.Open(initialPath, suggestedName);
    m_saveConfirmed = false;
    m_cachedSaveSelection = "";
}

void CanvasModalRenderer::RenderSaveFilePickerModal()
{
    // Single rendering point per frame
    m_saveFileModal.Render();

    // Cache confirmation state
    if (!m_saveConfirmed && m_saveFileModal.IsConfirmed())
    {
        m_saveConfirmed = true;
        m_cachedSaveSelection = m_saveFileModal.GetSelectedFile();
    }
}

bool CanvasModalRenderer::IsSaveFileModalConfirmed() const
{
    return m_saveConfirmed;
}

std::string CanvasModalRenderer::GetSelectedSaveFilePath() const
{
    return m_cachedSaveSelection;
}

bool CanvasModalRenderer::IsSaveFileModalOpen() const
{
    return m_saveFileModal.IsOpen();
}

void CanvasModalRenderer::CloseSaveFileModal()
{
    m_saveFileModal.Close();
    m_saveConfirmed = false;
    m_cachedSaveSelection = "";
}

} // namespace Olympe
