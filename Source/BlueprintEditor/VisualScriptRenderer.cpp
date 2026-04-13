/**
 * @file VisualScriptRenderer.cpp
 * @brief IGraphRenderer adapter that wraps VisualScriptEditorPanel.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details C++14 compliant.
 */

#include "VisualScriptRenderer.h"

#include "../TaskSystem/TaskGraphLoader.h"
#include "BTtoVSMigrator.h"
#include "../third_party/nlohmann/json.hpp"
#include "../system/system_utils.h"
#include "../DataManager.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

namespace Olympe {

VisualScriptRenderer::VisualScriptRenderer()
{
    m_panel.Initialize();
}

VisualScriptRenderer::~VisualScriptRenderer()
{
    m_panel.Shutdown();
}

void VisualScriptRenderer::Render()
{
    m_panel.RenderContent();
}

bool VisualScriptRenderer::Load(const std::string& path)
{
    if (path.empty())
        return false;

    // Phase 38: Resolve relative paths using DataManager
    std::string resolvedPath = ResolvePath(path);

    nlohmann::json fileJson;
    {
        std::ifstream ifs(resolvedPath.c_str());
        if (!ifs.good())
        {
            SYSTEM_LOG << "[VisualScriptRenderer] Cannot open file: " << resolvedPath << "\n";
            return false;
        }
        try
        {
            ifs >> fileJson;
        }
        catch (...)
        {
            SYSTEM_LOG << "[VisualScriptRenderer] JSON parse error: " << resolvedPath << "\n";
            return false;
        }
    }

    if (!fileJson.is_object())
        return false;

    int schemaVersion = 0;
    std::string graphType;
    std::string blueprintType;

    if (fileJson.contains("schema_version") && fileJson["schema_version"].is_number())
        schemaVersion = fileJson["schema_version"].get<int>();
    if (fileJson.contains("graphType") && fileJson["graphType"].is_string())
        graphType = fileJson["graphType"].get<std::string>();
    if (fileJson.contains("blueprintType") && fileJson["blueprintType"].is_string())
        blueprintType = fileJson["blueprintType"].get<std::string>();

    // VS v4 graph
    if (schemaVersion == 4 && graphType == "VisualScript")
    {
        std::vector<std::string> errors;
        TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromJson(fileJson, errors);
        if (!tmpl)
        {
            SYSTEM_LOG << "[VisualScriptRenderer] Failed to parse VS v4 graph: " << resolvedPath << "\n";
            return false;
        }
        m_panel.LoadTemplate(tmpl, resolvedPath);
        delete tmpl;
        SYSTEM_LOG << "[VisualScriptRenderer] Loaded VS v4 graph: " << resolvedPath << "\n";
        return true;
    }

    // Legacy BT v2 — auto-migrate to VS v4
    if (blueprintType == "BehaviorTree")
    {
        std::vector<std::string> errors;
        TaskGraphTemplate converted = BTtoVSMigrator::Convert(fileJson, errors);
        m_panel.LoadTemplate(&converted, resolvedPath);
        SYSTEM_LOG << "[VisualScriptRenderer] Auto-migrated BT v2 -> VS v4: " << resolvedPath << "\n";
        return true;
    }

    SYSTEM_LOG << "[VisualScriptRenderer] Unknown graph format in: " << resolvedPath << "\n";
    return false;
}

bool VisualScriptRenderer::Save(const std::string& path)
{
    if (!path.empty())
        return m_panel.SaveAs(path);
    return m_panel.Save();
}

bool VisualScriptRenderer::IsDirty() const
{
    return m_panel.IsDirty();
}

std::string VisualScriptRenderer::GetGraphType() const
{
    return "VisualScript";
}

std::string VisualScriptRenderer::GetCurrentPath() const
{
    return m_panel.GetCurrentPath();
}

// Phase 38: Path resolution using DataManager enhanced resolver
std::string VisualScriptRenderer::ResolvePath(const std::string& path) const
{
    // Use DataManager's robust path resolution
    std::string resolved = DataManager::Get().ResolveFilePath(path);

    if (resolved.empty())
    {
        SYSTEM_LOG << "[VisualScriptRenderer] Warning: Could not resolve path: " << path << "\n";
        return path;  // Return original, will fail gracefully in Load()
    }

    return resolved;
}

// Phase 35.0: Canvas state management
void VisualScriptRenderer::SaveCanvasState()
{
    // Phase 35.0: VisualScript uses imnodes native pan/zoom which is context-global
    // For now, viewport state is preserved via ImNodes::EditorContext
    // This stub ensures compatibility with the IGraphRenderer interface
}

void VisualScriptRenderer::RestoreCanvasState()
{
    // Phase 35.0: VisualScript pan/zoom restoration (handled by ImNodes context)
    // This stub ensures compatibility with the IGraphRenderer interface
}

std::string VisualScriptRenderer::GetCanvasStateJSON() const
{
    // Return empty for now - can be extended to persist canvas state in JSON files
    return "";
}

void VisualScriptRenderer::SetCanvasStateJSON(const std::string& json)
{
    // Parse and restore from JSON - can be extended for persistence
    (void)json;
}

void VisualScriptRenderer::RenderFrameworkModals()
{
    // Phase 43: Render toolbar modals through the panel's framework
    // The panel coordinates with CanvasFramework which has the toolbar
    // that needs Save/SaveAs/Browse modal rendering
    m_panel.RenderFrameworkModals();
}

} // namespace Olympe
