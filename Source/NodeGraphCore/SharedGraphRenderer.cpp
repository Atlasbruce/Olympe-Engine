/**
 * @file SharedGraphRenderer.cpp
 * @brief Shared renderer for BlueprintEditor and RuntimeDebugger.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * This file provides the SharedGraphRenderer implementation.
 * When compiled as part of a build that includes ImGui and ImNodes
 * (e.g. OlympeCore linked into BlueprintEditor or RuntimeEngine), the render
 * methods issue real draw calls.
 *
 * In headless builds (unit tests, server processes) ImGui is not present; the
 * methods compile to empty stubs via the OLYMPE_BLUEPRINT_EDITOR_ENABLED guard.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "SharedGraphRenderer.h"

#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
// Full ImGui / ImNodes rendering available.
// GraphDocument, EditorContext, LocalBlackboard headers are only available in
// editor builds that link OlympeCore.
#include "GraphDocument.h"
#include "EditorContext.h"
#include "../TaskSystem/LocalBlackboard.h"

#include "imgui.h"
#include "imnodes.h"
#endif

namespace Olympe {
namespace NodeGraphTypes {

// ============================================================================
// Config factories
// ============================================================================

SharedGraphRenderer::Config SharedGraphRenderer::MakeEditorConfig()
{
    Config cfg;
    cfg.readOnly             = false;
    cfg.showRuntimeHighlight = false;
    cfg.showBlackboardPanel  = true;
    cfg.showToolbar          = true;
    return cfg;
}

SharedGraphRenderer::Config SharedGraphRenderer::MakeDebuggerConfig()
{
    Config cfg;
    cfg.readOnly             = true;
    cfg.showRuntimeHighlight = true;
    cfg.showBlackboardPanel  = true;
    cfg.showToolbar          = false;
    return cfg;
}

// ============================================================================
// Render (main entry point)
// ============================================================================

void SharedGraphRenderer::Render(GraphDocument*       doc,
                                  const EditorContext& ctx,
                                  const Config&        cfg,
                                  int                  activeNodeID)
{
    if (!doc) return;

#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED

    if (cfg.showToolbar)
    {
        RenderToolbar(doc, ctx);
    }

    ImNodes::BeginNodeEditor();

    // Render all nodes.
    const auto& nodes = doc->GetNodes();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        RenderNode(nodes[i], cfg, activeNodeID);
    }

    // Render all links.
    const auto& links = doc->GetLinks();
    for (size_t i = 0; i < links.size(); ++i)
    {
        RenderLink(links[i]);
    }

    ImNodes::EndNodeEditor();

    if (cfg.showBlackboardPanel)
    {
        // Blackboard panel is rendered separately; caller passes bb explicitly
        // via RenderBlackboardPanel().
    }

#else
    // Headless / no-ImGui build: no-op.
    (void)ctx;
    (void)cfg;
    (void)activeNodeID;
#endif
}

// ============================================================================
// RenderBlackboardPanel
// ============================================================================

void SharedGraphRenderer::RenderBlackboardPanel(const LocalBlackboard* bb)
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    if (!ImGui::Begin("Blackboard"))
    {
        ImGui::End();
        return;
    }

    if (!bb)
    {
        ImGui::TextDisabled("(no blackboard)");
        ImGui::End();
        return;
    }

    const auto& names = bb->GetVariableNames();
    for (size_t i = 0; i < names.size(); ++i)
    {
        try
        {
            const Olympe::TaskValue val = bb->GetValue(names[i]);
            ImGui::Text("%s", names[i].c_str());
        }
        catch (...)
        {
            ImGui::TextDisabled("%s (error)", names[i].c_str());
        }
    }

    ImGui::End();
#else
    (void)bb;
#endif
}

// ============================================================================
// Private: RenderNode
// ============================================================================

void SharedGraphRenderer::RenderNode(const NodeData& node,
                                      const Config&   cfg,
                                      int             activeNodeID)
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    const bool isActive = (node.id.value == activeNodeID);

    if (cfg.showRuntimeHighlight && isActive)
    {
        ImNodes::PushColorStyle(ImNodesCol_NodeBackground,
                                IM_COL32(255, 200, 0, 200));
    }

    ImNodes::BeginNode(node.id.value);
    ImGui::TextUnformatted(node.name.c_str());
    ImNodes::EndNode();

    if (cfg.showRuntimeHighlight && isActive)
    {
        ImNodes::PopColorStyle();
    }
#else
    (void)node;
    (void)cfg;
    (void)activeNodeID;
#endif
}

// ============================================================================
// Private: RenderLink
// ============================================================================

void SharedGraphRenderer::RenderLink(const LinkData& link)
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    ImNodes::Link(link.id.value, link.fromPin.value, link.toPin.value);
#else
    (void)link;
#endif
}

// ============================================================================
// Private: RenderToolbar
// ============================================================================

void SharedGraphRenderer::RenderToolbar(GraphDocument*       doc,
                                         const EditorContext& ctx)
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    if (ImGui::Button("Auto Layout"))
    {
        // Trigger auto-layout via the document's built-in method.
        // Requires NodeGraphCore AutoLayout support.
        (void)doc;
    }
    ImGui::SameLine();
    ImGui::TextDisabled(ctx.GetMode() == Olympe::NodeGraphTypes::EditorMode::Debug ? "[Debug]" : "[Editor]");
#else
    (void)doc;
    (void)ctx;
#endif
}

} // namespace NodeGraphTypes
} // namespace Olympe
