/**
 * @file BTGraphValidator.cpp
 * @brief Implementation of BTGraphValidator
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "BTGraphValidator.h"
#include "BTNodeRegistry.h"
#include "../../system/system_utils.h"
#include <set>
#include <functional>

namespace Olympe {
namespace AI {

// Type aliases for backward compatibility
using GraphDocument = Olympe::NodeGraphTypes::GraphDocument;

std::vector<BTValidationMessage> BTGraphValidator::ValidateGraph(const GraphDocument* graph) {
    std::vector<BTValidationMessage> messages;
    
    // TODO: Reimplement with modern NodeGraphTypes schema
    // - Old validator uses removed children/decoratorChild fields
    // - New schema structure requires different validation logic
    // For now: Return empty (valid) to unblock rendering
    (void)graph;
    
    if (graph == nullptr) {
        messages.push_back({
            BTValidationSeverity::Error,
            0,
            "Graph is null",
            "Create a valid graph document"
        });
    }
    
    SYSTEM_LOG << "[BTGraphValidator] DEPRECATED - awaiting reimplementation (Phase 50.4)" << std::endl;
    return messages;
}

void BTGraphValidator::ValidateRootNode(const GraphDocument* graph, std::vector<BTValidationMessage>& messages) {
    // TODO: Reimplement with modern schema
    (void)graph;
    (void)messages;
}

void BTGraphValidator::ValidateCycles(const GraphDocument* graph, std::vector<BTValidationMessage>& messages) {
    // TODO: Reimplement with modern schema
    (void)graph;
    (void)messages;
}

} // namespace AI
} // namespace Olympe
