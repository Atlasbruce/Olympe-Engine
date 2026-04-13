/**
 * @file BehaviorTreeGraphDocument.cpp
 * @brief Implementation of BehaviorTreeGraphDocument adapter.
 * @author Olympe Engine - Phase 41 Framework Integration
 * @date 2026-03-09
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
            // Initialize with current path from renderer
            m_filePath = m_btRenderer->GetCurrentPath();
        }
    }

    // =========================================================================
    // IGraphDocument Implementation
    // =========================================================================

    bool BehaviorTreeGraphDocument::Load(const std::string& filePath)
    {
        if (!m_btRenderer)
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] ERROR: m_btRenderer is nullptr\n";
            return false;
        }

        try
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] Loading from: " << filePath << "\n";

            // Load using the renderer's Load method
            bool ok = m_btRenderer->Load(filePath);

            if (ok)
            {
                // Update path
                m_filePath = filePath;
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] SUCCESS: Loaded " << filePath << "\n";
            }
            else
            {
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] ERROR: Failed to load " << filePath << "\n";
            }

            return ok;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    bool BehaviorTreeGraphDocument::Save(const std::string& filePath)
    {
        if (!m_btRenderer)
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ERROR: m_btRenderer is nullptr\n";
            return false;
        }

        try
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] Saving to: " << filePath << "\n";

            // Save using the renderer's Save method
            bool ok = m_btRenderer->Save(filePath);

            if (ok)
            {
                // Update path
                m_filePath = filePath;
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] SUCCESS: Saved to " << filePath << "\n";
            }
            else
            {
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] ERROR: Failed to save " << filePath << "\n";
            }

            return ok;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] EXCEPTION: " << e.what() << "\n";
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
        // BehaviorTree uses imnodes rendering directly in the renderer
        // It doesn't use the separate IGraphRenderer interface like EntityPrefab
        // This method returns nullptr as rendering is handled by BehaviorTreeRenderer
        return nullptr;
    }

    const IGraphRenderer* BehaviorTreeGraphDocument::GetRenderer() const
    {
        // Const version - also returns nullptr
        return nullptr;
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
