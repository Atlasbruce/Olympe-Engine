#pragma once

#include "IEditorTool.h"
#include <map>
#include <vector>
#include <memory>
#include <string>

namespace Olympe {

/**
 * @class IEditorToolManager
 * @brief Registry and lifecycle manager for editor plugins/tools
 *
 * Centralized plugin management:
 * - RegisterTool() / UnregisterTool()
 * - GetTool() by ID or type
 * - Initialize/Shutdown lifecycle
 * - Update all tools each frame
 *
 * Usage:
 *   auto manager = std::make_unique<EditorToolManager>();
 *   manager->RegisterTool(std::make_unique<VerifierTool>());
 *   manager->RegisterTool(std::make_unique<OutputPanel>());
 *   manager->Initialize(editor, document);
 *   // Each frame:
 *   manager->Update();
 *   manager->RenderAll();
 */
class IEditorToolManager
{
public:
    virtual ~IEditorToolManager() = default;

    /// Register a tool (takes ownership)
    virtual void RegisterTool(std::unique_ptr<IEditorTool> tool) = 0;

    /// Unregister tool by ID
    virtual bool UnregisterTool(const std::string& toolId) = 0;

    /// Get tool by ID
    virtual IEditorTool* GetTool(const std::string& toolId) = 0;

    /// Get tool by type (cast to specific interface)
    template<typename T>
    T* GetToolAs(const std::string& toolId) {
        IEditorTool* tool = GetTool(toolId);
        return dynamic_cast<T*>(tool);
    }

    /// Get all registered tools
    virtual std::vector<IEditorTool*> GetAllTools() = 0;

    /// Get count of registered tools
    virtual int GetToolCount() const = 0;

    /// Initialize all tools
    virtual void Initialize(GraphEditorBase* editor, IGraphDocument* document) = 0;

    /// Shutdown all tools
    virtual void Shutdown() = 0;

    /// Update all tools (called each frame)
    virtual void Update() = 0;

    /// Render all tools (called each frame)
    virtual void RenderAll() = 0;

    /// Enable/disable tool by ID
    virtual void SetToolEnabled(const std::string& toolId, bool enabled) = 0;

    /// Query tool enabled state
    virtual bool IsToolEnabled(const std::string& toolId) const = 0;
};

/**
 * @class EditorToolManager
 * @brief Default implementation of IEditorToolManager
 */
class EditorToolManager : public IEditorToolManager
{
public:
    EditorToolManager();
    virtual ~EditorToolManager();

    /// Register a tool (takes ownership)
    virtual void RegisterTool(std::unique_ptr<IEditorTool> tool) override;

    /// Unregister tool by ID
    virtual bool UnregisterTool(const std::string& toolId) override;

    /// Get tool by ID
    virtual IEditorTool* GetTool(const std::string& toolId) override;

    /// Get all registered tools
    virtual std::vector<IEditorTool*> GetAllTools() override;

    /// Get count of registered tools
    virtual int GetToolCount() const override { return static_cast<int>(m_tools.size()); }

    /// Initialize all tools
    virtual void Initialize(GraphEditorBase* editor, IGraphDocument* document) override;

    /// Shutdown all tools
    virtual void Shutdown() override;

    /// Update all tools (called each frame)
    virtual void Update() override;

    /// Render all tools (called each frame)
    virtual void RenderAll() override;

    /// Enable/disable tool by ID
    virtual void SetToolEnabled(const std::string& toolId, bool enabled) override;

    /// Query tool enabled state
    virtual bool IsToolEnabled(const std::string& toolId) const override;

private:
    // toolId -> tool
    std::map<std::string, std::unique_ptr<IEditorTool>> m_tools;

    // Track initialization state
    bool m_initialized;
};

} // namespace Olympe
