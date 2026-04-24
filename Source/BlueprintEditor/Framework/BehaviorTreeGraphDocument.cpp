/**
 * @file BehaviorTreeGraphDocument.cpp
 * @brief Implementation of BehaviorTreeGraphDocument adapter.
 * @author Olympe Engine - Phase 44.3 Integration (Legacy Save/SaveAs into Framework)
 * @date 2026-03-11
 *
 * Phase 44.3 Update: Modified to use NodeGraphManager directly for persistence.
 * BehaviorTree graphs are managed by NodeGraphManager, so we delegate all
 * save/load operations to it directly instead of going through BehaviorTreeRenderer.
 */

#include "BehaviorTreeGraphDocument.h"
#include "../BehaviorTreeRenderer.h"
#include "../NodeGraphPanel.h"
#include "../BTNodeGraphManager.h"
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

            // Phase 44.3: Load using NodeGraphManager directly
            int graphId = NodeGraphManager::Get().LoadGraph(filePath);

            if (graphId >= 0)
            {
                // Update path
                m_filePath = filePath;
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] SUCCESS: Loaded graph ID " << graphId << " from " << filePath << "\n";
                return true;
            }
            else
            {
                SYSTEM_LOG << "[BehaviorTreeGraphDocument::Load] ERROR: Failed to load " << filePath << "\n";
                return false;
            }
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
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] TODO: Phase 50.4 - Reimplement with modern GraphDocument\n";
            SYSTEM_LOG << "[BehaviorTreeGraphDocument::Save] NodeGraph class removed in Phase 50.3 namespace collision fix\n";
            return false;
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
