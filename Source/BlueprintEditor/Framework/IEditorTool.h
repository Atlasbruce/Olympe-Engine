#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Olympe {

// Forward declarations
class IGraphDocument;
class GraphEditorBase;

/**
 * @class IEditorTool
 * @brief Base interface for all editor tools/plugins (verifiers, outputters, palette providers, etc.)
 *
 * Plugin architecture enables extensibility without modifying renderers.
 * Tools are registered with IEditorToolManager and lifecycle-managed by the framework.
 *
 * C++14 compliant
 */
class IEditorTool
{
public:
    virtual ~IEditorTool() = default;

    /// Get unique tool identifier
    virtual std::string GetToolId() const = 0;

    /// Get human-readable tool name
    virtual std::string GetToolName() const = 0;

    /// Initialize the tool (called once when tool registered)
    virtual void Initialize(GraphEditorBase* editor, IGraphDocument* document) = 0;

    /// Shutdown the tool (called when tool unregistered)
    virtual void Shutdown() = 0;

    /// Render tool UI (called each frame if tool visible)
    virtual void Render() = 0;

    /// Tool enabled/disabled query
    virtual bool IsEnabled() const = 0;

    /// Set enabled state
    virtual void SetEnabled(bool enabled) = 0;
};

/**
 * @class IGraphVerifier
 * @brief Plugin interface for graph validation tools
 *
 * Verifiers check graph validity and report errors.
 * Example: cycle detection, disconnected node detection, type validation
 */
class IGraphVerifier : public IEditorTool
{
public:
    struct ValidationError {
        std::string message;
        std::vector<int> affectedNodeIds;  // Nodes involved in error
        int severity;  // 0=info, 1=warning, 2=error
    };

    /// Run validation on current graph
    /// @return List of errors found (empty = graph valid)
    virtual std::vector<ValidationError> Verify() = 0;

    /// Get count of errors found in last verification
    virtual int GetErrorCount() const = 0;

    /// Highlight error in editor (render visual feedback on nodes)
    virtual void HighlightError(int nodeId, bool highlight = true) = 0;
};

/**
 * @class IOutputPanel
 * @brief Plugin interface for logging/output display
 *
 * Output panels display logs, warnings, errors, debug information.
 * Example: execution traces, verification results, debug output
 */
class IOutputPanel : public IEditorTool
{
public:
    enum class LogLevel {
        Info = 0,
        Warning = 1,
        Error = 2,
        Debug = 3
    };

    /// Add a log message
    virtual void AddLog(const std::string& message, LogLevel level = LogLevel::Info) = 0;

    /// Clear all logs
    virtual void Clear() = 0;

    /// Get count of logs at specific level
    virtual int GetLogCount(LogLevel level) const = 0;

    /// Get visibility state
    virtual bool IsVisible() const = 0;

    /// Set visibility state
    virtual void SetVisible(bool visible) = 0;
};

/**
 * @class INodePaletteProvider
 * @brief Plugin interface for node creation UI (drag-drop palette)
 *
 * Palette providers show available node types and enable drag-drop creation.
 * Example: component selector (Blue, Green, Magenta nodes for Placeholder),
 *          VisualScript node types, BehaviorTree node types
 */
class INodePaletteProvider : public IEditorTool
{
public:
    struct NodeTypeInfo {
        std::string typeId;           // Unique identifier
        std::string displayName;      // Human-readable name
        std::string category;         // Category for grouping (Core, Physics, etc.)
        std::string description;      // Tooltip/help text
    };

    /// Get list of available node types
    virtual std::vector<NodeTypeInfo> GetNodeTypes() const = 0;

    /// Filter node types by category
    virtual std::vector<NodeTypeInfo> GetNodeTypesByCategory(const std::string& category) const = 0;

    /// Get all unique categories
    virtual std::vector<std::string> GetCategories() const = 0;

    /// Create a node of given type at position
    /// Returns nodeId of created node
    virtual int CreateNode(const std::string& typeId, float x, float y) = 0;
};

} // namespace Olympe
