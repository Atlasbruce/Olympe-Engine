/*
 * Olympe Blueprint Editor - World Bridge Header
 * 
 * Provides function declarations for World-to-Editor communication
 * Include this header in both World.cpp and WorldBridge.cpp to avoid linker errors
 */

#pragma once

#include <cstdint>

// Bridge functions to notify Blueprint Editor of World changes
// These use C linkage to avoid name mangling issues
#ifdef __cplusplus
extern "C" {
#endif

// Called by World when a new entity is created
void NotifyEditorEntityCreated(uint64_t entity);

// Called by World when an entity is destroyed
void NotifyEditorEntityDestroyed(uint64_t entity);

#ifdef __cplusplus
}

namespace Olympe
{
    /**
     * @brief Register the TaskSystem publish callback that routes live
     *        task-runner state to NodeGraphPanel and InspectorPanel.
     *
     * Call once during editor initialization.
     */
    void WorldBridge_RegisterTaskCallback();

    /**
     * @brief Unregister the TaskSystem publish callback.
     *
     * Call during editor shutdown to prevent dangling callbacks into
     * destroyed editor panels.
     */
    void WorldBridge_UnregisterTaskCallback();

} // namespace Olympe

#endif