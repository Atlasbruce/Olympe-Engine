/**
 * @file BehaviorTreeGraphDocument.cpp
 * @brief Implementation of BehaviorTreeGraphDocument adapter.
 * @author Olympe Engine - Phase 44.3 Integration (Legacy Save/SaveAs into Framework)
 * @date 2026-03-11
 *
 * The renderer remains the single owner of the active graph identity.  The
 * Framework document delegates persistence through it so Browse, Save and
 * Save As all leave the renderer, document adapter and TabManager synchronized.
 */

#include "BehaviorTreeGraphDocument.h"
#include "../BehaviorTreeRenderer.h"
#include "../../system/system_utils.h"

namespace Olympe
{
    BehaviorTreeGraphDocument::BehaviorTreeGraphDocument(BehaviorTreeRenderer* btRenderer)
        : m_btRenderer(btRenderer), m_filePath("")
    {
        if (m_btRenderer)
        {
            // Initialize with current path from renderer if available
            m_filePath = m_btRenderer->GetCurrentPath();
        }
    }

    // =========================================================================
    // IGraphDocument Implementation
    // =========================================================================

    bool BehaviorTreeGraphDocument::Load(const std::string& filePath)
    {
        try
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] Loading from: " << filePath << "\n";

            if (!m_btRenderer)
            {
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] ERROR: No renderer bound\n";
                return false;
            }

            // Do not load the backend directly here: BehaviorTreeRenderer::Load
            // also updates its active graph id, ImNodes adapter and canvas state.
            const bool success = m_btRenderer->Load(filePath);
            if (success)
            {
                m_filePath = filePath;
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] SUCCESS: Loaded " << filePath << "\n";
            }
            return success;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    bool BehaviorTreeGraphDocument::Save(const std::string& filePath)
    {
        try
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ============ SAVE START ============\n";
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] Target filepath: '" << filePath << "'\n";
            if (!m_btRenderer)
            {
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ERROR: No m_btRenderer bound, cannot save.\n";
                return false;
            }

            // Save using our bound BehaviorTreeRenderer 
            bool success = m_btRenderer->Save(filePath);
            if (success)
            {
                m_filePath = filePath;
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] SUCCESS: Saved graph to " << filePath << "\n";
            }
            else
            {
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ERROR: m_btRenderer->Save() returned false!\n";
            }
            return success;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ✗ EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    std::string BehaviorTreeGraphDocument::GetName() const
    {
        if (!m_filePath.empty())
        {
            // Extract filename from path
            size_t lastSlash = m_filePath.find_last_of("/\\");
            if (lastSlash != std::string::npos)
            {
                return m_filePath.substr(lastSlash + 1);
            }
            return m_filePath;
        }

        return "Untitled-BehaviorTree";
    }

    DocumentType BehaviorTreeGraphDocument::GetType() const
    {
        return DocumentType::BEHAVIOR_TREE;
    }

    std::string BehaviorTreeGraphDocument::GetFilePath() const
    {
        return m_filePath;
    }

    void BehaviorTreeGraphDocument::SetFilePath(const std::string& path)
    {
        m_filePath = path;
    }

    bool BehaviorTreeGraphDocument::IsDirty() const
    {
        if (!m_btRenderer)
        {
            return false;
        }

        return m_btRenderer->IsDirty();
    }

    IGraphRenderer* BehaviorTreeGraphDocument::GetRenderer()
    {
        return m_renderer;
    }

    const IGraphRenderer* BehaviorTreeGraphDocument::GetRenderer() const
    {
        return m_renderer;
    }

    void BehaviorTreeGraphDocument::OnDocumentModified()
    {
        if (m_btRenderer)
        {
            // Notification hook for future framework integration
            // BehaviorTree dirty flag is managed by the renderer/graph system
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::OnDocumentModified] Document marked as modified\n";
        }
    }

} // namespace Olympe
