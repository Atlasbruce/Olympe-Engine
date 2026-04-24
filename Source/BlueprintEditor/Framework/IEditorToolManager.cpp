#include "IEditorToolManager.h"
#include "IEditorTool.h"
#include <iostream>

namespace Olympe {

// EditorToolManager Implementation

EditorToolManager::EditorToolManager()
    : m_initialized(false)
{
}

EditorToolManager::~EditorToolManager()
{
    Shutdown();
}

void EditorToolManager::RegisterTool(std::unique_ptr<IEditorTool> tool)
{
    if (!tool)
    {
        std::cout << "[EditorToolManager] ERROR: Attempted to register null tool" << std::endl;
        return;
    }

    std::string toolId = tool->GetToolId();
    
    // Check if tool already registered
    if (m_tools.find(toolId) != m_tools.end())
    {
        std::cout << "[EditorToolManager] WARNING: Tool '" << toolId << "' already registered, replacing" << std::endl;
    }

    // Add tool to registry
    m_tools[toolId] = std::move(tool);

    // If manager already initialized, initialize the new tool
    if (m_initialized)
    {
        IEditorTool* newTool = m_tools[toolId].get();
        if (newTool)
        {
            newTool->Initialize(nullptr, nullptr);  // GraphEditorBase and document would be set elsewhere
        }
    }

    std::cout << "[EditorToolManager] Registered tool: " << toolId << std::endl;
}

bool EditorToolManager::UnregisterTool(const std::string& toolId)
{
    auto it = m_tools.find(toolId);
    if (it == m_tools.end())
    {
        std::cout << "[EditorToolManager] WARNING: Tool '" << toolId << "' not found for unregistration" << std::endl;
        return false;
    }

    // Shutdown tool before removal
    if (it->second)
    {
        it->second->Shutdown();
    }

    m_tools.erase(it);
    std::cout << "[EditorToolManager] Unregistered tool: " << toolId << std::endl;
    return true;
}

IEditorTool* EditorToolManager::GetTool(const std::string& toolId)
{
    auto it = m_tools.find(toolId);
    if (it == m_tools.end())
    {
        return nullptr;
    }
    return it->second.get();
}

std::vector<IEditorTool*> EditorToolManager::GetAllTools()
{
    std::vector<IEditorTool*> tools;
    for (auto& pair : m_tools)
    {
        if (pair.second)
        {
            tools.push_back(pair.second.get());
        }
    }
    return tools;
}

void EditorToolManager::Initialize(GraphEditorBase* editor, IGraphDocument* document)
{
    std::cout << "[EditorToolManager] Initializing " << m_tools.size() << " tools" << std::endl;

    for (auto& pair : m_tools)
    {
        if (pair.second)
        {
            pair.second->Initialize(editor, document);
        }
    }

    m_initialized = true;
    std::cout << "[EditorToolManager] All tools initialized" << std::endl;
}

void EditorToolManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    std::cout << "[EditorToolManager] Shutting down " << m_tools.size() << " tools" << std::endl;

    for (auto& pair : m_tools)
    {
        if (pair.second)
        {
            pair.second->Shutdown();
        }
    }

    m_tools.clear();
    m_initialized = false;
    std::cout << "[EditorToolManager] All tools shut down" << std::endl;
}

void EditorToolManager::Update()
{
    // Tools are updated via Render() each frame
    // No separate update phase needed (all logic in Render)
}

void EditorToolManager::RenderAll()
{
    for (auto& pair : m_tools)
    {
        if (pair.second && pair.second->IsEnabled())
        {
            pair.second->Render();
        }
    }
}

void EditorToolManager::SetToolEnabled(const std::string& toolId, bool enabled)
{
    IEditorTool* tool = GetTool(toolId);
    if (tool)
    {
        tool->SetEnabled(enabled);
        std::cout << "[EditorToolManager] Tool '" << toolId << "' " 
                  << (enabled ? "enabled" : "disabled") << std::endl;
    }
}

bool EditorToolManager::IsToolEnabled(const std::string& toolId) const
{
    auto it = m_tools.find(toolId);
    if (it == m_tools.end())
    {
        return false;
    }
    if (!it->second)
    {
        return false;
    }
    return it->second->IsEnabled();
}

} // namespace Olympe
