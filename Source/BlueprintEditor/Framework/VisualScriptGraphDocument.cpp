/**
 * @file VisualScriptGraphDocument.cpp
 * @brief Implementation of VisualScriptGraphDocument adapter.
 * @author Olympe Engine - Phase 41 Framework Integration
 * @date 2026-03-09
 */

#include "VisualScriptGraphDocument.h"
#include "../VisualScriptEditorPanel.h"
#include "../../system/system_utils.h"
#include "../../TaskSystem/TaskGraphLoader.h"

namespace Olympe
{
    VisualScriptGraphDocument::VisualScriptGraphDocument(VisualScriptEditorPanel* vsPanel)
        : m_vsPanel(vsPanel), m_filePath("")
    {
        if (m_vsPanel)
        {
            // Initialize with current path from panel
            m_filePath = m_vsPanel->GetCurrentPath();
        }
    }

    // =========================================================================
    // IGraphDocument Implementation
    // =========================================================================

    bool VisualScriptGraphDocument::Load(const std::string& filePath)
    {
        if (!m_vsPanel)
        {
            SYSTEM_LOG << "[VisualScriptGraphDocument::Load] ERROR: m_vsPanel is nullptr\n";
            return false;
        }

        try
        {
            SYSTEM_LOG << "[VisualScriptGraphDocument::Load] Loading from: " << filePath << "\n";

            // Load the template from file
            std::vector<std::string> errors;
            TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromFile(filePath, errors);
            if (!tmpl)
            {
                SYSTEM_LOG << "[VisualScriptGraphDocument::Load] ERROR: Failed to load template from " << filePath << "\n";
                for (const auto& err : errors)
                {
                    SYSTEM_LOG << "  - " << err << "\n";
                }
                return false;
            }

            // Load into the panel
            m_vsPanel->LoadTemplate(tmpl, filePath);

            // Clean up template (panel makes a copy in LoadTemplate)
            delete tmpl;

            // Update path
            m_filePath = filePath;

            SYSTEM_LOG << "[VisualScriptGraphDocument::Load] SUCCESS: Loaded " << filePath << "\n";
            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[VisualScriptGraphDocument::Load] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    bool VisualScriptGraphDocument::Save(const std::string& filePath)
    {
        if (!m_vsPanel)
        {
            SYSTEM_LOG << "[VisualScriptGraphDocument::Save] ERROR: m_vsPanel is nullptr\n";
            return false;
        }

        try
        {
            SYSTEM_LOG << "[VisualScriptGraphDocument::Save] Saving to: " << filePath << "\n";

            // Save using the panel's SaveAs method
            bool ok = m_vsPanel->SaveAs(filePath);

            if (ok)
            {
                // Update path
                m_filePath = filePath;
                SYSTEM_LOG << "[VisualScriptGraphDocument::Save] SUCCESS: Saved to " << filePath << "\n";
            }
            else
            {
                SYSTEM_LOG << "[VisualScriptGraphDocument::Save] ERROR: SaveAs failed for " << filePath << "\n";
            }

            return ok;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[VisualScriptGraphDocument::Save] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    std::string VisualScriptGraphDocument::GetName() const
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

        return "Untitled-VisualScript";
    }

    DocumentType VisualScriptGraphDocument::GetType() const
    {
        return DocumentType::VISUAL_SCRIPT;
    }

    std::string VisualScriptGraphDocument::GetFilePath() const
    {
        return m_filePath;
    }

    void VisualScriptGraphDocument::SetFilePath(const std::string& path)
    {
        m_filePath = path;
    }

    bool VisualScriptGraphDocument::IsDirty() const
    {
        if (!m_vsPanel)
        {
            return false;
        }

        return m_vsPanel->IsDirty();
    }

    IGraphRenderer* VisualScriptGraphDocument::GetRenderer()
    {
        // VisualScript uses ImNodes rendering directly in the panel
        // It doesn't use the separate IGraphRenderer interface like EntityPrefab
        // This method returns nullptr as rendering is handled by VisualScriptEditorPanel
        return nullptr;
    }

    const IGraphRenderer* VisualScriptGraphDocument::GetRenderer() const
    {
        // Const version - also returns nullptr
        return nullptr;
    }

    void VisualScriptGraphDocument::OnDocumentModified()
    {
        if (m_vsPanel)
        {
            // Force dirty flag
            // Note: VisualScriptEditorPanel doesn't have explicit SetDirty method
            // Modifications are automatically tracked when nodes change
            // This is a notification hook for future framework integration
            SYSTEM_LOG << "[VisualScriptGraphDocument::OnDocumentModified] Document marked as modified\n";
        }
    }

} // namespace Olympe
