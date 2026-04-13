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
 * Delegates to SubGraphFilePickerModal for actual modal logic.
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
    : m_subGraphConfirmed(false)
{
    // m_subGraphModal initialized via default constructor of SubGraphFilePickerModal
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
    m_cachedSelection = "";
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
        m_cachedSelection = m_subGraphModal.GetSelectedFile();
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
    return m_cachedSelection;
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
    m_cachedSelection = "";
}

} // namespace Olympe
