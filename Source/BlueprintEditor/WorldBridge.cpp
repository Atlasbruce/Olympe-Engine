/*
 * Olympe Blueprint Editor - World Bridge
 * 
 * Provides hooks between World ECS and Blueprint Editor
 * Allows World to notify editor of entity/component changes without direct dependency
 */

#include "WorldBridge.h"
#include "BlueprintEditor.h"
#include "EntityInspectorManager.h"
#include "InspectorPanel.h"
#include "NodeGraphPanel.h"
#include "../ECS_Entity.h"
#include "../TaskSystem/LocalBlackboard.h"
#include "../TaskSystem/TaskSystem.h"
#include <cstdint>  // Pour uint64_t

// Bridge functions called by World (via extern declarations)
// These avoid circular dependencies between World and BlueprintEditor

extern "C" void NotifyEditorEntityCreated(uint64_t entity)
{
    // Notify BlueprintEditor backend (for Asset Browser runtime entities)
    Olympe::BlueprintEditor::Get().NotifyEntityCreated(entity);
    
    // Also notify EntityInspectorManager (for Inspector panel)
    if (Olympe::EntityInspectorManager::Get().IsInitialized())
    {
        Olympe::EntityInspectorManager::Get().OnEntityCreated(static_cast<EntityID>(entity));
    }
}

extern "C" void NotifyEditorEntityDestroyed(uint64_t entity)
{
    // Notify BlueprintEditor backend
    Olympe::BlueprintEditor::Get().NotifyEntityDestroyed(entity);
    
    // Also notify EntityInspectorManager
    if (Olympe::EntityInspectorManager::Get().IsInitialized())
    {
        Olympe::EntityInspectorManager::Get().OnEntityDestroyed(static_cast<EntityID>(entity));
    }
}

namespace Olympe
{

// Static frame-local copy of the blackboard snapshot published by TaskSystem.
// Updated once per frame in the TaskEditorPublishFn lambda below.
// InspectorPanel holds a non-owning pointer to this instance.
static LocalBlackboard s_FrameBlackboard;

/**
 * @brief Registers the WorldBridge task-running callback with TaskSystem.
 *
 * Call once during editor startup so that TaskSystem publishes live state
 * to the editor panels (NodeGraphPanel active-node highlight and
 * InspectorPanel live blackboard display) each frame.
 */
void WorldBridge_RegisterTaskCallback()
{
    TaskSystem::SetEditorPublishCallback(
        [](EntityID /*entity*/, int nodeIndex, const LocalBlackboard* bb)
        {
            // Copy blackboard snapshot; the pointer from TaskSystem is only
            // valid for the duration of this call.
            if (bb != nullptr)
            {
                s_FrameBlackboard = *bb;
                InspectorPanel::SetDebugBlackboard(&s_FrameBlackboard);
            }
            else
            {
                InspectorPanel::SetDebugBlackboard(nullptr);
            }

            // Highlight the executing node in the NodeGraph panel.
            NodeGraphPanel::SetActiveDebugNode(nodeIndex);
        });
}

} // namespace Olympe
