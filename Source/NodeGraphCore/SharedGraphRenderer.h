/**
 * @file SharedGraphRenderer.h
 * @brief Shared renderer for BlueprintEditor and RuntimeDebugger.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * SharedGraphRenderer encapsulates graph rendering logic that is common to
 * both the BlueprintEditor (edit mode) and the Visual Runtime Debugger (debug
 * mode).  It is configured via a Config struct that enables/disables features
 * such as read-only mode and runtime node highlighting.
 *
 * Usage:
 * @code
 *   // In BlueprintEditor:
 *   SharedGraphRenderer::Render(doc, ctx,
 *       SharedGraphRenderer::MakeEditorConfig());
 *
 *   // In NodeGraphPanel (Runtime Debugger):
 *   SharedGraphRenderer::Render(doc, ctx,
 *       SharedGraphRenderer::MakeDebuggerConfig(), activeNodeID);
 * @endcode
 *
 * @note
 * SharedGraphRenderer is a rendering abstraction layer.  When ImNodes is
 * available (BlueprintEditor / NodeGraphPanel context), implementations should
 * call the real ImNodes API.  In headless builds (tests, server processes) the
 * render calls are no-ops.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

namespace Olympe {
namespace NodeGraph {

// Forward declarations
class GraphDocument;
class EditorContext;
class LocalBlackboard;
struct NodeData;
struct LinkData;

/**
 * @class SharedGraphRenderer
 * @brief Renderer shared by BlueprintEditor and RuntimeDebugger.
 *
 * @details
 * All methods are static.  No instance state is required because the renderer
 * is purely functional: it reads from GraphDocument and EditorContext and
 * issues ImNodes / ImGui draw calls.
 */
class SharedGraphRenderer {
public:

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * @struct Config
     * @brief Rendering configuration knobs.
     */
    struct Config {
        bool readOnly             = false; ///< Disable node dragging / connection editing.
        bool showRuntimeHighlight = false; ///< Highlight the active debug node.
        bool showBlackboardPanel  = true;  ///< Show the blackboard side panel.
        bool showToolbar          = true;  ///< Show the graph toolbar.
    };

    /**
     * @brief Returns the default configuration for the BlueprintEditor (full-edit mode).
     *
     * readOnly=false, showRuntimeHighlight=false.
     */
    static Config MakeEditorConfig();

    /**
     * @brief Returns the default configuration for the Runtime Debugger (read-only mode).
     *
     * readOnly=true, showRuntimeHighlight=true.
     */
    static Config MakeDebuggerConfig();

    // -------------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------------

    /**
     * @brief Renders the graph inside the current ImGui window.
     *
     * @param doc          GraphDocument to render (non-owning).  Must not be null.
     * @param ctx          EditorContext (Editor or Debug mode).
     * @param cfg          Rendering configuration.
     * @param activeNodeID Node to highlight in debug mode (-1 = none).
     */
    static void Render(GraphDocument*       doc,
                       const EditorContext& ctx,
                       const Config&        cfg,
                       int                  activeNodeID = -1);

    /**
     * @brief Renders the blackboard variables panel.
     *
     * @param bb  LocalBlackboard to display.  May be null (panel shows empty state).
     */
    static void RenderBlackboardPanel(const LocalBlackboard* bb);

private:

    /**
     * @brief Renders a single node.
     * @param node         Node data.
     * @param cfg          Rendering config (controls highlight, read-only styling).
     * @param activeNodeID Highlighted node ID (-1 = none).
     */
    static void RenderNode(const NodeData& node, const Config& cfg, int activeNodeID);

    /**
     * @brief Renders a single link between two nodes.
     * @param link  Link data.
     */
    static void RenderLink(const LinkData& link);

    /**
     * @brief Renders the graph toolbar (save/load/auto-layout buttons).
     * @param doc  GraphDocument (commands are dispatched through it).
     * @param ctx  EditorContext (controls which actions are available).
     */
    static void RenderToolbar(GraphDocument* doc, const EditorContext& ctx);

    // Prevent instantiation
    SharedGraphRenderer() {}
};

} // namespace NodeGraph
} // namespace Olympe
