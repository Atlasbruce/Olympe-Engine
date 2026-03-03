/**
 * @file EditorContext.cpp
 * @brief Editor mode and capabilities management implementation
 * @author Olympe Engine - NodeGraph Core v2.0
 * @date 2025-02-19
 */

#include "EditorContext.h"

namespace Olympe
{
namespace NodeGraph
{

EditorContext::EditorContext(EditorMode mode)
    : m_mode(mode)
{
    // Configure capabilities based on mode
    switch (m_mode)
    {
    case EditorMode::Editor:
        // Full editing capabilities
        m_capabilities.canCreateNodes = true;
        m_capabilities.canDeleteNodes = true;
        m_capabilities.canMoveNodes = true;
        m_capabilities.canConnectPins = true;
        m_capabilities.canEditProperties = true;
        m_capabilities.canUndo = true;
        m_capabilities.canRedo = true;
        m_capabilities.canSave = true;
        m_capabilities.showRuntimeState = false;
        break;
        
    case EditorMode::Visualizer:
        // Read-only mode
        m_capabilities.canCreateNodes = false;
        m_capabilities.canDeleteNodes = false;
        m_capabilities.canMoveNodes = false;
        m_capabilities.canConnectPins = false;
        m_capabilities.canEditProperties = false;
        m_capabilities.canUndo = false;
        m_capabilities.canRedo = false;
        m_capabilities.canSave = false;
        m_capabilities.showRuntimeState = false;
        break;
        
    case EditorMode::Debug:
        // Read-only + runtime state
        m_capabilities.canCreateNodes = false;
        m_capabilities.canDeleteNodes = false;
        m_capabilities.canMoveNodes = false;
        m_capabilities.canConnectPins = false;
        m_capabilities.canEditProperties = false;
        m_capabilities.canUndo = false;
        m_capabilities.canRedo = false;
        m_capabilities.canSave = false;
        m_capabilities.showRuntimeState = true;
        break;
    }
}

EditorContext EditorContext::CreateEditor()
{
    return EditorContext(EditorMode::Editor);
}

EditorContext EditorContext::CreateVisualizer()
{
    return EditorContext(EditorMode::Visualizer);
}

EditorContext EditorContext::CreateDebugger()
{
    return EditorContext(EditorMode::Debug);
}

} // namespace NodeGraph
} // namespace Olympe
