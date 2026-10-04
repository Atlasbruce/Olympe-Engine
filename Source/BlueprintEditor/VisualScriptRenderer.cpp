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
    m_savedCanvasState.panX = 0.0f;
    m_savedCanvasState.panY = 0.0f;
    m_savedCanvasState.zoom = 1.0f;

    m_panel.Initialize();
    // Phase 55: Bind this renderer wrapper to the document adapter
    // so that the CanvasToolbarRenderer can query our minimap / verify / simulation capabilities
    if (m_panel.m_document)
    {
        m_panel.m_document->SetRenderer(this);
    }
}

VisualScriptRenderer::~VisualScriptRenderer()
{
    m_panel.Shutdown();
}

IGraphDocument* VisualScriptRenderer::GetDocument() const
{
    // Phase 44.2: Return the document adapter from the wrapped panel
    // This allows TabManager to reuse the same document instance
    // instead of creating a new VisualScriptGraphDocument wrapper
    return m_panel.m_document.get();
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

        // Phase 50.1.5: CRITICAL - Sync filepath to framework document
        // This ensures CanvasToolbarRenderer sees the loaded filepath
        // so Save button works directly (no SaveAs modal)
        if (m_panel.m_document)
        {
            m_panel.m_document->SetFilePath(resolvedPath);
            SYSTEM_LOG << "[VisualScriptRenderer] Synced filepath to document: " << resolvedPath << "\n";
        }

        SYSTEM_LOG << "[VisualScriptRenderer] Loaded VS v4 graph: " << resolvedPath << "\n";
        return true;
    }

    // Legacy BT v2 — auto-migrate to VS v4
    if (blueprintType == "BehaviorTree")
    {
        std::vector<std::string> errors;
        TaskGraphTemplate converted = BTtoVSMigrator::Convert(fileJson, errors);
        m_panel.LoadTemplate(&converted, resolvedPath);

        // Phase 50.1.5: Sync filepath to document for legacy BT v2 migration path
        if (m_panel.m_document)
        {
            m_panel.m_document->SetFilePath(resolvedPath);
            SYSTEM_LOG << "[VisualScriptRenderer] Synced filepath to document (BT v2 migration): " << resolvedPath << "\n";
        }

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
    if (!m_panel.m_canvasEditor)
        return;

    const ImVec2 pan = m_panel.m_canvasEditor->GetPan();
    m_savedCanvasState.panX = pan.x;
    m_savedCanvasState.panY = pan.y;
    m_savedCanvasState.zoom = m_panel.m_canvasEditor->GetZoom();
}

void VisualScriptRenderer::RestoreCanvasState()
{
    if (!m_panel.m_canvasEditor)
        return;

    m_panel.m_canvasEditor->SetZoom(m_savedCanvasState.zoom);
    m_panel.m_canvasEditor->SetPan(
        ImVec2(m_savedCanvasState.panX, m_savedCanvasState.panY));
}

std::string VisualScriptRenderer::GetCanvasStateJSON() const
{
    if (!m_panel.m_canvasEditor)
        return "{}";

    const ImVec2 pan = m_panel.m_canvasEditor->GetPan();
    nlohmann::json state;
    state["panX"] = pan.x;
    state["panY"] = pan.y;
    state["zoom"] = m_panel.m_canvasEditor->GetZoom();
    return state.dump();
}

void VisualScriptRenderer::SetCanvasStateJSON(const std::string& json)
{
    if (!m_panel.m_canvasEditor || json.empty())
        return;

    try
    {
        const nlohmann::json state = nlohmann::json::parse(json);
        if (!state.is_object())
            return;

        const float panX = state.value("panX", 0.0f);
        const float panY = state.value("panY", 0.0f);
        const float zoom = state.value("zoom", 1.0f);

        m_savedCanvasState.panX = panX;
        m_savedCanvasState.panY = panY;
        m_savedCanvasState.zoom = zoom;
        RestoreCanvasState();
    }
    catch (const std::exception&)
    {
        SYSTEM_LOG << "[VisualScriptRenderer] Ignored invalid canvas state JSON\n";
    }
}

void VisualScriptRenderer::SetMinimapVisible(bool visible)
{
    m_panel.m_minimapVisible = visible;
    if (m_panel.m_canvasEditor)
        m_panel.m_canvasEditor->SetMinimapVisible(visible);
}

void VisualScriptRenderer::SetMinimapSize(float size)
{
    m_panel.m_minimapSize = size;
    if (m_panel.m_canvasEditor)
        m_panel.m_canvasEditor->SetMinimapSize(size);
}

void VisualScriptRenderer::SetMinimapPosition(int pos)
{
    m_panel.m_minimapPosition = pos;
    if (m_panel.m_canvasEditor)
        m_panel.m_canvasEditor->SetMinimapPosition(pos);
}

bool VisualScriptRenderer::IsGridVisible() const
{
    return m_panel.m_canvasEditor && m_panel.m_canvasEditor->IsGridVisible();
}

void VisualScriptRenderer::SetGridVisible(bool visible)
{
    if (m_panel.m_canvasEditor)
        m_panel.m_canvasEditor->SetGridVisible(visible);
}

void VisualScriptRenderer::ResetView()
{
    if (m_panel.m_canvasEditor)
        m_panel.m_canvasEditor->ResetView();
}

void VisualScriptRenderer::RenderFrameworkModals()
{
    // Phase 43: Render toolbar modals through the panel's framework
    // The panel coordinates with CanvasFramework which has the toolbar
    // that needs Save/SaveAs/Browse modal rendering
    m_panel.RenderFrameworkModals();
}

} // namespace Olympe
