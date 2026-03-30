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

    nlohmann::json fileJson;
    {
        std::ifstream ifs(path.c_str());
        if (!ifs.good())
        {
            SYSTEM_LOG << "[VisualScriptRenderer] Cannot open file: " << path << "\n";
            return false;
        }
        try
        {
            ifs >> fileJson;
        }
        catch (...)
        {
            SYSTEM_LOG << "[VisualScriptRenderer] JSON parse error: " << path << "\n";
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
            SYSTEM_LOG << "[VisualScriptRenderer] Failed to parse VS v4 graph: " << path << "\n";
            return false;
        }
        m_panel.LoadTemplate(tmpl, path);
        delete tmpl;
        SYSTEM_LOG << "[VisualScriptRenderer] Loaded VS v4 graph: " << path << "\n";
        return true;
    }

    // Legacy BT v2 — auto-migrate to VS v4
    if (blueprintType == "BehaviorTree")
    {
        std::vector<std::string> errors;
        TaskGraphTemplate converted = BTtoVSMigrator::Convert(fileJson, errors);
        m_panel.LoadTemplate(&converted, path);
        SYSTEM_LOG << "[VisualScriptRenderer] Auto-migrated BT v2 -> VS v4: " << path << "\n";
        return true;
    }

    SYSTEM_LOG << "[VisualScriptRenderer] Unknown graph format in: " << path << "\n";
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

} // namespace Olympe
