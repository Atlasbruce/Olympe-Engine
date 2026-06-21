#pragma once

#include "BehaviorTree.h"
#include "../ECS_Entity.h"

extern "C" {
	// C linkage API so runtime can call without depending on debugger C++ types
	// Use plain C types for linkage boundary: nodeName as null-terminated C string
	void BTDebug_AddExecutionEntry(EntityID entity, uint32_t nodeId, const char* nodeName, uint8_t status);
	bool BTDebug_IsVisible();
	// Add a JSON-serialized execution entry. Used to pass rich payloads with
	// timestamps, treeId, entityName and optional details.
	void BTDebug_AddExecutionJson(const char* jsonLine);
}
